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
#ifndef SECURITY_RANDOM_H
#define SECURITY_RANDOM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t (*SecurityTrng)(uint8_t *buf, uint32_t len);

/**
 * @brief 安全随机数初始化
 *
 * @return 0成功，非0失败
 */
int32_t SecurityRandomInit(void);

/**
 * @brief 安全随机数去初始化
 *
 */
void SecurityRandomDeinit(void);

void SecurityRandomRegTrng(SecurityTrng trng);

/**
 * @brief 获取安全随机数
 *
 * @param buf [OUT] 随机数缓冲区
 * @param bufLen [IN] 缓冲区长度
 * @return 0成功，非0失败
 */
int32_t SecurityRandom(uint8_t *buf, uint32_t bufLen);

uint32_t SecurityRandomUint32(void);

#ifdef __cplusplus
}
#endif

#endif /* SECURITY_RANDOM_H */
