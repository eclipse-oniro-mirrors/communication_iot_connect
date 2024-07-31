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
#ifndef SECURITY_STORE_H
#define SECURITY_STORE_H

#include <stdint.h>
#include "comm_def.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SECURITY_STORE_FLAG_PLAIN,
    SECURITY_STORE_FLAG_VERIFY,
    SECURITY_STORE_FLAG_ENCRYPT,
} SecurityStoreMode;

typedef struct {
    int32_t keyInt;
    const char *keyChar;
    SecurityStoreMode mode;
    uint8_t *buffer;
    uint32_t len;
    uint32_t size;
} SecurityStoreItem;

typedef struct {
    int32_t (*onInit)(const char *tag);
    int32_t (*onDeinit)(void);
    int32_t (*onSetValue)(const char *key, const uint8_t *value, uint32_t len);
    int32_t (*onGetValue)(const char *key, uint8_t *buf, uint32_t *len);
    int32_t (*onGetValueLen)(const char *key, uint32_t *len);
    int32_t (*onDelValue)(const char *key);
} SecurityStoreCallback;

int32_t SecurityStoreInit(const char *path, SecurityStoreCallback *cb);

void SecurityStoreDeinit(void);

int32_t SecurityStoreRegStoreList(SecurityStoreItem *list, uint32_t num);

void SecurityStoreUnregStoreList(SecurityStoreItem *list);

int32_t SecurityStoreSet(int32_t key, const uint8_t *value, uint32_t len);

int32_t SecurityStoreGet(int32_t key, uint8_t *value, uint32_t *len);

int32_t SecurityStoreGetLen(int32_t key, uint32_t *len);

int32_t SecurityStorePull(int32_t key);

int32_t SecurityStoreCommit(int32_t key);

int32_t SecurityStoreDel(int32_t key);

int32_t SecurityStorePullList(SecurityStoreItem *list);

int32_t SecurityStoreCommitList(SecurityStoreItem *list);

int32_t SecurityStoreDelList(SecurityStoreItem *list);

int32_t SecurityStorePullAll(void);

int32_t SecurityStoreCommitAll(void);

int32_t SecurityStoreDelAll(void);

#ifdef __cplusplus
}
#endif

#endif /* SECURITY_STORE_H */
