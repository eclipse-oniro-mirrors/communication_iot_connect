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
#include "event_source_fd.h"
#include "utils_list.h"
#include "iotc_log.h"
#include "adapter_mem.h"
#include "adapter_os.h"
#include "utils_bit_map.h"
#include "utils_common.h"
#include "securec.h"
#include "utils_assert.h"

#define EVENT_SOURCE_NAME "FD"
static const uint8_t WATCH_NODE_BIT_MAP_SUSPEND = 0;
static const uint8_t WATCH_NODE_BIT_MAP_READ_EVENT = 1;
static const uint8_t WATCH_NODE_BIT_MAP_WRITE_EVENT = 2;
static const uint8_t WATCH_NODE_BIT_MAP_EXCEPT_EVENT = 3;

typedef struct {
    ListEntry node;
    uint8_t bitMap;
    /* 实际优先级，每次loop只处理一个fd，其他fd的优先级+1 */
    uint32_t curPrio;
    FdWatchParam watch;
} WatchNode;

typedef struct {
    EventSource base;
    uint32_t eventCnt;
    EventSourceFdPoll poll;
    uint32_t cnt;
    ListEntry watchList;
} EventSourceFd;

static bool FdSourcePrepare(EventSource *self, uint32_t *timeout)
{
    CHECK_RETURN(self != NULL && timeout != NULL, false);
    EventSourceFd *fdSource = (EventSourceFd *)self;
    /* 实际受eventloop的pooltimemax与pooltimemin约束 */
    if (fdSource->eventCnt > 0) {
        *timeout = 0;
    } else {
        *timeout = UINT32_MAX;
    }
    return true;
}

static void FdSourceEventSet(EventSourceFd *fdSource, uint8_t *bitMap, uint8_t event)
{
    if (UTILS_IS_BIT_SET(*bitMap, event)) {
        return;
    }
    UTILS_BIT_SET(*bitMap, event);
    fdSource->eventCnt++;
}

static void FdSourceEventUnset(EventSourceFd *fdSource, uint8_t *bitMap, uint8_t event)
{
    if (!UTILS_IS_BIT_SET(*bitMap, event)) {
        return;
    }
    UTILS_BIT_RESET(*bitMap, event);
    fdSource->eventCnt = fdSource->eventCnt > 0 ? fdSource->eventCnt - 1 : 0;
}

static void FdSourcePrepareFdSet(EventSourceFd *fdSource, EventSourceFdSet *readSet, EventSourceFdSet *writeSet,
    EventSourceFdSet *exceptSet)
{
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &fdSource->watchList) {
        WatchNode *node = CONTAINER_OF(item, WatchNode, node);
        if (UTILS_IS_BIT_SET(node->bitMap, WATCH_NODE_BIT_MAP_SUSPEND)) {
            continue;
        }
        /* 已有的事件处理完毕才可以继续poll */
        if (node->watch.recvEvent != NULL && !UTILS_IS_BIT_SET(node->bitMap, WATCH_NODE_BIT_MAP_READ_EVENT)) {
            readSet->fd[readSet->num++] = node->watch.fd;
        }
        if (node->watch.sendEvent != NULL && !UTILS_IS_BIT_SET(node->bitMap, WATCH_NODE_BIT_MAP_WRITE_EVENT)) {
            writeSet->fd[writeSet->num++] = node->watch.fd;
        }
        if (node->watch.exceEvent != NULL && !UTILS_IS_BIT_SET(node->bitMap, WATCH_NODE_BIT_MAP_EXCEPT_EVENT)) {
            exceptSet->fd[exceptSet->num++] = node->watch.fd;
        }
    }
}

static bool IsFdEvent(int32_t fd, EventSourceFdSet *set)
{
    for (uint32_t i = 0 ; i < set->num; ++i) {
        if (set->fd[i] == fd) {
            return true;
        }
    }
    return false;
}

static void FdSourceGetFdSetEvent(EventSourceFd *fdSource, EventSourceFdSet *readSet, EventSourceFdSet *writeSet,
    EventSourceFdSet *exceptSet)
{
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &fdSource->watchList) {
        WatchNode *node = CONTAINER_OF(item, WatchNode, node);
        if (UTILS_IS_BIT_SET(node->bitMap, WATCH_NODE_BIT_MAP_SUSPEND)) {
            continue;
        }

        if (node->watch.recvEvent != NULL && !UTILS_IS_BIT_SET(node->bitMap, WATCH_NODE_BIT_MAP_READ_EVENT) &&
            IsFdEvent(node->watch.fd, readSet)) {
            FdSourceEventSet(fdSource, &node->bitMap, WATCH_NODE_BIT_MAP_READ_EVENT);
            IOTC_LOGD("fd recv event %d", node->watch.fd);
        }
        if (node->watch.sendEvent != NULL && !UTILS_IS_BIT_SET(node->bitMap, WATCH_NODE_BIT_MAP_WRITE_EVENT) &&
            IsFdEvent(node->watch.fd, writeSet)) {
            FdSourceEventSet(fdSource, &node->bitMap, WATCH_NODE_BIT_MAP_WRITE_EVENT);
            IOTC_LOGD("fd send event %d", node->watch.fd);
        }
        if (node->watch.exceEvent != NULL && !UTILS_IS_BIT_SET(node->bitMap, WATCH_NODE_BIT_MAP_EXCEPT_EVENT) &&
            IsFdEvent(node->watch.fd, exceptSet)) {
            FdSourceEventSet(fdSource, &node->bitMap, WATCH_NODE_BIT_MAP_EXCEPT_EVENT);
            IOTC_LOGD("fd exce event %d", node->watch.fd);
        }
    }
}

