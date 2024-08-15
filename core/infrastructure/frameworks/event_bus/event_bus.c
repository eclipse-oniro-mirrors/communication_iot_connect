
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
#include "event_bus.h"
#include "securec.h"
#include "utils_list.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "adapter_mem.h"
#include "utils_mutex_ex.h"
#include "utils_common.h"

enum {
    EVENT_TYPE_SINGLE = 1,
    EVENT_TYPE_ALL,
    EVENT_TYPE_MATCH,
};

typedef struct {
    ListEntry list;
    uint32_t type;
    uint32_t event;
    EventMatchFunc match;
    EventBusCallback listener;
} EventList;

typedef struct {
    uint32_t event;
    const char *name;
    void *param;
    uint32_t len;
    EventBusParamFreeHandler freeFunc;
} AsyncEventParam;

static ListEntry g_eventList = LIST_DECLARE_INIT(&g_eventList);
static UtilsExMutex *g_eventMutex = NULL;
EventBusAsyncExecutor g_asyncExecutor = NULL;

static bool IsExisted(uint32_t type, const EventBusCallback listener, uint32_t event, EventMatchFunc match)
{
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &g_eventList) {
        EventList *node = CONTAINER_OF(item, EventList, list);
        if (node->type == EVENT_TYPE_SINGLE) {
            if ((listener == node->listener) && (event == node->event)) {
                return true;
            }
        } else if (node->type == EVENT_TYPE_ALL) {
            if (listener == node->listener) {
                return true;
            }
        } else if (node->type == EVENT_TYPE_MATCH) {
            if (match == node->match) {
                return true;
            }
        }
    }
    return false;
}

static void AsyncHandle(void *param)
{
    if (param == NULL) {
        return;
    }
    AsyncEventParam *asyncParam = (AsyncEventParam *)param;
    EventBusPublishSyncInner(asyncParam->event, asyncParam->name, asyncParam->param, asyncParam->len);
    if (asyncParam->freeFunc != NULL) {
        asyncParam->freeFunc(asyncParam->event, asyncParam->param, asyncParam->len);
    }
    AdapterFree(asyncParam);
}

static int32_t SubScribe(const EventBusCallback listener, uint32_t event)
{
    if (IsExisted(EVENT_TYPE_SINGLE, listener, event, NULL)) {
        return IOTC_OK;
    }
    EventList *newNode = (EventList *)AdapterMalloc(sizeof(EventList));
    if (newNode == NULL) {
        IOTC_LOGW("malloc");
        return IOTC_ERROR;
    }
    (void)memset_s(newNode, sizeof(EventList), 0, sizeof(EventList));
    newNode->type = EVENT_TYPE_SINGLE;
    newNode->event = event;
    newNode->listener = listener;
    LIST_INIT(&newNode->list);
    LIST_INSERT_BEFORE(&newNode->list, &g_eventList);
    return IOTC_OK;
}

static int32_t SubScribeMatch(const EventBusCallback listener, EventMatchFunc match)
{
    if (IsExisted(EVENT_TYPE_MATCH, listener, 0, match)) {
        return IOTC_OK;
    }
    EventList *newNode = (EventList *)AdapterMalloc(sizeof(EventList));
    if (newNode == NULL) {
        IOTC_LOGW("malloc");
        return IOTC_ERROR;
    }
    (void)memset_s(newNode, sizeof(EventList), 0, sizeof(EventList));
    newNode->type = EVENT_TYPE_MATCH;
    newNode->listener = listener;
    newNode->match = match;
    LIST_INIT(&newNode->list);
    LIST_INSERT_BEFORE(&newNode->list, &g_eventList);
    return IOTC_OK;
}

