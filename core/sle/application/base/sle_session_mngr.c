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
#include "sle_session_mngr.h"
#include "config_authinfo.h"
#include "utils_common.h"
#include "utils_assert.h"
#include "iotc_kdf.h"
#include "security_random.h"
#include "iotc_aes.h"
#include "event_bus_sub.h"
#include "ble_linklayer.h"
#include "securec.h"
#include "iotc_svc_dev.h"
#include "iotc_errcode.h"
#include "iotc_event.h"
#include "dev_info.h"

#define SESS_HMAC_LEN   32
#define SESS_IV_LEN     12
#define SESS_TAG_LEN    16

#define SEQ_MAX_INTERVAL    30  /* seq有效范围为已收到最大值的正负30内 */
#define ITER_TIMES          1 /* 基于authcode派生，迭代一次表示依赖authcode安全强度 */

#define SLE_SESS_INIT { \
    .negoFinish = false, \
    .sessInfo = { \
        .maxRecvSeq = 0, \
        .recvBitMap = 0, \
        .nextSendSeq = 1, \
        .sessId = { 0 }, \
        .salt = { 0 }, \
        .key = { 0 } \
    } \
}

static SleSessParam g_sessParam = SLE_SESS_INIT;

void SleSessRecvSeqInit(uint32_t recvSeq)
{
    SleSessKeyInfo *sessInfo = &g_sessParam.sessInfo;
    sessInfo->maxRecvSeq = recvSeq;
    sessInfo->recvBitMap = 0;
    sessInfo->recvBitMap |= 0x1;
}

bool SleSessRecvSeqCheck(uint32_t recvSeq)
{
    SleSessKeyInfo *sessInfo = &g_sessParam.sessInfo;
    IOTC_LOGI("sle sess recvSeq:%u, lastSeq:%u(%u)", recvSeq, sessInfo->maxRecvSeq, sessInfo->recvBitMap);

    if (sessInfo->recvBitMap == 0) {
        return true;
    }

    uint32_t minusAbs = 0;
    if (recvSeq == sessInfo->maxRecvSeq) {
        IOTC_LOGE("sle sess recvSeq:%u invalid, lastSeq:%u(%u)",
            recvSeq, sessInfo->maxRecvSeq, sessInfo->recvBitMap);
        return false;
    } else if (recvSeq > sessInfo->maxRecvSeq) {
        if (recvSeq - sessInfo->maxRecvSeq <= SEQ_MAX_INTERVAL) {
            return true;
        }
        /* 考虑反转 */
        if (UINT32_MAX - recvSeq + sessInfo->maxRecvSeq + 1 > SEQ_MAX_INTERVAL) {
            IOTC_LOGE("sle sess recvSeq:%u invalid, lastSeq:%u(%u)",
                recvSeq, sessInfo->maxRecvSeq, sessInfo->recvBitMap);
            return false;
        }
        minusAbs = UINT32_MAX - recvSeq + sessInfo->maxRecvSeq + 1;
    } else {
        /* 考虑反转 */
        if (UINT32_MAX - sessInfo->maxRecvSeq + recvSeq + 1 <= SEQ_MAX_INTERVAL) {
            return true;
        }
        if (sessInfo->maxRecvSeq - recvSeq > SEQ_MAX_INTERVAL) {
            return false;
        }
        minusAbs = sessInfo->maxRecvSeq - recvSeq;
    }

    if (sessInfo->recvBitMap & (0x1 << minusAbs)) {
        IOTC_LOGE("sle sess recvSeq:%u invalid, lastSeq:%u(%u)",
            recvSeq, sessInfo->maxRecvSeq, sessInfo->recvBitMap);
        return false;
    }
    return true;
}

