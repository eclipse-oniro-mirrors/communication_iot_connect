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
#include "utils_msg_queue.h"
#include "securec.h"
#include "iotc_log.h"
#include "iotc_errcode.h"
#include "adapter_os.h"
#include "adapter_mem.h"
#include "utils_list.h"
#include "utils_queue.h"
#include "utils_mutex_ex.h"
#include "utils_assert.h"

struct UtilsMsgQueue {
    UtilsQueue *queue;
    UtilsExMutex *lock;
    AdapterSemId *sendSem;
    AdapterSemId *recvSem;
    bool pushOnOff;
};

static int32_t MsgQueueDataInit(UtilsMsgQueue *msgQueue, uint32_t capacity, QueueFreeValue freeValue)
{
    /* 资源外部统一释放 */
    msgQueue->queue = UtilsQueueCreate(capacity, freeValue);
    if (msgQueue->queue == NULL) {
        IOTC_LOGW("create queue err");
        return IOTC_ERROR;
    }
    msgQueue->lock = UtilsCreateExMutex();
    if (msgQueue->lock == NULL) {
        IOTC_LOGW("create mutex err");
        return IOTC_ERROR;
    }
    msgQueue->sendSem = AdapterCreateSem(0);
    if (msgQueue->sendSem == NULL) {
        IOTC_LOGW("create sem err");
        return IOTC_ERROR;
    }
    msgQueue->recvSem = AdapterCreateSem(0);
    if (msgQueue->recvSem == NULL) {
        IOTC_LOGW("create sem err");
        return IOTC_ERROR;
    }
    msgQueue->pushOnOff = true;
    return IOTC_OK;
}

static void MsgQueueDestroy(UtilsMsgQueue **msgQueueAddr)
{
    UtilsMsgQueue *msgQueue = *msgQueueAddr;
    bool lock = false;
    if (msgQueue->lock != NULL) {
        lock = UtilsExMutexLock(msgQueue->lock);
    }
    UtilsQueueDestroy(&msgQueue->queue);
    if (msgQueue->sendSem != NULL) {
        AdapterDestroySem(msgQueue->sendSem);
        msgQueue->sendSem = NULL;
    }
    if (msgQueue->recvSem != NULL) {
        AdapterDestroySem(msgQueue->recvSem);
        msgQueue->recvSem = NULL;
    }
    if (msgQueue->lock != NULL) {
        if (lock) {
            UtilsExMutexUnlock(msgQueue->lock);
        }
        UtilsDestroyExMutex(&msgQueue->lock);
        msgQueue->lock = NULL;
    }
    AdapterFree(msgQueue);
    *msgQueueAddr = NULL;
}

static bool CheckPushOn(UtilsMsgQueue *msgQueue)
{
    bool pushOnOff = false;
    bool lock = UtilsExMutexLock(msgQueue->lock);
    pushOnOff = msgQueue->pushOnOff;
    if (lock) {
        UtilsExMutexUnlock(msgQueue->lock);
    }
    return pushOnOff;
}

static int32_t PushWait(UtilsMsgQueue *msgQueue, uint32_t timeout)
{
    bool full = false;
    if (!UtilsExMutexLock(msgQueue->lock)) {
        IOTC_LOGW("lock");
        return IOTC_ERR_TIMEOUT;
    }
    if (UtilsQueueIsFull(msgQueue->queue)) {
        if (timeout == 0) {
            IOTC_LOGW("full");
            UtilsExMutexUnlock(msgQueue->lock);
            return IOTC_CORE_COMM_UTILS_ERR_MSG_QUEUE_FULL;
        }
        full = true;
    }
    UtilsExMutexUnlock(msgQueue->lock);
    if (full && (timeout > 0)) {
        IOTC_LOGI("wait sem enter");
        if (AdapterWaitSem(msgQueue->recvSem, timeout) != IOTC_OK) {
            IOTC_LOGW("wait sem err");
            return IOTC_ADAPTER_OS_ERR_WAIT_SEM;
        }
        IOTC_LOGI("wait sem exit");
    }
    return IOTC_OK;
}

static int32_t PopWait(UtilsMsgQueue *msgQueue, uint32_t timeout)
{
    bool empty = false;
    if (!UtilsExMutexLock(msgQueue->lock)) {
        IOTC_LOGW("lock");
        return IOTC_ERR_TIMEOUT;
    }
    if (UtilsQueueGetCount(msgQueue->queue) == 0) {
        if (timeout == 0) {
            UtilsExMutexUnlock(msgQueue->lock);
            return IOTC_CORE_COMM_UTILS_ERR_MSG_QUEUE_EMPTY;
        }
        empty = true;
    }
    UtilsExMutexUnlock(msgQueue->lock);
    if (empty && (timeout > 0)) {
        int32_t ret = AdapterWaitSem(msgQueue->sendSem, timeout);
        if (ret != IOTC_OK) {
            return ret;
        }
    }
    return IOTC_OK;
}

