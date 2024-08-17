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
#include "dfx_watch_dog.h"
#include <stdbool.h>
#include <stdio.h>
#include "securec.h"
#include "adapter_os.h"
#include "iotc_errcode.h"
#include "utils_list.h"
#include "utils_mutex_ex.h"
#include "utils_assert.h"

#ifndef DFX_WATCH_DOG_SCHEDULE_INTERVAL
#define DFX_WATCH_DOG_SCHEDULE_INTERVAL UTILS_SEC_TO_MS(2)
#endif

#define DFX_WATCH_DOG_API_WAIT_TIME UTILS_SEC_TO_MS(30)

#define DFX_WATCH_DOG_TASK_NAME "iotc_watchdog"
#define DFX_WATCH_DOG_DEFAULT_TASK_STACK_SIZE (2 * 1024)
#define DFX_WATCH_DOG_TASK_NAME_LEN 128

typedef struct {
    bool flag;
    uint32_t timeoutMs;
    uint32_t runningMs;
    DfxWatchDogTimeoutHandler hdl;
    const char *name;
    IotcTaskId *id;
    ListEntry list;
} WatchdogNode;

typedef struct {
    const char *name;
    IotcTaskId *id;
    ListEntry list;
} RecordNode;

typedef struct {
    IotcTaskId *id;
    bool isRunning;
    ListEntry watchDogList;
    ListEntry recordList;
    UtilsExMutex *mutex;
    IotcSemId *sem;
} WatchdogContext;

static uint32_t g_taskSize = DFX_WATCH_DOG_DEFAULT_TASK_STACK_SIZE;
static DfxWatchDogTimeoutHandler g_commHdl = NULL;
static WatchdogContext *GetWatchDogCtx(void)
{
    static WatchdogContext ctx;
    return &ctx;
}

#define WATCH_DOG_LOCK() (void)UtilsExMutexLock(GetWatchDogCtx()->mutex)
#define WATCH_DOG_UNLOCK() UtilsExMutexUnlock(GetWatchDogCtx()->mutex)
            
static bool WatchDogProc(void)
{
    bool isRunning = true;
    DfxWatchDogTimeoutHandler func = NULL;
    WATCH_DOG_LOCK();
    isRunning = GetWatchDogCtx()->isRunning;
    if (!isRunning) {
        WATCH_DOG_UNLOCK();
        return isRunning;
    }

    bool timeout = false;
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &GetWatchDogCtx()->watchDogList) {
        WatchdogNode *node = CONTAINER_OF(item, WatchdogNode, list);
        if (node->timeoutMs == 0 || !node->flag) {
            continue;
        } else if (node->timeoutMs > node->runningMs) {
            node->runningMs += DFX_WATCH_DOG_SCHEDULE_INTERVAL;
        } else {
            node->runningMs = 0;
            IOTC_LOGF("Watchdog[%s,%u] timeout", node->name, node->timeoutMs);
            func = node->hdl;
            timeout = true;
            break;
        }
    }
    WATCH_DOG_UNLOCK();
    if (timeout) {
        DfxDumpAllTask();
        if (func == NULL) {
            IOTC_LOGI("use comm hdl");
            func = g_commHdl;
        }
        if (func != NULL) {
            IOTC_LOGI("watch dog hdl process");
            func();
        }
    }
    return isRunning;
}

static void WatchDogTaskBody(void *arg)
{
    NOT_USED(arg);
    GetWatchDogCtx()->isRunning = true;
    bool isRunning = true;
    do {
        IotcSleepMs(DFX_WATCH_DOG_SCHEDULE_INTERVAL);
        isRunning = WatchDogProc();
    } while (isRunning);

    WATCH_DOG_LOCK();
    if (GetWatchDogCtx()->sem != NULL) {
        IotcSemPost(GetWatchDogCtx()->sem);
    }

    WATCH_DOG_UNLOCK();
    IOTC_LOGN("Watch Dog task exit");
    IotcTaskDelete(NULL);
}

