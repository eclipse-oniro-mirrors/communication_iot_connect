/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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
#include "security_speke_client.h"
#include "security_speke_defs.h"
#include "security_speke_common.h"
#include "security_speke_session.h"
#include "iotc_log.h"
#include "adapter_mem.h"
#include "securec.h"
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

static int32_t ClientInitNegoCtx(const uint8_t *pinCode, uint32_t pinCodeLen,
    const IotcJson *payload, uint32_t remotePubKeyLen, NegoContext **negoContext)
{
    /* 作为客户端需要根据服务端返回的公钥长度来决定是否使用256模式 */
    PrimeType primeType = GetSpekePrimeType(remotePubKeyLen);
    if (primeType == PRIME_INVALID) {
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_PUBKEY;
    }

    uint8_t *salt = NULL;
    uint32_t saltLen = 0;
    int32_t ret = SpekeCommonParseDataFromJson(payload, SPEKE_SEC_DATA_SALT_JSON, &salt, &saltLen);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke client get remote salt err:%d", ret);
        return ret;
    }

    /* 客户端使用对端 salt 初始化协商上下文句柄 */
    NegoContext *negoCtx = NegoContextInit(pinCode, pinCodeLen, salt, saltLen, primeType);
    if (negoCtx == NULL) {
        AdapterFree(salt);
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_NEGOCTX_INIT;
    }
    AdapterFree(salt);

    uint8_t *remoteChallenge = NULL;
    uint32_t remoteChallengeLen = 0;
    ret = SpekeCommonParseDataFromJson(payload, SPEKE_SEC_DATA_CHALLENGE_JSON, &remoteChallenge, &remoteChallengeLen);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke client get remote challenge err:%d", ret);
        NegoContextFree(negoCtx);
        return ret;
    }
    ret = NegoContextSetRemoteChallenge(negoCtx, remoteChallenge, remoteChallengeLen);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke client set remote challenge err:%d", ret);
        NegoContextFree(negoCtx);
    } else {
        *negoContext = negoCtx;
    }

    AdapterFree(remoteChallenge);
    return ret;
}

static int32_t InitNegoCtxFromPayload(const SpekeSession *session, const IotcJson *payload,
    NegoContext **negoContext)
{
    uint8_t *remotePubKey = NULL;
    uint32_t remotePubKeyLen = 0;
    int32_t ret = SpekeCommonParseDataFromJson(payload, SPEKE_SEC_DATA_EPK_JSON, &remotePubKey, &remotePubKeyLen);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke client get remote pubKey err:%d", ret);
        return ret;
    }

    NegoContext *negoCtx = NULL;
    ret = ClientInitNegoCtx(session->pinCode, session->pinCodeLen, payload, remotePubKeyLen, &negoCtx);
    if (ret != IOTC_OK) {
        AdapterFree(remotePubKey);
        return ret;
    }
    ret = NegoContextGenSessionKey(negoCtx, remotePubKey, remotePubKeyLen);
    if (ret != IOTC_OK) {
        AdapterFree(remotePubKey);
        NegoContextFree(negoCtx);
        return ret;
    }

    AdapterFree(remotePubKey);
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
        (param.sessionId == NULL) || (param.secDataPayload == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    SpekeSession *session = param.session;
    if (session->spekeType != SPEKE_TYPE_CLIENT) {
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_TYPE;
    }

    int32_t ret = SpekeCommonVerifyVersion(param.secDataPayload);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke client proc rsp verify ver err:%d", ret);
        return ret;
    }

    NegoContext *negoCtx = NULL;
    ret = InitNegoCtxFromPayload(session, param.secDataPayload, &negoCtx);
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
        (param.sessionId == NULL) || (param.secDataPayload == NULL)) {
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

    uint8_t *remoteHmac = NULL;
    uint32_t remoteHmacLen = 0;
    int32_t ret = SpekeCommonParseDataFromJson(param.secDataPayload, SPEKE_SEC_DATA_KCF_JSON,
        &remoteHmac, &remoteHmacLen);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke client get remote hmac err:%d", ret);
        return ret;
    }
    ret = NegoContextVerifyHmac(session->negoContext, remoteHmac, remoteHmacLen);
    if (ret != IOTC_OK) {
        AdapterFree(remoteHmac);
        return ret;
    }
    AdapterFree(remoteHmac);

    return NegoContextGenDataEncKey(session->negoContext, session->dataEncKey, sizeof(session->dataEncKey));
}