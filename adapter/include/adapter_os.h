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
#ifndef ADAPTER_OS_H
#define ADAPTER_OS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ADAPTER_WAIT_FOREVER 0xFFFFFFFFU

typedef void AdapterTaskId;

typedef void AdapterMutexId;

typedef void AdapterSemId;

typedef void (*AdapterTaskEntryFunc)(void *arg);

typedef enum {
    ADAPTER_TASK_PRIORITY_MIN = 0,
    ADAPTER_TASK_PRIORITY_LOW,
    ADAPTER_TASK_PRIORITY_MID,
    ADAPTER_TASK_PRIORITY_HIGH,
    ADAPTER_TASK_PRIORITY_MAX,
} AdapterTaskPrio;

typedef struct AdapterTaskParam {
    AdapterTaskEntryFunc func;
    AdapterTaskPrio prio;
    uint32_t stackSize;
    void *arg;
    const char *name;
} AdapterTaskParam;

AdapterTaskId *AdapterCreateTask(AdapterTaskParam *param);

int32_t AdapterSuspendTask(AdapterTaskId *id);

int32_t AdapterResumeTask(AdapterTaskId *id);

void AdapterDeleteTask(AdapterTaskId *id);

AdapterTaskId *AdapterGetCurrentTaskId(void);

AdapterMutexId *AdapterCreateMutex(void);

int32_t AdapterLockMutex(AdapterMutexId *id, uint32_t ms);

int32_t AdapterUnlockMutex(AdapterMutexId *id);

void AdapterDestroyMutex(AdapterMutexId *id);

AdapterSemId *AdapterCreateSem(uint32_t count);

int32_t AdapterWaitSem(AdapterSemId *id, uint32_t ms);

int32_t AdapterPostSem(AdapterSemId *id);

uint32_t AdapterGetSemCount(AdapterSemId *id);

void AdapterDestroySem(AdapterSemId *id);

int32_t AdapterSleepMs(uint32_t ms);

void AdapterSchedYield(void);

uint32_t AdapterGetSysTimeMs(void);

int32_t AdapterGetErrno(void);

#ifdef __cplusplus
}
#endif

#endif /* ADAPTER_OS_H */
