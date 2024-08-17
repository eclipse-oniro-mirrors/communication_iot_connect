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
#ifndef SECURITY_KEY_H
#define SECURITY_KEY_H

#include <stdint.h>
#include "adapter_md.h"
#include "comm_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SECURITY_PSK_LEN 16
#define SECURITY_HKDF_LOCAL_KEY_LEN IOTC_MD_SHA256_BYTE_LEN
#define SECURITY_UDID_LEN (IOTC_MD_SHA256_BYTE_LEN * 2)

typedef int32_t (*SecurityGetPskCallback)(uint8_t *buf, uint32_t len);

typedef int32_t (*SecurityGetAcKey)(uint8_t *buf, uint32_t len);

typedef int32_t (*SecurityGetUdid)(uint8_t *buf, uint32_t len);

void SecurityRegUdidCallback(SecurityGetUdid udidCb);

void SecurityRegPskCallback(SecurityGetPskCallback pskCb);

void SecurityRegAcKeyCallback(SecurityGetAcKey acCb);

int32_t SecurityGetPsk(uint8_t *buf, uint32_t len);

int32_t SecurityGenHkdfLocalKey(const uint8_t *salt, uint32_t saltLen, uint8_t out[SECURITY_HKDF_LOCAL_KEY_LEN]);

#ifdef __cplusplus
}
#endif

#endif /* SECURITY_KEY_H */
