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
#include "utils_mutex_ex.h"
#include "securec.h"
#include "iotc_log.h"
#include "utils_common.h"
#include "iotc_errcode.h"

struct UtilsExMutex {
    AdapterMutexId *mutexId;
    const char *func;
    uint32_t time;
    bool isLock;
    uint16_t thread;
};

UtilsExMutex *UtilsCreateExMutex(void)
{
    UtilsExMutex *mutex = AdapterMalloc(sizeof(UtilsExMutex));
    if (mutex == NULL) {
        IOTC_LOGW("malloc error");
        return NULL;
    }

    (void)memset_s(mutex, sizeof(UtilsExMutex), 0, sizeof(UtilsExMutex));

    mutex->func = "INIT";
    mutex->mutexId = AdapterCreateMutex();
    if (mutex->mutexId != NULL) {
        return mutex;
    }
    IOTC_LOGW("create mutex error");

    UtilsDestroyExMutex(&mutex);
    return NULL;
}

bool UtilsExMutexLockInner(UtilsExMutex *mutex, uint32_t timeout, const char *func)
{
    if (mutex == NULL || func == NULL || mutex->mutexId == NULL) {
        IOTC_LOGW("param invalid");
        return false;
    }

    int32_t ret = AdapterLockMutex(mutex->mutexId, timeout);
    uint32_t curTime = AdapterGetSysTimeMs();
    uint16_t curThread = UtilsGetCurTaskIdShort();
    if (ret != IOTC_OK) {
        /* 持有函数/持有时间/持有线程，当前函数/超时时间/当前线程 */
        IOTC_LOGF("lock tmo! %s/%lu/%u,%s/%u/%u",
            mutex->func, UtilsDeltaTime(curTime, mutex->time), mutex->thread, func, timeout, curThread);
        return false;
    }

    mutex->thread = curThread;
    mutex->time = curTime;
    mutex->func = func;
    mutex->isLock = true;
    return true;
}

void UtilsExMutexUnlockInner(UtilsExMutex *mutex, const char *func)
{
    if (mutex == NULL || func == NULL || mutex->mutexId == NULL) {
        IOTC_LOGW("param invalid");
        return;
    }

    uint32_t curTime = AdapterGetSysTimeMs();
    uint16_t curThread = UtilsGetCurTaskIdShort();

    if (!mutex->isLock) {
        /* 仅做维测，不退出 释放函数/释放时间/释放线程，当前函数/当前线程 */
        IOTC_LOGF("multi unlock! %s/%lu/%u,%s/%u",
            mutex->func, UtilsDeltaTime(curTime, mutex->time), mutex->thread, func, curThread);
    }

    mutex->thread = curThread;
    mutex->time = curTime;
    mutex->func = func;
    mutex->isLock = false;
    int32_t ret = AdapterUnlockMutex(mutex->mutexId);
    if (ret != IOTC_OK) {
        IOTC_LOGE("unlock error %s/%d", func, ret);
    }
    return;
}

void UtilsDestroyExMutex(UtilsExMutex **mutex)
{
    if (mutex == NULL || *mutex == NULL) {
        IOTC_LOGW("param invalid");
        return;
    }

    if ((*mutex)->mutexId != NULL) {
        AdapterDestroyMutex((*mutex)->mutexId);
        (*mutex)->mutexId = NULL;
    }

    UTILS_FREE_2_NULL(*mutex);
    return;
}