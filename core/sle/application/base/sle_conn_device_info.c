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

#include "sle_conn_device_info.h"
#include "securec.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "iotc_mem.h"
#include "utils_assert.h"
#include "utils_mutex_global.h"
#include "utils_common.h"
#include "security_speke.h"
#include "sle_svc_device_info.h"
#include "iotc_json.h"

static uint32_t g_sleConnDevListNum = 0;
static ListEntry g_sleConnDevList = LIST_DECLARE_INIT(&g_sleConnDevList);
static uint32_t g_sleDeviceInfoNum = 0;
static ListEntry g_sleDeviceInfoList = LIST_DECLARE_INIT(&g_sleDeviceInfoList);
typedef struct {
    SleConnDeviceInfo info;
    ListEntry node;
} SlDeviceInfoNode;


static ListEntry *GetSleConnDevListHead(void)
{
    return &g_sleConnDevList;
}

int32_t SleConnDevMgt(const SleConnRetDeviceInfo *retDev)
{
    int32_t ret = IOTC_ERROR;
    if (retDev->status != IOTC_CONN_SLE_STATE_CONNECTED && retDev->status != IOTC_CONN_SLE_STATE_DISCONNECTED) {
        IOTC_LOGE("conn status:[%u]", retDev->status);
        return IOTC_ERR_PARAM_INVALID;
    }

    if (retDev->status == IOTC_CONN_SLE_STATE_DISCONNECTED) {
        SleDeleteConnDev(retDev->connID);
        IOTC_LOGI("delete connID:[%u]", retDev->connID);
        return IOTC_OK;
    }

    ret = SleAddConnDev(retDev);
    if (ret != IOTC_OK) {
        return ret;
    }

    return IOTC_OK;
}

bool IsExitSleConnDev(const uint16_t connID)
{
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &g_sleConnDevList) {
        SleConnDevList *node = CONTAINER_OF(item, SleConnDevList, list);
        if (node->info.connID == connID) {
            return true;
        }
    }
    return false;
}

