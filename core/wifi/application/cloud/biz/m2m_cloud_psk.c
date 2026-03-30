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
#include "m2m_cloud_reg.h"
#include "m2m_cloud_psk.h"
#include "utils_assert.h"
#include "utils_json.h"
#include "config_register_info.h"
#include "comm_def.h"
#include "securec.h"
#include "utils_common.h"
#include "config_login_info.h"
#include "m2m_cloud_errcode.h"
#include "utils_bit_map.h"
#include "m2m_cloud_utils.h"
#include "iotc_errcode.h"
#include "iotc_svc_dev.h"
#include "security_sess_key.h"
#include "iotc_errcode.h"
#include "iotc_kdf.h"
#include "security_random.h"
#include "iotc_aes.h"
#include "config_authinfo.h"


uint8_t sn1[SESS_SN_LEN] = {0};
uint8_t sn2[SESS_SN_LEN] = {0};


int32_t StationSessKeyGen(M2mCloudContext *ctx, const uint8_t *sn1, uint32_t sn1Len,
    const uint8_t *sn2, uint32_t sn2Len);

const CloudOption *M2mCloudGetPskOption(void)
{
    static const char *sysPsk[] = {STR_URI_PATH_SYS, STR_URI_PATH_PSK};
    static const CloudOption PSK_OPTION = {
        .uri = sysPsk,
        .num = ARRAY_SIZE(sysPsk),
        .opBitMap = UTILS_BIT(CLOUD_OPTION_BIT_SEQ_NUM_ID) |
                    UTILS_BIT(CLOUD_OPTION_BIT_REQ_ID) |
                    UTILS_BIT(CLOUD_OPTION_BIT_DEV_ID),
    };
    return &PSK_OPTION;
}

IotcJson *M2mCloudBuildPskRequest(M2mCloudContext *ctx)
{
    CHECK_RETURN_LOGW(ctx != NULL, NULL, "param invalid");

    int32_t ret = IOTC_OK;
    IotcJson *rootJson = IotcJsonCreate();
    if (rootJson == NULL) {
        IOTC_LOGW("create json error");
        return NULL;
    }

    //生成随机数SN1
    SecurityRandom((uint8_t *)sn1, SESS_SN_LEN);

    uint32_t seq = 0;
    do {
        if (UTILS_IS_BIT_SET(ctx->bitMap, M2M_CLOUD_CTX_BIT_REGISTER)) {
            ret = IotcJsonAddStr2Obj(rootJson, STR_NETINFO_DEVICE_ID, ctx->authInfo.regInfo.devId);
        } else {
            ret = IotcJsonAddStr2Obj(rootJson, STR_NETINFO_DEVICE_ID, ctx->authInfo.loginInfo.devId);
        }
        if (ret != IOTC_OK) {
            IOTC_LOGW("add devid error %d", ret);
            break;
        }
        /* 添加sn1 */
        ret = UtilsJsonAddHexify(rootJson, STR_JSON_SN1, sn1, sizeof(sn1));
        if (ret != IOTC_OK) {
            IOTC_LOGW("add devid error %d", ret);
            break;
        }
        /* 添加seq */
        SecurityRandom((uint8_t *)&seq, 1);
        ret = IotcJsonAddNum2Obj(rootJson, STR_JSON_SEQ, seq);
        if (ret != IOTC_OK) {
            IOTC_LOGW("add devid error %d", ret);
            break;
        }
    } while (0);

    if (ret == IOTC_OK) {
        return rootJson;
    }

    IotcJsonDelete(rootJson);
    return NULL;
}

