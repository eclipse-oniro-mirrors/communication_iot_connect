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
#include "linklayer_encrypt_sesskey.h"
#include "linklayer_service.h"
#include "ble_linklayer.h"
#include "utils_assert.h"
#include "adapter_mem.h"
#include "securec.h"
#include "iotc_errcode.h"

#define BITS_PER_BYTE 8

#define SESS_ID_LEN     32
#define SESS_HMAC_LEN   32
#define SESS_IV_LEN     12
#define SESS_TAG_LEN    16

static LinkLayerSessKeyCallback g_sessKeyCb = { 0 };

int32_t LinkLayerRegisterSessKeyCb(LinkLayerSessKeyCallback cb)
{
    int32_t ret = memcpy_s(&g_sessKeyCb, sizeof(LinkLayerSessKeyCallback), &cb, sizeof(LinkLayerSessKeyCallback));
    CHECK_RETURN(ret == EOK, IOTC_ERR_SECUREC_MEMCPY);
    return IOTC_OK;
}

static int32_t SessKeyEncrypt(const uint8_t *data, uint32_t dataLen, uint8_t *outBuff, uint32_t outBuffLen)
{
    CHECK_RETURN_LOGE(g_sessKeyCb.sessKeyEncrypt != NULL, IOTC_ERR_CALLBACK_NULL, "enc cb null");
    return g_sessKeyCb.sessKeyEncrypt(data, dataLen, outBuff, outBuffLen);
}

static int32_t SessKeyDecrypt(const uint8_t *data, uint32_t dataLen, uint8_t *outBuff, uint32_t outBuffLen)
{
    CHECK_RETURN_LOGE(g_sessKeyCb.sessKeyDecrypt != NULL, IOTC_ERR_CALLBACK_NULL, "dec cb null");
    return g_sessKeyCb.sessKeyDecrypt(data, dataLen, outBuff, outBuffLen);
}

static int32_t SessKeyCalHmac(const uint8_t *data, uint32_t dataLen, uint8_t *calHmac, uint32_t calHmacLen)
{
    CHECK_RETURN_LOGE(g_sessKeyCb.sessKeyCalHmac != NULL, IOTC_ERR_CALLBACK_NULL, "cal hmac cb null");
    return g_sessKeyCb.sessKeyCalHmac(data, dataLen, calHmac, calHmacLen);
}

static int32_t SessKeyCheckHmac(const uint8_t *data, uint32_t dataLen, const uint8_t *hmacBuf, uint32_t hmacLen)
{
    CHECK_RETURN_LOGE(g_sessKeyCb.sessKeyCheckHmac != NULL, IOTC_ERR_CALLBACK_NULL, "check hmac cb null");
    return g_sessKeyCb.sessKeyCheckHmac(data, dataLen, hmacBuf, hmacLen);
}

/* sessKey仅加密payload中的body, 不加密整个payload */
int32_t LinkLayerSessKeyEncrypt(const uint8_t *data, uint32_t dataLen, uint8_t **outData, uint32_t *outDataLen)
{
    CHECK_RETURN((data != NULL) && (dataLen > 0), IOTC_ERR_PARAM_INVALID);
    CHECK_RETURN((outData != NULL) && (outDataLen != NULL), IOTC_ERR_PARAM_INVALID);

    BtCmdParam cmdParam = { 0 };
    int32_t ret = DecodeCmdData(data, dataLen, &cmdParam);
    CHECK_RETURN(ret == IOTC_OK, ret);
    CHECK_RETURN_LOGE((cmdParam.request != NULL) && (cmdParam.requestLen > 0) && (cmdParam.requestLen < dataLen),
        IOTC_CORE_BLE_LL_ERR_BODY, "body err, len:%u", cmdParam.requestLen);

    uint32_t svcHeaderLen = dataLen - cmdParam.requestLen;
    /* body加密后长度会增加SESS_TAG_LEN */
    uint32_t encBodyLen = cmdParam.requestLen + SESS_TAG_LEN;
    /* iv + encbody + sessId 组成新的body */
    uint32_t totalBodyLen = SESS_IV_LEN + encBodyLen + SESS_ID_LEN;
    uint32_t outLen = svcHeaderLen + totalBodyLen + SESS_HMAC_LEN;
    uint8_t *out = (uint8_t *)IotcCalloc(outLen, sizeof(uint8_t));
    CHECK_RETURN_LOGE(out != NULL, IOTC_ADAPTER_MEM_ERR_CALLOC, "out calloc err");

    /* 设置新的svcHeader */
    ret = memcpy_s(out, outLen, data, svcHeaderLen);
    if (ret != EOK) {
        ret = IOTC_ERR_SECUREC_MEMCPY;
        goto ERROR_EXIT;
    }
    /* 重新设置svcHeader中的body长度, 2字节 */
    uint32_t pos = svcHeaderLen - SVC_PAYLOAD_LEN_LEN;
    out[pos++] = totalBodyLen & 0xFF;
    out[pos++] = (totalBodyLen >> BITS_PER_BYTE) & 0xFF;
    /* 加密获取新的body = iv + encbody + sessId */
    ret = SessKeyEncrypt(cmdParam.request, cmdParam.requestLen,
        out + svcHeaderLen, outLen - svcHeaderLen - SESS_HMAC_LEN);
    if (ret != IOTC_OK) {
        goto ERROR_EXIT;
    }

    /* 计算整个payload的hmac */
    ret = SessKeyCalHmac(out, outLen - SESS_HMAC_LEN, out + outLen - SESS_HMAC_LEN, SESS_HMAC_LEN);
    if (ret != IOTC_OK) {
        goto ERROR_EXIT;
    }

    *outData = out;
    *outDataLen = outLen;
    return IOTC_OK;

ERROR_EXIT:
    IotcFree(out);
    return ret;
}