/* 此处不校验recvSeq是否合法 */
void SleSessRecvSeqUpdate(uint32_t recvSeq)
{
    SleSessKeyInfo *sessInfo = &g_sessParam.sessInfo;
    if (sessInfo->recvBitMap == 0) {
        SleSessRecvSeqInit(recvSeq);
        return;
    }
    bool recvLarger = false;
    uint32_t minusAbs = 0;
    if (recvSeq == sessInfo->maxRecvSeq) {
        return;
    } else if (recvSeq > sessInfo->maxRecvSeq) {
        if (recvSeq - sessInfo->maxRecvSeq <= SEQ_MAX_INTERVAL) {
            recvLarger = true;
            minusAbs = recvSeq - sessInfo->maxRecvSeq;
        } else {
            /* 考虑反转 */
            recvLarger = false;
            minusAbs = UINT32_MAX - recvSeq + sessInfo->maxRecvSeq + 1;
        }
    } else {
        /* 考虑反转 */
        if (UINT32_MAX - sessInfo->maxRecvSeq + recvSeq + 1 <= SEQ_MAX_INTERVAL) {
            recvLarger = true;
            minusAbs = UINT32_MAX - sessInfo->maxRecvSeq + recvSeq + 1;
        } else {
            recvLarger = false;
            minusAbs = sessInfo->maxRecvSeq - recvSeq;
        }
    }

    if (recvLarger) {
        sessInfo->recvBitMap = (sessInfo->recvBitMap << minusAbs) | 0x1;
        sessInfo->maxRecvSeq = recvSeq;
    } else {
        sessInfo->recvBitMap |= (0x1 << minusAbs);
    }
}

void SleSessSendSeqUpdate(void)
{
    g_sessParam.sessInfo.nextSendSeq++;
}

uint32_t SleSessSendSeqGet(void)
{
    return g_sessParam.sessInfo.nextSendSeq;
}

int32_t SleSessKeyGen(const uint8_t *sn1, uint32_t sn1Len, const uint8_t *sn2, uint32_t sn2Len)
{
    CHECK_RETURN_LOGE((sn1 != NULL) && (sn1Len > 0) && (sn2 != NULL) && (sn2Len > 0),
        IOTC_ERR_PARAM_INVALID, "param invalid, sn1Len:%u, sn2Len:%u", sn1Len, sn2Len);
    SleSessKeyInfo *sessInfo = &g_sessParam.sessInfo;
    int32_t ret = memcpy_s(sessInfo->salt, RAND_SN_LEN, sn1, sn1Len);
    CHECK_RETURN_LOGE(ret == EOK, IOTC_ERR_SECUREC_MEMCPY, "cpy sn1 err:%d", ret);
    ret = memcpy_s(sessInfo->salt + RAND_SN_LEN, RAND_SN_LEN, sn2, sn2Len);
    CHECK_RETURN_LOGE(ret == EOK, IOTC_ERR_SECUREC_MEMCPY, "cpy sn2 err:%d", ret);

    DevAuthInfo authInfo = {0};
    bool isAuthInfoExist = false;
    ret = DevSvcProxyGetAuthInfo(&isAuthInfoExist, &authInfo);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get auth info error %d", ret);
        return ret;
    }

    if (!isAuthInfoExist) {
        IOTC_LOGW("authinfo not exists");
        return IOTC_CORE_SLE_INVALID_AUTHCODE_ID;
    }

    IotcPbkdf2HmacParam param = {
        .md = IOTC_MD_SHA256,
        .password = authInfo.authCode,
        .passwordLen = sizeof(authInfo.authCode),
        .salt = sessInfo->salt,
        .saltLen = SALT_LEN,
        .iterCount = ITER_TIMES
    };
    ret = IotcPkcs5Pbkdf2Hmac(&param, sessInfo->key, SESSION_KEY_LEN);
    (void)memset_s(&authInfo, sizeof(DevAuthInfo), 0, sizeof(DevAuthInfo));
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "sle sess key gen err:%d", ret);
    g_sessParam.negoFinish = true;
    return IOTC_OK;
}

int32_t SleSessIdGen(void)
{
    SleSessKeyInfo *sessInfo = &g_sessParam.sessInfo;
    int32_t ret = SecurityRandom(sessInfo->sessId, sizeof(sessInfo->sessId));
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "sle sess id gen err:%d", ret);
    return IOTC_OK;
}

uint8_t *SleSessIdGet(void)
{
    return g_sessParam.sessInfo.sessId;
}

bool SleSessIsExist(void)
{
    return g_sessParam.negoFinish;
}

