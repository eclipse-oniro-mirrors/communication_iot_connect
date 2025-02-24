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
#include "fwk_init.h"
#include "utils_list.h"
#include "utils_bit_map.h"
#include "iotc_mem.h"
#include "comm_def.h"
#include "securec.h"
#include "utils_bit_map.h"
#include "iotc_log.h"
#include "iotc_os.h"
#include "utils_mutex_global.h"
#include "utils_common.h"
#include "utils_assert.h"
#include "iotc_errcode.h"

typedef struct {
    ListEntry node;
    const char *name;
    const FwkInitUnit *units;
    uint32_t num;
    BitMap *initMap;
} InitUnitNode;

static ListEntry g_initList = LIST_DECLARE_INIT(&g_initList);

static int32_t DoInitTargetLevelUnitNode(InitUnitNode *node, int32_t level)
{
    IOTC_LOGI("DoInitTargetLevelUnitNode start");
    int32_t ret;
    for (uint32_t i = 0; i < node->num; ++i) {
        const FwkInitUnit *cur = &node->units[i];
        IOTC_LOGI("for FwkInitUnit i:%lu/%d cur name:%s", i, node->num, cur->name);
        if (cur->level != level || UtilsIsBitSet(node->initMap, i)) {
            continue;
        }
        if (cur->initFunc != NULL) {
            ret = cur->initFunc();
            if (ret != IOTC_OK) {
                IOTC_LOGW("init error [%s/%s/%d/%d]", NON_NULL_STR(node->name),
                    NON_NULL_STR(cur->name), level, ret);
                return ret;
            }
            IOTC_LOGI("init ok [%s/%s/%d]", NON_NULL_STR(node->name), NON_NULL_STR(cur->name), level);
        }
        UtilsBitMapSet(node->initMap, i);
    }
    IOTC_LOGI("DoInitTargetLevelUnitNode end");
    return IOTC_OK;
}

static int32_t InitAllUnit(void)
{
    IOTC_LOGI("InitAllUnit start  -----");
    int32_t ret;
    for (int32_t level = FWK_INIT_LVL_MIN; level <= FWK_INIT_LVL_MAX; level++) {
        ListEntry *item = NULL;
        LIST_FOR_EACH_ITEM(item, &g_initList) {
            InitUnitNode *node = CONTAINER_OF(item, InitUnitNode, node);
            IOTC_LOGI("InitAllUnit LIST_FOR_EACH_ITEM loop InitUnitNode name:%s",node->name);
            ret = DoInitTargetLevelUnitNode(node, level);
            if (ret != IOTC_OK) {
                return ret;
            }
        }
    }
    return IOTC_OK;
}

int32_t FwkInitAll(FwkInitPolicy policy)
{
    int32_t ret;
    bool retry = false;
    do {
        if (retry) {
            IotcSleepMs(UTILS_SEC_TO_MS(FWK_INIT_BLOCK_SLEEP_TIME_SEC));
        }
        retry = true;
        ret = InitAllUnit();
    } while (ret != IOTC_OK && policy == FWK_INIT_POLICY_BLOCK);
    if (policy == FWK_INIT_FAILED_DEINIT_ALL) {
        FwkDeinitAll();
    }
    IOTC_LOGN("FwkInitAll end ret:%d",ret);
    return ret;
}

static void DoDeinitTargetLevelUnitNode(InitUnitNode *node, int32_t level)
{
    /* 初始化正序则去初始化逆序，保证依赖顺序 */
    for (int32_t i = node->num - 1; i >= 0; --i) {
        const FwkInitUnit *cur = &node->units[i];
        if (cur->level != level || !UtilsIsBitSet(node->initMap, i)) {
            continue;
        }
        UtilsBitMapReset(node->initMap, i);
        if (cur->deinitFunc != NULL) {
            cur->deinitFunc();
            IOTC_LOGI("deinit ok [%s/%s/%d]", NON_NULL_STR(node->name), NON_NULL_STR(cur->name), level);
        }
    }
}

void FwkDeinitAll(void)
{
    IOTC_LOGN("FwkDeinitAll start ");
    for (int32_t level = FWK_INIT_LVL_MAX; level >= FWK_INIT_LVL_MIN; level--) {
        ListEntry *item = NULL;
        LIST_FOR_EACH_ITEM(item, &g_initList) {
            InitUnitNode *node = CONTAINER_OF(item, InitUnitNode, node);
            if (node->num == 0) {
                continue;
            }
            DoDeinitTargetLevelUnitNode(node, level);
        }
    }
    IOTC_LOGN("FwkDeinitAll end ");
}

int32_t FwkRegInitUnits(const FwkInitUnit *units, uint32_t num, const char *mdlName)
{
    if (units == NULL || num == 0) {
        return IOTC_OK;
    }
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &g_initList) {
        InitUnitNode *node = CONTAINER_OF(item, InitUnitNode, node);
        if (node->units == units) {
            return IOTC_OK;
        }
    }

    InitUnitNode *node = (InitUnitNode *)IotcMalloc(sizeof(InitUnitNode));
    if (node == NULL) {
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(node, sizeof(InitUnitNode), 0, sizeof(InitUnitNode));

    node->initMap = UtilsCreateBitMap(num);
    if (node->initMap == NULL) {
        IotcFree(node);
        return IOTC_CORE_COMM_UTILS_ERR_BIT_MAP_CREATE;
    }
    node->name = mdlName;
    node->num = num;
    node->units = units;
    LIST_INSERT_BEFORE(&node->node, &g_initList);
    return IOTC_OK;
}

static void FreeInitUnitNode(InitUnitNode *node)
{
    UtilsFreeBitMap(node->initMap);
    IotcFree(node);
}

void FwkUnregInitUnit(const FwkInitUnit *units)
{
    if (units == NULL) {
        return;
    }

    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &g_initList) {
        InitUnitNode *node = CONTAINER_OF(item, InitUnitNode, node);
        if (node->units != units) {
            continue;
        }
        LIST_REMOVE(item);
        FreeInitUnitNode(node);
        return;
    }
    return;
}

void FwkUnregAll(void)
{
    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &g_initList) {
        InitUnitNode *node = CONTAINER_OF(item, InitUnitNode, node);
        LIST_REMOVE(item);
        FreeInitUnitNode(node);
    }
    return;
}