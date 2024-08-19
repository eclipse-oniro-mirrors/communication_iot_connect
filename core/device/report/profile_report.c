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
#include "profile_report.h"
#include "utils_assert.h"
#include "utils_common.h"
#include "securec.h"
#include "utils_list.h"
#include "utils_mutex_local.h"
#include "iotc_errcode.h"

typedef struct {
    const char *name;
    UplinkReportFunc func;
} Uplink;

typedef struct {
    ListEntry node;
    const char *name;
    UplinkReportFunc func;
} UplinkNode;

static uint32_t g_uplinkNum = 0;

static ListEntry *GetUplinkList(void)
{
    static ListEntry uplinkList = LIST_DECLARE_INIT(&uplinkList);
    return &uplinkList;
}

static uint32_t GetUplinkNum(void)
{
    return g_uplinkNum;
}

static UtilsMutexLocal *GetMutexLocal(void)
{
    static UtilsMutexLocal mutex;
    return &mutex;
}

int32_t ProfileReportInit(void)
{
    int32_t ret = UtilsCreateMutexLocal(GetMutexLocal());
    if (ret != IOTC_OK) {
        IOTC_LOGW("create local mutex error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

void ProfileReportDeinit(void)
{
    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, GetUplinkList()) {
        UplinkNode *curNode = CONTAINER_OF(item, UplinkNode, node);
        LIST_REMOVE(item);
        IotcFree(curNode);
    }
    g_uplinkNum = 0;
    UtilsDestroyMutexLocal(GetMutexLocal());
}

int32_t ProfileReportRegUplink(UplinkReportFunc func, const char *name)
{
    CHECK_RETURN_LOGW(func != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    UplinkNode *newNode = (UplinkNode *)IotcMalloc(sizeof(UplinkNode));
    if (newNode == NULL) {
        IOTC_LOGW("malloc error");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(newNode, sizeof(UplinkNode), 0, sizeof(UplinkNode));

    newNode->func = func;
    newNode->name = name;

    if (!UtilsMutexLocalLock(GetMutexLocal())) {
        IOTC_LOGW("lock error");
        IotcFree(newNode);
        return IOTC_ERR_TIMEOUT;
    }

    LIST_INSERT_BEFORE(&newNode->node, GetUplinkList());
    ++g_uplinkNum;
    UtilsMutexLocalUnlock(GetMutexLocal());

    return IOTC_OK;
}

void ProfileReportUnregUplink(const UplinkReportFunc uplink)
{
    CHECK_V_RETURN_LOGW(uplink != NULL, "param invalid");

    (void)UtilsMutexLocalLock(GetMutexLocal());
    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, GetUplinkList()) {
        UplinkNode *curNode = CONTAINER_OF(item, UplinkNode, node);
        if (curNode->func != uplink) {
            continue;
        }
        LIST_REMOVE(item);
        IotcFree(curNode);
        --g_uplinkNum;
        break;
    }
    UtilsMutexLocalUnlock(GetMutexLocal());
}

int32_t ProfileReportCharState(const IotcCharState state[], uint32_t num)
{
    CHECK_RETURN_LOGW(state != NULL && num != 0, IOTC_ERR_PARAM_INVALID, "param invalid");

    if (!UtilsMutexLocalLock(GetMutexLocal())) {
        IOTC_LOGW("lock error");
        return IOTC_ERR_TIMEOUT;
    }

    int32_t ret;
    Uplink *uplinks = NULL;
    uint32_t index = 0;
    do {
        if (GetUplinkNum() == 0) {
            IOTC_LOGW("no uplink reg");
            ret = IOTC_ERR_NOT_INIT;
            break;
        }

        uplinks = (Uplink *)IotcCalloc(GetUplinkNum(), sizeof(Uplink));
        if (uplinks == NULL) {
            IOTC_LOGW("calloc error %u", GetUplinkNum());
            ret = IOTC_ADAPTER_MEM_ERR_CALLOC;
            break;
        }

        ListEntry *item = NULL;
        LIST_FOR_EACH_ITEM(item, GetUplinkList()) {
            UplinkNode *curNode = CONTAINER_OF(item, UplinkNode, node);
            uplinks[index].func = curNode->func;
            uplinks[index++].name = curNode->name;
        }
    } while (0);
    UtilsMutexLocalUnlock(GetMutexLocal());
    if (uplinks == NULL) {
        return ret;
    }

    int32_t err;
    ret = IOTC_OK;
    for (uint32_t i = 0; i < index; ++i) {
        if (uplinks[i].func != NULL) {
            err = uplinks[i].func(state, num);
            if (err != IOTC_OK) {
                IOTC_LOGW("uplink %s report error %d", NON_NULL_STR(uplinks[i].name), ret);
                ret = err;
            } else {
                IOTC_LOGI("uplink %s report success", NON_NULL_STR(uplinks[i].name));
            }
        }
    }
    IotcFree(uplinks);
    return ret;
}