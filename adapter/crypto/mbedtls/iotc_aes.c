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
#include "crypto_mbedtls_common.h"
#include "iotc_aes.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "securec.h"
#include "mbedtls/gcm.h"
#include "mbedtls/aes.h"
#include "mbedtls/cipher.h"
#include "mbedtls/ccm.h"

#define BITS_PER_BYTES 8

static bool IsAesGcmParamValid(const IotcAesGcmParam *param)
{
    if (param == NULL) {
        IOTC_LOGW("invalid param");
        return false;
    }

    if ((param->iv == NULL) || (param->ivLen == 0)) {
        IOTC_LOGW("invalid iv %u", param->ivLen);
        return false;
    }

    if ((param->key == NULL) ||
        ((param->keyLen != IOTC_AES_128_KEY_BYTE_LEN) &&
        (param->keyLen != IOTC_AES_192_KEY_BYTE_LEN) &&
        (param->keyLen != IOTC_AES_256_KEY_BYTE_LEN))) {
        IOTC_LOGW("invalid key %u", param->keyLen);
        return false;
    }

    if ((param->data == NULL) || (param->dataLen == 0)) {
        IOTC_LOGW("invalid data");
        return false;
    }

    if ((param->addLen != 0) && (param->add == NULL)) {
        IOTC_LOGW("invalid add");
        return false;
    }

    return true;
}

int32_t IotcAesGcmEncrypt(const IotcAesGcmParam *param, uint8_t *tag, uint32_t tagLen, uint8_t *buf)
{
    if (!IsAesGcmParamValid(param)) {
        return IOTC_ERR_PARAM_INVALID;
    }

    mbedtls_gcm_context context;
    mbedtls_gcm_init(&context);
    int32_t ret;
    do {
        ret = mbedtls_gcm_setkey(&context, MBEDTLS_CIPHER_ID_AES,
            param->key, param->keyLen * BITS_PER_BYTES);
        if (ret != 0) {
            IOTC_LOGW("set key err %d", ret);
            break;
        }

        ret = mbedtls_gcm_crypt_and_tag(&context, MBEDTLS_GCM_ENCRYPT, param->dataLen,
            param->iv, param->ivLen, param->add, param->addLen, param->data,
            buf, tagLen, tag);
        if (ret != 0) {
            IOTC_LOGW("encrypt err %d", ret);
        }
    } while (false);

    mbedtls_gcm_free(&context);
    return ret == 0 ? IOTC_OK : IOTC_ADAPTER_CRYPTO_ERR_AES_GCM_ENC;
}

int32_t IotcAesGcmDecrypt(const IotcAesGcmParam *param, const uint8_t *tag, uint32_t tagLen, uint8_t *buf)
{
    if (!IsAesGcmParamValid(param)) {
        return IOTC_ERR_PARAM_INVALID;
    }

    mbedtls_gcm_context context;
    mbedtls_gcm_init(&context);
    int32_t ret;
    do {
        ret = mbedtls_gcm_setkey(&context, MBEDTLS_CIPHER_ID_AES,
            param->key, param->keyLen * BITS_PER_BYTES);
        if (ret != 0) {
            IOTC_LOGW("set key err %d", ret);
            break;
        }

        ret = mbedtls_gcm_auth_decrypt(&context, param->dataLen, param->iv, param->ivLen,
            param->add, param->addLen, tag, tagLen,
            param->data, buf);
        if (ret != 0) {
            IOTC_LOGW("decrypt err %d", ret);
            break;
        }
    } while (false);

    mbedtls_gcm_free(&context);
    return ret == 0 ? IOTC_OK : IOTC_ADAPTER_CRYPTO_ERR_AES_GCM_DEC;
}

static mbedtls_cipher_padding_t GetMbedtlsPaddingMode(IotcPaddingMode mode)
{
    switch (mode) {
        case ADAPTER_PADDING_PKCS7:
            return MBEDTLS_PADDING_PKCS7;
        case ADAPTER_PADDING_ONE_AND_ZEROS:
            return MBEDTLS_PADDING_ONE_AND_ZEROS;
        case ADAPTER_PADDING_ZEROS_AND_LEN:
            return MBEDTLS_PADDING_ZEROS_AND_LEN;
        case ADAPTER_PADDING_ZEROS:
            return MBEDTLS_PADDING_ZEROS;
        default:
            return MBEDTLS_PADDING_NONE;
    }
}

