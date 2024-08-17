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
#include "security_speke_session.h"
#include "security_speke_defs.h"
#include "security_speke_nego_ctx.h"
#include "security_speke_common.h"
#include "security_speke_server.h"
#include "security_speke_client.h"
#include "iotc_log.h"
#include "adapter_mem.h"
#include "securec.h"
#include "adapter_aes.h"
#include "security_random.h"
#include "iotc_errcode.h"

#define GCM_VER_LEN 1
#define GCM_IV_LEN 12
#define GCM_TAG_LEN 16
#define SPEKE_ENC_DATA_MIN_LEN (GCM_VER_LEN + GCM_IV_LEN + GCM_TAG_LEN)
#define SPEKE_DEC_DATA_MAX_LEN (1024 * 1024)
#define GCM_VERSION 0

static int32_t GetPinCode(SpekeSession *session)
{
    if (session->spekeCb.getPinCode == NULL) {
        IOTC_LOGW("getPinCode cb null");
        return IOTC_ERR_PARAM_INVALID;
    }
    session->pinCodeLen = PIN_MAX_LEN;
    int32_t ret = session->spekeCb.getPinCode(session, session->user, session->pinCode, &session->pinCodeLen);
    if ((ret != 0) || (session->pinCodeLen == 0)) {
        IOTC_LOGE("SpekeCb getPinCode err:%d, len:%u", ret, session->pinCodeLen);
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_GET_PINCODE;
    }
    return IOTC_OK;
}

static void NotifyNegoFinish(SpekeSession *session, int32_t errCode)
{
    if (session->spekeCb.notifySpekeFinished == NULL) {
        IOTC_LOGW("SpekeCb notifySpekeFinished NULL");
        return;
    }
    int32_t ret = session->spekeCb.notifySpekeFinished(session, session->user, errCode);
    if (ret != 0) {
        IOTC_LOGE("SpekeCb notifySpekeFinished ret:%d", ret);
    }
}

SpekeSession *SpekeInitSession(SpekeType spekeType, const SpekeCallback *cb, void *user)
{
    if ((spekeType != SPEKE_TYPE_CLIENT) && (spekeType != SPEKE_TYPE_SERVER)) {
        IOTC_LOGE("Speke init type invalid:%d", spekeType);
        return NULL;
    }
    if (cb == NULL) {
        IOTC_LOGE("Speke init cb NULL");
        return NULL;
    }

    SpekeSession *session = (SpekeSession *)AdapterMalloc(sizeof(SpekeSession));
    if (session == NULL) {
        IOTC_LOGE("Speke session malloc err");
        return NULL;
    }
    (void)memset_s(session, sizeof(SpekeSession), 0, sizeof(SpekeSession));
    session->spekeType = spekeType;
    session->user = user;

    int32_t ret = memcpy_s(&session->spekeCb, sizeof(SpekeCallback), cb, sizeof(SpekeCallback));
    if (ret != EOK) {
        SpekeFreeSession(session);
        return NULL;
    }

    ret = GetPinCode(session);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke getPinCode err:%d", ret);
        SpekeFreeSession(session);
        return NULL;
    }

    if (spekeType == SPEKE_TYPE_CLIENT) {
        return session;
    }
    /* 服务端在此处初始化协商上下文, 防止客户端多次请求协商时上下文有差异导致协商失败 */
    uint8_t salt[MAX_SALT_LEN] = { 0 };
    ret = SecurityRandom(salt, MAX_SALT_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGW("Speke server gen salt err:%d", ret);
        SpekeFreeSession(session);
        return NULL;
    }
    session->negoContext = NegoContextInit(session->pinCode, session->pinCodeLen,
        salt, MAX_SALT_LEN, SUPPORT_PRIME_TYPE);
    if (session->negoContext == NULL) {
        SpekeFreeSession(session);
        return NULL;
    }

    return session;
}

void SpekeFreeSession(SpekeSession *session)
{
    if (session == NULL) {
        IOTC_LOGW("session double free");
        return;
    }

    if (session->negoContext != NULL) {
        NegoContextFree(session->negoContext);
    }

    (void)memset_s(session, sizeof(SpekeSession), 0, sizeof(SpekeSession));
    AdapterFree(session);
}

