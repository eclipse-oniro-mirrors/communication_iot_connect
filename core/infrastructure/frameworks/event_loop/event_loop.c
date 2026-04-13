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
#include "event_loop.h"
#include "event_source_priv.h"
#include "securec.h"
#include "utils_list.h"
#include "comm_def.h"
#include "iotc_log.h"
#include "utils_mutex_ex.h"
#include "utils_bit_map.h"
#include "iotc_mem.h"
#include "iotc_os.h"
#include "utils_common.h"
#include "utils_assert.h"
#include "iotc_errcode.h"

typedef enum {
    EVENT_LOOP_FLAG_QUIT = 0,
    EVENT_LOOP_FLAG_SUSPEND,
} EventLoopFlag;

typedef enum {
    EVENT_NODE_FLAG_READY = 0,
    EVENT_NODE_FLAG_DELETE,
    EVENT_NODE_FLAG_POLL,
} EventNodeFlag;

typedef struct {
    ListEntry node;
    uint8_t bitMap;
    EventSource *source;
} EventNode;

struct EventLoop {
    const char *name;
    uint8_t bitMap;
    UtilsExMutex *lock;
    IotcSemId *sem;
    EventLoopIdleCallback idle;
    IotcTaskId *taskId;
    uint32_t pollMinTime;
    uint32_t pollMaxTime;
    uint32_t pollTimeout;
    uint32_t pollSourceCnt;
    uint32_t suspendTime;
    /* runtime list */
    ListEntry sourceList;
    /* async update list */
    ListEntry addList;
};

#define LOCK_LOOP_RETURN_IF_FAIL(loop) do { \
    if (!UtilsExMutexLockAnyway((loop)->lock)) { \
        IOTC_LOGW("lock loop failed\n"); \
        return IOTC_ERR_TIMEOUT; \
    } \
} while (0)

#define LOCK_LOOP_V_RETURN_IF_FAIL(loop) do { \
    if (!UtilsExMutexLockAnyway((loop)->lock)) { \
        IOTC_LOGW("lock loop failed\n"); \
        return; \
    } \
} while (0)

#define UNLOCK_LOOP(loop) UtilsExMutexUnlock((loop)->lock)

static void EventLoopStaticInit(EventLoop *loop)
{
    LIST_INIT(&loop->sourceList);
    LIST_INIT(&loop->addList);
    loop->pollMinTime = EVENT_LOOP_DEFAULT_POLL_MIN_TIME_MS;
    loop->pollMaxTime = EVENT_LOOP_DEFAULT_POLL_MAX_TIME_MS;
}

EventLoop *EventLoopNew(const char *name)
{
    EventLoop *loop = (EventLoop *)IotcMalloc(sizeof(EventLoop));
    if (loop == NULL) {
        IOTC_LOGW("malloc error");
        return NULL;
    }
    (void)memset_s(loop, sizeof(EventLoop), 0, sizeof(EventLoop));

    loop->lock = UtilsCreateExMutex();
    if (loop->lock == NULL) {
        IOTC_LOGW("create ex mutex error");
        IotcFree(loop);
        return NULL;
    }

    loop->name = name;
    EventLoopStaticInit(loop);
    return loop;
}

static void LoopSourceNodeInsert(EventLoop *loop, EventNode *addNode)
{
    /* Inserted at the front of list for polling */
    if (UTILS_IS_BIT_SET(addNode->bitMap, EVENT_NODE_FLAG_POLL)) {
        LIST_INSERT(&addNode->node, &loop->sourceList);
        loop->pollSourceCnt++;
    } else {
        LIST_INSERT_BEFORE(&addNode->node, &loop->sourceList);
    }
}

static void LoopUpdateSource(EventLoop *loop)
{
    if (LIST_EMPTY(&loop->addList)) {
        return;
    }

    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &loop->addList) {
        EventNode *eventNode = CONTAINER_OF(item, EventNode, node);
        LIST_REMOVE(item);
        LoopSourceNodeInsert(loop, eventNode);
    }
}

void EventLoopFree(EventLoop *loop)
{
    CHECK_V_RETURN(loop != NULL);

    EventLoopReset(loop);
    UtilsDestroyExMutex(&loop->lock);
    if (loop->sem != NULL) {
        IotcSemDestroy(loop->sem);
        loop->sem = NULL;
    }

    IotcFree(loop);
}

