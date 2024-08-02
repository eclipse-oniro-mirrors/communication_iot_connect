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
#include "main_loop.h"
#include "securec.h"
#include "comm_def.h"
#include "iotc_log.h"
#include "utils_list.h"
#include "adapter_os.h"
#include "utils_mutex_global.h"
#include "adapter_mem.h"
#include "utils_common.h"
#include "utils_assert.h"
#include "iotc_errcode.h"

typedef struct {
    ListEntry node;
    FwkLoop loop;
    bool taskFlag;
} LoopNode;

struct MainLoopCtx {
    AdapterSemId *stop;
    ListEntry loopList;
} g_mainLoopCtx;

int32_t FwkMainLoopInit(void)
{
    (void)memset_s(&g_mainLoopCtx, sizeof(g_mainLoopCtx), 0, sizeof(g_mainLoopCtx));
    LIST_INIT(&g_mainLoopCtx.loopList);
    return IOTC_OK;
}

void FwkMainLoopDeinit(void)
{
    if (g_mainLoopCtx.stop != NULL) {
        AdapterDestroySem(g_mainLoopCtx.stop);
        g_mainLoopCtx.stop = NULL;
    }
    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &g_mainLoopCtx.loopList) {
        LoopNode *node = CONTAINER_OF(item, LoopNode, node);
        LIST_REMOVE(item);
        AdapterFree(node);
    }
}

static void LoopTask(void *param)
{
    CHECK_V_RETURN(param != NULL);
    FwkLoop *loop = (FwkLoop *)param;
    CHECK_V_RETURN(loop->entry != NULL);

    int32_t ret = loop->entry(loop->userData);
    IOTC_LOGN("loop exit with ret %s/%d", loop->name, ret);

    if (!UtilsGlobalMutexLock()) {
        IOTC_LOGE("global lock error");
        return;
    }

    bool isTask = false;
    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &g_mainLoopCtx.loopList) {
        LoopNode *node = CONTAINER_OF(item, LoopNode, node);
        if (memcmp(loop, &node->loop, sizeof(FwkLoop)) == 0) {
            isTask = node->taskFlag;
            LIST_REMOVE(item);
            AdapterFree(node);
            break;
        }
    }
    /* 最后一个任务退出时如果非主线程则使用信号量通知主线程，通知后变量loop被释放不可用 */
    if (LIST_EMPTY(&g_mainLoopCtx.loopList) && g_mainLoopCtx.stop != NULL) {
        ret = AdapterPostSem(g_mainLoopCtx.stop);
        if (ret != IOTC_OK) {
            IOTC_LOGW("post stop sem error %d", ret);
        }
    }
    UtilsGlobalMutexUnlock();
    if (isTask) {
        AdapterDeleteTask(NULL);
    }
}

int32_t FwkMainLoopReg(const FwkLoop *loop)
{
    CHECK_RETURN(loop != NULL && loop->entry != NULL && loop->exit != NULL && loop->name != NULL,
        IOTC_ERR_PARAM_INVALID);

    LoopNode *newNode = (LoopNode *)AdapterMalloc(sizeof(LoopNode));
    if (newNode == NULL) {
        IOTC_LOGW("malloc error");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(newNode, sizeof(LoopNode), 0, sizeof(LoopNode));
    int32_t ret = memcpy_s(&newNode->loop, sizeof(FwkLoop), loop, sizeof(FwkLoop));
    if (ret != EOK) {
        AdapterFree(newNode);
        return IOTC_ERR_SECUREC_MEMCPY;
    }

    if (!UtilsGlobalMutexLock()) {
        AdapterFree(newNode);
        return IOTC_ERR_TIMEOUT;
    }

    LIST_INSERT_BEFORE(&newNode->node, &g_mainLoopCtx.loopList);
    UtilsGlobalMutexUnlock();
    return IOTC_OK;
}

static FwkLoop *GetLoopArray(uint32_t *cnt)
{
    *cnt = 0;
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &g_mainLoopCtx.loopList) {
        ++(*cnt);
    }

    if (*cnt == 0) {
        return NULL;
    }
    FwkLoop *ret = (FwkLoop *)AdapterCalloc(*cnt, sizeof(FwkLoop));
    if (ret == NULL) {
        IOTC_LOGW("calloc error");
        return NULL;
    }
    uint32_t index = 0;
    int32_t error = 0;
    LIST_FOR_EACH_ITEM(item, &g_mainLoopCtx.loopList) {
        LoopNode *curNode = CONTAINER_OF(item, LoopNode, node);
        error = memcpy_s(ret + index, sizeof(FwkLoop), &curNode->loop, sizeof(FwkLoop));
        if (error != EOK) {
            AdapterFree(ret);
            return NULL;
        }
        if (index != 0) {
            curNode->taskFlag = true;
        }
        ++index;
    }
    return ret;
}

