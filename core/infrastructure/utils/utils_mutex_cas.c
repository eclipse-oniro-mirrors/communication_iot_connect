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
#include "utils_cas_mutex.h"
#include "securec.h"
#include "iotc_log.h"
#include "utils_common.h"

#define UTILS_CAS_OLD_VAL 0
#define UTILS_CAS_NEW_VAL 1
#define UTILS_CAS_SCHEDULE_INTERVAL 1

struct tagUtilsCasMutex {
    int32_t mutexId;
    const char *func;
    uint32_t time;
    bool isLock;
    uint16_t thread;
};

static bool CasMutexLock(int32_t *mutex, uint32_t timeoutMs)
{
    uint32_t count = 0;
    while ((count++ < timeoutMs) || (timeoutMs == UTILS_MUTEX_CAS_WAIT_FOREVER)) {
        if (__sync_bool_compare_and_swap(mutex, UTILS_CAS_OLD_VAL, UTILS_CAS_NEW_VAL)) {
            return true;
        }
        AdapterSleepMs(UTILS_CAS_SCHEDULE_INTERVAL);
    }

    return false;
}

UtilsCasMutex *UtilsCreateCasMutex(void)
{
    UtilsCasMutex *mutex = AdapterMalloc(sizeof(UtilsCasMutex));
    if (mutex == NULL) {
        IOTC_LOGW("malloc error");
        return NULL;
    }

    (void)memset_s(mutex, sizeof(UtilsCasMutex), 0, sizeof(UtilsCasMutex));

    mutex->func = "INIT";
    mutex->mutexId = UTILS_CAS_OLD_VAL;
    return mutex;
}

bool UtilsCasMutexLockInner(UtilsCasMutex *mutex, uint32_t timeout, const char *func)
{
    if (mutex == NULL || func == NULL) {
        IOTC_LOGW("param invalid");
        return false;
    }

    bool ret = CasMutexLock(&mutex->mutexId, timeout);
    uint32_t curTime = AdapterGetSysTimeMs();
    uint16_t curThread = UtilsGetCurTaskIdShort();
    if (!ret) {
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

void UtilsCasMutexUnlockInner(UtilsCasMutex *mutex, const char *func)
{
    if (mutex == NULL || func == NULL) {
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
    mutex->mutexId = UTILS_CAS_OLD_VAL;
    return;
}

void UtilsDestroyCasMutex(UtilsCasMutex **mutex)
{
    if (mutex == NULL || *mutex == NULL) {
        IOTC_LOGW("param invalid");
        return;
    }

    UTILS_FREE_2_NULL(*mutex);
    return;
}