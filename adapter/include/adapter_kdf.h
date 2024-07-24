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
#ifndef ADAPTER_KDF_H
#define ADAPTER_KDF_H

#include "adapter_md.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 基于HMAC的PKCS#5 PBKDF2密钥派生参数 */
typedef struct {
    AdapterMdType md;               /**< 摘要算法类型 */
    const uint8_t *password; /**< 密码，可读长度至少为passwordLen */
    uint32_t passwordLen;      /**< 密码长度 */
    const uint8_t *salt;     /**< 盐值，可读长度至少为saltLen */
    uint32_t saltLen;          /**< 盐值长度 */
    uint32_t iterCount;        /**< 迭代次数 */
} AdapterPbkdf2HmacParam;

/** @brief HKDF密钥派生参数 */
typedef struct {
    AdapterMdType md;               /**< 摘要算法类型 */
    const uint8_t *salt;     /**< 盐值，可读长度至少为saltLen */
    uint32_t saltLen;          /**< 盐值长度，可以为0 */
    const uint8_t *info;     /**< 可选字符串，可读长度至少为infoLen */
    uint32_t infoLen;          /**< 可选字符串长度，可以为0 */
    const uint8_t *material; /**< 密钥派生材料，可读长度至少为material */
    uint32_t materialLen;      /**< 密钥派生材料长度 */
} AdapterHkdfParam;

/**
 * @brief 基于HMAC的PKCS#5 PBKDF2密钥派生
 *
 * @param param [IN] 密钥派生参数
 * @param key [OUT] 生成密钥缓冲区，可写长度至少为keyLen
 * @param keyLen [IN] 生成密钥的长度，取决于param->md摘要算法
 * @return 0成功，其他失败
 */
int32_t AdapterPkcs5Pbkdf2Hmac(const AdapterPbkdf2HmacParam *param, uint8_t *key, uint32_t keyLen);

/**
 * @brief HKDF密钥派生
 *
 * @param param [IN] 密钥派生参数
 * @param key [OUT] 生成密钥缓冲区，可写长度至少为keyLen
 * @param keyLen [IN] 生成密钥的长度，不超过摘要算法类型字节数的255倍
 * @return 0成功，其他失败
 */
int32_t AdapterHkdf(const AdapterHkdfParam *param, uint8_t *key, uint32_t keyLen);

#ifdef __cplusplus
}
#endif

#endif /* AdapterMD_H */