int32_t SpekeStartSession(const SpekeSession *session, uint8_t **msg, uint32_t *len)
{
    if ((session == NULL) || (msg == NULL) || (len == NULL)) {
        IOTC_LOGE("Speke start session param NULL");
        return IOTC_ERR_PARAM_INVALID;
    }

    return SpekeClientStartReq(session, msg, len);
}

static int32_t ParseCommonData(const IotcJson *root, SpekeProcessParam *param)
{
    IotcJson *sessionIdObj = IotcJsonGetObj(root, SPEKE_SESSION_ID_JSON);
    if (sessionIdObj == NULL) {
        IOTC_LOGE("Speke parse sessionId JSON err");
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    const char *sessionIdStr = IotcJsonGetStr(sessionIdObj);
    if (sessionIdStr == NULL) {
        IOTC_LOGE("Speke parse sessionId str err");
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }

    IotcJson *secDataObj = IotcJsonGetObj(root, SPEKE_SEC_DATA_JSON);
    if (secDataObj == NULL) {
        IOTC_LOGE("Speke parse secData JSON err");
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }

    IotcJson *msgTypeObj = IotcJsonGetObj(secDataObj, SPEKE_SEC_DATA_MESSAGE_JSON);
    if (msgTypeObj == NULL) {
        IOTC_LOGE("Speke parse msgType JSON err");
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    int64_t msgTypeInt = 0;
    int32_t ret = IotcJsonGetNum(msgTypeObj, &msgTypeInt);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke parse msgType err");
        return ret;
    }

    IotcJson *payloadObj = IotcJsonGetObj(secDataObj, SPEKE_SEC_DATA_PAYLOAD_JSON);
    if (payloadObj == NULL) {
        IOTC_LOGE("Speke parse payload JSON err");
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }

    param->sessionId = sessionIdStr;
    param->msgType = (int32_t)msgTypeInt;
    param->secDataPayload = payloadObj;
    return IOTC_OK;
}

static void CreateErrCodeInformMsg(const char *sessionId, int32_t errCode, uint8_t **msg, uint32_t *len)
{
    IotcJson *payload = IotcJsonCreate();
    if (payload == NULL) {
        IOTC_LOGE("Speke create err inform payload JSON err");
        return;
    }

    int32_t ret = IotcJsonAddFloat2Obj(payload, SPEKE_SEC_DATA_ERR_JSON, errCode);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke err inform add errCode to JSON err:%d", ret);
        IotcJsonDelete(payload);
        return;
    }

    ret = SpekeCommonCreateNegoMsg(sessionId, SPEKE_SEC_DATA_MSG_TYPE_INFORM_MSG, payload, msg, len);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke err inform create nego msg err:%d", ret);
    }
    IotcJsonDelete(payload);
}

int32_t SpekeProcessPacket(SpekeSession *session, const char *requestPayload, uint8_t **msg, uint32_t *len)
{
    if ((session == NULL) || (requestPayload == NULL) || (msg == NULL) || (len == NULL)) {
        IOTC_LOGE("Speke proc packet param NULL");
        return IOTC_ERR_PARAM_INVALID;
    }

    IotcJson *root = IotcJsonParse(requestPayload);
    if (root == NULL) {
        IOTC_LOGE("Speke proc reqPayload err");
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }

    SpekeProcessParam param = { 0 };
    param.session = session;

    int32_t ret = ParseCommonData(root, &param);
    if (ret != IOTC_OK) {
        IotcJsonDelete(root);
        return ret;
    }

    switch (param.msgType) {
        case SPEKE_SEC_DATA_MSG_TYPE_CLIENT_REQ:
            ret = SpekeServerProcessReq(param, msg, len);
            break;
        case SPEKE_SEC_DATA_MSG_TYPE_SERVER_RSP:
            ret = SpekeClientProcessRsp(param, msg, len);
            break;
        case SPEKE_SEC_DATA_MSG_TYPE_CLIENT_CFM:
            ret = SpekeServerProcessCfm(param, msg, len);
            NotifyNegoFinish(session, ret);
            break;
        case SPEKE_SEC_DATA_MSG_TYPE_SERVER_CFM:
            ret = SpekeClientProcessCfm(param, msg, len);
            NotifyNegoFinish(session, ret);
            break;
        default:
            ret = IOTC_CORE_COMM_SEC_ERR_SPEKE_MSG_TYPE;
            break;
    }
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke proc packet err:%d", ret);
        CreateErrCodeInformMsg(param.sessionId, ret, msg, len);
    }

    IotcJsonDelete(root);
    return ret;
}

