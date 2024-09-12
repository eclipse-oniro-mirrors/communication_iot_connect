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
#include "fwk_main.h"
#include "utils_mutex_global.h"
#include "iotc_os.h"
#include "iotc_log.h"
#include "utils_common.h"
#include "main_loop.h"
#include "sched_executor.h"
#include "fwk_init.h"
#include "event_bus.h"
#include "iotc_errcode.h"
#include "product_adapter.h"

static bool g_existsFlag = false;
static IotcSemId *g_resetSem = NULL;
static IotcSemId *g_exitSem = NULL;
static IotcTaskId *g_mainTaskId = NULL;
static uint32_t g_stackSize = 0;
#define API_MAX_BLOCK_SLEEP_TIME_SEC UTILS_SEC_TO_MS(30)

static void MainTaskBehaviorNotice(IotcSemId **sem)
{
    if (!UtilsGlobalMutexLock()) {
        return;
    }
    if (*sem != NULL)  {
        int32_t ret = IotcSemPost(*sem);
        if (ret != IOTC_OK) {
            IOTC_LOGE("post sem error %d", ret);
        }
    }
    UtilsGlobalMutexUnlock();
}

static void MainTaskExistsFlagChange(bool flag)
{
    if (!UtilsGlobalMutexLock()) {
        return;
    }
    g_existsFlag = flag;
    UtilsGlobalMutexUnlock();
}

static void IotcFwkMainTaskBody(void *arg)
{
    NOT_USED(arg);
    IOTC_LOGN("iotc main task entry");

    do {
        /* 复位通知 */
        MainTaskBehaviorNotice(&g_resetSem);

        /* 全局初始化 */
        if (FwkInitAll(FWK_INIT_FAILED_RETURN) != IOTC_OK) {
            IotcSleepMs(UTILS_SEC_TO_MS(FWK_INIT_BLOCK_SLEEP_TIME_SEC));
            continue;
        }
        EventBusPublishAsync(IOTC_CORE_COMM_EVENT_MAIN_INITIALIZED, NULL, 0, NULL);

        /* 主业务循环 */
        FwkMainLoopEntry(g_stackSize);

        if (g_existsFlag) {
            EventBusPublishSync(IOTC_CORE_COMM_EVENT_MAIN_RESET, NULL, 0);
        } else {
            EventBusPublishSync(IOTC_CORE_COMM_EVENT_MAIN_QUIT, NULL, 0);
        }
        /* 全局去初始化 */
        FwkDeinitAll();
    } while (g_existsFlag);

    /* 退出通知 */
    g_mainTaskId = NULL;
    MainTaskBehaviorNotice(&g_exitSem);
    IOTC_LOGN("iotc main task exit");
}

int32_t IotcFwkMain(uint32_t stackSize)
{
    IOTC_LOGN("iotc main");
    if (stackSize == 0) {
        return IOTC_ERR_PARAM_INVALID;
    }
    g_stackSize = stackSize;

    IotcTaskParam task = {
        .arg = NULL,
        .func = IotcFwkMainTaskBody,
        .name = "iotc_main_task",
        .prio = IOTC_TASK_PRIORITY_MID,
        .stackSize = stackSize,
    };

    MainTaskExistsFlagChange(true);
    g_mainTaskId = IotcTaskCreate(&task);
    if (g_mainTaskId == NULL) {
        IOTC_LOGF("create iotc main task error %u", task.stackSize);
        MainTaskExistsFlagChange(false);
        return IOTC_ADAPTER_OS_ERR_CREATE_TASK;
    }

    return IOTC_OK;
}

static int32_t MainTaskCreateSem(IotcSemId **sem)
{
    if (!UtilsGlobalMutexLock()) {
        return IOTC_ERR_TIMEOUT;
    }

    if (*sem != NULL) {
        UtilsGlobalMutexUnlock();
        IOTC_LOGE("reset api multi invoke");
        return IOTC_SDK_MNGR_ERR_MULTI_INVOKE;
    }

    *sem = IotcSemCreate(0);
    UtilsGlobalMutexUnlock();
    if (*sem == NULL) {
        IOTC_LOGE("create sem error");
        return IOTC_ADAPTER_OS_ERR_CREATE_SEM;
    }
    return IOTC_OK;
}

static int32_t MainTaskWaitAndDestroySem(IotcSemId **sem)
{
    int32_t ret = IotcSemWait(*sem, API_MAX_BLOCK_SLEEP_TIME_SEC);
    if (ret != IOTC_OK) {
        IOTC_LOGE("sem timeout %d", ret);
    }

    (void)UtilsGlobalMutexLock();
    IotcSemDestroy(*sem);
    *sem = NULL;
    UtilsGlobalMutexUnlock();
    return ret;
}

int32_t IotcFwkReset(void)
{
    IOTC_LOGN("iotc reset");
    if (!g_existsFlag) {
        IOTC_LOGW("main task not exists");
        return IOTC_SDK_MNGR_ERR_MAIN_TASK_NOT_EXISTS;
    }
    if (IotcTaskGetCurrentTaskId() == g_mainTaskId) {
        IOTC_LOGW("invoke reset in main task");
        return IOTC_CORE_COMM_FWK_ERR_MAIN_INVOKE_RESET_INVALID_TASK;
    }

    int32_t ret = MainTaskCreateSem(&g_resetSem);
    if (ret != IOTC_OK) {
        return ret;
    }

    FwkMainLoopExit();

    return MainTaskWaitAndDestroySem(&g_resetSem);
}

int32_t IotcFwkStop(void)
{
    IOTC_LOGN("iotc stop");
    if (!g_existsFlag) {
        IOTC_LOGI("main task not exists");
        return IOTC_OK;
    }

    if (IotcTaskGetCurrentTaskId() == g_mainTaskId) {
        IOTC_LOGW("invoke stop in main task");
        return IOTC_CORE_COMM_FWK_ERR_MAIN_INVOKE_STOP_INVALID_TASK;
    }

    int32_t ret = MainTaskCreateSem(&g_exitSem);
    if (ret != IOTC_OK) {
        return ret;
    }

    MainTaskExistsFlagChange(false);
    FwkMainLoopExit();

    return MainTaskWaitAndDestroySem(&g_exitSem);
}

int32_t IotcFwkRestore(void)
{
    IOTC_LOGN("iotc restore");
    /* 发布恢复出厂事件，由订阅者完成操作 */
    EventBusPublishSync(IOTC_CORE_COMM_EVENT_MAIN_RESTORE, NULL, 0);

    int32_t ret = ProductDevReboot(IOTC_REBOOT_RESTORE);
    if (ret != IOTC_OK) {
        IOTC_LOGW("can not reboot %d", ret);
        IotcFwkNotifyReset();
    }
    return IOTC_OK;
}

void IotcFwkNotifyReset(void)
{
    IOTC_LOGN("notify reset");
    FwkMainLoopExit();
}

void IotcFwkNotifyStop(void)
{
    IOTC_LOGN("notify stop");
    MainTaskExistsFlagChange(false);
    FwkMainLoopExit();
}

bool IotcIsFwkMainRuning(void)
{
    return g_mainTaskId != NULL;
}