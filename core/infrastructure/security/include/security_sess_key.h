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
#ifndef SECURITY_SESS_KEY_H
#define SECURITY_SESS_KEY_H

#include <stdint.h>
#include "utils_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SESS_KEY_SN1 = 0,
    SESS_KEY_SN2,
} SessKeySnType;

#define SESS_SN_LEN 8
#define SESS_SN_HEX_LEN HEXIFY_LEN(SESS_SN_LEN)
#define SESS_TRANS_KEY_LEN 16
#define SESS_AUTH_KEY_LEN 32

typedef struct SessKeyContext SessKeyContext;

SessKeyContext *SecuritySessKeyInit(void);

int32_t SecuritySessKeyAddSn(SessKeyContext *ctx, const uint8_t sn[SESS_SN_LEN], SessKeySnType type);

int32_t SecuritySessKeyGenTransKey(SessKeyContext *ctx, const uint8_t *pwd, uint32_t pwdLen,
    uint8_t transKey[SESS_TRANS_KEY_LEN]);

int32_t SecuritySessKeyGenAuthKey(SessKeyContext *ctx, uint8_t authKey[SESS_AUTH_KEY_LEN]);

void SecuritySessDestroy(SessKeyContext *ctx);

#ifdef __cplusplus
}
#endif

#endif /* SECURITY_SESS_KEY_H */