void EventLoopReset(EventLoop *loop)
{
    CHECK_V_RETURN(loop != NULL);

    ListEntry *item = NULL;
    ListEntry *next = NULL;

    LIST_FOR_EACH_ITEM_SAFE(item, next, &loop->addList) {
        EventNode *eventNode = CONTAINER_OF(item, EventNode, node);
        LIST_REMOVE(item);
        EventSourceFree(eventNode->source);
        IotcFree(eventNode);
    }
    LIST_FOR_EACH_ITEM_SAFE(item, next, &loop->sourceList) {
        EventNode *eventNode = CONTAINER_OF(item, EventNode, node);
        LIST_REMOVE(item);
        EventSourceFree(eventNode->source);
        IotcFree(eventNode);
    }

    EventLoopStaticInit(loop);
}

void EventLoopSuspend(EventLoop *loop, uint32_t timeoutMs)
{
    if (loop == NULL || timeoutMs == 0) {
        IOTC_LOGW("param invalid");
        return;
    }

    LOCK_LOOP_V_RETURN_IF_FAIL(loop);
    UTILS_BIT_SET(loop->bitMap, EVENT_LOOP_FLAG_SUSPEND);
    loop->suspendTime = timeoutMs;
    UNLOCK_LOOP(loop);
}

int32_t EventLoopResume(EventLoop *loop)
{
    CHECK_RETURN(loop != NULL, IOTC_ERR_PARAM_INVALID);

    int32_t ret = IOTC_CORE_COMM_FWK_ERR_LOOP_NOT_SUSPEND;
    LOCK_LOOP_RETURN_IF_FAIL(loop);

    if (loop->sem != NULL) {
        ret = IotcSemPost(loop->sem);
        if (ret != 0) {
            IOTC_LOGW("sem post error %d", ret);
        }
    }

    UNLOCK_LOOP(loop);
    return ret;
}

static void LoopPrepareEvent(EventLoop *loop)
{
    loop->pollTimeout = loop->pollMaxTime;
    uint32_t timeout = 0;
    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &loop->sourceList) {
        EventNode *eventNode = CONTAINER_OF(item, EventNode, node);
        /* 移除空节点 */
        if (UTILS_IS_BIT_SET(eventNode->bitMap, EVENT_NODE_FLAG_DELETE)) {
            LIST_REMOVE(item);
            if (UTILS_IS_BIT_SET(eventNode->bitMap, EVENT_NODE_FLAG_POLL)) {
                loop->pollSourceCnt = loop->pollSourceCnt > 0 ? loop->pollSourceCnt - 1 : 0;
            }
            IotcFree(eventNode);
            continue;
        }

        if (EventSourcePrepare(eventNode->source, &timeout)) {
            loop->pollTimeout = UTILS_MIN(loop->pollTimeout, timeout);
        }
    }
    loop->pollTimeout = UTILS_MAX(loop->pollMinTime, loop->pollTimeout);
}

static void LoopCheckSuspend(EventLoop *loop)
{
    /* lock-free read bitmap, confirm suspend by check suspendTime with lock */
    if (!UTILS_IS_BIT_SET(loop->bitMap, EVENT_LOOP_FLAG_SUSPEND)) {
        return;
    }

    LOCK_LOOP_V_RETURN_IF_FAIL(loop);
    if (loop->suspendTime == 0) {
        UNLOCK_LOOP(loop);
        return;
    }
    UTILS_BIT_RESET(loop->bitMap, EVENT_LOOP_FLAG_SUSPEND);
    loop->sem = IotcSemCreate(0);
    if (loop->sem == NULL) {
        IOTC_LOGE("create sem error");
        UNLOCK_LOOP(loop);
        return;
    }

    UNLOCK_LOOP(loop);
    IOTC_LOGN("loop suspend for max %u ms", loop->suspendTime);
    int32_t ret = IotcSemWait(loop->sem, loop->suspendTime);
    if (ret == IOTC_OK) {
        IOTC_LOGN("loop resume by sem");
    } else if (ret == IOTC_ERR_TIMEOUT) {
        IOTC_LOGN("loop resume timeout");
    } else {
        IOTC_LOGN("wait sem error %d", ret);
    }

    LOCK_LOOP_V_RETURN_IF_FAIL(loop);
    IotcSemDestroy(loop->sem);
    loop->sem = NULL;
    UNLOCK_LOOP(loop);
}

