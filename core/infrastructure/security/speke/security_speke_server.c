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
#include <stddef.h>
#include "security_speke_server.h"
#include "security_speke_defs.h"
#include "security_speke_common.h"
#include "security_speke_session.h"
#include "iotc_log.h"
#include "adapter_mem.h"
#include "security_random.h"
#include "iotc_errcode.h"

static int32_t VerifyClientReqPayload(const IotcJson *payload)
{
    IotcJson *opCodeObj = IotcJsonGetObj(payload, SPEKE_SEC_DATA_OPCODE_JSON);
    if (opCodeObj == NULL) {
        IOTC_LOGE("Speke server verify req get opcode item err");
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    int64_t opCode = 0;
    int32_t ret = IotcJsonGetNum(opCodeObj, &opCode);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke server get opcode err:%d", ret);
        return ret;
    }
    if (opCode != SPEKE_OPCODE) {
        IOTC_LOGE("Speke server verify opcode:%ld err", opCode);
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_OPCODE;
    }

    ret = SpekeCommonVerifyVersion(payload);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke server verify version err:%d", ret);
        return ret;
    }

    return IOTC_OK;
}

static int32_t CreateSpekeServerRspSecPayload(const NegoContext *negoCtx, IotcJson **rspPayload)
{
    IotcJson *payload = IotcJsonCreate();
    if (payload == NULL) {
        IOTC_LOGE("Speke server rsp create payload JSON err");
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }

    int32_t ret = SpekeCommonAddVerInfoToJson(payload);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke server rsp add ver info JSON err:%d", ret);
        IotcJsonDelete(payload);
        return ret;
    }
    ret = SpekeCommonAddDataToJson(payload, SPEKE_SEC_DATA_CHALLENGE_JSON, negoCtx->localChallenge, CHALLENGE_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke server rsp add challenge JSON err:%d", ret);
        IotcJsonDelete(payload);
        return ret;
    }
    ret = SpekeCommonAddDataToJson(payload, SPEKE_SEC_DATA_SALT_JSON, negoCtx->salt, negoCtx->saltLen);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke server rsp add salt JSON err:%d", ret);
        IotcJsonDelete(payload);
        return ret;
    }
    ret = SpekeCommonAddDataToJson(payload, SPEKE_SEC_DATA_EPK_JSON, negoCtx->pubKey, negoCtx->pubKeyLen);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke server rsp add epk JSON err:%d", ret);
        IotcJsonDelete(payload);
        return ret;
    }

    *rspPayload = payload;
    return IOTC_OK;
}

