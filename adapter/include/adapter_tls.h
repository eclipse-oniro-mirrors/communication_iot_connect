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
#ifndef ADAPTER_TLS_H
#define ADAPTER_TLS_H

#include <stdint.h>
#include <stdbool.h>

#if __cplusplus
extern "C" {
#endif

/** 定制化字符最大长度 */
#define ADAPTER_TLS_CUSTOM_MAX_STR_LEN 32
#define ADAPTER_TLS_HOSTNAME_MAX_STR_LEN 64
#define ADAPTER_TLS_CIPHERSUITE_MAX_NUM 20

/** tls加密套件 */
typedef enum {
    ADAPTER_TLS_CIPHERSUITE_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256 = 0xCCA8,
    ADAPTER_TLS_CIPHERSUITE_ECDHE_RSA_WITH_AES_256_GCM_SHA384 = 0xC030,
    ADAPTER_TLS_CIPHERSUITE_ECDHE_RSA_WITH_AES_128_GCM_SHA256 = 0xC02F,
    ADAPTER_TLS_CIPHERSUITE_PSK_WITH_AES_128_GCM_SHA256 = 0xA8,
    ADAPTER_TLS_CIPHERSUITE_PSK_WITH_AES_256_GCM_SHA384 = 0xA9,
} AdapterTlsCiphersuite;

/** tls分片长度 */
typedef enum {
    ADAPTER_TLS_MAX_FRAG_LEN_NONE = 0,
    ADAPTER_TLS_MAX_FRAG_LEN_512,
    ADAPTER_TLS_MAX_FRAG_LEN_1024,
    ADAPTER_TLS_MAX_FRAG_LEN_2048,
    ADAPTER_TLS_MAX_FRAG_LEN_4096,
    ADAPTER_TLS_MAX_FRAG_LEN_DEFAULT = 255,
} AdapterTlsMaxFragLen;

/** ca证书校验结果 */
typedef enum {
    ADAPTER_TLS_CERT_VERIFY_INVALID = 0,
    ADAPTER_TLS_CERT_VERIFY_VALID,
} AdapterTlsCertVerify;

/** tls会话客户端上下文 */
typedef void AdapterTlsClient;

/**
 * @brief 获取时间的回调函数
 *
 * @param ms [IN] utc时间ms时间戳
 * @return 0成功非0失败
 */
typedef int32_t (*AdapterGetTimeCallback)(uint64_t *ms);

/**
 * @brief 获取随机数的回调函数
 *
 * @param buf [OUT] 随机数缓冲区
 * @param len [IN] 缓冲区长度
 * @return 0成功非0失败
 */
typedef int32_t (*AdapterGetRandom)(uint8_t *buf, uint32_t len);

/** tls host配置参数 */
typedef struct {
    const char *hostname; /**< 对端域名 */
    uint16_t port;        /**< 对端端口 */
} AdapterTlsHost;

/** tls加密套件配置参数 */
typedef struct {
    int32_t *ciphersuites; /**< 加密套件 */
    uint32_t num;                        /** 加密套件数量 */
} AdapterTlsCiphersuites;

/** tls ca证书配置参数 */
typedef struct {
    const char **certs;   /**< ca证书列表 */
    uint32_t num;         /**< ca证书数量 */
    bool delayTimeVerify; /**< 延迟时间校验 */
    bool hostVerify;      /**< 域名校验 */
} AdapterTlsCerts;

/** tls psk参数配置 */
typedef struct {
    const uint8_t *psk;         /**< psk */
    uint32_t pskLen;            /**< psk 长度 */
    const uint8_t *pskIdentity; /**< psk id */
    uint32_t pskIdentityLen;    /**< psk id长度 */
} AdapterTlsPsk;

typedef enum {
    /* 为TLS客户端设置套接字，参数类型为int32_t */
    ADAPTER_TLS_OPTION_FD = 0,
    /* 为TLS客户端设置获取时间回调函数，参数类型为HiLinkMbedtlsGetTimeCb，对所有客户端生效 */
    ADAPTER_TLS_OPTION_REG_TIME_CALLBACK,
    /* 为TLS客户端设置对端地址，参数类型为HiLinkTlsHost */
    ADAPTER_TLS_OPTION_HOST,
    /* 为TLS客户端设置套件白名单，参数类型为HiLinkTlsCiphersuites */
    ADAPTER_TLS_OPTION_CIPHERSUITE,
    /* 为TLS客户端设置证书，参数类型为HiLlinkTlsCerts */
    ADAPTER_TLS_OPTION_CERT,
    /* 为TLS客户端设置psk，参数类型为HiLlinkTlsPsk */
    ADAPTER_TLS_OPTION_PSK,
    /* 为TLS客户端设置最大分片大小，参数类型为unsiged char，值参考HILINK_MBEDTLS_SSL_MAX_FRAG_LEN */
    ADAPTER_TLS_OPTION_MAX_FRAG_LEN,
    /** 为TLS客户端设置随机数回调函数，参数类型AdapterGetRandom */
    ADAPTER_TLS_OPTION_REG_RANDOM_CALLBACK,
    /** 握手超时时间，参数类型为uint32_t 单位ms */
    ADAPTER_TLS_OPTION_HANDSHAKE_TIMEOUT_MS,
    ADAPTER_TLS_OPTION_MAX,
} AdapterTlsOption;

/**
 * @brief 创建tls客户端
 *
 * @param custom [IN] 定制化字符串
 * @return NULL失败 非NULL客户端上下文
 */
AdapterTlsClient *AdapterCreateTlsClient(const char *custom);

/**
 * @brief 释放tls客户端
 *
 * @param cli [IN] 客户端上下文
 */
void AdapterFreeTlsClient(AdapterTlsClient *cli);

/**
 * @brief 配置tls客户端
 *
 * @param cli [IN] 客户端上下文
 * @param option [IN] 配置项
 * @param value [IN] 配置参数
 * @param len [IN] 参数长度
 * @return 0成功非0失败
 */
int32_t AdapterSetTlsClientOption(AdapterTlsClient *cli, AdapterTlsOption option, const void *value, uint32_t len);

/**
 * @brief tls客户端连接对端
 *
 * @param cli [IN] 客户端上下文
 * @return 0成功非0失败
 */
int32_t AdapterTlsClientConnect(AdapterTlsClient *cli);

/**
 * @brief 获取tls客户端的套接字
 *
 * @param cli [IN] 客户端上下文
 * @return 套接字
 */
int32_t AdapterTlsClientGetFd(AdapterTlsClient *cli);

/**
 * @brief tls客户端读取数据
 *
 * @param cli [IN] 客户端上下文
 * @param buf [OUT] 接收数据缓冲区
 * @param len [IN] 缓冲区长度
 * @return 大于0为读取数据长度 ，小于0参考错误码
 */
int32_t AdapterTlsClientRecv(AdapterTlsClient *cli, uint8_t *buf, uint32_t len);

/**
 * @brief tls客户端发送数据
 *
 * @param cli [IN] 客户端上下文
 * @param buf [OUT] 发送数据缓冲区
 * @param len [IN] 缓冲区长度
 * @return 大于0为接收数据长度 ，小于0参考错误码
 */
int32_t AdapterTlsClientSend(AdapterTlsClient *cli, const uint8_t *buf, uint32_t len);

/**
 * @brief 验证证书有效性
 *
 * @param cli [IN] 客户端上下文
 * @return 验证结果
 */
AdapterTlsCertVerify AdapterTlsClientVerifyCert(AdapterTlsClient *cli);

#if __cplusplus
}
#endif

#endif /* ADAPTER_TLS_H */
