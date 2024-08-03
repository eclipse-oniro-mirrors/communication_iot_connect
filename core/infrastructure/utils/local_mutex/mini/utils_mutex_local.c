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
#include "utils_mutex_global.h"
#include "iotc_errcode.h"

int32_t UtilsCreateMutexLocal(UtilsMutexLocal *mutex)
{
    if (mutex == NULL) {
        return IOTC_ERR_PARAM_INVALID;
    }
    static uint8_t initFlag = 0;
    mutex->handle = &initFlag;
    return IOTC_OK;
}

void UtilsDestroyMutexLocal(UtilsMutexLocal *mutex)
{
    if (mutex == NULL) {
        return;
    }
    mutex->handle = NULL;
    return;
}

bool UtilsMutexLocalLockInner(UtilsMutexLocal *mutex, const char *func)
{
    if (mutex == NULL || func == NULL || mutex->handle == NULL) {
        return false;
    }
    return UtilsGlobalMutexLockInner(func);
}

void UtilsMutexLocalUnlockInner(UtilsMutexLocal *mutex, const char *func)
{
    if (mutex == NULL || func == NULL || mutex->handle == NULL) {
        return;
    }
    UtilsGlobalMutexUnlockInner(func);
}