int32_t SpekeServerProcessReq(SpekeProcessParam param, uint8_t **msg, uint32_t *len)
{
    if ((msg == NULL) || (len == NULL) || (param.session == NULL) ||
        (param.sessionId == NULL) || (param.secDataPayload == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    SpekeSession *session = param.session;
    if (session->spekeType != SPEKE_TYPE_SERVER) {
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_TYPE;
    }

    int32_t ret = VerifyClientReqPayload(param.secDataPayload);
    if (ret != IOTC_OK) {
        return ret;
    }

    if (session->negoContext == NULL) {
        IOTC_LOGW("proc req err, nego ctx null");
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_NEGOCTX_NOT_INIT;
    }
    NegoContext *negoCtx = session->negoContext;

    IotcJson *rspPayload = NULL;
    ret = CreateSpekeServerRspSecPayload(negoCtx, &rspPayload);
    if (ret != IOTC_OK) {
        return ret;
    }

    ret = SpekeCommonCreateNegoMsg(param.sessionId, SPEKE_SEC_DATA_MSG_TYPE_SERVER_RSP, rspPayload, msg, len);
    IotcJsonDelete(rspPayload);
    return ret;
}

static int32_t ParseClientCfmPayload(NegoContext *negoCtx, const IotcJson *payload)
{
    uint8_t *remoteChallenge = NULL;
    uint32_t remoteChallengeLen = 0;
    int32_t ret = SpekeCommonParseDataFromJson(payload, SPEKE_SEC_DATA_CHALLENGE_JSON,
        &remoteChallenge, &remoteChallengeLen);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke server get remote challenge err:%d", ret);
        return ret;
    }
    ret = NegoContextSetRemoteChallenge(negoCtx, remoteChallenge, remoteChallengeLen);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke server set remote challenge err:%d", ret);
        IotcFree(remoteChallenge);
        return ret;
    }
    IotcFree(remoteChallenge);

    uint8_t *remotePubKey = NULL;
    uint32_t remotePubKeyLen = 0;
    ret = SpekeCommonParseDataFromJson(payload, SPEKE_SEC_DATA_EPK_JSON, &remotePubKey, &remotePubKeyLen);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke server get remote pubKey err:%d", ret);
        return ret;
    }
    ret = NegoContextGenSessionKey(negoCtx, remotePubKey, remotePubKeyLen);
    if (ret != IOTC_OK) {
        IotcFree(remotePubKey);
        return ret;
    }
    IotcFree(remotePubKey);

    uint8_t *remoteHmac = NULL;
    uint32_t remoteHmacLen = 0;
    ret = SpekeCommonParseDataFromJson(payload, SPEKE_SEC_DATA_KCF_JSON, &remoteHmac, &remoteHmacLen);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke server get remote hmac err:%d", ret);
        return ret;
    }
    ret = NegoContextVerifyHmac(negoCtx, remoteHmac, remoteHmacLen);
    if (ret != IOTC_OK) {
        IotcFree(remoteHmac);
        return ret;
    }
    IotcFree(remoteHmac);

    return IOTC_OK;
}

static int32_t CreateSpekeServerCfmSecPayload(const NegoContext *negoCtx, IotcJson **cfmPayload)
{
    IotcJson *payload = IotcJsonCreate();
    if (payload == NULL) {
        IOTC_LOGE("Speke server create cfm payload JSON err");
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }

    uint8_t hmac[HMAC_LEN] = { 0 };
    int32_t ret = NegoContextGenHmac(negoCtx, hmac, HMAC_LEN);
    if (ret != IOTC_OK) {
        IotcJsonDelete(payload);
        return ret;
    }
    ret = SpekeCommonAddDataToJson(payload, SPEKE_SEC_DATA_KCF_JSON, hmac, HMAC_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke server add hmac to cfm payload err:%d", ret);
        IotcJsonDelete(payload);
        return ret;
    }

    *cfmPayload = payload;
    return IOTC_OK;
}

int32_t SpekeServerProcessCfm(SpekeProcessParam param, uint8_t **msg, uint32_t *len)
{
    if ((msg == NULL) || (len == NULL) || (param.session == NULL) ||
        (param.sessionId == NULL) || (param.secDataPayload == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    SpekeSession *session = param.session;
    if (session->spekeType != SPEKE_TYPE_SERVER) {
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_TYPE;
    }
    if (session->negoContext == NULL) {
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_NEGOCTX_NOT_INIT;
    }

    int32_t ret = ParseClientCfmPayload(session->negoContext, param.secDataPayload);
    if (ret != IOTC_OK) {
        return ret;
    }

    IotcJson *cfmPayload = NULL;
    ret = CreateSpekeServerCfmSecPayload(session->negoContext, &cfmPayload);
    if (ret != IOTC_OK) {
        return ret;
    }

    ret = SpekeCommonCreateNegoMsg(param.sessionId, SPEKE_SEC_DATA_MSG_TYPE_SERVER_CFM, cfmPayload, msg, len);
    if (ret != IOTC_OK) {
        IotcJsonDelete(cfmPayload);
        return ret;
    }
    IotcJsonDelete(cfmPayload);

    return NegoContextGenDataEncKey(session->negoContext, session->dataEncKey, sizeof(session->dataEncKey));
}