static int32_t SubScribeAll(const EventBusCallback listener)
{
    if (IsExisted(EVENT_TYPE_ALL, listener, 0, NULL)) {
        return IOTC_OK;
    }
    EventList *newNode = (EventList *)AdapterMalloc(sizeof(EventList));
    if (newNode == NULL) {
        IOTC_LOGW("malloc");
        return IOTC_ERROR;
    }
    (void)memset_s(newNode, sizeof(EventList), 0, sizeof(EventList));
    newNode->type = EVENT_TYPE_ALL;
    newNode->listener = listener;
    LIST_INIT(&newNode->list);
    LIST_INSERT_BEFORE(&newNode->list, &g_eventList);
    return IOTC_OK;
}

static int32_t UnsubScribe(const EventBusCallback listener)
{
    bool find = false;
    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &g_eventList) {
        EventList *node = CONTAINER_OF(item, EventList, list);
        if (node->listener == listener) {
            LIST_REMOVE(item);
            AdapterFree(node);
            find = true;
        }
    }
    if (!find) {
        IOTC_LOGW("no find");
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t EventBusInit(void)
{
    if (g_eventMutex != NULL) {
        return IOTC_OK;
    }
    g_eventMutex = UtilsCreateExMutex();
    if (g_eventMutex == NULL) {
        IOTC_LOGW("create mutex");
        return IOTC_CORE_COMM_UTILS_ERR_EX_MUTEX_CREATE;
    }
    return IOTC_OK;
}

int32_t EventBusRegAsyncExecutor(EventBusAsyncExecutor asyncExecutor)
{
    if (!UtilsExMutexLock(g_eventMutex)) {
        IOTC_LOGW("lock");
        return IOTC_ERROR;
    }
    g_asyncExecutor = asyncExecutor;
    UtilsExMutexUnlock(g_eventMutex);
    return IOTC_OK;
}

void EventBusDeinit(void)
{
    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &g_eventList) {
        EventList *node = CONTAINER_OF(item, EventList, list);
        LIST_REMOVE(item);
        AdapterFree(node);
    }
    UtilsDestroyExMutex(&g_eventMutex);
}

int32_t EventBusSubscribe(const EventBusCallback listener, uint32_t event)
{
    if (listener == NULL) {
        IOTC_LOGW("invalid param");
        return IOTC_ERROR;
    }
    if (!UtilsExMutexLock(g_eventMutex)) {
        IOTC_LOGW("lock");
        return IOTC_ERROR;
    }
    int32_t ret = SubScribe(listener, event);
    UtilsExMutexUnlock(g_eventMutex);
    return ret;
}

int32_t EventBusSubscribeMatch(const EventBusCallback listener, EventMatchFunc match)
{
    if ((listener == NULL) || (match == NULL)) {
        IOTC_LOGW("invalid param");
        return IOTC_ERROR;
    }
    if (!UtilsExMutexLock(g_eventMutex)) {
        IOTC_LOGW("lock");
        return IOTC_ERROR;
    }
    int32_t ret = SubScribeMatch(listener, match);
    UtilsExMutexUnlock(g_eventMutex);
    return ret;
}

int32_t EventBusSubscribeAll(const EventBusCallback listener)
{
    if (listener == NULL) {
        IOTC_LOGW("invalid param");
        return IOTC_ERROR;
    }
    if (!UtilsExMutexLock(g_eventMutex)) {
        IOTC_LOGW("lock");
        return IOTC_ERROR;
    }
    int32_t ret = SubScribeAll(listener);
    UtilsExMutexUnlock(g_eventMutex);
    return ret;
}

int32_t EventBusUnsubscribe(const EventBusCallback listener)
{
    if (listener == NULL) {
        IOTC_LOGW("invalid param");
        return IOTC_ERROR;
    }
    if (!UtilsExMutexLock(g_eventMutex)) {
        IOTC_LOGW("lock");
        return IOTC_ERROR;
    }
    int32_t ret = UnsubScribe(listener);
    UtilsExMutexUnlock(g_eventMutex);
    return ret;
}

