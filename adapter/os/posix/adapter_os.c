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
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/select.h>
#include "iotc_errcode.h"
#include "adapter_log.h"
#include "adapter_mem.h"
#include "iotc_conf.h"
#if IOTC_CONF_MEM_DEBUG
#include <stdlib.h>
#endif

#ifndef MS_PER_SECOND
#define MS_PER_SECOND   (1000UL)
#endif
#ifndef US_PER_MS
#define US_PER_MS       (1000UL)
#endif
#ifndef NS_PER_MS
#define NS_PER_MS       (1000000UL)
#endif
#ifndef NS_PER_SECOND
#define NS_PER_SECOND   (1000000000UL)
#endif
#define NS_PER_US   1000

IotcTaskId *IotcTaskCreate(IotcTaskParam *param)
{
    if ((param == NULL) || (param->func == NULL) || (param->stackSize == 0)) {
        ADAPTER_LOGW("invalid param");
        return NULL;
    }

    pthread_t tid = 0;
    pthread_attr_t attr;

    uint32_t stackSize = param->stackSize > PTHREAD_STACK_MIN ? param->stackSize : PTHREAD_STACK_MIN;

    int32_t ret = pthread_attr_init(&attr);
    if (ret != 0) {
        ADAPTER_LOGW("pthread init attr error %d", ret);
        return NULL;
    }

    do {
        ret = pthread_attr_setstacksize(&attr, stackSize);
        if (ret != 0) {
            ADAPTER_LOGW("pthread set stack size %u error %d", stackSize, ret);
            break;
        }

        ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if (ret != 0) {
            ADAPTER_LOGW("pthread set detach error %d", ret);
            break;
        }

        ret = pthread_attr_setschedpolicy(&attr, SCHED_RR);
        if (ret != 0) {
            ADAPTER_LOGW("pthread set sched error %d", ret);
        }

        ret = pthread_create(&tid, &attr, (void *)param->func, param->arg);
        if (ret != 0) {
            ADAPTER_LOGW("pthread create error %d", ret);
            break;
        }
    } while (false);

    (void)pthread_attr_destroy(&attr);
    if (ret != 0) {
        return NULL;
    }

    return (IotcTaskId *)tid;
}

int32_t IotcTaskSuspend(IotcTaskId *id)
{
    (void)id;
    return IOTC_ERR_NOT_SUPPORT;
}

int32_t IotcTaskResume(IotcTaskId *id)
{
    (void)id;
    return IOTC_ERR_NOT_SUPPORT;
}

void IotcTaskDelete(IotcTaskId *id)
{
    (void)id;
    return;
}

IotcTaskId *IotcTaskGetCurrentTaskId(void)
{
    return (IotcTaskId *)pthread_self();
}

IotcMutexId *IotcMutexCreate(void)
{
#if IOTC_CONF_MEM_DEBUG
    pthread_mutex_t *mutex = malloc(sizeof(pthread_mutex_t));
#else
    pthread_mutex_t *mutex = IotcMalloc(sizeof(pthread_mutex_t));
#endif
    if (mutex == NULL) {
        ADAPTER_LOGW("malloc error");
        return NULL;
    }

    int32_t ret = pthread_mutex_init(mutex, NULL);
    if (ret != 0) {
        IotcFree(mutex);
        ADAPTER_LOGW("pthread mutex init error %d", ret);
        return NULL;
    }

    return (IotcMutexId *)mutex;
}