/* 生成随机 iv 进行加密, 并拼接 iv + encData + sessId 输出至 outBuff 中
 * outBuffLen 需至少为 SESS_IV_LEN + dataLen + SESS_TAG_LEN + SESSION_ID_LEN
 */
static int32_t SleSessEncryptData(const uint8_t *data, uint32_t dataLen, uint8_t *outBuff, uint32_t outBuffLen)
{
    uint32_t outLen = SESS_IV_LEN + dataLen + SESS_TAG_LEN + SESSION_ID_LEN;
    CHECK_RETURN_LOGE((data != NULL) && (dataLen > 0) && (outBuff != NULL) && (outBuffLen >= outLen),
        IOTC_ERR_PARAM_INVALID, "param invalid, dataLen:%u, outBuffLen:%u", dataLen, outBuffLen);
    CHECK_RETURN_LOGE(SleSessIsExist(), IOTC_CORE_SLE_ERR_SESSION_NOT_CREATE, "sess not create");

    (void)memset_s(outBuff, outLen, 0, outLen);
    /* 生成iv */
    int32_t ret = SecurityRandom(outBuff, SESS_IV_LEN);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "gen iv err:%d", ret);

    const char *pid = ModelGetDevProId();
    CHECK_RETURN_LOGE(pid != NULL && strlen(pid) > 0, IOTC_CORE_SLE_INVALID_PID, "pid err");
    /* 生成密文 */
    SleSessKeyInfo *sessInfo = &g_sessParam.sessInfo;
    IotcAesGcmParam param = {
        .key        = sessInfo->key,
        .keyLen     = SESSION_KEY_LEN,
        .iv         = outBuff,
        .ivLen      = SESS_IV_LEN,
        .add        = (const uint8_t *)pid,
        .addLen     = strlen(pid),
        .data       = data,
        .dataLen    = dataLen,
    };
    ret = IotcAesGcmEncrypt(&param, outBuff + SESS_IV_LEN + dataLen, SESS_TAG_LEN,
        outBuff + SESS_IV_LEN);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "gen encData err:%d", ret);

    /* 填充 sessId */
    ret = memcpy_s(outBuff + outLen - SESSION_ID_LEN, SESSION_ID_LEN, sessInfo->sessId, SESSION_ID_LEN);
    CHECK_RETURN(ret == EOK, IOTC_ERR_SECUREC_MEMCPY);
    
    return IOTC_OK;
}

/* 剥离 iv, sessId 并进行解密, 明文输出至 outBuff 中
 * outBuffLen 需至少为 dataLen - SESS_IV_LEN - SESS_TAG_LEN - SESSION_ID_LEN
 */
static int32_t SleSessDecryptData(const uint8_t *data, uint32_t dataLen, uint8_t *outBuff, uint32_t outBuffLen)
{
    int32_t outLen = dataLen - SESS_IV_LEN - SESS_TAG_LEN - SESSION_ID_LEN;
    CHECK_RETURN_LOGE((data != NULL) && (outLen > 0) && (outBuff != NULL) && (outBuffLen >= outLen),
        IOTC_ERR_PARAM_INVALID, "param invalid, dataLen:%u, outBuffLen:%u", dataLen, outBuffLen);
    CHECK_RETURN_LOGE(SleSessIsExist(), IOTC_CORE_SLE_ERR_SESSION_NOT_CREATE, "sess not create");

    SleSessKeyInfo *sessInfo = &g_sessParam.sessInfo;
    int32_t ret = memcmp(sessInfo->sessId, data + dataLen - SESSION_ID_LEN, SESSION_ID_LEN);
    CHECK_RETURN_LOGE(ret == 0, IOTC_CORE_SLE_ERR_SESSION_ID, "sess id not match");

    const char *pid = ModelGetDevProId();
    CHECK_RETURN_LOGE(pid != NULL && strlen(pid) > 0, IOTC_CORE_SLE_INVALID_PID, "pid err");
    (void)memset_s(outBuff, outLen, 0, outLen);
    /* AES解密 */
    IotcAesGcmParam param = {
        .key        = sessInfo->key,
        .keyLen     = SESSION_KEY_LEN,
        .iv         = data,
        .ivLen      = SESS_IV_LEN,
        .add        = (const uint8_t *)pid,
        .addLen     = strlen(pid),
        .data       = data + SESS_IV_LEN,
        .dataLen    = outLen,
    };
    ret = IotcAesGcmDecrypt(&param, data + SESS_IV_LEN + outLen, SESS_TAG_LEN, outBuff);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "gen decData err:%d", ret);
    
    return IOTC_OK;
}

