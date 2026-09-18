/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdbool.h>
#include <string.h>
#include "security_speke_client.h"
#include "security_speke_defs.h"
#include "security_speke_common.h"
#include "security_speke_session.h"
#include "iotc_log.h"
#include "iotc_mem.h"
#include "securec.h"
#include "security_speke_flat.h"
#include "utils_common.h"
#include "security_random.h"
#include "iotc_errcode.h"

/* SessionId 长度 */
#define SPEKE_SESSION_ID_HEX_LEN 16

static int32_t CreateSpekeClientReqSecPayload(IotcJson **reqPayload)
{
    IotcJson *payload = IotcJsonCreate();
    if (payload == NULL) {
        IOTC_LOGE("Speke client req create payload JSON err");
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }

    int32_t ret = SpekeCommonAddVerInfoToJson(payload);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke client req add ver info JSON err:%d", ret);
        IotcJsonDelete(payload);
        return ret;
    }
    ret = IotcJsonAddFloat2Obj(payload, SPEKE_SEC_DATA_OPCODE_JSON, SPEKE_OPCODE);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke client req add opcode JSON err:%d", ret);
        IotcJsonDelete(payload);
        return ret;
    }
    ret = IotcJsonAddBool2Obj(payload, SPEKE_SEC_DATA_256MODE_JSON, true);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke client req add 256mode JSON err:%d", ret);
        IotcJsonDelete(payload);
        return ret;
    }

    *reqPayload = payload;
    return IOTC_OK;
}

int32_t SpekeClientStartReq(const SpekeSession *session, uint8_t **msg, uint32_t *len)
{
    if ((session == NULL) || (msg == NULL) || (len == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    if (session->spekeType != SPEKE_TYPE_CLIENT) {
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_TYPE;
    }

    uint8_t sessionIdHex[SPEKE_SESSION_ID_HEX_LEN] = { 0 };
    int32_t ret = SecurityRandom(sessionIdHex, sizeof(sessionIdHex));
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke client req gen random err:%d", ret);
        return ret;
    }
    char sessionId[HEXIFY_LEN(SPEKE_SESSION_ID_HEX_LEN) + 1] = { 0 };
    if (!UtilsHexify(sessionIdHex, sizeof(sessionIdHex), sessionId, sizeof(sessionId))) {
        IOTC_LOGE("Speke client req hexify sessionId err");
        return IOTC_CORE_COMM_UTILS_ERR_HEXIFY;
    }

    IotcJson *reqPayload = NULL;
    ret = CreateSpekeClientReqSecPayload(&reqPayload);
    if (ret != IOTC_OK) {
        return ret;
    }

    ret = SpekeCommonCreateNegoMsg(sessionId, SPEKE_SEC_DATA_MSG_TYPE_CLIENT_REQ, reqPayload, msg, len);
    IotcJsonDelete(reqPayload);
    return ret;
}

static PrimeType GetSpekePrimeType(uint32_t pubKeyLen)
{
    if (pubKeyLen <= SPEKE_256_MODE_PUB_KEY_LEN) {
        return PRIME_NORMAL;
    } else if (pubKeyLen <= SPEKE_384_MODE_PUB_KEY_LEN) {
        return PRIME_BIG;
    } else {
        return PRIME_INVALID;
    }
}

/* 提取对端 challenge 并写入协商上下文；任一步失败释放 negoCtx 并返回错误 */
static int32_t ClientSetChallenge(NegoContext *negoCtx, const SpekeStrView *payload)
{
    SpekeStrView challengeView;
    int32_t ret = SpekeFlatGetStr(payload->start, payload->len, SPEKE_SEC_DATA_CHALLENGE_JSON, &challengeView);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke client get remote challenge flat err:%d", ret);
        NegoContextFree(negoCtx);
        return ret;
    }
    uint8_t challengeBuf[UNHEXIFY_LEN(SPEKE_FLAT_CHALLENGE_MAX_LEN)];
    uint32_t cLen = UNHEXIFY_LEN(challengeView.len);
    if (!UtilsUnhexify(challengeView.start, challengeView.len, challengeBuf, sizeof(challengeBuf))) {
        IOTC_LOGE("Speke client unhexify challenge err");
        NegoContextFree(negoCtx);
        return IOTC_CORE_COMM_UTILS_ERR_UNHEXIFY;
    }
    ret = NegoContextSetRemoteChallenge(negoCtx, challengeBuf, cLen);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke client set remote challenge err:%d", ret);
        NegoContextFree(negoCtx);
    }
    return ret;
}

