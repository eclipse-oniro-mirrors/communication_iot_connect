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
#include "security_speke_session.h"
#include "security_speke_defs.h"
#include "security_speke_nego_ctx.h"
#include "security_speke_common.h"
#include "security_speke_server.h"
#include "security_speke_client.h"
#include "iotc_log.h"
#include "iotc_mem.h"
#include "securec.h"
#include "iotc_aes.h"
#include "security_random.h"
#include "security_speke_flat.h"
#include "iotc_errcode.h"

/* GCM_VER_LEN/GCM_IV_LEN/GCM_TAG_LEN/SPEKE_ENC_DATA_MIN_LEN 上移至 security_speke.h */
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

    SpekeSession *session = (SpekeSession *)IotcMalloc(sizeof(SpekeSession));
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
    IotcFree(session);
}

void SpekeFreeNegoContext(SpekeSession *session)
{
    if (session == NULL) {
        IOTC_LOGW("session double free");
        return;
    }

    if (session->negoContext != NULL) {
        NegoContextFree(session->negoContext);
        session->negoContext = NULL;
    }
}

int32_t SpekeStartSession(const SpekeSession *session, uint8_t **msg, uint32_t *len)
{
    if ((session == NULL) || (msg == NULL) || (len == NULL)) {
        IOTC_LOGE("Speke start session param NULL");
        return IOTC_ERR_PARAM_INVALID;
    }

    return SpekeClientStartReq(session, msg, len);
}

static int32_t ParseCommonData(const char *root, uint32_t rootLen, SpekeProcessParam *param,
    char *sessionIdBuf, uint32_t sessionIdBufLen)
{
    /* flat extraction -- no cJSON tree */
    SpekeStrView sessionIdView;
    int32_t ret = SpekeFlatGetStr(root, rootLen, SPEKE_SESSION_ID_JSON, &sessionIdView);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke parse sessionId flat err");
        return ret;
    }
    if (sessionIdView.len == 0) {
        IOTC_LOGE("Speke parse sessionId empty");
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    if (sessionIdView.len >= sessionIdBufLen) {
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    if (memcpy_s(sessionIdBuf, sessionIdBufLen, sessionIdView.start, sessionIdView.len) != EOK) {
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    sessionIdBuf[sessionIdView.len] = '\0';
    param->sessionId = sessionIdBuf;

    SpekeStrView secDataView;
    ret = SpekeFlatGetObj(root, rootLen, SPEKE_SEC_DATA_JSON, &secDataView);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke parse secData flat err");
        return ret;
    }

    int64_t msgTypeInt = 0;
    ret = SpekeFlatGetNum(secDataView.start, secDataView.len, SPEKE_SEC_DATA_MESSAGE_JSON, &msgTypeInt);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke parse msgType flat err");
        return ret;
    }

    SpekeStrView payloadView;
    ret = SpekeFlatGetObj(secDataView.start, secDataView.len, SPEKE_SEC_DATA_PAYLOAD_JSON, &payloadView);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke parse payload flat err");
        return ret;
    }

    param->msgType = (int32_t)msgTypeInt;
    param->payload = payloadView.start;
    param->payloadLen = payloadView.len;
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

/* best-effort error inform on early-parse failure; inform only when the
 * sessionId is still extractable (otherwise the peer is unreachable by sessionId) */
static void SpekeTryInformParseErr(const char *payload, uint32_t len,
    int32_t errCode, uint8_t **msg, uint32_t *outLen)
{
    SpekeStrView sessionIdView = { 0 };
    if (SpekeFlatGetStr(payload, len, SPEKE_SESSION_ID_JSON, &sessionIdView) != IOTC_OK) {
        return;
    }
    char sessionIdBuf[SPEKE_FLAT_SESSION_ID_MAX_LEN + 1] = {0};
    if ((sessionIdView.len == 0) || (sessionIdView.len >= sizeof(sessionIdBuf)) ||
        (memcpy_s(sessionIdBuf, sizeof(sessionIdBuf), sessionIdView.start, sessionIdView.len) != EOK)) {
        return;
    }
    sessionIdBuf[sessionIdView.len] = '\0';
    CreateErrCodeInformMsg(sessionIdBuf, errCode, msg, outLen);
}

/* 公共实现。prefixLen>0 时返回缓冲前置预留段——CLIENT_REQ（SERVER_RSP
   大报文）由服务端直接在偏移处序列化；其余小报文与错误 INFORM 经 SpekeCommonWrapPrefix
   一次拷贝前缀化 */

static int32_t SpekeProcessDispatch(const SpekeProcessParam *param, uint8_t **msg, uint32_t *len);

