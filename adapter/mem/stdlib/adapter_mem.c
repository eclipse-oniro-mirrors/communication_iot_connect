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
#include "adapter_mem.h"
#include <string.h>
#include <stdlib.h>

#if IOTC_CONF_MEM_DEBUG
#include <stdbool.h>
#include "adapter_os.h"
#include "utils_list.h"
#include "securec.h"
#include "adapter_log.h"
typedef struct {
    ListEntry node;
    uint8_t magic;
    uint8_t flag;
    const char *func;
    uint32_t time;
    uint32_t line;
    uint32_t size;
    uint8_t mem[];
} MemNode;
#define MEM_NODE_MAGIC 0xAA
#define MEM_FLAG_USED 0xA0
#ifndef NON_NULL_STR
#define NON_NULL_STR(_s) ((_s) == NULL? "NULL" : (_s))
#endif

static AdapterMutexId *g_mutex = NULL;
static ListEntry g_memList = LIST_DECLARE_INIT(&g_memList);

static uint32_t DeltaTime(uint32_t timeNew, uint32_t timeOld)
{
    uint32_t deltaTime;

    if (timeNew >= timeOld) {
        deltaTime = timeNew - timeOld;
    } else {
        deltaTime = (UINT32_MAX - timeOld) + timeNew + 1; /* 处理时间翻转 */
    }

    return deltaTime;
}

void MemLock(void)
{
    if (g_mutex == NULL) {
        g_mutex = AdapterCreateMutex();
    }
    if (g_mutex != NULL) {
        (void)AdapterLockMutex(g_mutex, ADAPTER_WAIT_FOREVER);
    }
}

void MemUnlock(void)
{
    if (g_mutex != NULL) {
        (void)AdapterUnlockMutex(g_mutex);
    }
}

static void *AdapterDebugMallocInsert(uint32_t size, const char *func, uint32_t line, bool init)
{
    MemNode *newNode = (MemNode *)malloc(size + sizeof(MemNode));
    if (newNode == NULL) {
        ADAPTER_LOGD("malloc error %u", size);
        return NULL;
    }

    newNode->magic = MEM_NODE_MAGIC;
    newNode->flag = MEM_FLAG_USED;
    newNode->func = func;
    newNode->line = line;
    newNode->size = size;
    newNode->time = AdapterGetSysTimeMs();

    if (init) {
        /* 用与内存测试，malloc时不对用户使用的空间做初始化 */
        (void)memset_s(newNode->mem, size, 0, size);
    }
    ADAPTER_LOGD("malloc %p/%u/%s/%u/", newNode->mem, size, func, line);
    MemLock();
    LIST_INSERT(&newNode->node, &g_memList);
    MemUnlock();
    return newNode->mem;
}

void *AdapterDebugMalloc(uint32_t size, const char *func, uint32_t line)
{
    if (size == 0) {
        ADAPTER_LOGF("malloc zero %s/%u!!", NON_NULL_STR(func), line);
        return NULL;
    }

    return AdapterDebugMallocInsert(size, func, line, false);
}

void *AdapterDebugCalloc(uint32_t num, uint32_t size, const char *func, uint32_t line)
{
    if ((size == 0) || (num == 0)) {
        ADAPTER_LOGF("calloc zero %s/%u!!", NON_NULL_STR(func), line);
        return NULL;
    }

    return AdapterDebugMallocInsert(num * size, func, line, true);
}

void AdapterDebugFree(void *pt, const char *func, uint32_t line)
{
    if (pt == NULL) {
        ADAPTER_LOGF("free null %s/%u!!", NON_NULL_STR(func), line);
        return;
    }
    MemNode *node = CONTAINER_OF(pt, MemNode, mem);
    if (node->magic != MEM_NODE_MAGIC) {
        ADAPTER_LOGF("free invalid mem %p/%s/%u!!", pt, NON_NULL_STR(func), line);
        return;
    }
    if (node->flag != MEM_FLAG_USED) {
        ADAPTER_LOGF("may double free %p/%s/%u!!", pt, NON_NULL_STR(func), line);
        return;
    }
    node->flag = 0;

    bool find = false;
    MemLock();
    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &g_memList) {
        MemNode *curNode = CONTAINER_OF(item, MemNode, node);
        if (curNode != node) {
            continue;
        }
        LIST_REMOVE(item);
        find = true;
        ADAPTER_LOGD("free %p/%u/%s/%u/%s/%u", curNode->mem, curNode->size, NON_NULL_STR(func), line,
            NON_NULL_STR(curNode->func), curNode->line);
        break;
    }
    MemUnlock();
    if (!find) {
        ADAPTER_LOGF("may double free %s/%u!!", NON_NULL_STR(func), line);
        return;
    }
#if IOTC_CONF_MEM_DEBUG_NO_FREE
    return;
#else
    free(node);
#endif
}

void AdapterMemDump(void)
{
    uint32_t cur = AdapterGetSysTimeMs();
    MemLock();
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM_REV(item, &g_memList) {
        MemNode *curNode = CONTAINER_OF(item, MemNode, node);
        ADAPTER_LOGD("mem node %p/%u/%u/%s/%u", curNode->mem, curNode->size, DeltaTime(cur, curNode->time),
            NON_NULL_STR(curNode->func), curNode->line);
    }
    MemUnlock();
}

#else /* IOTC_CONF_MEM_DEBUG */

void *AdapterMalloc(uint32_t size)
{
    if (size == 0) {
        return NULL;
    }

    return malloc(size);
}

void *AdapterCalloc(uint32_t num, uint32_t size)
{
    if ((size == 0) || (num == 0)) {
        return NULL;
    }

    return calloc(num, size);
}

void AdapterFree(void *pt)
{
    if (pt == NULL) {
        return;
    }
    free(pt);
}
#endif