static void LoopPollEvent(EventLoop *loop)
{
    if (loop->pollSourceCnt == 0) {
        IotcSleepMs(loop->pollTimeout);
        return;
    }

    uint32_t pollTmo = loop->pollTimeout;
    /* If there is more than 1 poll source, poll min time for each source until the time is met */
    uint32_t sglTmo =  loop->pollSourceCnt == 1 ? pollTmo : loop->pollMinTime;

    bool isEvent = false;
    do {
        uint32_t pollCnt = loop->pollSourceCnt;
        ListEntry *item = NULL;
        LIST_FOR_EACH_ITEM(item, &loop->sourceList) {
            EventNode *eventNode = CONTAINER_OF(item, EventNode, node);
            if (!UTILS_IS_BIT_SET(eventNode->bitMap, EVENT_NODE_FLAG_POLL) ||
                UTILS_IS_BIT_SET(eventNode->bitMap, EVENT_NODE_FLAG_DELETE)) {
                continue;
            }

            if (EventSourcePoll(eventNode->source, sglTmo)) {
                isEvent = true;
            }

            pollCnt--;
            pollTmo = pollTmo > sglTmo ? (pollTmo - sglTmo) : 0;
            if (pollCnt == 0) {
                break;
            }
        }
    } while (pollTmo > 0 && sglTmo > 0 && !isEvent);
    return;
}

static void LoopCheckAndDispatchEvent(EventLoop *loop)
{
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &loop->sourceList) {
        EventNode *eventNode = CONTAINER_OF(item, EventNode, node);
        if (UTILS_IS_BIT_SET(eventNode->bitMap, EVENT_NODE_FLAG_DELETE) || eventNode->source == NULL) {
            continue;
        }
        if (EventSourceCheck(eventNode->source)) {
            if (!EventSourceDispatch(eventNode->source)) {
                UTILS_BIT_SET(eventNode->bitMap, EVENT_NODE_FLAG_DELETE);
                IOTC_LOGW("[EL] dispatch error %s/%s", NON_NULL_STR(loop->name), NON_NULL_STR(eventNode->source->name));
            }
        }
    }
}

static void LoopProcessIdle(EventLoop *loop)
{
    if (loop->idle == NULL) {
        return;
    }
    LOCK_LOOP_V_RETURN_IF_FAIL(loop);
    EventLoopIdleCallback idle = loop->idle;
    UNLOCK_LOOP(loop);
    if (idle != NULL) {
        idle();
    }
}

static void LoopCheckQuit(EventLoop *loop, bool *quit)
{
    /* lock-free read bitmap, confirm quit by check bitmap with lock */
    if (!UTILS_IS_BIT_SET(loop->bitMap, EVENT_LOOP_FLAG_QUIT)) {
        return;
    }
    LOCK_LOOP_V_RETURN_IF_FAIL(loop);
    *quit = UTILS_IS_BIT_SET(loop->bitMap, EVENT_LOOP_FLAG_QUIT);
    UNLOCK_LOOP(loop);
}

void EventLoopRun(EventLoop *loop)
{
    CHECK_V_RETURN(loop != NULL);

    LOCK_LOOP_V_RETURN_IF_FAIL(loop);
    loop->taskId = IotcTaskGetCurrentTaskId();
    UNLOCK_LOOP(loop);

    bool isQuit = false;
    do {
        LoopCheckSuspend(loop);

        LoopUpdateSource(loop);

        LoopPrepareEvent(loop);

        LoopPollEvent(loop);

        LoopCheckAndDispatchEvent(loop);

        LoopProcessIdle(loop);

        LoopCheckQuit(loop, &isQuit);
    } while (!isQuit);
    IOTC_LOGN("event loop quit");

    LOCK_LOOP_V_RETURN_IF_FAIL(loop);
    loop->taskId = NULL;
    UNLOCK_LOOP(loop);
}

void EventLoopQuit(EventLoop *loop)
{
    CHECK_V_RETURN(loop != NULL);
    LOCK_LOOP_V_RETURN_IF_FAIL(loop);
    UTILS_BIT_SET(loop->bitMap, EVENT_LOOP_FLAG_QUIT);
    UNLOCK_LOOP(loop);
    IOTC_LOGN("event loop quit invoke");
}

