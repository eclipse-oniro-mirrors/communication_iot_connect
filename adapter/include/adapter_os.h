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

#define IOTC_WAIT_FOREVER 0xFFFFFFFFU

typedef void IotcTaskId;

typedef void IotcMutexId;

typedef void IotcSemId;

typedef void (*IotcTaskEntryFunc)(void *arg);

typedef enum {
    IOTC_TASK_PRIORITY_MIN = 0,
    IOTC_TASK_PRIORITY_LOW,
    IOTC_TASK_PRIORITY_MID,
    IOTC_TASK_PRIORITY_HIGH,
    IOTC_TASK_PRIORITY_MAX,
} IotcTaskPrio;

typedef struct IotcTaskParam {
    IotcTaskEntryFunc func;
    IotcTaskPrio prio;
    uint32_t stackSize;
    void *arg;
    const char *name;
} IotcTaskParam;

IotcTaskId *IotcTaskCreate(IotcTaskParam *param);

int32_t IotcTaskSuspend(IotcTaskId *id);

int32_t IotcTaskResume(IotcTaskId *id);

void IotcTaskDelete(IotcTaskId *id);

IotcTaskId *IotcTaskGetCurrentTaskId(void);

IotcMutexId *IotcMutexCreate(void);

int32_t IotcMutexLock(IotcMutexId *id, uint32_t ms);

int32_t IotcMutexUnlock(IotcMutexId *id);

void IotcMutexDestroy(IotcMutexId *id);

IotcSemId *IotcSemCreate(uint32_t count);

int32_t IotcSemWait(IotcSemId *id, uint32_t ms);

int32_t IotcSemPost(IotcSemId *id);

uint32_t IotcSemGetCount(IotcSemId *id);

void IotcSemDestroy(IotcSemId *id);

int32_t IotcSleepMs(uint32_t ms);

void IotcSchedYield(void);

uint32_t IotcGetSysTimeMs(void);

int32_t IotcGetErrno(void);

#ifdef __cplusplus
}
#endif

#endif /* ADAPTER_OS_H */