int32_t M2mCloudParsePskResponse(M2mCloudContext *ctx, const CoapPacket *resp, int32_t *errcode)
{
    CHECK_RETURN_LOGW(ctx != NULL && resp != NULL && errcode != NULL && resp->payload.data != NULL &&
        resp->payload.len != 0, IOTC_ERR_PARAM_INVALID, "invalid param");
    char sn2Str[HEXIFY_LEN(SESSION_KEY_LEN + 1) + 1] = {0};
    char encryptModeStr[130] = {0};  //128字节

    IotcJson *respJson = IotcJsonParseWithLen((const char *)resp->payload.data, resp->payload.len);
    if (respJson == NULL) {
        IOTC_LOGW("create json error");
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    int32_t ret = IOTC_OK;
    
    ret = UtilsJsonGetNum(respJson, STR_ERRCODE, errcode);
    if (ret != IOTC_OK) {
        IOTC_LOGE("json get errcode error %d", ret);
        IotcJsonDelete(respJson);
        return ret;
    }
    if (*errcode != CLOUD_ERRCODE_OK) {
        IOTC_LOGE("cloud psk error %d", *errcode);
        IotcJsonDelete(respJson);
        return IOTC_ERROR;
    }

    // sn2
    ret = UtilsJsonGetString(respJson, STR_JSON_SN2, (char *)sn2Str, sizeof(sn2Str));
    if (ret != IOTC_OK) {
        IOTC_LOGE("json get sn error %d", ret);
        IotcJsonDelete(respJson);
        return ret;
    }

    ret = UtilsJsonGetString(respJson, STR_JSON_ENCRYPT, (char *)encryptModeStr, sizeof(encryptModeStr));
    if (ret != IOTC_OK) {
        IOTC_LOGE("json get encryptModeStr error %d", ret);
        IotcJsonDelete(respJson);
        return ret;
    }

    if (!strncmp(encryptModeStr, STR_JSON_PLAINTEXT, sizeof(STR_JSON_PLAINTEXT))) {
        ctx->pskInfo.encrypt = true;
    } else {
        ctx->pskInfo.encrypt = false;
    }

    UtilsUnhexify(sn2Str, strlen(sn2Str), sn2, sizeof(sn2));
    // 根据sn1 sn2 生成PSK
    StationSessKeyGen(ctx, sn1, SESS_SN_LEN, sn2, SESS_SN_LEN);

    return IOTC_OK;
}

int32_t StationSessKeyGen(M2mCloudContext *ctx, const uint8_t *sn1, uint32_t sn1Len,
    const uint8_t *sn2, uint32_t sn2Len)
{
    CHECK_RETURN_LOGE((sn1 != NULL) && (sn1Len > 0) && (sn2 != NULL) && (sn2Len > 0),
        IOTC_ERR_PARAM_INVALID, "param invalid, sn1Len:%u, sn2Len:%u", sn1Len, sn2Len);
    int32_t ret = memcpy_s(ctx->pskInfo.salt, RAND_SN_LEN, sn1, sn1Len);
    CHECK_RETURN_LOGE(ret == EOK, IOTC_ERR_SECUREC_MEMCPY, "cpy sn1 err:%d", ret);
    ret = memcpy_s(ctx->pskInfo.salt + RAND_SN_LEN, RAND_SN_LEN, sn2, sn2Len);
    CHECK_RETURN_LOGE(ret == EOK, IOTC_ERR_SECUREC_MEMCPY, "cpy sn2 err:%d", ret);
    
    IotcPbkdf2HmacParam param = {
        .md = IOTC_MD_SHA256,
        .password = ((UTILS_IS_BIT_SET(ctx->bitMap, M2M_CLOUD_CTX_BIT_REGISTER)) ? 
            ctx->authInfo.regInfo.psk : ctx->authInfo.loginInfo.psk),
 	    .passwordLen = sizeof(((UTILS_IS_BIT_SET(ctx->bitMap, M2M_CLOUD_CTX_BIT_REGISTER)) ? 
            ctx->authInfo.regInfo.psk : ctx->authInfo.loginInfo.psk)),
        .salt = ctx->pskInfo.salt,
        .saltLen = SALT_LEN,
        .iterCount = ITER_TIMES
    };
    
    ret = IotcPkcs5Pbkdf2Hmac(&param, ctx->pskInfo.key, SESSION_KEY_LEN);

    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "ble sess key gen err:%d", ret);
    ctx->pskInfo.pskFinish = true;
    return IOTC_OK;
}