static void StartTaskForLoop(const FwkLoop *loops, uint32_t cnt, uint32_t taskSize)
{
    for (uint32_t i = 0; i < cnt; ++i) {
        if (loops[i].entry == NULL) {
            IOTC_LOGE("loop no entry %s", NON_NULL_STR(loops[i].name));
            continue;
        }

        AdapterTaskParam task = {
            .arg = (void *)&loops[i],
            .func = LoopTask,
            .name = loops[i].name,
            .prio = ADAPTER_TASK_PRIORITY_MID,
            .stackSize = taskSize,
        };
        AdapterTaskId *id = AdapterCreateTask(&task);
        if (id == NULL) {
            IOTC_LOGE("create loop task error %s/%u", task.name, task.stackSize);
        } else {
            IOTC_LOGN("create loop task success %s", task.name);
        }
    }
}

void FwkMainLoopEntry(uint32_t taskSize)
{
    if (!UtilsGlobalMutexLock()) {
        IOTC_LOGE("global lock error");
        return;
    }

    uint32_t cnt;
    FwkLoop *loops = GetLoopArray(&cnt);
    UtilsGlobalMutexUnlock();
    if (loops == NULL) {
        IOTC_LOGW("no reg main loop");
        return;
    }

    /* 第1个loop当前线程执行，其余单独起线程执行 */
    if (cnt > 1) {
        /* 移除第1个 */
        StartTaskForLoop(loops + 1, cnt - 1, taskSize);
    }

    if (loops[0].entry != NULL) {
        LoopTask(&loops[0]);
    } else {
        IOTC_LOGE("loop no entry %s", NON_NULL_STR(loops[0].name));
    }
    
    if (!UtilsGlobalMutexLock()) {
        AdapterFree(loops);
        return;
    }

    if (LIST_EMPTY(&g_mainLoopCtx.loopList)) {
        UtilsGlobalMutexUnlock();
        AdapterFree(loops);
        IOTC_LOGN("main loop exit no wait");
        return;
    }

    if (g_mainLoopCtx.stop != NULL) {
        g_mainLoopCtx.stop = AdapterCreateSem(0);
    }
    UtilsGlobalMutexUnlock();
    if (g_mainLoopCtx.stop == NULL) {
        IOTC_LOGE("create sem error");
        AdapterFree(loops);
        return;
    }
    /* 等待所有线程退出 */
    int32_t ret = AdapterWaitSem(g_mainLoopCtx.stop, ADAPTER_WAIT_FOREVER);
    if (ret != IOTC_OK) {
        IOTC_LOGE("wait all task stop error %d", ret);
    } else {
        IOTC_LOGN("main loop exit");
    }
    AdapterFree(loops);
    AdapterDestroySem(g_mainLoopCtx.stop);
    g_mainLoopCtx.stop = NULL;
    return;
}

void FwkMainLoopExit(void)
{
    if (!UtilsGlobalMutexLock()) {
        IOTC_LOGE("global lock error");
        return;
    }
    uint32_t cnt;
    FwkLoop *loops = GetLoopArray(&cnt);
    UtilsGlobalMutexUnlock();

    if (loops == NULL || cnt == 0) {
        return;
    }
    for (uint32_t i = 0; i < cnt; ++i) {
        if (loops[i].exit != NULL) {
            loops[i].exit(loops[i].userData);
        }
    }
    AdapterFree(loops);
}