static int32_t SleSessCalHmac(const uint8_t *data, uint32_t dataLen, uint8_t *calHmac, uint32_t calHmacLen)
{
    CHECK_RETURN_LOGE((data != NULL) && (dataLen > 0) && (calHmac != NULL) && (calHmacLen >= SESS_HMAC_LEN),
        IOTC_ERR_PARAM_INVALID, "param invalid, dataLen:%u, calHmacLen:%u", dataLen, calHmacLen);
    CHECK_RETURN_LOGE(SleSessIsExist(), IOTC_CORE_SLE_ERR_SESSION_NOT_CREATE, "sess not create");

    SleSessKeyInfo *sessInfo = &g_sessParam.sessInfo;
    uint8_t hmacKey[SESS_HMAC_LEN] = { 0 };
    IotcPbkdf2HmacParam param = {
        .md = IOTC_MD_SHA256,
        .password = sessInfo->key,
        .passwordLen = SESSION_KEY_LEN,
        .salt = sessInfo->salt,
        .saltLen = SALT_LEN,
        .iterCount = ITER_TIMES
    };
    int32_t ret = IotcPkcs5Pbkdf2Hmac(&param, hmacKey, SESS_HMAC_LEN);
    CHECK_RETURN(ret == IOTC_OK, ret);

    IotcHmacParam hmacParam = {
        .md = IOTC_MD_SHA256,
        .key = hmacKey,
        .keyLen = SESS_HMAC_LEN,
        .data = data,
        .dataLen = dataLen
    };
    ret = IotcHmacCalc(&hmacParam, calHmac, SESS_HMAC_LEN);
    (void)memset_s(hmacKey, SESS_HMAC_LEN, 0, SESS_HMAC_LEN);
    return ret;
}

static int32_t SleSessCheckHmac(const uint8_t *data, uint32_t dataLen, const uint8_t *hmacBuf, uint32_t hmacLen)
{
    CHECK_RETURN_LOGE((data != NULL) && (dataLen > 0) && (hmacBuf != NULL) && (hmacLen == SESS_HMAC_LEN),
        IOTC_ERR_PARAM_INVALID, "param invalid, dataLen:%u, hmacLen:%u", dataLen, hmacLen);
    CHECK_RETURN_LOGE(SleSessIsExist(), IOTC_CORE_SLE_ERR_SESSION_NOT_CREATE, "sess not create");

    uint8_t hmac[SESS_HMAC_LEN] = { 0 };
    int32_t ret = SleSessCalHmac(data, dataLen, hmac, sizeof(hmac));
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "cal hmac err:%d", ret);
    CHECK_RETURN_LOGE(memcmp(hmac, hmacBuf, SESS_HMAC_LEN) == 0, IOTC_CORE_SLE_ERR_HMAC, "cmp hmac err");

    return IOTC_OK;
}

void SleSessReset(void)
{
    (void)memset_s(&g_sessParam, sizeof(g_sessParam), 0, sizeof(g_sessParam));
    g_sessParam.sessInfo.nextSendSeq = 1;
}

static void SleSessSsapDisconnectCb(uint32_t event, void *param, uint32_t len)
{
    NOT_USED(event);
    NOT_USED(param);
    NOT_USED(len);
    SleSessReset();
}

int32_t SleSessInit(void)
{
    int32_t ret = EventBusSubscribe(SleSessSsapDisconnectCb, IOTC_CORE_SLE_EVENT_SSAP_DISCONNECT);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "subscribe err:%d", ret);

    LinkLayerSessKeyCallback cb = {
        .sessKeyEncrypt = SleSessEncryptData,
        .sessKeyDecrypt = SleSessDecryptData,
        .sessKeyCalHmac = SleSessCalHmac,
        .sessKeyCheckHmac = SleSessCheckHmac,
        .sessKeyCheckExist = SleSessIsExist
    };
    return LinkLayerRegisterSessKeyCb(cb);
}