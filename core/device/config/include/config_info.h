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
#ifndef CONFIG_INFO_H
#define CONFIG_INFO_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CONFIG_INFO_KEY_REVOKE_FLAG = 0,
    CONFIG_INFO_KEY_DEVICE_ID,
    CONFIG_INFO_KEY_LOGIN_SECRET,
    CONFIG_INFO_KEY_LOGIN_PSK,
    CONFIG_INFO_KEY_LOGIN_URL,
    CONFIG_INFO_KEY_LOGIN_BACKUP_URL,
    CONFIG_INFO_KEY_UID_HASH,
    CONFIG_INFO_KEY_AUTHCODE_ID,
    CONFIG_INFO_KEY_AUTHCODE,
    CONFIG_INFO_KEY_NETINFO_FLAG,
} ConfigInfoKey;

typedef int32_t (*OnConfigInfoGet)(ConfigInfoKey key, uint8_t *buf, uint32_t *len);
typedef int32_t (*OnConfigInfoSet)(ConfigInfoKey key, const uint8_t *buf, uint32_t len);
typedef int32_t (*OnConfigInfoClear)(ConfigInfoKey key);
typedef int32_t (*OnConfigInfoSave)(void);

typedef struct {
    OnConfigInfoGet get;
    OnConfigInfoSet set;
    OnConfigInfoClear clear;
    OnConfigInfoSave save;
} ConfigInfoCallback;

int32_t ConfigInfoInit(void);
void ConfigInfoDeinit(void);
int32_t ConfigInfoGet(ConfigInfoKey key, uint8_t *buf, uint32_t *len);
int32_t ConfigInfoSet(ConfigInfoKey key, const uint8_t *buf, uint32_t len);
int32_t ConfigInfoClear(ConfigInfoKey key);
int32_t ConfigInfoSave(void);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_INFO_H */
