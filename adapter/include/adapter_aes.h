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
#ifndef IOTC_AES_H
#define IOTC_AES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IOTC_AES_CBC_IV_LEN      16 /**< AES-CBC 初始向量长度 */
#define IOTC_AES_GCM_TAG_MAX_LEN 16 /**< AES-GCM TAG最大长度 */
#define IOTC_AES_GCM_TAG_MIN_LEN 4  /**< AES-GCM TAG最短长度 */
#define IOTC_AES_CCM_IV_MAX_LEN  13 /**< AES-CCM 初始向量最大长度 */
#define IOTC_AES_CCM_IV_MIN_LEN  7  /**< AES-CCM 初始向量最短长度 */
#define IOTC_AES_CCM_TAG_MAX_LEN 16 /**< AES-CCM TAG最大长度 */
#define IOTC_AES_CCM_TAG_MIN_LEN 4  /**< AES-CCM TAG最短长度 */

/** AES加解密密钥长度 */
enum IotcAesKeyByteLen {
    IOTC_AES_128_KEY_BYTE_LEN = 16, /**< AES-128 密钥字节数 */
    IOTC_AES_192_KEY_BYTE_LEN = 24, /**< AES-192 密钥字节数 */
    IOTC_AES_256_KEY_BYTE_LEN = 32, /**< AES-256 密钥字节数 */
};

/** AES-GCM加解密所需的数据 */
typedef struct {
    const uint8_t *key;  /**< 加解密密钥，有效可读长度至少为keyLen */
    uint32_t keyLen;     /**< 密钥长度，应该是IotcAesKeyByteLen中的数值 */
    const uint8_t *iv;   /**< 初始向量，有效可读长度至少为ivLen */
    uint32_t ivLen;      /**< 初始向量长度 */
    const uint8_t *add;  /**< 附加值，有效可读长度至少为addLen */
    uint32_t addLen;     /**< 附加值长度，可以为0 */
    const uint8_t *data; /**< 待加解密的数据，有效可读长度至少为dataLen */
    uint32_t dataLen;    /**< 数据长度 */
} IotcAesGcmParam;

/** 填充模式 */
typedef enum {
    ADAPTER_PADDING_PKCS7 = 0,     /**< PKCS7填充模式，使用填充长度进行填充 */
    ADAPTER_PADDING_ONE_AND_ZEROS, /**< 使用0x80 0x00 ... 0x00进行填充 */
    ADAPTER_PADDING_ZEROS_AND_LEN, /**< 使用0x00 ... 0x00 0xll进行填充，其中0xll为填充长度 */
    ADAPTER_PADDING_ZEROS,         /**< 使用0x00进行填充 */
    ADAPTER_PADDING_NONE,          /**< 不填充 */
} IotcPaddingMode;

/** AES-CBC加解密所需数据 */
typedef struct {
    IotcPaddingMode mode;    /**< 填充模式，IotcPaddingMode */
    const uint8_t *key;      /**< 加解密密钥，有效可读长度至少为keyLen */
    uint32_t keyLen;         /**< 密钥长度，应该是IotcAesCryptKeyByteLen中的数值 */
    const uint8_t *iv;       /**< 初始向量，有效可读长度至少为16 */
    const uint8_t *data;     /**< 待加解密的数据，有效可读长度至少为dataLen */
    uint32_t dataLen;        /**< 数据长度 */
} IotcAesCbcParam;

/** AES-CCM加解密所需的数据 */
typedef struct {
    const uint8_t *key;  /**< 加解密密钥，长度为keyLen */
    uint32_t keyLen;     /**< 密钥长度 */
    const uint8_t *iv;   /**< 加解密向量，长度为ivLen */
    uint32_t ivLen;      /**< 加解密向量长度 */
    const uint8_t *add;  /**< 附加值，长度为addLen */
    uint32_t addLen;     /**< 附加值长度，为0 */
    const uint8_t *data; /**< 待解密的数据，长度为dataLen */
    uint32_t dataLen;    /**< 待解密数据长度 */
} IotcAesCcmParam;

/**
 * @brief AES-GCM加密
 *
 * @param param [IN] AES-GCM加密必要的参数
 * @param tag [OUT] 消息验证码输出缓冲区，可写长度至少为tagLen
 * @param tagLen [IN] 消息验证码缓冲区长度，范围为[4,16]
 * @param buf [OUT] 加密数据输出缓冲区，可写长度至少为param->dataLen
 * @return 0成功，非0失败
 */
int32_t IotcAesGcmEncrypt(const IotcAesGcmParam *param, uint8_t *tag, uint32_t tagLen, uint8_t *buf);

/**
 * @brief AES-GCM解密并校验消息验证码
 *
 * @param param [IN] AES-GCM解密必要的参数
 * @param tag [OUT] 消息验证码，可读长度至少为tagLen
 * @param tagLen [IN] 消息验证码长度，范围应为[4,16]
 * @param buf [OUT] 解密密数据输出缓冲区，可写长度至少为param->dataLen
 * @return 0成功，非0失败
 */
int32_t IotcAesGcmDecrypt(const IotcAesGcmParam *param, const uint8_t *tag, uint32_t tagLen, uint8_t *buf);

/**
 * @brief AES-CBC加密
 *
 * @param param [IN] AES-CBC加密必要的参数
 * @param buf [OUT] 加密数据输出缓冲区，可写长度至少为bufLen
 * @param bufLen [IN/OUT] 输入为输出缓冲区长度，输出为增加padding后的数据长度
 * @return 0成功，非0失败
 */
int32_t IotcAesCbcEncrypt(const IotcAesCbcParam *param, uint8_t *buf, uint32_t *bufLen);

/**
 * @brief AES-CBC解密
 *
 * @param param [IN] AES-CBC解密必要的参数
 * @param buf [OUT] 解密数据输出缓冲区，可写长度至少为param->dataLen
 * @param bufLen [IN/OUT] 输入为输出缓冲区长度，输出为去除padding后的数据长度
 * @return 0成功，非0失败
 */
int32_t IotcAesCbcDecrypt(const IotcAesCbcParam *param, uint8_t *buf, uint32_t *bufLen);

/**
 * @brief AES-CCM解密并校验消息验证码
 *
 * @param param [IN] AES-CCM解密必要的参数
 * @param tag [OUT] 消息验证码，可读长度至少为tagLen
 * @param tagLen [IN] 消息验证码长度，范围应为[4,16]
 * @param buf [OUT] 解密密数据输出缓冲区，可写长度至少为param->dataLen
 * @return 0成功，非0失败
 */
int32_t IotcAesCcmDecrypt(const IotcAesCcmParam *param, const uint8_t *tag, uint32_t tagLen, uint8_t *buf);

/**
 * @brief AES-CCM加密
 *
 * @param param [IN] AES-CCM加密必要的参数
 * @param tag [OUT] 消息验证码输入缓冲区，可写长度至少为tagLen
 * @param tagLen [IN] 消息验证码缓冲区长度，范围应为[4,16]
 * @param buf [OUT] 加密数据输出缓冲区，可写长度至少为param->dataLen
 * @return 0成功，非0失败
 */
int32_t IotcAesCcmEncrypt(const IotcAesCcmParam *param, uint8_t *tag, uint32_t tagLen, uint8_t *buf);

#ifdef __cplusplus
}
#endif

#endif /* IOTC_AES_H */