/* 按消息类型分发到角色处理；CFM 消息处理后通知协商完成（意见3：主入口拆分出的分发子过程） */
static int32_t SpekeProcessDispatch(const SpekeProcessParam *param, uint8_t **msg, uint32_t *len)
{
    int32_t ret;
    switch (param->msgType) {
        case SPEKE_SEC_DATA_MSG_TYPE_CLIENT_REQ:
            ret = SpekeServerProcessReq(*param, msg, len);
            break;
        case SPEKE_SEC_DATA_MSG_TYPE_SERVER_RSP:
            ret = SpekeClientProcessRsp(*param, msg, len);
            break;
        case SPEKE_SEC_DATA_MSG_TYPE_CLIENT_CFM:
            ret = SpekeServerProcessCfm(*param, msg, len);
            NotifyNegoFinish(param->session, ret);
            break;
        case SPEKE_SEC_DATA_MSG_TYPE_SERVER_CFM:
            ret = SpekeClientProcessCfm(*param, msg, len);
            NotifyNegoFinish(param->session, ret);
            break;
        default:
            ret = IOTC_CORE_COMM_SEC_ERR_SPEKE_MSG_TYPE;
            break;
    }
    return ret;
}

static int32_t SpekeProcessPacketCommon(SpekeSession *session, const SpekePktView *pkt,
    uint32_t prefixLen, uint8_t **msg, uint32_t *len)
{
    const char *requestPayload = pkt->data;
    uint32_t payloadLen = pkt->len;
    bool payloadWritable = pkt->writable;

    if ((session == NULL) || (requestPayload == NULL) || (payloadLen == 0) || (msg == NULL) || (len == NULL)) {
        IOTC_LOGE("Speke proc packet param NULL");
        return IOTC_ERR_PARAM_INVALID;
    }

    /* flat validate -- no cJSON tree, no IotcJsonParse; length passed by the caller,
     * no NUL-termination dependency (7.7) */
    uint32_t reqLen = payloadLen;
    int32_t ret = SpekeFlatValidate(requestPayload, reqLen);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke flat validate err:%d", ret);
        SpekeTryInformParseErr(requestPayload, reqLen, IOTC_ADAPTER_JSON_ERR_PARSE, msg, len);
        (void)SpekeCommonWrapPrefix(msg, len, prefixLen);
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }

    char sessionIdBuf[SPEKE_FLAT_SESSION_ID_MAX_LEN + 1] = {0};
    SpekeProcessParam param = { 0 };
    param.session = session;
    param.payloadWritable = payloadWritable;   /* 透传宿主可写性，供 CFM 路径就地 unhexify */
    param.prefixLen = prefixLen;               /* SERVER_RSP 直写帧内偏移 */

    ret = ParseCommonData(requestPayload, reqLen, &param, sessionIdBuf, sizeof(sessionIdBuf));
    if (ret != IOTC_OK) {
        SpekeTryInformParseErr(requestPayload, reqLen, ret, msg, len);
        (void)SpekeCommonWrapPrefix(msg, len, prefixLen);
        return ret;
    }

    ret = SpekeProcessDispatch(&param, msg, len);
    /* CLIENT_REQ 的输出已由服务端按 param.prefixLen 前缀化，其余消息在此统一前缀化 */
    if ((ret == IOTC_OK) && (param.msgType != SPEKE_SEC_DATA_MSG_TYPE_CLIENT_REQ)) {
        ret = SpekeCommonWrapPrefix(msg, len, prefixLen);
    }
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke proc packet err:%d", ret);
        CreateErrCodeInformMsg(param.sessionId, ret, msg, len);
        (void)SpekeCommonWrapPrefix(msg, len, prefixLen);
    }

    /* no IotcJsonDelete -- no tree to delete */
    return ret;
}


int32_t SpekeProcessPacketView(SpekeSession *session, const SpekePktView *pkt,
    uint8_t **msg, uint32_t *len)
{
    return SpekeProcessPacketCommon(session, pkt, 0, msg, len);
}

/* 旧入口（WiFi/SLE 兼容）：基线签名（长度取 NUL 结尾，与基线一致），宿主按只读处理 */
int32_t SpekeProcessPacket(SpekeSession *session, const char *data,
    uint8_t **msg, uint32_t *len)
{
    SpekePktView pkt = { data, (uint32_t)strlen(data), false };
    return SpekeProcessPacketView(session, &pkt, msg, len);
}

int32_t SpekeProcessPacketPrefix(SpekeSession *session, const SpekePktView *pkt,
    uint32_t prefixLen, uint8_t **msg, uint32_t *len)
{
    return SpekeProcessPacketCommon(session, pkt, prefixLen, msg, len);
}

int32_t SpekeDecryptDataTo(SpekeSession *session, const uint8_t *data, uint32_t dataLen,
    const SpekeDataBuf *out)
{
    if ((session == NULL) || (data == NULL) || (dataLen <= SPEKE_ENC_DATA_MIN_LEN) ||
        (out->buff == NULL) || (out->buffLen == NULL)) {
        IOTC_LOGE("Speke decrypt to param err, dataLen:%u", dataLen);
        return IOTC_ERR_PARAM_INVALID;
    }

    uint32_t outDataLen = dataLen - SPEKE_ENC_DATA_MIN_LEN;
    if (out->buffCap < outDataLen) {
        IOTC_LOGE("Speke decrypt to cap err, cap:%u need:%u", out->buffCap, outDataLen);
        return IOTC_ERR_PARAM_INVALID;
    }
    /* AES decrypt: byte 0 is ver, bytes 1-12 are IV, last 16 bytes are TAG.
     * on auth failure mbedtls_gcm_auth_decrypt zeroes the output. */
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
    int32_t ret = IotcAesGcmDecrypt(&param, data + dataLen - GCM_TAG_LEN, GCM_TAG_LEN, out->buff);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke decrypt err:%d", ret);
        return ret;
    }

    *out->buffLen = outDataLen;
    return IOTC_OK;
}

