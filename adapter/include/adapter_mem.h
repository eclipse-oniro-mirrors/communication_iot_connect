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
#ifndef IOTC_MEM_H
#define IOTC_MEM_H

#include <stdint.h>
#include "iotc_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !IOTC_CONF_MEM_DEBUG
void *AdapterMalloc(uint32_t size);

void *AdapterCalloc(uint32_t num, uint32_t size);

void AdapterFree(void *pt);

#else

void *AdapterDebugMalloc(uint32_t size, const char *func, uint32_t line);

void *AdapterDebugCalloc(uint32_t num, uint32_t size, const char *func, uint32_t line);

void AdapterDebugFree(void *pt, const char *func, uint32_t line);

void AdapterMemDump(void);

#define AdapterMalloc(size) AdapterDebugMalloc(size, __func__, __LINE__)

#define AdapterCalloc(num, size) AdapterDebugCalloc(num, size, __func__, __LINE__)

#define AdapterFree(size) AdapterDebugFree(size, __func__, __LINE__)

#endif

#ifdef __cplusplus
}
#endif

#endif /* IOTC_MEM_H */