UtilsMsgQueue *UtilsMsgQueueCreate(uint32_t capacity, QueueFreeValue freeValue)
{
    UtilsMsgQueue *msgQueue = (UtilsMsgQueue *)AdapterMalloc(sizeof(UtilsMsgQueue));
    if (msgQueue == NULL) {
        IOTC_LOGW("malloc");
        return NULL;
    }
    (void)memset_s(msgQueue, sizeof(UtilsMsgQueue), 0, sizeof(UtilsMsgQueue));

    if (MsgQueueDataInit(msgQueue, capacity, freeValue) != IOTC_OK) {
        IOTC_LOGW("init");
        MsgQueueDestroy(&msgQueue);
        return NULL;
    }

    IOTC_LOGD("create msg queue success");
    return msgQueue;
}

int32_t UtilsMsgQueuePush(UtilsMsgQueue *msgQueue, const void *value, uint32_t valueLen, uint32_t timeout)
{
    if ((msgQueue == NULL) || (value == NULL) || (valueLen == 0)) {
        IOTC_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    if (!CheckPushOn(msgQueue)) {
        return IOTC_CORE_COMM_UTILS_ERR_MSG_QUEUE_NOT_ALLOW_PUSH;
    }
    int32_t ret = PushWait(msgQueue, timeout);
    if (ret != IOTC_OK) {
        return ret;
    }
    if (!UtilsExMutexLock(msgQueue->lock)) {
        IOTC_LOGW("lock");
        return IOTC_ERR_TIMEOUT;
    }
    if (UtilsQueueIsFull(msgQueue->queue)) {
        IOTC_LOGW("full");
        UtilsExMutexUnlock(msgQueue->lock);
        return IOTC_CORE_COMM_UTILS_ERR_MSG_QUEUE_FULL;
    }
    if (UtilsQueuePush(msgQueue->queue, value, valueLen) != IOTC_OK) {
        IOTC_LOGW("push");
        UtilsExMutexUnlock(msgQueue->lock);
        return IOTC_CORE_COMM_UTILS_ERR_MSG_QUEUE_PUSH;
    }
    if (AdapterGetSemCount(msgQueue->sendSem) == 0) {
        AdapterPostSem(msgQueue->sendSem);
    }
    UtilsExMutexUnlock(msgQueue->lock);

    IOTC_LOGD("msg queue push success valueLen=%u,timeout=%u", valueLen, timeout);
    return IOTC_OK;
}

int32_t UtilsMsgQueuePushMem(UtilsMsgQueue *msgQueue, void **value, uint32_t valueLen, uint32_t timeout)
{
    if ((msgQueue == NULL) || (value == NULL) || (valueLen == 0)) {
        IOTC_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    if (!CheckPushOn(msgQueue)) {
        return IOTC_CORE_COMM_UTILS_ERR_MSG_QUEUE_NOT_ALLOW_PUSH;
    }
    int32_t ret = PushWait(msgQueue, timeout);
    if (ret != IOTC_OK) {
        return ret;
    }
    if (!UtilsExMutexLock(msgQueue->lock)) {
        IOTC_LOGW("lock");
        return IOTC_ERR_TIMEOUT;
    }
    if (UtilsQueueIsFull(msgQueue->queue)) {
        IOTC_LOGW("full");
        UtilsExMutexUnlock(msgQueue->lock);
        return IOTC_CORE_COMM_UTILS_ERR_MSG_QUEUE_FULL;
    }
    if (UtilsQueuePushMem(msgQueue->queue, value, valueLen) != IOTC_OK) {
        IOTC_LOGW("push");
        UtilsExMutexUnlock(msgQueue->lock);
        return IOTC_CORE_COMM_UTILS_ERR_MSG_QUEUE_PUSH;
    }
    if (AdapterGetSemCount(msgQueue->sendSem) == 0) {
        AdapterPostSem(msgQueue->sendSem);
    }
    UtilsExMutexUnlock(msgQueue->lock);

    IOTC_LOGD("msg queue push success valueLen=%u,timeout=%u", valueLen, timeout);
    return IOTC_OK;
}

int32_t UtilsMsgQueuePop(UtilsMsgQueue *msgQueue, void *value, uint32_t valueSize, uint32_t *valueLen,
    uint32_t timeout)
{
    if ((msgQueue == NULL) || (value == NULL) || (valueSize == 0) || (valueLen == NULL)) {
        IOTC_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    int32_t ret = PopWait(msgQueue, timeout);
    if (ret != IOTC_OK) {
        return ret;
    }
    if (!UtilsExMutexLock(msgQueue->lock)) {
        IOTC_LOGW("lock");
        return IOTC_ERR_TIMEOUT;
    }
    if (UtilsQueueGetCount(msgQueue->queue) == 0) {
        UtilsExMutexUnlock(msgQueue->lock);
        return IOTC_CORE_COMM_UTILS_ERR_MSG_QUEUE_EMPTY;
    }
    if (UtilsQueuePop(msgQueue->queue, value, valueSize, valueLen) != IOTC_OK) {
        IOTC_LOGW("pop");
        UtilsExMutexUnlock(msgQueue->lock);
        return IOTC_CORE_COMM_UTILS_ERR_MSG_QUEUE_POP;
    }
    if (AdapterGetSemCount(msgQueue->recvSem) == 0) {
        AdapterPostSem(msgQueue->recvSem);
    }
    UtilsExMutexUnlock(msgQueue->lock);

    IOTC_LOGD("msg queue pop success valueSize=%u,timeout=%u,valueLen=%u", valueSize, timeout, *valueLen);
    return IOTC_OK;
}

int32_t UtilsMsgQueuePopMem(UtilsMsgQueue *msgQueue, void **value, uint32_t *valueLen, uint32_t timeout)
{
    if ((msgQueue == NULL) || (value == NULL) || (valueLen == NULL)) {
        IOTC_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    int32_t ret = PopWait(msgQueue, timeout);
    if (ret != IOTC_OK) {
        return ret;
    }
    if (!UtilsExMutexLock(msgQueue->lock)) {
        IOTC_LOGW("lock");
        return IOTC_ERR_TIMEOUT;
    }
    if (UtilsQueueGetCount(msgQueue->queue) == 0) {
        UtilsExMutexUnlock(msgQueue->lock);
        return IOTC_CORE_COMM_UTILS_ERR_MSG_QUEUE_EMPTY;
    }
    if (UtilsQueuePopMem(msgQueue->queue, value, valueLen) != IOTC_OK) {
        IOTC_LOGW("pop");
        UtilsExMutexUnlock(msgQueue->lock);
        return IOTC_CORE_COMM_UTILS_ERR_MSG_QUEUE_POP;
    }
    if (AdapterGetSemCount(msgQueue->recvSem) == 0) {
        AdapterPostSem(msgQueue->recvSem);
    }
    UtilsExMutexUnlock(msgQueue->lock);
    IOTC_LOGD("msg queue pop success timeout=%u,valueLen=%u", timeout, *valueLen);
    return IOTC_OK;
}


uint32_t UtilsMsgQueueGetCount(UtilsMsgQueue *msgQueue)
{
    if (msgQueue == NULL) {
        IOTC_LOGW("invalid param");
        return 0;
    }
    uint32_t count = 0;
    if (!UtilsExMutexLock(msgQueue->lock)) {
        IOTC_LOGW("lock");
        return 0;
    }
    count = UtilsQueueGetCount(msgQueue->queue);
    UtilsExMutexUnlock(msgQueue->lock);
    return count;
}

void UtilsMsgQueueDestroy(UtilsMsgQueue **msgQueueAddr)
{
    if ((msgQueueAddr == NULL) || (*msgQueueAddr == NULL)) {
        IOTC_LOGW("invalid param");
        return;
    }
    MsgQueueDestroy(msgQueueAddr);
    IOTC_LOGD("msg queue destroy success");
}

int32_t UtilsMsgQueueSuspend(UtilsMsgQueue *msgQueue)
{
    if (msgQueue == NULL) {
        IOTC_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    if (!UtilsExMutexLock(msgQueue->lock)) {
        IOTC_LOGW("lock");
        return IOTC_ERR_TIMEOUT;
    }
    msgQueue->pushOnOff = false;
    UtilsExMutexUnlock(msgQueue->lock);
    return IOTC_OK;
}

void UtilsMsgQueueResume(UtilsMsgQueue *msgQueue)
{
    if (msgQueue == NULL) {
        IOTC_LOGW("invalid param");
        return;
    }
    if (!UtilsExMutexLock(msgQueue->lock)) {
        IOTC_LOGW("lock");
        return;
    }
    msgQueue->pushOnOff = true;
    UtilsExMutexUnlock(msgQueue->lock);
}