int32_t SpekeDecryptDataInPlace(SpekeSession *session, uint8_t *data, uint32_t dataLen, uint32_t *decDataLen)
{
    if ((session == NULL) || (data == NULL) || (dataLen <= SPEKE_ENC_DATA_MIN_LEN) ||
        (decDataLen == NULL)) {
        IOTC_LOGE("Speke decrypt data param err, dataLen:%u", dataLen);
        return IOTC_ERR_PARAM_INVALID;
    }

    /* In-place decrypt: plaintext written back at data head, output(data) and
     * input(data+13) do not overlap, write stays 13B ahead of read so unread
     * ciphertext is not overwritten; output < input is the one overlap direction
     * mbedtls GCM documents as allowed. */
    SpekeDataBuf dst = { data, dataLen - SPEKE_ENC_DATA_MIN_LEN, decDataLen };
    return SpekeDecryptDataTo(session, data, dataLen, &dst);
}

/* 旧入口（WiFi/SLE 兼容）：基线签名，库外 malloc 输出缓冲、所有权移交调用方（基线行为） */
int32_t SpekeDecryptData(SpekeSession *session, const uint8_t *data, uint32_t dataLen,
    uint8_t **decData, uint32_t *decDataLen)
{
    if (data == NULL || dataLen <= SPEKE_ENC_DATA_MIN_LEN || decData == NULL || decDataLen == NULL) {
        return IOTC_ERR_PARAM_INVALID;
    }
    uint32_t binCap = dataLen - SPEKE_ENC_DATA_MIN_LEN;
    uint8_t *bin = (uint8_t *)IotcMalloc(binCap);
    if (bin == NULL) {
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    SpekeDataBuf dst = { bin, binCap, decDataLen };
    int32_t ret = SpekeDecryptDataTo(session, data, dataLen, &dst);
    if (ret != IOTC_OK) {
        IotcFree(bin);
        return ret;
    }
    *decData = bin;
    return IOTC_OK;
}

/* 加密直写到 out->buff（IV/tag/密文全部直写），省 stage 路径的 encData
   中转分配与整块 memcpy。与 SpekeEncryptData 同一布局：第0字节版本、第1-12字节IV、
   尾部16字节TAG，密文区间 (out->buff+13, out->buff+13+dataLen)。 */
int32_t SpekeEncryptDataInto(SpekeSession *session, const uint8_t *data, uint32_t dataLen,
    const SpekeDataBuf *out)
{
    if ((session == NULL) || (data == NULL) || (dataLen == 0) || (dataLen > SPEKE_DEC_DATA_MAX_LEN) ||
        (out->buff == NULL) || (out->buffLen == NULL)) {
        IOTC_LOGE("Speke encrypt into param err, dataLen:%u", dataLen);
        return IOTC_ERR_PARAM_INVALID;
    }

    uint32_t needed = dataLen + SPEKE_ENC_DATA_MIN_LEN;
    if (out->buffCap < needed) {
        IOTC_LOGE("Speke encrypt into out->buffCap:%u < needed:%u", out->buffCap, needed);
        return IOTC_ERR_PARAM_INVALID;
    }

    (void)memset_s(out->buff, needed, 0, needed);

    int32_t ret = SecurityRandom(out->buff + GCM_VER_LEN, GCM_IV_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke encrypt into gen iv err:%d", ret);
        return ret;
    }

    /* AES加密, 第0字节为版本, 第1-12字节为IV, 最后16字节为TAG */
    IotcAesGcmParam param = {
        .key        = session->dataEncKey,
        .keyLen     = SESSION_KEY_LEN,
        .iv         = out->buff + GCM_VER_LEN,
        .ivLen      = GCM_IV_LEN,
        .add        = NULL,
        .addLen     = 0,
        .data       = data,
        .dataLen    = dataLen,
    };
    ret = IotcAesGcmEncrypt(&param, out->buff + needed - GCM_TAG_LEN, GCM_TAG_LEN,
        out->buff + GCM_VER_LEN + GCM_IV_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke encrypt into err:%d", ret);
        return ret;
    }

    out->buff[0] = GCM_VERSION;
    *out->buffLen = needed;
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
    uint8_t *outData = (uint8_t *)IotcMalloc(outDataLen);
    if (outData == NULL) {
        IOTC_LOGE("Speke encrypt malloc(%u) err", outDataLen);
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(outData, outDataLen, 0, outDataLen);

    int32_t ret = SecurityRandom(outData + GCM_VER_LEN, GCM_IV_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke encrypt gen iv err:%d", ret);
        IotcFree(outData);
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
        IotcFree(outData);
        return ret;
    }

    outData[0] = GCM_VERSION;
    *encData = outData;
    *encDataLen = outDataLen;
    return IOTC_OK;
}