int32_t SleAddConnDev(const SleConnRetDeviceInfo *retDev)
{
    CHECK_RETURN_LOGW(retDev != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");

    if (IsExitSleConnDev(retDev->connID)) {
        IOTC_LOGI("connID is exited:%u", retDev->connID);
        return IOTC_OK;
    }

    SleConnDevList *newNode = (SleConnDevList *)IotcMalloc(sizeof(SleConnDevList));
    if (newNode == NULL) {
        IOTC_LOGW("malloc error");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(newNode, sizeof(SleConnDevList), 0, sizeof(SleConnDevList));
    newNode->info.connID = retDev->connID;
    memcpy_s(newNode->info.devAddr, IOTC_ADPT_SLE_ADDR_LEN, retDev->devAddr, sizeof(retDev->devAddr));

    LIST_INIT(&newNode->list);
    (void)UtilsGlobalMutexLock();
    LIST_INSERT_BEFORE(&newNode->list, GetSleConnDevListHead());
    g_sleConnDevListNum++;
    UtilsGlobalMutexUnlock();
    return IOTC_OK;
}

SleDeviceInfo *SleGetSleConnRetDeviceInfo(uint32_t connId)
{
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &g_sleConnDevList) {
        SleConnDevList *list = CONTAINER_OF(item, SleConnDevList, list);
        if (list->info.connID == connId) {
            return &list->info;
        }
    }
    return NULL;
}

SleConnDeviceInfo *SleFindRetDeviceInfoNode(const char *devId)
{
    ListEntry *item;
    LIST_FOR_EACH_ITEM(item, &g_sleDeviceInfoList) {
        SlDeviceInfoNode *node = CONTAINER_OF(item, SlDeviceInfoNode, node);
        if(node == NULL) {
            IOTC_LOGE("invalid param");
            return NULL;
        }

        if(memcmp(node->info.devId, devId, strlen(devId)) == 0) {
            return &node->info;
        }
    }
    return NULL;
}

int32_t SleAddDeviceInfoNode(SleConnDeviceInfo *info)
{
    CHECK_RETURN_LOGW(info != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");

    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &g_sleDeviceInfoList) {
        SlDeviceInfoNode *node = CONTAINER_OF(item, SlDeviceInfoNode, node);
        if (memcmp(node->info.devId, info->devId, DEVICE_ID_MAX_STR_LEN + 1) == 0) {
            return true;
        }
    }

    SlDeviceInfoNode *newNode = (SlDeviceInfoNode *)IotcMalloc(sizeof(SlDeviceInfoNode));
    if (newNode == NULL) {
        IOTC_LOGW("malloc error");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(newNode, sizeof(SlDeviceInfoNode), 0, sizeof(SlDeviceInfoNode));
    memcpy_s(&(newNode->info), sizeof(SleConnDeviceInfo), info, sizeof(SleConnDeviceInfo));

    (void)UtilsGlobalMutexLock();
    LIST_INSERT(&newNode->node, &g_sleDeviceInfoList);
    g_sleDeviceInfoNum++;
    UtilsGlobalMutexUnlock();
    return IOTC_OK;
}

void SleDeleteConnDev(uint16_t connID)
{
    (void)UtilsGlobalMutexLock();
    ListEntry *item;
    ListEntry *next;
    LIST_FOR_EACH_ITEM_SAFE(item, next, GetSleConnDevListHead()) {
        SleConnDevList *node = CONTAINER_OF(item, SleConnDevList, list);
        if (node->info.connID == connID) {
            LIST_REMOVE(&node->list);
            IotcFree(node);
            g_sleConnDevListNum--;
            IOTC_LOGI("delete connID:[%u], devAddr[%s]", node->info.connID, node->info.devAddr);
            break;
        }
    }
    (void)UtilsGlobalMutexLock();
}

bool SleFindConnDevByDevId(const char *devID, uint16_t *connID)
{
    ListEntry *item;
    ListEntry *next;
    LIST_FOR_EACH_ITEM_SAFE(item, next, GetSleConnDevListHead()) {
        SleConnDevList *node = CONTAINER_OF(item, SleConnDevList, list);
        if (memcmp(node->info.devId, devID, strlen(devID)) == 0) {
            IOTC_LOGI("find connID:[%u], devID[%s]", node->info.connID, node->info.devId);
            *connID = node->info.connID;
            return true;
        }
    }
    return false;
}

void DestroySleConnDevList(void)
{
    (void)UtilsGlobalMutexLock();
    ListEntry *item;
    ListEntry *next;
    LIST_FOR_EACH_ITEM_SAFE(item, next, GetSleConnDevListHead()) {
        SleConnDevList *node = CONTAINER_OF(item, SleConnDevList, list);
        LIST_REMOVE(&node->list);
        IotcFree(node);
    }
    g_sleConnDevListNum = 0;
    UtilsGlobalMutexUnlock();
}

void PrintSleConnDevList(void)
{
    (void)UtilsGlobalMutexLock();
    ListEntry *item;
    LIST_FOR_EACH_ITEM(item, GetSleConnDevListHead()) {
        SleConnDevList *node = CONTAINER_OF(item, SleConnDevList, list);
        IOTC_LOGI("connID:[%u], devAddr[%s]", node->info.connID, node->info.devAddr);
    }
}


void IotcOhSendCustomSecData(const char *devId, const char *service, const uint8_t *data, uint32_t len)
{
    uint16_t *connId = (uint16_t *)malloc(sizeof(uint16_t));
    if (connId == NULL) {
        return;
    }

    if (!SleFindConnDevByDevId(devId, connId)) {
        IOTC_LOGI("not find connID, devID[%s]", devId);
        free(connId);
        return;
    }
    SleLinkLayerReportSvcDataEnc(*connId, service, data, len, SLE_OPTYPE_GET);
    free(connId);
}
