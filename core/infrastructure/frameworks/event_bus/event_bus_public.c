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
#include <stddef.h>
#include "event_bus_public.h"
#include "utils_list.h"
#include "iotc_errcode.h"
#include "event_bus.h"
#include "utils_common.h"
#include "utils_mutex_global.h"
#include "iotc_log.h"
#include "utils_assert.h"
#include "securec.h"

typedef struct {
    ListEntry node;
    IotcEventCallback listener;
} EventListenerNode;

static ListEntry g_listenerList = LIST_DECLARE_INIT(&g_listenerList);
static uint32_t g_listenerNum = 0;
static const uint8_t LISTENER_MAX_NUM = UINT8_MAX;

static bool ListenerEventMatchFunc(uint32_t event)
{
    /* 仅订阅提供给外部的事件 */
    if (event >= IOTC_EVENT_ID_BASE(IOTC_SUB_MODULE_PUBLIC) &&
        event < IOTC_EVENT_ID_BASE(IOTC_SUB_MODULE_PUBLIC + 1)) {
        return true;
    }
    return false;
}

static void ListenerEventBusCallback(uint32_t event, void *param, uint32_t len)
{
    NOT_USED(param);
    NOT_USED(len);

    IotcEventCallback *listeners = NULL;
    uint32_t index = 0;
    (void)UtilsGlobalMutexLock();
    do {
        if (g_listenerNum == 0) {
            break;
        }

        /* 将回调函数拷贝出来，避免在锁中执行 */
        listeners = (IotcEventCallback *)AdapterCalloc(g_listenerNum, sizeof(IotcEventCallback));
        if (listeners == NULL) {
            IOTC_LOGW("calloc error %u", g_listenerNum);
            break;
        }
        
        ListEntry *item = NULL;
        LIST_FOR_EACH_ITEM(item, &g_listenerList) {
            EventListenerNode *curNode = CONTAINER_OF(item, EventListenerNode, node);
            listeners[index++] = curNode->listener;
        }
    } while (0);
    UtilsGlobalMutexUnlock();
    if (listeners == NULL) {
        return;
    }
    for (uint32_t i = 0; i < index; ++i) {
        if (listeners[i] != NULL) {
            listeners[i](event);
        }
    }
    AdapterFree(listeners);
}

int32_t IotcPublicEventListenerInit(void)
{
    int32_t ret = EventBusSubscribeMatch(ListenerEventBusCallback, ListenerEventMatchFunc);
    if (ret != IOTC_OK) {
        IOTC_LOGW("sub event error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

void IotcPublicEventListenerDeinit(void)
{
    (void)EventBusUnsubscribe(ListenerEventBusCallback);
}

int32_t IotcRegPublicEventListener(IotcEventCallback listener)
{
    CHECK_RETURN_LOGW(listener != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    CHECK_RETURN_LOGW(g_listenerNum <= LISTENER_MAX_NUM, IOTC_SDK_AILIFE_COMM_ERR_EVENT_LISTENER_MAX,
        "event listener over size");

    EventListenerNode *newNode = (EventListenerNode *)AdapterMalloc(sizeof(EventListenerNode));
    if (newNode == NULL) {
        IOTC_LOGW("malloc error");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(newNode, sizeof(EventListenerNode), 0, sizeof(EventListenerNode));
    newNode->listener = listener;
    
    (void)UtilsGlobalMutexLock();
    LIST_INSERT_BEFORE(&newNode->node, &g_listenerList);
    ++g_listenerNum;
    UtilsGlobalMutexUnlock();
    return IOTC_OK;
}

int32_t IotcUnregPublicEventListener(IotcEventCallback listener)
{
    CHECK_RETURN_LOGW(listener != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    (void)UtilsGlobalMutexLock();

    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &g_listenerList) {
        EventListenerNode *curNode = CONTAINER_OF(item, EventListenerNode, node);
        if (curNode->listener != listener) {
            continue;
        }
        LIST_REMOVE(item);
        AdapterFree(curNode);
        --g_listenerNum;
    }

    UtilsGlobalMutexUnlock();
    return IOTC_OK;
}

int32_t IotcUnregAllPublicEventListener(void)
{
    (void)UtilsGlobalMutexLock();

    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &g_listenerList) {
        EventListenerNode *curNode = CONTAINER_OF(item, EventListenerNode, node);
        LIST_REMOVE(item);
        AdapterFree(curNode);
    }
    g_listenerNum = 0;
    UtilsGlobalMutexUnlock();
    return IOTC_OK;
}