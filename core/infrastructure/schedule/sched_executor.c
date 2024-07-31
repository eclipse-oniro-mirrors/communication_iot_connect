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
#include "sched_executor.h"
#include "sched_event_loop.h"
#include "sched_msg_queue.h"
#include "adapter_os.h"
#include "utils_assert.h"
#include "comm_def.h"
#include "securec.h"
#include "utils_mutex_ex.h"
#include "utils_bit_map.h"
#include "service_manager.h"
#include "event_bus.h"
#include "iotc_errcode.h"

typedef enum  {
    EXECUTOR_TASK_BIT_MAP_FINISH = 0,
} ExecutorTaskBitMap;

typedef struct {
    SchedExecutorCallback cb;
    void *userData;
} ExecutorParam;

typedef struct {
    uint8_t bitMap;
    int32_t *errcode;
    void *inData;
    void **outData;
    SchedExecutorWaitCallback cb;
} ExecutorTaskInfo;

typedef struct {
    UtilsExMutex *taskLock;
    UtilsExMutex *apiLock;
    AdapterSemId *waitSem;
    ExecutorTaskInfo taskInfo;
} SchedExecutorContext;

static SchedExecutorContext *GetSchedExecutorCtx(void)
{
    static SchedExecutorContext ctx;
    return &ctx;
}

static void TimeoutUpdate(uint32_t *timeout, uint32_t begin)
{
    uint32_t delta = UtilsDeltaTime(AdapterGetSysTimeMs(), begin);
    *timeout = delta > *timeout ? 0 : *timeout - delta;
}

static void ExecutorMsgWaitHandler(const uint8_t *msg, uint32_t len)
{
    NOT_USED(msg);
    NOT_USED(len);
    (void)UtilsExMutexLock(GetSchedExecutorCtx()->taskLock);
    ExecutorTaskInfo *taskInfo = &GetSchedExecutorCtx()->taskInfo;
    if (taskInfo->cb != NULL && taskInfo->errcode != NULL) {
        *taskInfo->errcode = taskInfo->cb(taskInfo->inData, taskInfo->outData);
        IOTC_LOGD("executor task ret %d", *taskInfo->errcode);
        UTILS_BIT_SET(taskInfo->bitMap, EXECUTOR_TASK_BIT_MAP_FINISH);
        AdapterPostSem(GetSchedExecutorCtx()->waitSem);
    } else {
        IOTC_LOGW("invalid task");
    }
    UtilsExMutexUnlock(GetSchedExecutorCtx()->taskLock);
}

static int32_t SetTaskInfo(SchedExecutorWaitCallback cb, void *inData, void **outData,
    int32_t *errcode, uint32_t timeout)
{
    if (!UtilsExMutexLockTimeout(GetSchedExecutorCtx()->taskLock, timeout)) {
        IOTC_LOGW("task lock timeout %u", timeout);
        return IOTC_ERR_TIMEOUT;
    }

    UTILS_BIT_CLR(GetSchedExecutorCtx()->taskInfo.bitMap);
    ExecutorTaskInfo *taskInfo = &GetSchedExecutorCtx()->taskInfo;
    taskInfo->cb = cb;
    taskInfo->errcode = errcode;
    taskInfo->inData = inData;
    taskInfo->outData = outData;
    UtilsExMutexUnlock(GetSchedExecutorCtx()->taskLock);
    return IOTC_OK;
}

static void ClearTaskInfo(void)
{
    (void)UtilsExMutexLock(GetSchedExecutorCtx()->taskLock);
    (void)memset_s(&GetSchedExecutorCtx()->taskInfo, sizeof(ExecutorTaskInfo), 0, sizeof(ExecutorTaskInfo));
    UtilsExMutexUnlock(GetSchedExecutorCtx()->taskLock);
}

static int32_t GetTaskResult(int32_t waitRet)
{
    (void)UtilsExMutexLock(GetSchedExecutorCtx()->taskLock);
    int32_t ret = IOTC_ERR_TIMEOUT;
    if (UTILS_IS_BIT_SET(GetSchedExecutorCtx()->taskInfo.bitMap, EXECUTOR_TASK_BIT_MAP_FINISH)) {
        ret = IOTC_OK;
        if (waitRet != IOTC_OK) {
            (void)AdapterWaitSem(GetSchedExecutorCtx()->waitSem, 0);
        }
    }
    (void)memset_s(&GetSchedExecutorCtx()->taskInfo, sizeof(ExecutorTaskInfo), 0, sizeof(ExecutorTaskInfo));
    UtilsExMutexUnlock(GetSchedExecutorCtx()->taskLock);
    return ret;
}

