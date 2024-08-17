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
#include "iotc_oh_option.h"
#include "utils_list.h"
#include "utils_mutex_global.h"
#include "utils_assert.h"
#include "iotc_errcode.h"
#include "iotc_mem.h"
#include "securec.h"

typedef struct {
    ListEntry node;
    const OptionItem *items;
    uint32_t len;
} OptionNode;

static ListEntry g_optionList = LIST_DECLARE_INIT(&g_optionList);

static bool CheckIfOptionExist(ListEntry *list, const OptionItem *items)
{
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, list) {
        OptionNode *node = CONTAINER_OF(item, OptionNode, node);
        if (node->items == items) {
            return true;
        }
    }
    return false;
}

static OptionNode *OptionNodeNew(const OptionItem *items, uint32_t len)
{
    OptionNode *newNode = (OptionNode *)IotcMalloc(sizeof(OptionNode));
    if (newNode == NULL) {
        IOTC_LOGW("malloc error");
        return NULL;
    }
    (void)memset_s(newNode, sizeof(OptionNode), 0, sizeof(OptionNode));
    newNode->items = items;
    newNode->len = len;
    return newNode;
}

int32_t IotcOhOptionRegister(const OptionItem *items, uint32_t len)
{
    CHECK_RETURN_LOGW(items != NULL && len != 0, IOTC_ERR_PARAM_INVALID, "param invalid");

    (void)UtilsGlobalMutexLock();
    if (CheckIfOptionExist(&g_optionList, items)) {
        UtilsGlobalMutexUnlock();
        return IOTC_OK;
    }

    OptionNode *newNode = OptionNodeNew(items, len);
    if (newNode == NULL) {
        UtilsGlobalMutexUnlock();
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }

    LIST_INSERT_BEFORE(&newNode->node, &g_optionList);
    UtilsGlobalMutexUnlock();
    return IOTC_OK;
}

void IotcOhOptionUnregister(const OptionItem *items)
{
    CHECK_V_RETURN_LOGW(items != NULL, "param invalid");

    (void)UtilsGlobalMutexLock();
    if (!CheckIfOptionExist(&g_optionList, items)) {
        UtilsGlobalMutexUnlock();
        return;
    }

    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &g_optionList) {
        OptionNode *node = CONTAINER_OF(item, OptionNode, node);
        if (node->items != items) {
            continue;
        }
        LIST_REMOVE(item);
        IotcFree(node);
        break;
    }
    UtilsGlobalMutexUnlock();
    return;
}

static IotcOhOptionSetFunc GetOptionSetFunc(int32_t option, ListEntry *list)
{
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, list) {
        OptionNode *node = CONTAINER_OF(item, OptionNode, node);
        if (node->items == NULL || node->len == 0) {
            continue;
        }
        for (uint32_t i = 0; i < node->len; ++i) {
            if (node->items[i].option != option) {
                continue;
            }
            return node->items[i].setFunc;
        }
    }
    return NULL;
}

int32_t IotcOhSetOptionInner(int32_t option, va_list args)
{
    (void)UtilsGlobalMutexLock();
    IotcOhOptionSetFunc setFunc = GetOptionSetFunc(option, &g_optionList);
    UtilsGlobalMutexUnlock();

    if (setFunc == NULL) {
        IOTC_LOGW("invalid option %d", option);
        return IOTC_ERR_PARAM_INVALID;
    }

    int32_t ret = setFunc(args);
    if (ret != IOTC_OK) {
        IOTC_LOGW("set option %d error %d", option, ret);
    }

    return ret;
}