int32_t DfxWatchDogInit(void)
{
    (void)memset_s(GetWatchDogCtx(), sizeof(WatchdogContext), 0, sizeof(WatchdogContext));
    LIST_INIT(&GetWatchDogCtx()->watchDogList);
    LIST_INIT(&GetWatchDogCtx()->recordList);
    GetWatchDogCtx()->mutex = UtilsCreateExMutex();
    if (GetWatchDogCtx()->mutex == NULL) {
        return IOTC_CORE_COMM_UTILS_ERR_EX_MUTEX_CREATE;
    }

    IotcTaskParam taskParam = {
        .func = WatchDogTaskBody,
        .prio = IOTC_TASK_PRIORITY_MID,
        .stackSize = g_taskSize,
        .arg = NULL,
        .name = DFX_WATCH_DOG_TASK_NAME,
    };
    GetWatchDogCtx()->id = IotcTaskCreate(&taskParam);
    if (GetWatchDogCtx()->id == NULL) {
        IOTC_LOGW("WatchDog task create fail %u", g_taskSize);
        UtilsDestroyExMutex(&GetWatchDogCtx()->mutex);
        return IOTC_ADAPTER_OS_ERR_CREATE_TASK;
    }

    return IOTC_OK;
}

void DfxWatchDogDeinit(void)
{
    int32_t ret;
    if (GetWatchDogCtx()->isRunning) {
        WATCH_DOG_LOCK();
        GetWatchDogCtx()->sem = IotcSemCreate(0);
        GetWatchDogCtx()->isRunning = false;
        WATCH_DOG_UNLOCK();
        if (GetWatchDogCtx()->sem != NULL) {
            ret = IotcSemWait(GetWatchDogCtx()->sem, DFX_WATCH_DOG_API_WAIT_TIME);
            if (ret != IOTC_OK) {
                IOTC_LOGW("wait sem error %d", ret);
            }
            WATCH_DOG_LOCK();
            IotcSemDestroy(GetWatchDogCtx()->sem);
            GetWatchDogCtx()->sem = NULL;
            WATCH_DOG_UNLOCK();
        } else {
            IOTC_LOGF("create sem error");
        }
    }

    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &GetWatchDogCtx()->recordList) {
        RecordNode *node = CONTAINER_OF(item, RecordNode, list);
        LIST_REMOVE(item);
        IotcFree(node);
    }

    LIST_FOR_EACH_ITEM_SAFE(item, next, &GetWatchDogCtx()->watchDogList) {
        WatchdogNode *node = CONTAINER_OF(item, WatchdogNode, list);
        LIST_REMOVE(item);
        IotcFree(node);
    }

    if (GetWatchDogCtx()->mutex != NULL) {
        UtilsDestroyExMutex(&GetWatchDogCtx()->mutex);
    }

    (void)memset_s(GetWatchDogCtx(), sizeof(WatchdogContext), 0, sizeof(WatchdogContext));
    return;
}

