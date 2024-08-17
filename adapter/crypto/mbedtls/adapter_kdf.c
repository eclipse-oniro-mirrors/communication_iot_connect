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
#include "adapter_kdf.h"
#include "iotc_errcode.h"
#include "mbedtls/md.h"
#include "mbedtls/hkdf.h"
#include "mbedtls/pkcs5.h"
#include "adapter_log.h"

static mbedtls_md_type_t GetMbedtlsMdType(IotcMdType type)
{
    switch (type) {
        case IOTC_MD_NONE:
            return MBEDTLS_MD_NONE;
        case IOTC_MD_SHA256:
            return MBEDTLS_MD_SHA256;
        case IOTC_MD_SHA384:
            return MBEDTLS_MD_SHA384;
        case IOTC_MD_SHA512:
            return MBEDTLS_MD_SHA512;
        default:
            ADAPTER_LOGW("invalid mode");
            return MBEDTLS_MD_NONE;
    }
}

int32_t IotcPkcs5Pbkdf2Hmac(const IotcPbkdf2HmacParam *param, uint8_t *key, uint32_t keyLen)
{
    if ((param == NULL) || (param->password == NULL) || (param->passwordLen == 0) ||
        (param->salt == NULL) || (param->saltLen == 0) || (key == NULL) || (keyLen == 0)) {
        ADAPTER_LOGW("invalid param\r\n");
        return IOTC_ERR_PARAM_INVALID;
    }

    mbedtls_md_context_t context;
    mbedtls_md_init(&context);
    int32_t ret;

    do {
        /* 数字1表示使用HMAC */
        ret = mbedtls_md_setup(&context, mbedtls_md_info_from_type(GetMbedtlsMdType(param->md)), 1);
        if (ret != 0) {
            ADAPTER_LOGW("md setup error [-0x%04x]", -ret);
            break;
        }

        ret = mbedtls_pkcs5_pbkdf2_hmac(&context, param->password, param->passwordLen,
            param->salt, param->saltLen, param->iterCount, keyLen, key);
        if (ret != 0) {
            ADAPTER_LOGW("mbedtls_pkcs5_pbkdf2_hmac error [-0x%04x]", -ret);
            break;
        }
    } while (0);

    mbedtls_md_free(&context);
    return ret == 0? IOTC_OK : IOTC_ADAPTER_CRYPTO_ERR_PKCS5_PBKDF2_HMAC;
}

int32_t IotcHkdf(const IotcHkdfParam *param, uint8_t *key, uint32_t keyLen)
{
    if ((param == NULL) || (param->material == NULL) || (key == NULL) ||
        (param->materialLen == 0) || (keyLen == 0)) {
        ADAPTER_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    int32_t ret = mbedtls_hkdf(mbedtls_md_info_from_type(GetMbedtlsMdType(param->md)), param->salt, param->saltLen,
        param->material, param->materialLen, param->info, param->infoLen, key, keyLen);
    if (ret != 0) {
        ADAPTER_LOGW("hkdf error [-0x%04x]", -ret);
        return IOTC_ADAPTER_CRYPTO_ERR_HKDF;
    }

    return IOTC_OK;
}