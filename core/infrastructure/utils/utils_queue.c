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
#include "utils_queue.h"
#include "securec.h"
#include "iotc_log.h"
#include "iotc_errcode.h"
#include "adapter_mem.h"
#include "utils_list.h"

typedef struct {
    ListEntry list;
    void *value;
    uint32_t valueLen;
} QueueNode;

struct UtilsQueue {
    ListEntry head;
    uint32_t count;
    uint32_t capacity;
    QueueFreeValue freeValue;
};

UtilsQueue *UtilsQueueCreate(uint32_t capacity, QueueFreeValue freeValue)
{
    if (capacity == 0) {
        IOTC_LOGW("invalid param");
        return NULL;
    }

    UtilsQueue *queue = (UtilsQueue *)AdapterMalloc(sizeof(UtilsQueue));
    if (queue == NULL) {
        IOTC_LOGW("malloc");
        return NULL;
    }
    (void)memset_s(queue, sizeof(UtilsQueue), 0, sizeof(UtilsQueue));
    queue->capacity = capacity;
    queue->freeValue = freeValue;
    queue->count = 0;
    LIST_INIT(&queue->head);
    IOTC_LOGI("queue create success count=%u,capacity=%u",
        queue->count, queue->capacity);
    return queue;
}

int32_t UtilsQueuePush(UtilsQueue *queue, const void *value, uint32_t valueLen)
{
    if ((queue == NULL) || (value == NULL) || (valueLen == 0)) {
        IOTC_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    if (valueLen > MAX_QUEUE_VALUE_LEN) {
        IOTC_LOGW("too long");
        return IOTC_CORE_COMM_UTILS_ERR_QUEUE_VAL_TOO_LONG;
    }
    if (queue->count >= queue->capacity) {
        IOTC_LOGW("full");
        return IOTC_CORE_COMM_UTILS_ERR_QUEUE_FULL;
    }

    QueueNode *newNode = (QueueNode *)AdapterMalloc(sizeof(QueueNode));
    if (newNode == NULL) {
        IOTC_LOGW("malloc");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(newNode, sizeof(QueueNode), 0, sizeof(QueueNode));
    newNode->value = AdapterMalloc(valueLen);
    if (newNode->value == NULL) {
        IOTC_LOGW("malloc");
        AdapterFree(newNode);
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    if (memcpy_s(newNode->value, valueLen, value, valueLen) != EOK) {
        IOTC_LOGW("memcpy_s");
        AdapterFree(newNode->value);
        AdapterFree(newNode);
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    newNode->valueLen = valueLen;
    LIST_INIT(&newNode->list);
    LIST_INSERT_BEFORE(&newNode->list, &queue->head);
    queue->count++;
    IOTC_LOGI("queue push success count=%u,capacity=%u,valueLen=%u",
        queue->count, queue->capacity, valueLen);
    return IOTC_OK;
}

int32_t UtilsQueuePushMem(UtilsQueue *queue, void **value, uint32_t valueLen)
{
    if ((queue == NULL) || (value == NULL) || (valueLen == 0) || (*value == NULL)) {
        IOTC_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    if (queue->count >= queue->capacity) {
        IOTC_LOGW("full");
        return IOTC_CORE_COMM_UTILS_ERR_QUEUE_FULL;
    }

    QueueNode *newNode = (QueueNode *)AdapterMalloc(sizeof(QueueNode));
    if (newNode == NULL) {
        IOTC_LOGW("malloc");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(newNode, sizeof(QueueNode), 0, sizeof(QueueNode));
    newNode->value = *value;
    *value = NULL;
    newNode->valueLen = valueLen;
    LIST_INIT(&newNode->list);
    LIST_INSERT_BEFORE(&newNode->list, &queue->head);
    queue->count++;
    IOTC_LOGI("queue push mem success count=%u,capacity=%u,valueLen=%u",
        queue->count, queue->capacity, valueLen);
    return IOTC_OK;
}

int32_t UtilsQueuePop(UtilsQueue *queue, void *value, uint32_t valueSize, uint32_t *valueLen)
{
    if ((queue == NULL) || (value == NULL) || (valueSize == 0) || (valueLen == NULL)) {
        IOTC_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    if (queue->count == 0) {
        IOTC_LOGW("empty");
        return IOTC_CORE_COMM_UTILS_ERR_QUEUE_EMPTY;
    }

    QueueNode *node = CONTAINER_OF(LIST_HEAD(&queue->head), QueueNode, list);
    if (memcpy_s(value, valueSize, node->value, node->valueLen) != EOK) {
        IOTC_LOGW("memcpy_s");
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    *valueLen = node->valueLen;
    LIST_REMOVE(&node->list);
    AdapterFree(node->value);
    AdapterFree(node);
    queue->count--;
    IOTC_LOGI("queue pop success count=%u,capacity=%u,valueLen=%u,valueSize=%u",
        queue->count, queue->capacity, *valueLen, valueSize);
    return IOTC_OK;
}

int32_t UtilsQueuePopMem(UtilsQueue *queue, void **value, uint32_t *valueLen)
{
    if ((queue == NULL) || (value == NULL) || (valueLen == NULL)) {
        IOTC_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    if (queue->count == 0) {
        IOTC_LOGW("empty");
        return IOTC_CORE_COMM_UTILS_ERR_QUEUE_EMPTY;
    }

    QueueNode *node = CONTAINER_OF(LIST_HEAD(&queue->head), QueueNode, list);
    *value = node->value;
    *valueLen = node->valueLen;

    LIST_REMOVE(&node->list);
    AdapterFree(node);
    queue->count--;
    IOTC_LOGI("queue pop success count=%u,capacity=%u,valueLen=%u",
        queue->count, queue->capacity, *valueLen);
    return IOTC_OK;
}

uint32_t UtilsQueueGetCount(UtilsQueue *queue)
{
    if (queue == NULL) {
        IOTC_LOGW("invalid param");
        return 0;
    }
    return queue->count;
}

bool UtilsQueueIsFull(UtilsQueue *queue)
{
    if (queue == NULL) {
        IOTC_LOGW("invalid param");
        return 0;
    }
    return (queue->count >= queue->capacity);
}

void UtilsQueueDestroy(UtilsQueue **queueAddr)
{
    if ((queueAddr == NULL) || (*queueAddr == NULL)) {
        IOTC_LOGW("invalid param");
        return;
    }

    UtilsQueue *queue = *queueAddr;
    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &queue->head) {
        QueueNode *node = CONTAINER_OF(item, QueueNode, list);
        LIST_REMOVE(item);
        if (queue->freeValue != NULL) {
            queue->freeValue(node->value);
        }
        AdapterFree(node->value);
        AdapterFree(node);
    }
    AdapterFree(queue);
    *queueAddr = NULL;
    IOTC_LOGI("queue destroy success");
}