void EventLoopSetPollTime(EventLoop *loop, uint32_t minTime, uint32_t maxTime)
{
    CHECK_V_RETURN(loop != NULL && minTime < maxTime);

    loop->pollMinTime = UTILS_MAX(minTime, EVENT_LOOP_DEFAULT_POLL_MIN_TIME_MS);
    loop->pollMaxTime = UTILS_MIN(maxTime, EVENT_LOOP_DEFAULT_POLL_MAX_TIME_MS);
    IOTC_LOGN("set poll time [%u/%u]", loop->pollMinTime, loop->pollMaxTime);
}

void EventLoopGetPollTime(EventLoop *loop, uint32_t *minTime, uint32_t *maxTime)
{
    CHECK_V_RETURN(loop != NULL);

    if (minTime != NULL) {
        *minTime = loop->pollMinTime;
    }
    if (maxTime != NULL) {
        *maxTime = loop->pollMaxTime;
    }
}

static EventNode *EventNodeNew(EventSource *source)
{
    EventNode *newNode = (EventNode *)IotcMalloc(sizeof(EventNode));
    if (newNode == NULL) {
        IOTC_LOGW("malloc error");
        return NULL;
    }
    (void)memset_s(newNode, sizeof(EventNode), 0, sizeof(EventNode));
    newNode->source = source;
    if (EventSourceIsPoll(source)) {
        UTILS_BIT_SET(newNode->bitMap, EVENT_NODE_FLAG_POLL);
    }
    return newNode;
}

int32_t EventLoopAddSource(EventLoop *loop, EventSource *source)
{
    CHECK_RETURN(loop != NULL && source != NULL && source->ops != NULL, IOTC_ERR_PARAM_INVALID);

    /* The same source can only be added once */
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &loop->sourceList) {
        EventNode *eventNode = CONTAINER_OF(item, EventNode, node);
        if (eventNode->source == source) {
            return IOTC_OK;
        }
    }
    LIST_FOR_EACH_ITEM(item, &loop->addList) {
        EventNode *eventNode = CONTAINER_OF(item, EventNode, node);
        if (eventNode->source == source) {
            return IOTC_OK;
        }
    }

    EventNode *newNode = EventNodeNew(source);
    if (newNode == NULL) {
        IOTC_LOGW("node new error");
        return IOTC_CORE_COMM_FWK_ERR_LOOP_NODE_NEW;
    }

    LIST_INSERT_BEFORE(&newNode->node, &loop->addList);
    return IOTC_OK;
}

void EventLoopDelSource(EventLoop *loop, EventSource *source)
{
    CHECK_V_RETURN(source != NULL && loop != NULL);

    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &loop->sourceList) {
        EventNode *eventNode = CONTAINER_OF(item, EventNode, node);
        if (eventNode->source != source) {
            continue;
        }
        /* Mark delete flag if source in update list */
        UTILS_BIT_SET(eventNode->bitMap, EVENT_NODE_FLAG_DELETE);
        eventNode->source = NULL;
        EventSourceFree(source);
        return;
    }

    ListEntry *temp = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, temp, &loop->addList) {
        EventNode *eventNode = CONTAINER_OF(item, EventNode, node);
        if (eventNode->source != source) {
            continue;
        }
        LIST_REMOVE(item);
        EventSourceFree(eventNode->source);
        IotcFree(eventNode);
        return;
    }

    IOTC_LOGE("invalid source to free");
}

void EventLoopAddIdle(EventLoop *loop, EventLoopIdleCallback cb)
{
    CHECK_V_RETURN(loop != NULL && cb != NULL);

    LOCK_LOOP_V_RETURN_IF_FAIL(loop);
    loop->idle = cb;
    UNLOCK_LOOP(loop);
}

IotcTaskId *EventLoopGetTaskId(EventLoop *loop)
{
    CHECK_RETURN_LOGE(loop != NULL, NULL, "param invalid");
    if (!UtilsExMutexLockAnyway(loop->lock)) {
        IOTC_LOGW("lock loop failed");
        return NULL;
    }
    IotcTaskId *taskId = loop->taskId;
    UNLOCK_LOOP(loop);
    return taskId;
}