int32_t LinkLayerSessKeyDecrypt(uint8_t *data, uint32_t *dataLen)
{
    CHECK_RETURN((data != NULL) && (dataLen != NULL), IOTC_ERR_PARAM_INVALID);
    CHECK_RETURN_LOGE(*dataLen > SESS_HMAC_LEN, IOTC_ERR_PARAM_INVALID, "sess data len:%u err", dataLen);

    int32_t ret = SessKeyCheckHmac(data, *dataLen - SESS_HMAC_LEN,
        data + *dataLen - SESS_HMAC_LEN, SESS_HMAC_LEN);
    CHECK_RETURN(ret == IOTC_OK, ret);

    BtCmdParam cmdParam = { 0 };
    ret = DecodeCmdData(data, *dataLen - SESS_HMAC_LEN, &cmdParam);
    CHECK_RETURN(ret == IOTC_OK, ret);
    CHECK_RETURN_LOGE(cmdParam.request != NULL, IOTC_CORE_BLE_LL_ERR_BODY, "body err");
    CHECK_RETURN_LOGE((cmdParam.requestLen > SESS_TAG_LEN + SESS_IV_LEN + SESS_ID_LEN) &&
        (cmdParam.requestLen < *dataLen - SESS_HMAC_LEN), IOTC_CORE_BLE_LL_ERR_BODY,
        "body len:%u err", cmdParam.requestLen);

    uint32_t svcHeaderLen = *dataLen - SESS_HMAC_LEN - cmdParam.requestLen;
    uint32_t decBodyLen = cmdParam.requestLen - SESS_TAG_LEN - SESS_IV_LEN - SESS_ID_LEN;
    uint32_t outLen = svcHeaderLen + decBodyLen;
    uint8_t *out = (uint8_t *)IotcCalloc(outLen, sizeof(uint8_t));
    CHECK_RETURN_LOGE(out != NULL, IOTC_ADAPTER_MEM_ERR_CALLOC, "out calloc err");

    /* 设置新的svcHeader */
    ret = memcpy_s(out, outLen, data, svcHeaderLen);
    if (ret != EOK) {
        ret = IOTC_ERR_SECUREC_MEMCPY;
        goto EXIT;
    }
    /* 重新设置svcHeader中的body长度, 2字节 */
    uint32_t pos = svcHeaderLen - SVC_PAYLOAD_LEN_LEN;
    out[pos++] = decBodyLen & 0xFF;
    out[pos++] = (decBodyLen >> BITS_PER_BYTE) & 0xFF;
    /* 解密 */
    ret = SessKeyDecrypt(cmdParam.request, cmdParam.requestLen, out + svcHeaderLen, outLen - svcHeaderLen);
    if (ret != IOTC_OK) {
        goto EXIT;
    }

    ret = memcpy_s(data, *dataLen, out, outLen);
    if (ret != EOK) {
        ret = IOTC_ERR_SECUREC_MEMCPY;
        goto EXIT;
    }
    if (outLen < *dataLen) {
        data[outLen] = 0;
    }
    *dataLen = outLen;
    ret = IOTC_OK;

EXIT:
    IotcFree(out);
    return ret;
}

bool LinkLayerSessKeyExist(void)
{
    CHECK_RETURN(g_sessKeyCb.sessKeyCheckExist != NULL, false);
    return g_sessKeyCb.sessKeyCheckExist();
}