static int32_t ClientInitNegoCtx(const uint8_t *pinCode, uint32_t pinCodeLen,
    const SpekeStrView *payload, uint32_t remotePubKeyLen, NegoContext **negoContext)
{
    /* flat extraction */
    PrimeType primeType = GetSpekePrimeType(remotePubKeyLen);
    if (primeType == PRIME_INVALID) {
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_PUBKEY;
    }

    SpekeStrView saltView;
    int32_t ret = SpekeFlatGetStr(payload->start, payload->len, SPEKE_SEC_DATA_SALT_JSON, &saltView);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke client get remote salt flat err:%d", ret);
        return ret;
    }
    uint8_t saltBuf[MAX_SALT_LEN];
    uint32_t saltLen = UNHEXIFY_LEN(saltView.len);
    if (!UtilsUnhexify(saltView.start, saltView.len, saltBuf, sizeof(saltBuf))) {
        IOTC_LOGE("Speke client unhexify salt err");
        return IOTC_CORE_COMM_UTILS_ERR_UNHEXIFY;
    }

    /* 客户端使用对端 salt 初始化协商上下文句柄 */
    NegoContext *negoCtx = NegoContextInit(pinCode, pinCodeLen, saltBuf, saltLen, primeType);
    if (negoCtx == NULL) {
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_NEGOCTX_INIT;
    }

    ret = ClientSetChallenge(negoCtx, payload);
    if (ret == IOTC_OK) {
        *negoContext = negoCtx;
    }
    return ret;
}

static int32_t InitNegoCtxFromPayload(const SpekeSession *session, const char *payload,
    uint32_t payloadLen, NegoContext **negoContext)
{
    /* flat extraction + hex direct read */
    SpekeStrView epkView;
    int32_t ret = SpekeFlatGetStr(payload, payloadLen, SPEKE_SEC_DATA_EPK_JSON, &epkView);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke client get remote pubKey flat err:%d", ret);
        return ret;
    }
    /* flat extraction; the remote pubKey hex is unhexified into a short-lived
     * binary buffer and imported with IotcMpiReadBinary (no NUL dependency) */
    uint32_t remotePubKeyLen = UNHEXIFY_LEN(epkView.len);
    if (remotePubKeyLen == 0) {
        IOTC_LOGE("Speke client remote pubKey empty");
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    uint8_t *remotePubKey = (uint8_t *)IotcMalloc(remotePubKeyLen);
    if (remotePubKey == NULL) {
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    if (!UtilsUnhexify(epkView.start, epkView.len, remotePubKey, remotePubKeyLen)) {
        IOTC_LOGE("Speke client remote pubKey unhexify err");
        IotcFree(remotePubKey);
        return IOTC_CORE_COMM_UTILS_ERR_UNHEXIFY;
    }

    NegoContext *negoCtx = NULL;
    SpekeStrView payloadView = { payload, payloadLen };
    ret = ClientInitNegoCtx(session->pinCode, session->pinCodeLen, &payloadView,
        remotePubKeyLen, &negoCtx);
    if (ret != IOTC_OK) {
        IotcFree(remotePubKey);
        return ret;
    }
    ret = NegoContextGenSessionKey(negoCtx, remotePubKey, remotePubKeyLen);
    IotcFree(remotePubKey);
    if (ret != IOTC_OK) {
        NegoContextFree(negoCtx);
        return ret;
    }

    *negoContext = negoCtx;
    return IOTC_OK;
}

static int32_t CreateSpekeClientCfmSecPayload(const NegoContext *negoCtx, IotcJson **cfmPayload)
{
    IotcJson *payload = IotcJsonCreate();
    if (payload == NULL) {
        IOTC_LOGE("Speke client create cfm payload JSON err");
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }

    int32_t ret = SpekeCommonAddDataToJson(payload, SPEKE_SEC_DATA_CHALLENGE_JSON,
        negoCtx->localChallenge, CHALLENGE_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke client add challenge to cfm payload err:%d", ret);
        IotcJsonDelete(payload);
        return ret;
    }

    ret = SpekeCommonAddDataToJson(payload, SPEKE_SEC_DATA_EPK_JSON, negoCtx->pubKey, negoCtx->pubKeyLen);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke client add pubKey to cfm payload err:%d", ret);
        IotcJsonDelete(payload);
        return ret;
    }

    uint8_t hmac[HMAC_LEN] = { 0 };
    ret = NegoContextGenHmac(negoCtx, hmac, HMAC_LEN);
    if (ret != IOTC_OK) {
        IotcJsonDelete(payload);
        return ret;
    }
    ret = SpekeCommonAddDataToJson(payload, SPEKE_SEC_DATA_KCF_JSON, hmac, HMAC_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke client add hmac to cfm payload err:%d", ret);
        IotcJsonDelete(payload);
        return ret;
    }

    *cfmPayload = payload;
    return IOTC_OK;
}

