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
#include "event_source_timer.h"
#include "utils_list.h"
#include "iotc_log.h"
#include "adapter_mem.h"
#include "adapter_os.h"
#include "utils_bit_map.h"
#include "utils_common.h"
#include "securec.h"
#include "utils_assert.h"
#include "iotc_errcode.h"

#define EVENT_SOURCE_TIMER_NAME "TIMER"
static const uint32_t EVENT_TIMER_MAX_NUM = UINT8_MAX;

typedef enum {
    TIMER_SOURCE_BIT_MAP_DELETE = 0,
    TIMER_SOURCE_BIT_MAP_UPDATE,
    TIMER_SOURCE_BIT_MAP_FINISH,
} TimerSourceBit;

typedef struct {
    ListEntry node;
    uint8_t bitMap;
    int32_t id;
    uint32_t interval;
    uint32_t remainTime;
    uint32_t lastTime;
    EventSourceTimerType type;
    EventSourceTimerCallback cb;
    void *userData;
    EventSourceTimerStatus status;
} TimerNode;

typedef struct {
    ListEntry node;
    int32_t id;
    EventSourceTimerStatus status;
} TimerStatusNode;

typedef struct {
    EventSource base;
    uint32_t cnt;
    int32_t idBase;
    /* runtime list */
    ListEntry timerList;
    /* update list for add new timer */
    ListEntry addList;
} EventSourceTimer;

#define LOCK_TIMER_RETURN_IF_FAIL(timerSource) do { \
    if (!UtilsMutexLocalLock(&((timerSource)->mutex))) { \
        IOTC_LOGW("lock timer source failed\n"); \
        return IOTC_ERR_TIMEOUT; \
    } \
} while (0)

#define LOCK_TIMER_V_RETURN_IF_FAIL(timerSource) do { \
    if (!UtilsMutexLocalLock(&((timerSource)->mutex))) { \
        IOTC_LOGW("lock timer source failed\n"); \
        return; \
    } \
} while (0)

#define UNLOCK_TIMER(timerSource) UtilsMutexLocalUnlock(&((timerSource)->mutex))

static void TimerSourceUpdate(EventSourceTimer *timerSource)
{
    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &timerSource->addList) {
        LIST_REMOVE(item);
        LIST_INSERT_BEFORE(item, &timerSource->timerList);
        ++timerSource->cnt;
    }
    return;
}

static bool TimerSourcePrepare(EventSource *self, uint32_t *timeout)
{
    CHECK_RETURN(self != NULL && timeout != NULL, false);
    EventSourceTimer *timerSource = (EventSourceTimer *)self;
    if (!LIST_EMPTY(&timerSource->addList)) {
        TimerSourceUpdate(timerSource);
    }

    *timeout = UINT32_MAX;
    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &timerSource->timerList) {
        TimerNode *node = CONTAINER_OF(item, TimerNode, node);
        if (UTILS_IS_BIT_SET(node->bitMap, TIMER_SOURCE_BIT_MAP_DELETE)) {
            LIST_REMOVE(item);
            AdapterFree(node);
            continue;
        }
        *timeout = UTILS_MIN(*timeout, node->remainTime);
    }
    return true;
}

static bool TimerSourceCheck(EventSource *self)
{
    CHECK_RETURN(self != NULL, false);
    EventSourceTimer *timer = (EventSourceTimer *)self;
    uint32_t now = AdapterGetSysTimeMs();

    bool ready = false;
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &timer->timerList) {
        TimerNode *node = CONTAINER_OF(item, TimerNode, node);
        uint32_t delta = UtilsDeltaTime(now, node->lastTime);
        if (delta > node->remainTime) {
            node->remainTime = 0;
            ready = true;
        } else {
            node->remainTime -= delta;
        }
        node->lastTime = now;
    }
    return ready;
}

static bool TimerSourceDispatch(EventSource *self)
{
    CHECK_RETURN(self != NULL, false);
    EventSourceTimer *timerSource = (EventSourceTimer *)self;

    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &timerSource->timerList) {
        TimerNode *node = CONTAINER_OF(item, TimerNode, node);
        if (node->remainTime != 0 || node->bitMap != 0) {
            continue;
        }
        if (node->cb == NULL) {
            UTILS_BIT_SET(node->bitMap, TIMER_SOURCE_BIT_MAP_DELETE);
            continue;
        }

        node->remainTime = node->interval;
        node->cb(node->id, node->userData);
        if (node->type == EVENT_SOURCE_TIMER_TYPE_ONCE) {
            UTILS_BIT_SET(node->bitMap, TIMER_SOURCE_BIT_MAP_FINISH);
            node->status = EVENT_SOURCE_TIMER_STATUS_FINISH;
        }
    }
    return true;
}

static void TimerSourceFinalize(EventSource *self)
{
    CHECK_V_RETURN(self != NULL);
    EventSourceTimer *timerSource = (EventSourceTimer *)self;

    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &timerSource->addList) {
        TimerNode *node = CONTAINER_OF(item, TimerNode, node);
        LIST_REMOVE(item);
        AdapterFree(node);
    }

    LIST_FOR_EACH_ITEM_SAFE(item, next, &timerSource->timerList) {
        TimerNode *node = CONTAINER_OF(item, TimerNode, node);
        LIST_REMOVE(item);
        AdapterFree(node);
    }
}