static int32_t GetPublishList(EventList **list, uint32_t *num, uint32_t event)
{
    if (!UtilsExMutexLock(g_eventMutex)) {
        IOTC_LOGW("lock");
        return IOTC_ERR_TIMEOUT;
    }

    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &g_eventList) {
        EventList *node = CONTAINER_OF(item, EventList, list);
        if (node->type == EVENT_TYPE_SINGLE && event != node->event) {
            continue;
        };
        ++(*num);
    }
    if (*num == 0) {
        UtilsExMutexUnlock(g_eventMutex);
        return IOTC_OK;
    }

    *list = (EventList *)AdapterCalloc(*num, sizeof(EventList));
    if (*list == NULL) {
        IOTC_LOGW("calloc error %u", *num);
        UtilsExMutexUnlock(g_eventMutex);
        return IOTC_ADAPTER_MEM_ERR_CALLOC;
    }

    int32_t ret;
    uint32_t index = 0;
    LIST_FOR_EACH_ITEM(item, &g_eventList) {
        EventList *node = CONTAINER_OF(item, EventList, list);
        if (node->type == EVENT_TYPE_SINGLE && event != node->event) {
            continue;
        };

        ret = memcpy_s(&((*list)[index++]), sizeof(EventList), node, sizeof(EventList));
        if (ret != EOK) {
            UTILS_FREE_2_NULL(*list);
            UtilsExMutexUnlock(g_eventMutex);
            return IOTC_ERR_SECUREC_MEMCPY;
        }
        if (index >= *num) {
            break;
        }
    }
    UtilsExMutexUnlock(g_eventMutex);
    return IOTC_OK;
}

void EventBusPublishSyncInner(uint32_t event, const char *name, void *param, uint32_t len)
{
    EventList *list = NULL;
    uint32_t num = 0;
    int32_t ret = GetPublishList(&list, &num, event);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get pub list error %d", ret);
        return;
    }
    if (list == NULL) {
        IOTC_LOGI("[EVENT BUS] pub [%u/%s/%u] no sub", event, NON_NULL_STR(name), len);
        return;
    }

    for (uint32_t i = 0; i < num; ++i) {
        EventList *cur = list + i;
        if (cur->listener == NULL) {
            continue;
        }
        if (cur->type == EVENT_TYPE_SINGLE) {
            if (event == cur->event) {
                cur->listener(event, param, len);
            }
        } else if (cur->type == EVENT_TYPE_ALL) {
            cur->listener(event, param, len);
        } else if (cur->type == EVENT_TYPE_MATCH) {
            if (cur->match(event)) {
                cur->listener(event, param, len);
            }
        }
    }
    IOTC_LOGN("[EVENT BUS] pub [%u/%s/%u] to [%u] sub", event, NON_NULL_STR(name), len, num);
    AdapterFree(list);
    return;
}

int32_t EventBusPublishAsyncInner(uint32_t event, const char *name, void *param, uint32_t len,
    EventBusParamFreeHandler freeFunc)
{
    if (!UtilsExMutexLock(g_eventMutex)) {
        IOTC_LOGW("lock");
        return IOTC_ERROR;
    }
    EventBusAsyncExecutor asyncExecutor = g_asyncExecutor;
    UtilsExMutexUnlock(g_eventMutex);
    if (asyncExecutor == NULL) {
        IOTC_LOGW("executor null");
        return IOTC_ERR_PARAM_INVALID;
    }
    AsyncEventParam *newParam = (AsyncEventParam *)AdapterMalloc(sizeof(AsyncEventParam));
    if (newParam == NULL) {
        IOTC_LOGW("malloc");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(newParam, sizeof(AsyncEventParam), 0, sizeof(AsyncEventParam));
    newParam->event = event;
    newParam->name = name;
    newParam->param = param;
    newParam->len = len;
    newParam->freeFunc = freeFunc;
    int32_t ret = asyncExecutor(AsyncHandle, newParam);
    if (ret != IOTC_OK) {
        IOTC_LOGW("executor error %d", ret);
        AdapterFree(newParam);
        return ret;
    }
    IOTC_LOGN("[EVENT BUS] add async pub [%u/%s/%u]", event, NON_NULL_STR(name), len);
    return IOTC_OK;
}

