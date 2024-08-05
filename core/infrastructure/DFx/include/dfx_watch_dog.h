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
#ifndef DFX_WATCH_DOG_H
#define DFX_WATCH_DOG_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef IOTC_CONF_WATCH_DOG_TIMEOUT
#define IOTC_CONF_WATCH_DOG_TIMEOUT (1 * 60 * 1000)
#endif

typedef void (*DfxWatchDogTimeoutHandler)(void);

int32_t DfxWatchDogInit(void);

void DfxWatchDogDeinit(void);

int32_t DfxAddWatchDog(uint32_t timeoutMs, DfxWatchDogTimeoutHandler hdl, const char *name);

int32_t DfxFeedWatchDog(void);

void DfxWatchDogRegCommTimeoutHandler(DfxWatchDogTimeoutHandler hdl);

void DfxDelWatchDog(void);

void DfxRecordNoDogTask(const char *name);

void DfxDelNoDogTaskRecord(void);

void DfxSetWatchDogTaskSize(uint32_t size);

void DfxDumpAllTask(void);

#ifdef __cplusplus
}
#endif

#endif /* DFX_WATCH_DOG_H */