int32_t SpekeDecryptData(SpekeSession *session, const uint8_t *data, uint32_t dataLen,
    uint8_t **decData, uint32_t *decDataLen)
{
    if ((session == NULL) || (data == NULL) || (dataLen <= SPEKE_ENC_DATA_MIN_LEN) ||
        (decData == NULL) || (decDataLen == NULL)) {
        IOTC_LOGE("Speke decrypt data param err, dataLen:%u", dataLen);
        return IOTC_ERR_PARAM_INVALID;
    }

    uint32_t outDataLen = dataLen - SPEKE_ENC_DATA_MIN_LEN;
    uint8_t *outData = (uint8_t *)AdapterMalloc(outDataLen);
    if (outData == NULL) {
        IOTC_LOGE("Speke decrypt malloc(%u) err", outDataLen);
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(outData, outDataLen, 0, outDataLen);

    /* AES解密, 第0字节为版本, 第1-12字节为IV, 最后16字节为TAG */
    IotcAesGcmParam param = {
        .key        = session->dataEncKey,
        .keyLen     = SESSION_KEY_LEN,
        .iv         = data + GCM_VER_LEN,
        .ivLen      = GCM_IV_LEN,
        .add        = NULL,
        .addLen     = 0,
        .data       = data + GCM_VER_LEN + GCM_IV_LEN,
        .dataLen    = outDataLen,
    };
    int32_t ret = IotcAesGcmDecrypt(&param, data + dataLen - GCM_TAG_LEN, GCM_TAG_LEN, outData);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke decrypt err:%d", ret);
        AdapterFree(outData);
        return ret;
    }

    *decData = outData;
    *decDataLen = outDataLen;
    return IOTC_OK;
}

int32_t SpekeEncryptData(SpekeSession *session, const uint8_t *data, uint32_t dataLen,
    uint8_t **encData, uint32_t *encDataLen)
{
    if ((session == NULL) || (data == NULL) || (dataLen == 0) || (dataLen > SPEKE_DEC_DATA_MAX_LEN) ||
        (encData == NULL) || (encDataLen == NULL)) {
        IOTC_LOGE("Speke encrypt data param err, dataLen:%u", dataLen);
        return IOTC_ERR_PARAM_INVALID;
    }

    uint32_t outDataLen = dataLen + SPEKE_ENC_DATA_MIN_LEN;
    uint8_t *outData = (uint8_t *)AdapterMalloc(outDataLen);
    if (outData == NULL) {
        IOTC_LOGE("Speke encrypt malloc(%u) err", outDataLen);
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(outData, outDataLen, 0, outDataLen);

    int32_t ret = SecurityRandom(outData + GCM_VER_LEN, GCM_IV_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke encrypt gen iv err:%d", ret);
        AdapterFree(outData);
        return ret;
    }

    /* AES加密, 第0字节为版本, 第1-12字节为IV, 最后16字节为TAG */
    IotcAesGcmParam param = {
        .key        = session->dataEncKey,
        .keyLen     = SESSION_KEY_LEN,
        .iv         = outData + GCM_VER_LEN,
        .ivLen      = GCM_IV_LEN,
        .add        = NULL,
        .addLen     = 0,
        .data       = data,
        .dataLen    = dataLen,
    };
    ret = IotcAesGcmEncrypt(&param, outData + outDataLen - GCM_TAG_LEN, GCM_TAG_LEN,
        outData + GCM_VER_LEN + GCM_IV_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke encrypt err:%d", ret);
        AdapterFree(outData);
        return ret;
    }

    outData[0] = GCM_VERSION;
    *encData = outData;
    *encDataLen = outDataLen;
    return IOTC_OK;
}