// 报文加密
int32_t PskEncryptData(M2mCloudContext *ctx, const uint8_t *data, uint32_t dataLen,
    uint8_t **encData, uint32_t *encDataLen)
{
    if (ctx == NULL || (ctx->pskInfo.pskFinish == false) || (data == NULL) || (dataLen == 0) || \
        (encData == NULL) || (encDataLen == NULL)) {
        IOTC_LOGE("psk encrypt data param err, dataLen:%u", dataLen);
        return IOTC_ERR_PARAM_INVALID;
    }

    uint32_t outDataLen = dataLen + dataLen % 16 + 1 + 29;
    uint8_t *outData = (uint8_t *)IotcMalloc(outDataLen);
    if (outData == NULL) {
        IOTC_LOGE("psk encrypt malloc(%u) err", outDataLen);
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(outData, outDataLen, 0, outDataLen);

    IotcAesCbcParam param = {
        .mode = ADAPTER_PADDING_PKCS7,
        .key = ctx->pskInfo.key,
        .keyLen = 16,
        .iv = ctx->pskInfo.key + 16,
        .data = data,
        .dataLen = dataLen,
    };

    int32_t ret = IotcAesCbcEncrypt(&param, outData, &outDataLen);
    if (ret != IOTC_OK) {
        IOTC_LOGE("psk encrypt err:%d", ret);
        IotcFree(outData);
        return ret;
    }

    *encData = outData;
    *encDataLen = outDataLen;
    return IOTC_OK;
}

int32_t PskDecryptData(M2mCloudContext *ctx, const uint8_t *data, uint32_t dataLen,
    uint8_t **decData, uint32_t *decDataLen)
{
    if ((ctx == NULL) || (ctx->pskInfo.pskFinish == false)  || (data == NULL) || \
        (dataLen == 0) || (decData == NULL) || (decDataLen == NULL)) {
        IOTC_LOGE("psk decrypt data param err, dataLen:%u", dataLen);
        return IOTC_ERR_PARAM_INVALID;
    }

    uint32_t outDataLen = dataLen;
    uint8_t *outData = (uint8_t *)IotcMalloc(outDataLen);
    if (outData == NULL) {
        IOTC_LOGE("psk decrypt malloc(%u) err", outDataLen);
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(outData, outDataLen, 0, outDataLen);

    IotcAesCbcParam param = {
        .mode = ADAPTER_PADDING_PKCS7,
        .key = ctx->pskInfo.key,
        .keyLen = 16,
        .iv = ctx->pskInfo.key + 16,
        .data = data,
        .dataLen = dataLen,
    };
    int32_t ret = IotcAesCbcDecrypt(&param, outData, &outDataLen);

    outData[outDataLen] = '\0';
    if (ret != IOTC_OK) {
        IOTC_LOGE("psk decrypt err:%d", ret);
        IotcFree(outData);
        return ret;
    }

    *decData = outData;
    *decDataLen = outDataLen;
    return IOTC_OK;
}


int32_t StationSessCalHmac(M2mCloudContext *ctx, const uint8_t *data, uint32_t dataLen,
    uint8_t *calHmac, uint32_t calHmacLen)
{
    CHECK_RETURN_LOGE((ctx != NULL) && (data != NULL) && (dataLen > 0) && (calHmac != NULL) &&
        (calHmacLen >= SESS_HMAC_LEN), IOTC_ERR_PARAM_INVALID, "param invalid, dataLen:%u, calHmacLen:%u",
        dataLen, calHmacLen);

    uint8_t hmacKey[SESS_HMAC_LEN] = { 0 };

    IotcPbkdf2HmacParam param = {
        .md = IOTC_MD_SHA256,
        .password = ctx->pskInfo.key,
        .passwordLen = SALT_LEN,
        .salt = ctx->pskInfo.salt,
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
