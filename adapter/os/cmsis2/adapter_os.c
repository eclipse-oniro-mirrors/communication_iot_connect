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
#include "adapter_os.h"
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include "cmsis_os2.h"
#include "securec.h"
#include "adapter_log.h"
#include "iotc_errcode.h"

#ifndef MS_PER_SECOND
#define MS_PER_SECOND   1000
#endif
#ifndef US_PER_MS
#define US_PER_MS       1000
#endif

AdapterTaskId *AdapterCreateTask(AdapterTaskParam *param)
{
    if ((param == NULL) || (param->func == NULL) || (param->prio > ADAPTER_TASK_PRIORITY_MAX) ||
        (param->stackSize == 0)) {
        ADAPTER_LOGW("invalid param");
        return NULL;
    }

    const osPriority_t prioMap[] = {
        osPriorityLow1,
        osPriorityBelowNormal1,
        osPriorityNormal2,
        osPriorityNormal7,
        osPriorityHigh
    };

    osThreadAttr_t attr;
    (void)memset_s(&attr, sizeof(osThreadAttr_t), 0, sizeof(osThreadAttr_t));
    attr.name = param->name;
    attr.priority = prioMap[param->prio];
    attr.stack_size = param->stackSize;

    return (AdapterTaskId *)osThreadNew((osThreadFunc_t)param->func, param->arg, &attr);
}

int32_t AdapterSuspendTask(AdapterTaskId *id)
{
    if (id == NULL) {
        ADAPTER_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    osStatus_t status = osThreadSuspend((osThreadId_t)id);
    if (status != osOK) {
        ADAPTER_LOGW("suspend error %d", status);
        return IOTC_ADAPTER_OS_ERR_SUSPEND_TASK;
    }
    return IOTC_OK;
}

int32_t AdapterResumeTask(AdapterTaskId *id)
{
    if (id == NULL) {
        ADAPTER_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    osStatus_t status = osThreadResume((osThreadId_t)id);
    if (status != osOK) {
        ADAPTER_LOGW("resume error %d", status);
        return IOTC_ADAPTER_OS_ERR_RESUME_TASK;
    }
    return IOTC_OK;
}

void AdapterDeleteTask(AdapterTaskId *id)
{
    if (id == NULL) {
        return;
    }

    osStatus_t status = osThreadTerminate((osThreadId_t)id);
    if (status != osOK) {
        ADAPTER_LOGW("delete task error %d", status);
    }
}

AdapterTaskId *AdapterGetCurrentTaskId(void)
{
    return (AdapterTaskId *)osThreadGetId();
}

AdapterMutexId *AdapterCreateMutex(void)
{
    return (AdapterMutexId *)osMutexNew(NULL);
}

static inline uint32_t MsToTick(uint32_t ms)
{
    uint64_t tick = ms * osKernelGetTickFreq() / MS_PER_SECOND;
    if (tick > UINT32_MAX) {
        return UINT32_MAX;
    }
    return (uint32_t)tick;
}

int32_t AdapterLockMutex(AdapterMutexId *id, uint32_t ms)
{
    if (id == NULL) {
        ADAPTER_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    osStatus_t status = osMutexAcquire((osMutexId_t)id, MsToTick(ms));
    if (status != osOK) {
        if (status == osErrorTimeout) {
            return IOTC_ERR_TIMEOUT;
        }
        ADAPTER_LOGW("mutex lock error %d", status);
        return IOTC_ADAPTER_OS_ERR_MUTEX_LOCK;
    }

    return IOTC_OK;
}

int32_t AdapterUnlockMutex(AdapterMutexId *id)
{
    if (id == NULL) {
        ADAPTER_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    osStatus_t status = osMutexRelease((osMutexId_t)id);
    if (status != osOK) {
        ADAPTER_LOGW("mutex unlock error %d", status);
        return IOTC_ADAPTER_OS_ERR_MUTEX_UNLOCK;
    }
    return IOTC_OK;
}

void AdapterDestroyMutex(AdapterMutexId *id)
{
    if (id == NULL) {
        ADAPTER_LOGW("invalid param");
        return;
    }

    osStatus_t status = osMutexDelete((osMutexId_t)id);
    if (status != osOK) {
        ADAPTER_LOGW("mutex delete error %d", status);
    }
}

AdapterSemId *AdapterCreateSem(uint32_t count)
{
    /* count为0是默认为二元信号量 */
    return (AdapterSemId *)osSemaphoreNew(count > 0 ? count : 1, count, NULL);
}

int32_t AdapterWaitSem(AdapterSemId *id, uint32_t ms)
{
    if (id == NULL) {
        ADAPTER_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    osStatus_t status = osSemaphoreAcquire((osSemaphoreId_t)id, MsToTick(ms));
    if (status == osOK) {
        return IOTC_OK;
    }
    if (status == osErrorTimeout) {
        return IOTC_ERR_TIMEOUT;
    }
    ADAPTER_LOGW("wait sem error %d", status);

    return IOTC_ADAPTER_OS_ERR_WAIT_SEM;
}

int32_t AdapterPostSem(AdapterSemId *id)
{
    if (id == NULL) {
        ADAPTER_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    osStatus_t status = osSemaphoreRelease((osSemaphoreId_t)id);
    if (status != osOK) {
        ADAPTER_LOGW("post sem error %d", status);
        return IOTC_ADAPTER_OS_ERR_POST_SEM;
    }

    return IOTC_OK;
}

uint32_t AdapterGetSemCount(AdapterSemId *id)
{
    if (id == NULL) {
        ADAPTER_LOGW("invalid param");
        return 0;
    }
    return osSemaphoreGetCount((osSemaphoreId_t)id);
}

void AdapterDestroySem(AdapterSemId *id)
{
    if (id == NULL) {
        ADAPTER_LOGW("invalid param");
        return;
    }

    osStatus_t status = osSemaphoreDelete((osSemaphoreId_t)id);
    if (status != osOK) {
        ADAPTER_LOGW("sem delete error %d", status);
    }
}

int32_t AdapterSleepMs(uint32_t ms)
{
    (void)osDelay(MsToTick(ms));
    return IOTC_OK;
}

void AdapterSchedYield(void)
{
    (void)osThreadYield();
}

uint32_t AdapterGetSysTimeMs(void)
{
    if (osKernelGetTickFreq() == 0) {
        ADAPTER_LOGW("invalid tick freq");
        return 0;
    }
    return ((uint32_t)osKernelGetTickCount() * MS_PER_SECOND / osKernelGetTickFreq());
}

int32_t AdapterGetErrno(void)
{
    return errno;
}