EventSource *EventSourceTimerNew(const char *name)
{
    static const EventSourceOps TIMER_SOURCE_OPS = {
        .prepare = TimerSourcePrepare,
        .poll = NULL,
        .check = TimerSourceCheck,
        .dispatch = TimerSourceDispatch,
        .finalize = TimerSourceFinalize,
    };

    EventSource *source = EventSourceNew(&TIMER_SOURCE_OPS, sizeof(EventSourceTimer),
        name == NULL ? EVENT_SOURCE_TIMER_NAME : name, NULL);
    if (source == NULL) {
        IOTC_LOGW("timer source new error");
        return NULL;
    }

    EventSourceTimer *timerSource = (EventSourceTimer *)source;
    LIST_INIT(&timerSource->timerList);
    LIST_INIT(&timerSource->addList);
    return source;
}

static void TimerNodeUpdate(TimerNode *node, EventSourceTimerType type, uint32_t ms)
{
    UTILS_BIT_CLR(node->bitMap);
    node->type = type;
    node->interval = ms;
    node->remainTime = ms;
    node->lastTime = AdapterGetSysTimeMs();
    node->status = EVENT_SOURCE_TIMER_STATUS_RUNNING;
}

static TimerNode *TimerNodeNew(int32_t id, EventSourceTimerType type, EventSourceTimerCallback cb,
    uint32_t ms, void *userData)
{
    TimerNode *newNode = (TimerNode *)AdapterMalloc(sizeof(TimerNode));
    if (newNode == NULL) {
        return NULL;
    }

    (void)memset_s(newNode, sizeof(TimerNode), 0, sizeof(TimerNode));
    newNode->id = id;
    newNode->cb = cb;
    newNode->userData = userData;
    TimerNodeUpdate(newNode, type, ms);
    return newNode;
}

int32_t EventSourceTimerAdd(EventSource *source, EventSourceTimerType type,
    EventSourceTimerCallback cb, uint32_t ms, void *userData)
{
    CHECK_RETURN(source != NULL, IOTC_ERR_PARAM_INVALID);
    EventSourceTimer *timerSource = (EventSourceTimer *)source;
    if (timerSource->cnt >= EVENT_TIMER_MAX_NUM) {
        return IOTC_CORE_COMM_FWK_ERR_SOURCE_TIMER_MAX;
    }

    int32_t id = ++timerSource->idBase;
    TimerNode *newNode = TimerNodeNew(id, type, cb, ms, userData);
    if (newNode == NULL) {
        IOTC_LOGW("new timer node error");
        return IOTC_CORE_COMM_FWK_ERR_SOURCE_TIMER_CREATE;
    }

    LIST_INSERT_BEFORE(&newNode->node, &timerSource->addList);
    return id;
}

void EventSourceTimerRemove(EventSource *source, int32_t id)
{
    CHECK_V_RETURN(source != NULL && id > 0);
    EventSourceTimer *timerSource = (EventSourceTimer *)source;

    ListEntry *item = NULL;
    ListEntry *next = NULL;
    /* Directly delete the newly added timer and node */
    LIST_FOR_EACH_ITEM_SAFE(item, next, &timerSource->addList) {
        TimerNode *node = CONTAINER_OF(item, TimerNode, node);
        if (node->id != id) {
            continue;
        }
        LIST_REMOVE(item);
        AdapterFree(node);
        return;
    }

    /* Asynchronously delete the running timer */
    LIST_FOR_EACH_ITEM_SAFE(item, next, &timerSource->timerList) {
        TimerNode *node = CONTAINER_OF(item, TimerNode, node);
        if (node->id != id) {
            continue;
        }
        UTILS_BIT_SET(node->bitMap, TIMER_SOURCE_BIT_MAP_DELETE);
        node->status = EVENT_SOURCE_TIMER_STATUS_INVALID;
        return;
    }

    IOTC_LOGW("invalid timer to del %d", id);
}

void EventSourceTimerUpdate(EventSource *source, int32_t id, EventSourceTimerType type, uint32_t ms)
{
    CHECK_V_RETURN(source != NULL && id > 0);
    EventSourceTimer *timerSource = (EventSourceTimer *)source;

    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &timerSource->addList) {
        TimerNode *node = CONTAINER_OF(item, TimerNode, node);
        if (node->id != id) {
            continue;
        }
        TimerNodeUpdate(node, type, ms);
        return;
    }

    LIST_FOR_EACH_ITEM(item, &timerSource->timerList) {
        TimerNode *node = CONTAINER_OF(item, TimerNode, node);
        if (node->id != id) {
            continue;
        }
        TimerNodeUpdate(node, type, ms);
        return;
    }

    IOTC_LOGW("invalid timer to update %d", id);
}

EventSourceTimerStatus EventSourceTimerGetStatus(EventSource *source, int32_t id)
{
    CHECK_RETURN_LOGW(source != NULL && id > 0, EVENT_SOURCE_TIMER_STATUS_INVALID, "param invalid");
    EventSourceTimer *timerSource = (EventSourceTimer *)source;

    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &timerSource->addList) {
        TimerNode *node = CONTAINER_OF(item, TimerNode, node);
        if (node->id != id) {
            continue;
        }
        return node->status;
    }

    LIST_FOR_EACH_ITEM(item, &timerSource->timerList) {
        TimerNode *node = CONTAINER_OF(item, TimerNode, node);
        if (node->id != id) {
            continue;
        }
        return node->status;
    }

    IOTC_LOGW("invalid timer to get status %d", id);
    return EVENT_SOURCE_TIMER_STATUS_INVALID;
}