int32_t IotcMutexLock(IotcMutexId *id, uint32_t ms)
{
    if (id == NULL) {
        ADAPTER_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    int32_t ret;
    if (ms == IOTC_WAIT_FOREVER) {
        ret = pthread_mutex_lock((pthread_mutex_t *)id);
        if (ret != 0) {
            ADAPTER_LOGW("mutex lock error %d", ret);
            return IOTC_ADAPTER_OS_ERR_MUTEX_LOCK;
        } else {
            return IOTC_OK;
        }
    }

    struct timespec ts;
    ret = clock_gettime(CLOCK_REALTIME, &ts);
    if (ret != 0) {
        ADAPTER_LOGW("get time error %d", ret);
        return IOTC_ADAPTER_OS_ERR_MUTEX_LOCK;
    }

    ts.tv_sec += ms / MS_PER_SECOND;
    int64_t nsec = (int64_t)ms % MS_PER_SECOND * NS_PER_MS;
    if (ts.tv_nsec >= (LONG_MAX - nsec)) {
        ts.tv_nsec -= (LONG_MAX - nsec);
        ts.tv_sec += (LONG_MAX % NS_PER_SECOND);
    } else {
        ts.tv_nsec += nsec;
    }

    ret = pthread_mutex_timedlock((pthread_mutex_t *)id, &ts);
    if (ret != 0) {
        if (ret == ETIMEDOUT) {
            ADAPTER_LOGN("wait mutex timeout %u ms", ms);
            return IOTC_ERR_TIMEOUT;
        }
        ADAPTER_LOGW("mutex lock timeout %u error %d", ms, ret);
        return IOTC_ADAPTER_OS_ERR_MUTEX_LOCK;
    }
    return IOTC_OK;
}

int32_t IotcMutexUnlock(IotcMutexId *id)
{
    if (id == NULL) {
        ADAPTER_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    int32_t ret = pthread_mutex_unlock((pthread_mutex_t *)id);
    if (ret != 0) {
        ADAPTER_LOGW("mutex unlock error %d", ret);
        return IOTC_ADAPTER_OS_ERR_MUTEX_UNLOCK;
    }

    return IOTC_OK;
}

void IotcMutexDestroy(IotcMutexId *id)
{
    if (id == NULL) {
        ADAPTER_LOGW("invalid param");
        return;
    }

    int32_t ret = pthread_mutex_destroy((pthread_mutex_t *)id);
    if (ret != 0) {
        ADAPTER_LOGW("mutex release error %d", ret);
    }
#if IOTC_CONF_MEM_DEBUG
    free(id);
#else
    IotcFree(id);
#endif
    return;
}

IotcSemId *IotcSemCreate(uint32_t count)
{
    sem_t *sem = IotcMalloc(sizeof(sem_t));
    if (sem == NULL) {
        ADAPTER_LOGW("malloc error");
        return NULL;
    }

    int32_t ret = sem_init(sem, 0, count);
    if (ret != 0) {
        IotcFree(sem);
        ADAPTER_LOGW("sem init error %d count %u", ret, count);
        return NULL;
    }

    return (IotcSemId *)sem;
}

int32_t IotcSemWait(IotcSemId *id, uint32_t ms)
{
    if (id == NULL) {
        ADAPTER_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    struct timespec ts;
    int32_t ret = clock_gettime(CLOCK_REALTIME, &ts);
    if (ret != 0) {
        ADAPTER_LOGW("get time error %d", ret);
        return IOTC_ADAPTER_OS_ERR_WAIT_SEM;
    }

    ts.tv_sec += ms / MS_PER_SECOND;
    int64_t nsec = (int64_t)ms % MS_PER_SECOND * NS_PER_MS;
    if (ts.tv_nsec >= (NS_PER_SECOND - nsec)) {
        ts.tv_nsec -= (NS_PER_SECOND - nsec);
        ts.tv_sec += 1;
    } else {
        ts.tv_nsec += nsec;
    }

    ret = sem_timedwait((sem_t *)id, &ts);
    if (ret != 0) {
        if (errno == ETIMEDOUT) {
            return IOTC_ERR_TIMEOUT;
        }
        ADAPTER_LOGW("sem wait error %d/%d/%u", ret, errno, ms);
        return IOTC_ADAPTER_OS_ERR_WAIT_SEM;
    }
    return IOTC_OK;
}

int32_t IotcSemPost(IotcSemId *id)
{
    if (id == NULL) {
        ADAPTER_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    int32_t ret = sem_post((sem_t *)id);
    if (ret != 0) {
        ADAPTER_LOGW("sem post error %d", ret);
        return IOTC_ADAPTER_OS_ERR_POST_SEM;
    }

    return IOTC_OK;
}

uint32_t IotcSemGetCount(IotcSemId *id)
{
    if (id == NULL) {
        ADAPTER_LOGW("invalid param");
        return 0;
    }
    uint32_t count = 0;
    int32_t ret = sem_getvalue((sem_t *)id, (int32_t *)&count);
    if (ret != 0) {
        ADAPTER_LOGW("get sem count error %d", ret);
        return 0;
    }
    return count;
}

void IotcSemDestroy(IotcSemId *id)
{
    if (id == NULL) {
        ADAPTER_LOGW("invalid param");
        return;
    }

    int32_t ret = sem_destroy((sem_t *)id);
    if (ret != 0) {
        ADAPTER_LOGW("sem release error %d", ret);
    }
    IotcFree(id);
    return;
}

int32_t IotcSleepMs(uint32_t ms)
{
    int32_t ret;
    struct timeval tv;
    tv.tv_sec = ms / MS_PER_SECOND;
    tv.tv_usec = (ms % MS_PER_SECOND) * US_PER_MS;

    do {
        ret = select(0, NULL, NULL, NULL, &tv);
    } while ((ret == -1) && (errno == EINTR));

    if (ret != 0) {
        return IOTC_ADAPTER_OS_ERR_SLEEP;
    }

    return IOTC_OK;
}

void IotcSchedYield(void)
{
    (void)sleep(0);
}

uint32_t IotcGetSysTimeMs(void)
{
    struct timespec ts;
    int32_t ret = clock_gettime(CLOCK_MONOTONIC, &ts);
    if (ret != 0) {
        ADAPTER_LOGW("get time error %d", ret);
        return 0;
    }

    return (uint32_t)(ts.tv_sec * MS_PER_SECOND + ts.tv_nsec / NS_PER_MS);
}

int32_t IotcGetErrno(void)
{
    return errno;
}