static bool FdSourcePoll(EventSource *self, uint32_t timeout)
{
    CHECK_RETURN(self != NULL, false);
    EventSourceFd *fdSource = (EventSourceFd *)self;

    EventSourceFdSet readSet = {0};
    EventSourceFdSet writeSet = {0};
    EventSourceFdSet exceptSet = {0};

    FdSourcePrepareFdSet(fdSource, &readSet, &writeSet, &exceptSet);
    int32_t event = fdSource->poll(&readSet, &writeSet, &exceptSet, timeout);
    if (event <= 0) {
        return false;
    }

    FdSourceGetFdSetEvent(fdSource, &readSet, &writeSet, &exceptSet);
    return true;
}

static bool FdSourceCheck(EventSource *self)
{
    CHECK_RETURN(self != NULL, false);
    EventSourceFd *fdSource = (EventSourceFd *)self;
    return fdSource->eventCnt > 0;
}

static bool FdEventProcess(WatchNode *node)
{
    bool notDel = true;
    if (UTILS_IS_BIT_SET(node->bitMap, WATCH_NODE_BIT_MAP_READ_EVENT) && node->watch.recvEvent != NULL) {
        notDel &= node->watch.recvEvent(node->watch.fd, node->watch.userData);
    }
    if (UTILS_IS_BIT_SET(node->bitMap, WATCH_NODE_BIT_MAP_WRITE_EVENT) && node->watch.recvEvent != NULL) {
        notDel &= node->watch.sendEvent(node->watch.fd, node->watch.userData);
    }
    if (UTILS_IS_BIT_SET(node->bitMap, WATCH_NODE_BIT_MAP_EXCEPT_EVENT) && node->watch.recvEvent != NULL) {
        notDel &= node->watch.exceEvent(node->watch.fd, node->watch.userData);
    }
    return notDel;
}

static bool FdSourceDispatch(EventSource *self)
{
    CHECK_RETURN(self != NULL, false);
    EventSourceFd *fdSource = (EventSourceFd *)self;
    WatchNode tmpNode = {0};

    ListEntry *maxItem = NULL;
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &fdSource->watchList) {
        WatchNode *node = CONTAINER_OF(item, WatchNode, node);
        if (UTILS_IS_BIT_SET(node->bitMap, WATCH_NODE_BIT_MAP_READ_EVENT) ||
            UTILS_IS_BIT_SET(node->bitMap, WATCH_NODE_BIT_MAP_WRITE_EVENT) ||
            UTILS_IS_BIT_SET(node->bitMap, WATCH_NODE_BIT_MAP_EXCEPT_EVENT)) {
            uint32_t prio = node->curPrio++;
            /* 选择优先级最高的有事件fd处理，并将其他fd优先级+1 */
            if (maxItem == NULL || prio > CONTAINER_OF(maxItem, WatchNode, node)->curPrio) {
                maxItem = item;
            }
        }
    }
    if (maxItem == NULL) {
        return true;
    }
    WatchNode *maxNode = CONTAINER_OF(maxItem, WatchNode, node);
    int32_t ret = memcpy_s(&tmpNode, sizeof(WatchNode), maxNode, sizeof(WatchNode));
    if (ret != EOK) {
        return false;
    }
    /* 本次处理的fd复位 */
    maxNode->curPrio = maxNode->watch.prio;
    FdSourceEventUnset(fdSource, &maxNode->bitMap, WATCH_NODE_BIT_MAP_READ_EVENT);
    FdSourceEventUnset(fdSource, &maxNode->bitMap, WATCH_NODE_BIT_MAP_WRITE_EVENT);
    FdSourceEventUnset(fdSource, &maxNode->bitMap, WATCH_NODE_BIT_MAP_EXCEPT_EVENT);
    bool notDel = FdEventProcess(&tmpNode);
    if (!notDel) {
        EventSourceFdRemove(self, tmpNode.watch.fd);
    }
    return true;
}

static void FdSourceFinalize(EventSource *self)
{
    CHECK_V_RETURN(self != NULL);
    EventSourceFd *fdSource = (EventSourceFd *)self;

    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &fdSource->watchList) {
        WatchNode *node = CONTAINER_OF(item, WatchNode, node);
        LIST_REMOVE(item);
        AdapterFree(node);
    }
}

