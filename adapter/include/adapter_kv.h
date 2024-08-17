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
#ifndef IOTC_KV_H
#define IOTC_KV_H


#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IOTC_KV_MAX_KEY_LEN 32
#define IOTC_KV_MAX_TAG_LEN 128

int32_t IotcKvInit(const char *tag);

int32_t IotcKvDeInit(void);

int32_t IotcKvSetValue(const char *key, const uint8_t *value, uint32_t len);

int32_t IotcKvGetValue(const char *key, uint8_t *buf, uint32_t *len);

int32_t IotcKvGetLen(const char *key, uint32_t *len);

int32_t IotcKvDelValue(const char *key);

#ifdef __cplusplus
}
#endif

#endif /* IOTC_KV_H */