static bool IsAesCbcParamValid(const IotcAesCbcParam *param)
{
    if (param == NULL) {
        IOTC_LOGW("invalid param");
        return false;
    }

    if (param->iv == NULL) {
        IOTC_LOGW("invalid iv");
        return false;
    }

    if ((param->key == NULL) ||
        ((param->keyLen != IOTC_AES_128_KEY_BYTE_LEN) &&
        (param->keyLen != IOTC_AES_192_KEY_BYTE_LEN) &&
        (param->keyLen != IOTC_AES_256_KEY_BYTE_LEN))) {
        IOTC_LOGW("invalid key %u", param->keyLen);
        return false;
    }

    if ((param->data == NULL) || (param->dataLen == 0) ||
        /* cbc加解密要求数据长度为16的倍数 */
        ((param->mode == ADAPTER_PADDING_NONE) && (param->dataLen % 16 != 0))) {
        IOTC_LOGW("invalid data %u", param->dataLen);
        return false;
    }

    return true;
}

static mbedtls_cipher_type_t GetMbedtlsCbcCipherType(uint32_t keyLen)
{
    switch (keyLen) {
        case IOTC_AES_128_KEY_BYTE_LEN:
            return MBEDTLS_CIPHER_AES_128_CBC;
        case IOTC_AES_192_KEY_BYTE_LEN:
            return MBEDTLS_CIPHER_AES_192_CBC;
        case IOTC_AES_256_KEY_BYTE_LEN:
            return MBEDTLS_CIPHER_AES_256_CBC;
        default:
            return MBEDTLS_CIPHER_NONE;
    }
}

static int32_t AesCbcCrypt(const IotcAesCbcParam *param, int32_t mode, uint8_t *buf, uint32_t *bufLen)
{
    mbedtls_cipher_context_t ctx;
    mbedtls_cipher_init(&ctx);

    int32_t ret = mbedtls_cipher_setup(&ctx, mbedtls_cipher_info_from_type(GetMbedtlsCbcCipherType(param->keyLen)));
    if (ret != 0) {
        IOTC_LOGW("cipher setup error [-0x%04x]", -ret);
        return IOTC_ADAPTER_CRYPTO_ERR_AES_CBC_SETUP;
    }

    if (mode == MBEDTLS_DECRYPT) {
        ret = mbedtls_cipher_setkey(&ctx, param->key, param->keyLen * BITS_PER_BYTES, MBEDTLS_DECRYPT);
    } else {
        ret = mbedtls_cipher_setkey(&ctx, param->key, param->keyLen * BITS_PER_BYTES, MBEDTLS_ENCRYPT);
    }
    if (ret != 0) {
        IOTC_LOGW("set key err [-0x%04x]", -ret);
        return IOTC_ADAPTER_CRYPTO_ERR_AES_CBC_SETUP;
    }

    ret = mbedtls_cipher_set_padding_mode(&ctx, GetMbedtlsPaddingMode(param->mode));
    if (ret != 0) {
        IOTC_LOGW("set padding mode err [-0x%04x]", -ret);
        return IOTC_ADAPTER_CRYPTO_ERR_AES_CBC_SETUP;
    }

    size_t bufLenTmp = *bufLen;
    ret = mbedtls_cipher_crypt(&ctx, param->iv, IOTC_AES_CBC_IV_LEN, param->data, param->dataLen, buf, &bufLenTmp);
    if (ret != 0) {
        IOTC_LOGW("crypt err [-0x%04x]", -ret);
        return (mode == MBEDTLS_DECRYPT) ? IOTC_ADAPTER_CRYPTO_ERR_AES_CBC_ENC : IOTC_ADAPTER_CRYPTO_ERR_AES_CBC_DEC;
    }
    *bufLen = bufLenTmp;

    return IOTC_OK;
}

int32_t IotcAesCbcEncrypt(const IotcAesCbcParam *param, uint8_t *buf,  uint32_t *bufLen)
{
    if (!IsAesCbcParamValid(param) || buf == NULL || bufLen == NULL || *bufLen == 0) {
        return IOTC_ERR_PARAM_INVALID;
    }
    return AesCbcCrypt(param, MBEDTLS_DECRYPT, buf, bufLen);
}

