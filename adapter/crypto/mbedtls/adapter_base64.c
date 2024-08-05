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
#include "adapter_base64.h"
#include "mbedtls/base64.h"
#include "iotc_errcode.h"
#include "adapter_log.h"

int32_t AdapterBase64Encode(const uint8_t *inData, uint32_t inLen, uint8_t *outData, uint32_t *outLen)
{
    if ((inData == NULL) || (inLen == 0) || (outLen == NULL)) {
        ADAPTER_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    size_t outSize = *outLen;
    int32_t ret;

    if ((outData == NULL) || (*outLen == 0)) {
        ret = mbedtls_base64_encode(NULL, 0, &outSize, inData, inLen);
        if ((ret != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL) || (outSize == 0)) {
            ADAPTER_LOGW("get base64 max size error [-0x%04x]", -ret);
            return IOTC_ERR_PARAM_INVALID;
        }
        *outLen = outSize;
        return IOTC_OK;
    }

    ret = mbedtls_base64_encode(outData, outSize, &outSize, inData, inLen);
    if (ret != 0) {
        ADAPTER_LOGW("base64 encode error [-0x%04x]", -ret);
        return IOTC_ADAPTER_CRYPTO_ERR_BASE64_ENC;
    }
    *outLen = outSize;

    return IOTC_OK;
}

int AdapterBase64Decode(const uint8_t *inData, uint32_t inLen, uint8_t *outData, uint32_t *outLen)
{
    if ((inData == NULL) || (inLen == 0) || (outLen == NULL)) {
        ADAPTER_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    size_t outSize = *outLen;
    int32_t ret;

    if ((outData == NULL) || (*outLen == 0)) {
        ret = mbedtls_base64_decode(NULL, 0, &outSize, inData, inLen);
        if ((ret != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL) || (outSize == 0)) {
            ADAPTER_LOGW("get base64 max size error [-0x%04x]", -ret);
            return IOTC_ERR_PARAM_INVALID;
        }
        *outLen = outSize;
        return IOTC_OK;
    }

    ret = mbedtls_base64_decode(outData, outSize, &outSize, inData, inLen);
    if (ret != 0) {
        ADAPTER_LOGW("base64 decode error [-0x%04x]", -ret);
        return ret;
    }
    *outLen = outSize;

    return IOTC_OK;
}