int32_t DfxAddWatchDog(uint32_t timeoutMs, DfxWatchDogTimeoutHandler hdl, const char *name)
{
    CHECK_RETURN_LOGW(timeoutMs != 0 && name != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    WatchdogNode *newNode = (WatchdogNode *)IotcMalloc(sizeof(WatchdogNode));
    if (newNode == NULL) {
        IOTC_LOGE("malloc");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(newNode, sizeof(WatchdogNode), 0, sizeof(WatchdogNode));

    newNode->name = name;
    newNode->timeoutMs = timeoutMs;
    newNode->hdl = hdl;
    IotcTaskId *id = IotcTaskGetCurrentTaskId();
    newNode->id = id;

    WATCH_DOG_LOCK();
    LIST_INSERT_BEFORE(&newNode->list, &GetWatchDogCtx()->watchDogList);
    WATCH_DOG_UNLOCK();

    IOTC_LOGN("WatchDog[%s/%u/%u] add succ", NON_NULL_STR(name), timeoutMs, UtilsGetTaskIdShort(id));
    return IOTC_OK;
}

int32_t DfxFeedWatchDog(void)
{
    IotcTaskId *id = IotcTaskGetCurrentTaskId();
    int32_t ret = IOTC_CORE_COMM_DFX_WATCH_DOG_ERR_FEED;
    WATCH_DOG_LOCK();
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &GetWatchDogCtx()->watchDogList) {
        WatchdogNode *node = CONTAINER_OF(item, WatchdogNode, list);
        if (node->id != id) {
            continue;
        }
        node->flag = true;
        node->runningMs = 0;
        ret = IOTC_OK;
        break;
    }
    WATCH_DOG_UNLOCK();
    if (ret == IOTC_OK) {
        return ret;
    }

    IOTC_LOGE("WatchDog[%u] feed fail", UtilsGetTaskIdShort(id));
    return ret;
}

void DfxDelWatchDog(void)
{
    IotcTaskId *id = IotcTaskGetCurrentTaskId();
    WATCH_DOG_LOCK();
    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &GetWatchDogCtx()->watchDogList) {
        WatchdogNode *node = CONTAINER_OF(item, WatchdogNode, list);
        if (node->id != id) {
            continue;
        }
        IOTC_LOGN("WatchDog[%s/%u] del succ", NON_NULL_STR(node->name), UtilsGetTaskIdShort(node->id));
        LIST_REMOVE(item);
        IotcFree(node);
        break;
    }
    WATCH_DOG_UNLOCK();
}

void DfxRecordNoDogTask(const char *name)
{
    CHECK_V_RETURN_LOGW(name != NULL, "param invalid");
    RecordNode *newNode = (RecordNode *)IotcMalloc(sizeof(RecordNode));
    if (newNode == NULL) {
        IOTC_LOGE("malloc");
        return;
    }
    (void)memset_s(newNode, sizeof(RecordNode), 0, sizeof(RecordNode));
    newNode->name = name;
    newNode->id = IotcTaskGetCurrentTaskId();
    WATCH_DOG_LOCK();
    LIST_INSERT_BEFORE(&newNode->list, &GetWatchDogCtx()->recordList);
    WATCH_DOG_UNLOCK();
    return;
}

void DfxDelNoDogTaskRecord(void)
{
    IotcTaskId *id = IotcTaskGetCurrentTaskId();

    WATCH_DOG_LOCK();
    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &GetWatchDogCtx()->watchDogList) {
        RecordNode *node = CONTAINER_OF(item, RecordNode, list);
        if (node->id == id) {
            IOTC_LOGN("NoDog[%s/%p] del succ", NON_NULL_STR(node->name), UtilsGetTaskIdShort(node->id));
            LIST_REMOVE(item);
            IotcFree(node);
            return;
        }
    }
    WATCH_DOG_UNLOCK();
    return;
}

void DfxSetWatchDogTaskSize(uint32_t size)
{
    g_taskSize = size;
    IOTC_LOGI("watch dog task size %u", size);
}

void DfxDumpAllTask(void)
{
    WATCH_DOG_LOCK();
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &GetWatchDogCtx()->watchDogList) {
        WatchdogNode *node = CONTAINER_OF(item, WatchdogNode, list);
        IOTC_LOGN("watch dog task [%s/%u/%u/%u/%d]", NON_NULL_STR(node->name), UtilsGetTaskIdShort(node->id),
            node->timeoutMs, node->runningMs, node->flag);
    }
    LIST_FOR_EACH_ITEM(item, &GetWatchDogCtx()->recordList) {
        RecordNode *node = CONTAINER_OF(item, RecordNode, list);
        IOTC_LOGN("record task [%s/%u/%u/%u]", NON_NULL_STR(node->name), UtilsGetTaskIdShort(node->id));
    }

    WATCH_DOG_UNLOCK();
}

void DfxWatchDogRegCommTimeoutHandler(DfxWatchDogTimeoutHandler hdl)
{
    g_commHdl = hdl;
}