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
#ifndef ADAPTER_DRBG_H
#define ADAPTER_DRBG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 随机数生成器上下文 */
typedef void AdapterDrbgContext;

/**
 * @brief 真随机数种子获取函数
 *
 * @param buf [OUT] 随机数缓冲区
 * @param len [IN] 缓冲区长度
 * @return 0成功非0失败
 */
typedef int32_t (*AdapterTrngCallback)(uint8_t *buf, uint32_t len);

/**
 * @brief 安全用途伪随机数发生器初始化
 *
 * @param custom [IN] 自定义业务字符串
 * @param trng [IN] 真随机数种子生成器
 * @return 随机数发生器上下文
 * @warning 需外部做多线程保护
 */
AdapterDrbgContext *AdapterDrbgInit(const char *custom, AdapterTrngCallback trng);

/**
 * @brief 安全用途伪随机数获取
 *
 * @param ctx [IN] 随机数发生器上下文
 * @param out [OUT] 随机数缓冲区，可写长度至少为outLen
 * @param outLen [IN] 随机数缓冲区长度
 * @return 0成功，其他失败
 */
int32_t AdapterDrbgRandom(AdapterDrbgContext *ctx, uint8_t *out, uint32_t outLen);

/**
 * @brief 安全用途伪随机数去初始化
 *
 * @param ctx [IN] 随机数发生器上下文
 */
void AdapterDrbgDeinit(AdapterDrbgContext *ctx);

#ifdef __cplusplus
}
#endif

#endif /* HILINK_RANDOM_ADAPTER_H */