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
#ifndef ADAPTER_MD_H
#define ADAPTER_MD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 消息摘要计算上下文 */
typedef void AdapterMdContext;

/** 消息摘要算法类型 */
typedef enum {
    ADAPTER_MD_NONE = 0,
    ADAPTER_MD_SHA256,   /**< SHA-256消息摘要算法 */
    ADAPTER_MD_SHA384,   /**< SHA-384消息摘要算法 */
    ADAPTER_MD_SHA512,   /**< SHA-512消息摘要算法 */
} AdapterMdType;

/** 不同消息摘要算法摘要字节数 */
typedef enum {
    ADAPTER_MD_SHA256_BYTE_LEN = 32, /**< SHA-256消息摘要字节数 */
    ADAPTER_MD_SHA384_BYTE_LEN = 48, /**< SHA-384消息摘要字节数 */
    ADAPTER_MD_SHA512_BYTE_LEN = 64, /**< SHA-512消息摘要字节数 */
} AdapterMdByteLen;

/** HMAC计算参数 */
typedef struct {
    AdapterMdType md;    /**< 消息摘要算法 */
    const uint8_t *key;  /**< 密钥，可读长度至少为keyLen */
    uint32_t keyLen;     /**< 密钥长度 */
    const uint8_t *data; /**< 待计算HMAC数据，可读长度至少为dataLen */
    uint32_t dataLen;    /**< 数据长度 */
} AdapterHmacParam;

/**
 * @brief 计算消息摘要
 *
 * @param type [IN] 消息摘要算法
 * @param inData [IN] 待计算摘要的数据，可读长度至少为inLen
 * @param inLen [IN] 待计算摘要的数据长度
 * @param md [OUT] 生成的消息摘要缓冲区，可写长度至少为mdLen
 * @param mdLen [IN] 消息摘要缓冲区长度，长度根据type参考AdapterMdByteLen
 * @return 0成功，其他失败
 */
int32_t AdapterMdCalc(AdapterMdType type, const uint8_t *inData, uint32_t inLen, uint8_t *md, uint32_t mdLen);

/**
 * @brief 哈希运算消息认证码计算
 *
 * @param param [IN] 计算参数
 * @param hmac [OUT] hmac缓冲区，可写长度至少为hmacLen
 * @param hmacLen [IN] hmac缓冲区长度，长度根据param->md参考AdapterMdByteLen
 * @return 0成功，其他失败
 */
int32_t AdapterHmacCalc(const AdapterHmacParam *param, uint8_t *hmac, uint32_t hmacLen);

/**
 * @brief 初始化摘要计算上下文，用于持续的哈希计算
 *
 * @param type [IN] 消息摘要算法类型
 * @return NULL表示失败，其他表示成功
 * @attention 返回的上下文计算结束后需要使用AdapterMdFree释放
 */
AdapterMdContext *AdapterMdInit(AdapterMdType type);

/**
 * @brief 为持续的摘要计算导入数据
 *
 * @param ctx [IN] 摘要计算上下文
 * @param inData [IN] 输入数据，可读长度至少为inLen
 * @param inLen [IN] 输入数据长度
 * @return 0成功，其他失败
 */
int32_t AdapterMdUpdate(AdapterMdContext *ctx, const uint8_t *inData, uint32_t inLen);

/**
 * @brief 结束持续的摘要计算，并输出结果
 *
 * @param ctx [IN] 摘要计算上下文
 * @param outData [OUT] 输出数据缓冲区，有效长度至少为outLen
 * @param outLen [IN] 输出数据缓冲区长度，根据摘要算法类型有最小长度要求，参考AdapterMdByteLen
 * @return 0成功，其他失败
 */
int32_t AdapterMdFinish(AdapterMdContext *ctx, uint8_t *outData, uint32_t outLen);

/**
 * @brief 释放摘要计算上下文
 *
 * @param ctx [IN] 摘要计算上下文
 */
void AdapterMdFree(AdapterMdContext *ctx);

#ifdef __cplusplus
}
#endif

#endif /* ADAPTER_MD_H */