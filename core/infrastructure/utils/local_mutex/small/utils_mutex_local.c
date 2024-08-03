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
#include "utils_mutex_local.h"
#include <stddef.h>
#include "comm_def.h"
#include "utils_mutex_ex.h"
#include "iotc_errcode.h"

int32_t UtilsCreateMutexLocal(UtilsMutexLocal *mutex)
{
    if (mutex == NULL) {
        return IOTC_ERR_PARAM_INVALID;
    }
    mutex->handle = (void *)UtilsCreateExMutex();
    if (mutex->handle == NULL) {
        return IOTC_CORE_COMM_UTILS_ERR_EX_MUTEX_CREATE;
    }
    return IOTC_OK;
}

void UtilsDestroyMutexLocal(UtilsMutexLocal *mutex)
{
    if (mutex != NULL && mutex->handle != NULL) {
        UtilsDestroyExMutex(mutex->handle);
        mutex->handle = NULL;
    }
    return;
}

bool UtilsMutexLocalLockInner(UtilsMutexLocal *mutex, const char *func)
{
    if (mutex == NULL || func == NULL || mutex->handle == NULL) {
        return false;
    }
    return UtilsExMutexLockInner(mutex->handle, UTILS_MUTEX_EX_DEFAULT_TIMEOUT_MS, func);
}

void UtilsMutexLocalUnlockInner(UtilsMutexLocal *mutex, const char *func)
{
    if (mutex == NULL || func == NULL || mutex->handle == NULL) {
        return;
    }
    UtilsExMutexUnlockInner(mutex->handle, func);
}