int32_t SpekeClientProcessRsp(SpekeProcessParam param, uint8_t **msg, uint32_t *len)
{
    if ((msg == NULL) || (len == NULL) || (param.session == NULL) ||
        (param.sessionId == NULL) || (param.payload == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    SpekeSession *session = param.session;
    if (session->spekeType != SPEKE_TYPE_CLIENT) {
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_TYPE;
    }

    int32_t ret = SpekeCommonVerifyVersion(param.payload, param.payloadLen);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke client proc rsp verify ver err:%d", ret);
        return ret;
    }

    NegoContext *negoCtx = NULL;
    ret = InitNegoCtxFromPayload(session, param.payload, param.payloadLen, &negoCtx);
    if (ret != IOTC_OK) {
        return ret;
    }

    IotcJson *cfmPayload = NULL;
    ret = CreateSpekeClientCfmSecPayload(negoCtx, &cfmPayload);
    if (ret != IOTC_OK) {
        NegoContextFree(negoCtx);
        return ret;
    }

    ret = SpekeCommonCreateNegoMsg(param.sessionId, SPEKE_SEC_DATA_MSG_TYPE_CLIENT_CFM, cfmPayload, msg, len);
    if (ret != IOTC_OK) {
        IotcJsonDelete(cfmPayload);
        NegoContextFree(negoCtx);
        return ret;
    }

    if (session->negoContext != NULL) {
        NegoContextFree(session->negoContext);
    }
    session->negoContext = negoCtx;
    IotcJsonDelete(cfmPayload);
    return IOTC_OK;
}

int32_t SpekeClientProcessCfm(SpekeProcessParam param, uint8_t **msg, uint32_t *len)
{
    if ((msg == NULL) || (len == NULL) || (param.session == NULL) ||
        (param.sessionId == NULL) || (param.payload == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    SpekeSession *session = param.session;
    if (session->spekeType != SPEKE_TYPE_CLIENT) {
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_TYPE;
    }
    if (session->negoContext == NULL) {
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_NEGOCTX_NOT_INIT;
    }

    *msg = NULL;
    *len = 0;

    /* flat extraction */
    SpekeStrView kcfView;
    int32_t ret = SpekeFlatGetStr(param.payload, param.payloadLen, SPEKE_SEC_DATA_KCF_JSON, &kcfView);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke client get remote hmac flat err:%d", ret);
        return ret;
    }
    uint8_t hmacBuf[HMAC_LEN];
    uint32_t hLen = UNHEXIFY_LEN(kcfView.len);
    if (!UtilsUnhexify(kcfView.start, kcfView.len, hmacBuf, sizeof(hmacBuf))) {
        IOTC_LOGE("Speke client unhexify hmac err");
        return IOTC_CORE_COMM_UTILS_ERR_UNHEXIFY;
    }
    ret = NegoContextVerifyHmac(session->negoContext, hmacBuf, hLen);
    if (ret != IOTC_OK) {
        return ret;
    }

    return NegoContextGenDataEncKey(session->negoContext, session->dataEncKey, sizeof(session->dataEncKey));
}