EventSource *EventSourceFdNew(EventSourceFdPoll poll)
{
    CHECK_RETURN(poll != NULL, NULL);

    static EventSourceOps fdSourceOps = {
        .prepare = FdSourcePrepare,
        .poll = FdSourcePoll,
        .check = FdSourceCheck,
        .dispatch = FdSourceDispatch,
        .finalize = FdSourceFinalize,
    };

    EventSource *source = EventSourceNew(&fdSourceOps, sizeof(EventSourceFd), EVENT_SOURCE_NAME, NULL);
    if (source == NULL) {
        return NULL;
    }

    EventSourceFd *fdSource = (EventSourceFd *)source;
    LIST_INIT(&fdSource->watchList);
    fdSource->poll = poll;
    return source;
}

static WatchNode *WatchNodeNew(const FdWatchParam *watch)
{
    WatchNode *newNode = (WatchNode *)AdapterMalloc(sizeof(WatchNode));
    if (newNode == NULL) {
        return NULL;
    }
    (void)memset_s(newNode, sizeof(WatchNode), 0, sizeof(WatchNode));

    int32_t ret = memcpy_s(&newNode->watch, sizeof(FdWatchParam), watch, sizeof(FdWatchParam));
    if (ret != EOK) {
        AdapterFree(newNode);
        return NULL;
    }
    newNode->curPrio = watch->prio;
    return newNode;
}

bool EventSourceFdWatch(EventSource *source, const FdWatchParam *watch)
{
    CHECK_RETURN(source != NULL && watch != NULL, false);

    EventSourceFd *fdSource = (EventSourceFd *)source;
    if (fdSource->cnt >= EVENT_SOURCE_FD_MAX_WATCH_SIZE) {
        return false;
    }

    WatchNode *newNode = WatchNodeNew(watch);
    if (newNode == NULL) {
        IOTC_LOGW("new fd node error");
        return false;
    }

    fdSource->cnt++;
    IOTC_LOGD("watch fd %d/%u", watch->fd, watch->prio);
    if (LIST_EMPTY(&fdSource->watchList)) {
        LIST_INSERT_BEFORE(&newNode->node, &fdSource->watchList);
        return true;
    }
    ListEntry *tail = LIST_TAIL(&fdSource->watchList);
    if (tail == NULL || CONTAINER_OF(tail, WatchNode, node)->watch.prio >= watch->prio) {
        /* 插入链表尾部 */
        LIST_INSERT_BEFORE(&newNode->node, &fdSource->watchList);
        return true;
    }

    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &fdSource->watchList) {
        WatchNode *node = CONTAINER_OF(item, WatchNode, node);
        if (watch->prio >= node->watch.prio) {
            /* 插入小于等于优先级的节点前 */
            LIST_INSERT_BEFORE(&newNode->node, item);
            return true;
        }
    }
    AdapterFree(newNode);

    return false;
}

bool EventSourceFdSuspend(EventSource *source, int32_t fd)
{
    CHECK_RETURN(source != NULL && fd >= 0, false);
    EventSourceFd *fdSource = (EventSourceFd *)source;

    IOTC_LOGD("fd %d suspend", fd);
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &fdSource->watchList) {
        WatchNode *node = CONTAINER_OF(item, WatchNode, node);
        if (node->watch.fd != fd) {
            continue;
        }
        UTILS_BIT_SET(node->bitMap, WATCH_NODE_BIT_MAP_SUSPEND);
        /* 挂起后所有已有的事件不处理 */
        FdSourceEventUnset(fdSource, &node->bitMap, WATCH_NODE_BIT_MAP_READ_EVENT);
        FdSourceEventUnset(fdSource, &node->bitMap, WATCH_NODE_BIT_MAP_WRITE_EVENT);
        FdSourceEventUnset(fdSource, &node->bitMap, WATCH_NODE_BIT_MAP_EXCEPT_EVENT);
        return true;
    }

    return false;
}

bool EventSourceFdResume(EventSource *source, int32_t fd)
{
    CHECK_RETURN(source != NULL && fd >= 0, false);
    EventSourceFd *fdSource = (EventSourceFd *)source;

    IOTC_LOGD("fd %d resume", fd);
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &fdSource->watchList) {
        WatchNode *node = CONTAINER_OF(item, WatchNode, node);
        if (node->watch.fd != fd) {
            continue;
        }
        UTILS_BIT_RESET(node->bitMap, WATCH_NODE_BIT_MAP_SUSPEND);
        return true;
    }
    return false;
}

void EventSourceFdRemove(EventSource *source, int32_t fd)
{
    CHECK_V_RETURN(source != NULL && fd >= 0);

    EventSourceFd *fdSource = (EventSourceFd *)source;

    IOTC_LOGD("fd %d remove", fd);
    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &fdSource->watchList) {
        WatchNode *node = CONTAINER_OF(item, WatchNode, node);
        if (node->watch.fd != fd) {
            continue;
        }
        FdSourceEventUnset(fdSource, &node->bitMap, WATCH_NODE_BIT_MAP_READ_EVENT);
        FdSourceEventUnset(fdSource, &node->bitMap, WATCH_NODE_BIT_MAP_WRITE_EVENT);
        FdSourceEventUnset(fdSource, &node->bitMap, WATCH_NODE_BIT_MAP_EXCEPT_EVENT);
        LIST_REMOVE(item);
        AdapterFree(node);
        fdSource->cnt--;
        break;
    }
    return;
}