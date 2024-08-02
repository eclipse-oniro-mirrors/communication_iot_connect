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
#ifndef SECURITY_SPEKE_SESSION_H
#define SECURITY_SPEKE_SESSION_H

#include "security_speke.h"
#include "security_speke_nego_ctx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PIN 长度 */
#define PIN_MAX_LEN 16

struct TagSpekeSession {
    SpekeType spekeType;                        /* 存储会话协商身份信息 */
    SpekeCallback spekeCb;                      /* 会话相关回调 */
    void *user;                                 /* 上层业务缓存数据 */
    uint8_t pinCode[PIN_MAX_LEN];
    uint32_t pinCodeLen;
    NegoContext *negoContext;                   /* speke 协商上下文句柄 */
    uint8_t dataEncKey[SESSION_KEY_LEN];        /* speke 协商结果，会话加解密使用的密钥 */
};

#ifdef __cplusplus
}
#endif

#endif /* SECURITY_SPEKE_SESSION_H */