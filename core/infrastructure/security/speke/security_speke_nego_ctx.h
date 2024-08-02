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
#ifndef SECURITY_SPEKE_NEGO_CTX_H
#define SECURITY_SPEKE_NEGO_CTX_H

#include "adapter_mpi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 盐值最大长度 */
#define MAX_SALT_LEN 16
/* 协商时挑战值长度 */
#define CHALLENGE_LEN 16
/* 会话密钥长度 */
#define SESSION_KEY_LEN 16
/* 生成 hmac 的长度 */
#define HMAC_LEN 32

typedef enum {
    PRIME_NORMAL,
    PRIME_BIG,
    PRIME_INVALID
} PrimeType;

#define SPEKE_256_MODE_PUB_KEY_LEN 256
#define SPEKE_384_MODE_PUB_KEY_LEN 384

#ifdef SPEKE_SUPPORT_BIG_PRIME
#define SUPPORT_PRIME_TYPE PRIME_BIG
#define PRIME_KEY_LEN SPEKE_384_MODE_PUB_KEY_LEN
#else
#define SUPPORT_PRIME_TYPE PRIME_NORMAL
#define PRIME_KEY_LEN SPEKE_256_MODE_PUB_KEY_LEN
#endif

/* Speke协商上下文结构定义 */
typedef struct {
    uint8_t salt[MAX_SALT_LEN];                 /* 密钥派生时使用的盐值 */
    uint32_t saltLen;                           /* 盐值长度 */
    uint8_t *pubKey;                            /* 本端公钥 */
    uint32_t pubKeyLen;                         /* 本端公钥长度 */
    AdapterMpi *random;                         /* 本端随机数，随本次会话生存 */
    AdapterMpi *prime;                          /* 极大素数 SPEKE_PRIME */
    uint8_t localChallenge[CHALLENGE_LEN];      /* 本端挑战值 */
    uint8_t remoteChallenge[CHALLENGE_LEN];     /* 对端挑战值 */
    uint8_t identityEncKey[SESSION_KEY_LEN];    /* 协商时派生的sessionKey1 用于加密 */
    uint8_t hmacKey[SESSION_KEY_LEN];           /* 协商时派生的sessionKey2 用于校验 */
} NegoContext;

/**
 * @brief 初始化 speke 协商上下文
 *
 * @param pinCode [IN] PIN
 * @param pinCodeLen [IN] PIN 长度
 * @param salt [IN] 计算共享密钥时使用的盐值
 * @param saltLen [IN] 盐值长度
 * @param primeType [IN] 协商使用的大素数类型
 * @return 协商上下文句柄，NULL 失败
 */
NegoContext *NegoContextInit(const uint8_t *pinCode, uint32_t pinCodeLen,
    const uint8_t *salt, uint32_t saltLen, PrimeType primeType);

/**
 * @brief 释放 speke 协商上下文
 *
 * @param context [IN] speke 协商上下文句柄
 */
void NegoContextFree(NegoContext *context);

/**
 * @brief 记录对端的挑战值
 *
 * @param context [IN] speke 协商上下文句柄
 * @param challenge [IN] 对端挑战值
 * @param challengeLen [IN] 对端挑战值长度
 * @return 0成功，非0失败
 */
int32_t NegoContextSetRemoteChallenge(NegoContext *context, const uint8_t *challenge, uint32_t challengeLen);

/**
 * @brief 计算生成 sessionKey
 *
 * @param context [IN] speke 协商上下文句柄
 * @param pubKey [IN] 对端公钥
 * @param pubKeyLen [IN] 对端公钥长度
 * @return 0成功，非0失败
 */
int32_t NegoContextGenSessionKey(NegoContext *context, const uint8_t *pubKey, uint32_t pubKeyLen);

/**
 * @brief 计算生成 hmac
 *
 * @param context [IN] speke 协商上下文句柄
 * @param output [OUT] 存放生成的 hmac 的缓冲区
 * @param outputLen [IN] 缓冲区长度，需不小于 HMAC_LEN
 * @return 0成功，非0失败
 * @attention 需在 NegoContextSetRemoteChallenge 以及 NegoContextGenSessionKey 成功后调用
 */
int32_t NegoContextGenHmac(const NegoContext *context, uint8_t *output, uint32_t outputLen);

/**
 * @brief 校验对端 hmac
 *
 * @param context [IN] speke 协商上下文句柄
 * @param hmac [IN] 对端 hmac
 * @param hmacLen [IN] 对端 hmac 长度
 * @return 0成功，非0失败
 * @attention 需在 NegoContextSetRemoteChallenge 以及 NegoContextGenSessionKey 成功后调用
 */
int32_t NegoContextVerifyHmac(const NegoContext *context, const uint8_t *hmac, uint32_t hmacLen);

/**
 * @brief 计算生成 speke 会话的业务加密密钥
 *
 * @param context [IN] speke 协商上下文句柄
 * @param output [OUT] 存放生成的密钥的缓冲区
 * @param outputLen [IN] 缓冲区长度，需不小于 SESSION_KEY_LEN
 * @return 0成功，非0失败
 * @attention 需在 NegoContextGenSessionKey 成功后调用
 */
int32_t NegoContextGenDataEncKey(const NegoContext *context, uint8_t *output, uint32_t outputLen);

#ifdef __cplusplus
}
#endif

#endif /* SECURITY_SPEKE_NEGO_CTX_H */