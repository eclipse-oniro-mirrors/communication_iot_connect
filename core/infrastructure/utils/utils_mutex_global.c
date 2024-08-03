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
#include "utils_mutex_global.h"
#include "utils_mutex_ex.h"
#include "iotc_log.h"
#include "comm_def.h"
#include "iotc_errcode.h"

static UtilsExMutex *g_globalMutex = NULL;

int32_t UtilsGlobalMutexInit(void)
{
    if (g_globalMutex != NULL) {
        return IOTC_OK;
    }
    g_globalMutex = UtilsCreateExMutex();
    if (g_globalMutex == NULL) {
        IOTC_LOGF("create global lock error");
        return IOTC_CORE_COMM_UTILS_ERR_EX_MUTEX_CREATE;
    }
    return IOTC_OK;
}

void UtilsGlobalMutexDeinit(void)
{
    if (g_globalMutex != NULL) {
        UtilsDestroyExMutex(&g_globalMutex);
    }
}

bool UtilsGlobalMutexLockInner(const char *func)
{
    if (g_globalMutex == NULL) {
        g_globalMutex = UtilsCreateExMutex();
        if (g_globalMutex == NULL) {
            IOTC_LOGF("create global lock error");
            return false;
        }
    }

    return UtilsExMutexLockInner(g_globalMutex, UTILS_MUTEX_EX_DEFAULT_TIMEOUT_MS, func);
}

void UtilsGlobalMutexUnlockInner(const char *func)
{
    if (g_globalMutex == NULL) {
        IOTC_LOGW("unlock not init %s", func);
        return;
    }
    return UtilsExMutexUnlockInner(g_globalMutex, func);
}
