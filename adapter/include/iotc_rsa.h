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
#ifndef IOTC_RSA_H
#define IOTC_RSA_H

#include <stdint.h>
#include "iotc_md.h"
#include "iotc_mpi.h"

#ifdef __cplusplus
extern "C" {
#endif

/** RSA PKCS填充模式 */
typedef enum {
    ADAPTER_RSA_PKCS1_V15 = 0, /**< PKCS#1 v1.5 */
    ADAPTER_RSA_PKCS1_V21,     /**< PKCS#1 v2.1 */
} IotcRsaPkcs1Mode;

/** RSA 操作模式 */
typedef enum {
    ADAPTER_RSA_OP_PUBLIC = 0, /**< 公钥操作 */
    ADAPTER_RSA_OP_PRIVATE,    /**< 私钥操作 */
} IotcRsaOpMode;

/** RAS上下文 */
typedef void IotcRsaContext;

/** RSA核心参数 */
typedef struct {
    IotcMpi *n; /**< 模数 */
    IotcMpi *p; /**< 第一个素数因子 */
    IotcMpi *q; /**< 第二个素数因子 */
    IotcMpi *d; /**< 私有指数 */
    IotcMpi *e; /**< 公有指数 */
} IotcRsaParam;

/** RSA解密参数 */
typedef struct {
    IotcRsaContext *ctx;                     /**< RSA上下文 */
    IotcRsaOpMode mode;                      /**< 操作模式 */
    int32_t (*rng)(uint8_t *out, uint32_t len); /**< 随机数发生器，私钥解密、公钥加密、ADAPTER_RSA_PKCS1_V21加密时应该提供 */
    const uint8_t *input;                       /**< 密文，可写长度至少为inLen */
    uint32_t inLen;                             /**< 密文长度，解密时应该与模数长度一致 */
} IotcRsaCryptParam;

/** RSA签名校验参数 */
typedef struct {
    IotcMdType md;    /* hash类型 */
    const uint8_t *hash; /* hash值 */
    uint32_t hashLen;    /* hash长度 */
    const uint8_t *sig;  /* 签名 */
    uint32_t sigLen;     /* 签名长度 */
} IotcRsaVerifyParam;

/**
 * @brief RSA上下文初始化
 *
 * @param padding [IN] 填充模式
 * @param md [IN] 摘要算法类型，padding为ADAPTER_RSA_PKCS_V21时使用
 * @return NULL失败，非NULL为RSA上下文指针
 * @attention 返回的上下文使用完毕后应该使用IotcRsaFree释放
 */
IotcRsaContext *IotcRsaInit(IotcRsaPkcs1Mode padding, IotcMdType md);

/**
 * @brief RSA上下文释放
 *
 * @param ctx [IN] RSA上下文
 */
void IotcRsaFree(IotcRsaContext *ctx);

/**
 * @brief RSA导入核心参数
 *
 * @param ctx [IN] RSA上下文
 * @param param [IN] RSA参数
 * @return 0成功，非0失败
 */
int IotcRsaParamImport(IotcRsaContext *ctx, const IotcRsaParam *param);

/**
 * @brief 使用RSA公钥校验签名
 *
 * @param ctx [IN] RSA上下文
 * @param md [IN] 签名摘要算法
 * @param hash [IN] 消息摘要，可读长度至少为hashLen
 * @param hashLen [IN] 消息摘要长度
 * @param sig [IN] 签名，可读长度至少为sigLen
 * @param sigLen [IN] 签名长度，应为RSA模数长度
 * @return 0成功，非0失败
 */
int IotcRsaPkcs1Verify(IotcRsaContext *ctx, const IotcRsaVerifyParam *param);

/**
 * @brief RSA解密
 *
 * @param param [IN] 解密参数
 * @param buf [OUT] 解密缓冲区，可写长度至少为*len
 * @param len [IN,OUT] 输入为缓冲区长度，输出为解密数据长度
 * @return 0成功，非0失败
 */
int IotcRsaPkcs1Decrypt(const IotcRsaCryptParam *param, uint8_t *buf, uint32_t *len);

/**
 * @brief RSA加密
 *
 * @param param [IN] 加密参数
 * @param buf [OUT] 加密数据输出缓冲区，可写长度至少为*len
 * @param len [IN] 输出缓冲区长度，应该大于等于模数长度
 * @return int
 */
int IotcRsaPkcs1Encrypt(const IotcRsaCryptParam *param, uint8_t *buf, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* IOTC_RSA_H */