int32_t IotcAesCbcDecrypt(const IotcAesCbcParam *param, uint8_t *buf, uint32_t *bufLen)
{
    if (!IsAesCbcParamValid(param) || buf == NULL || bufLen == NULL || *bufLen == 0) {
        return IOTC_ERR_PARAM_INVALID;
    }
    return AesCbcCrypt(param, MBEDTLS_ENCRYPT, buf, bufLen);
}

static bool IsAesCcmParamValid(const IotcAesCcmParam *param)
{
    if (param == NULL) {
        IOTC_LOGW("invalid param");
        return false;
    }

    if ((param->iv == NULL) || (param->ivLen == 0)) {
        IOTC_LOGW("invalid iv");
        return false;
    }

    if ((param->key == NULL) ||
        ((param->keyLen != IOTC_AES_128_KEY_BYTE_LEN) &&
        (param->keyLen != IOTC_AES_192_KEY_BYTE_LEN) &&
        (param->keyLen != IOTC_AES_256_KEY_BYTE_LEN))) {
        IOTC_LOGW("invalid key %u", param->keyLen);
        return false;
    }

    if ((param->data == NULL) || (param->dataLen == 0)) {
        IOTC_LOGW("invalid data");
        return false;
    }

    if ((param->addLen != 0) && (param->add == NULL)) {
        IOTC_LOGW("invalid add");
        return false;
    }

    return true;
}

int32_t IotcAesCcmDecrypt(const IotcAesCcmParam *param, const uint8_t *tag, uint32_t tagLen, uint8_t *buf)
{
    if (!IsAesCcmParamValid(param)) {
        return IOTC_ERR_PARAM_INVALID;
    }
#if IOTC_CONF_ADAPTER_MBEDTLS_CCM_SUPPORT
    mbedtls_ccm_context context;
    mbedtls_ccm_init(&context);

    int32_t ret;
    do {
        ret = mbedtls_ccm_setkey(&context, MBEDTLS_CIPHER_ID_AES,
            param->key, param->keyLen * BITS_PER_BYTES);
        if (ret != 0) {
            IOTC_LOGW("set key err [-0x%04x]", -ret);
            break;
        }

        ret = mbedtls_ccm_auth_decrypt(&context, param->dataLen, param->iv, param->ivLen,
            param->add, param->addLen, param->data, buf,
            tag, tagLen);
        if (ret != 0) {
            IOTC_LOGW("decrypt err [-0x%04x]", -ret);
            break;
        }
    } while (0);

    mbedtls_ccm_free(&context);
    return ret == 0 ? IOTC_OK : IOTC_ADAPTER_CRYPTO_ERR_AES_CCM_DEC;
#endif
    return IOTC_ERR_NOT_SUPPORT;
}

int32_t IotcAesCcmEncrypt(const IotcAesCcmParam *param, uint8_t *tag, uint32_t tagLen, uint8_t *buf)
{
    if (!IsAesCcmParamValid(param)) {
        return IOTC_ERR_PARAM_INVALID;
    }
#if IOTC_CONF_ADAPTER_MBEDTLS_CCM_SUPPORT
    mbedtls_ccm_context context;
    mbedtls_ccm_init(&context);
    int32_t ret;
    do {
        ret = mbedtls_ccm_setkey(&context, MBEDTLS_CIPHER_ID_AES,
            param->key, param->keyLen * BITS_PER_BYTES);
        if (ret != 0) {
            IOTC_LOGW("set key err [-0x%04x]", -ret);
            break;
        }

        ret = mbedtls_ccm_encrypt_and_tag(&context, param->dataLen,
            param->iv, param->ivLen,
            param->add, param->addLen,
            param->data, buf,
            tag, tagLen);
        if (ret != 0) {
            IOTC_LOGW("encrypt err [-0x%04x]", -ret);
            break;
        }
    } while (0);

    mbedtls_ccm_free(&context);
    return ret == 0 ? IOTC_OK : IOTC_ADAPTER_CRYPTO_ERR_AES_CCM_ENC;
#endif
    return IOTC_ERR_NOT_SUPPORT;
}