int32_t SchedAsyncExecutorWait(SchedExecutorWaitCallback cb, void *inData, void **outData,
    int32_t *errcode, uint32_t timeout)
{
    CHECK_RETURN_LOGW(cb != NULL && errcode != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");
    if (GetSchedEventLoopTaskId() == AdapterGetCurrentTaskId()) {
        IOTC_LOGI("sched task invoke");
        *errcode = cb(inData, outData);
        return IOTC_OK;
    }

    uint32_t timeoutDynamic = timeout;
    uint32_t begin = AdapterGetSysTimeMs();
    if (!UtilsExMutexLockTimeout(GetSchedExecutorCtx()->apiLock, timeoutDynamic)) {
        IOTC_LOGW("api lock timeout %u", timeoutDynamic);
        return IOTC_ERR_TIMEOUT;
    }
    TimeoutUpdate(&timeoutDynamic, begin);
    int32_t ret = SetTaskInfo(cb, inData, outData, errcode, timeout);
    if (ret != IOTC_OK) {
        UtilsExMutexUnlock(GetSchedExecutorCtx()->apiLock);
        return ret;
    }

    TimeoutUpdate(&timeoutDynamic, begin);
    /* msg not use */
    ret = SchedMsgQueueSend((uint8_t *)&timeoutDynamic, sizeof(timeoutDynamic), ExecutorMsgWaitHandler, timeoutDynamic);
    if (ret != IOTC_OK) {
        IOTC_LOGW("task lock timeout %u", timeoutDynamic);
        ClearTaskInfo();
        UtilsExMutexUnlock(GetSchedExecutorCtx()->apiLock);
        return IOTC_ERR_TIMEOUT;
    }

    TimeoutUpdate(&timeoutDynamic, begin);
    IOTC_LOGD("executor wait %u", timeoutDynamic);
    ret = AdapterWaitSem(GetSchedExecutorCtx()->waitSem, timeoutDynamic);
    if (ret != IOTC_OK) {
        IOTC_LOGW("wait sem error %d", ret);
    }

    ret = GetTaskResult(ret);
    UtilsExMutexUnlock(GetSchedExecutorCtx()->apiLock);
    IOTC_LOGD("ret %d", ret);
    return ret;
}

int32_t SchedAsyncExecutorInit(void)
{
    (void)memset_s(GetSchedExecutorCtx(), sizeof(SchedExecutorContext), 0, sizeof(SchedExecutorContext));

    int32_t ret;
    do {
        GetSchedExecutorCtx()->apiLock = UtilsCreateExMutex();
        if (GetSchedExecutorCtx()->apiLock == NULL) {
            IOTC_LOGW("create ex mutex error");
            ret = IOTC_CORE_COMM_UTILS_ERR_EX_MUTEX_CREATE;
            break;
        }

        GetSchedExecutorCtx()->taskLock = UtilsCreateExMutex();
        if (GetSchedExecutorCtx()->taskLock == NULL) {
            IOTC_LOGW("create ex mutex error");
            ret = IOTC_CORE_COMM_UTILS_ERR_EX_MUTEX_CREATE;
            break;
        }

        GetSchedExecutorCtx()->waitSem = AdapterCreateSem(0);
        if (GetSchedExecutorCtx()->waitSem == NULL) {
            IOTC_LOGW("create sem error");
            ret = IOTC_ADAPTER_OS_ERR_CREATE_SEM;
            break;
        }
        ServiceManagerRegisterExecutor(SchedAsyncExecutor);
        ret = EventBusRegAsyncExecutor(SchedAsyncExecutor);
        if (ret != IOTC_OK) {
            IOTC_LOGW("reg async executor error %d", ret);
            break;
        }
        return IOTC_OK;
    } while (0);
    SchedAsyncExecutorDeinit();
    return ret;
}

void SchedAsyncExecutorDeinit(void)
{
    if (GetSchedExecutorCtx()->apiLock != NULL) {
        UtilsDestroyExMutex(&GetSchedExecutorCtx()->apiLock);
    }
    if (GetSchedExecutorCtx()->taskLock != NULL) {
        UtilsDestroyExMutex(&GetSchedExecutorCtx()->taskLock);
    }
    if (GetSchedExecutorCtx()->waitSem != NULL) {
        AdapterDestroySem(GetSchedExecutorCtx()->waitSem);
        GetSchedExecutorCtx()->waitSem = NULL;
    }
}

static void ExecutorMsgHandler(const uint8_t *msg, uint32_t len)
{
    CHECK_V_RETURN_LOGW(msg != NULL && len == sizeof(ExecutorParam), "invalid param");
    const ExecutorParam *param = (const ExecutorParam *)msg;

    if (param->cb != NULL) {
        param->cb(param->userData);
    }
    return;
}

int32_t SchedAsyncExecutor(SchedExecutorCallback cb, void *userData)
{
    CHECK_RETURN_LOGW(cb != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");

    ExecutorParam param = {
        .cb = cb,
        .userData = userData,
    };

    return SchedMsgQueueSend((const uint8_t *)&param, sizeof(param), ExecutorMsgHandler, 0);
}