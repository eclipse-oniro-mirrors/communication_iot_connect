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

    IotcDeviceInfo *deviceInfo = (IotcDeviceInfo *)IotcMalloc(sizeof(IotcDeviceInfo));
    if (deviceInfo == NULL) {
        IOTC_LOGW("malloc error");
        IotcFree(newNode);
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    newNode->info.devInfo = deviceInfo;

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

IotcConDeviceInfo* SleGetConnectionInfoByConnId(uint16_t connId)
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

int32_t SleGetDevIdByConnId(uint16_t connId, char *devId)
{
    // 参数校验
    CHECK_RETURN_LOGW(devId != NULL, IOTC_ERR_PARAM_INVALID, "invalid param: devId is NULL");
    if (connId < 0 ) {
        IOTC_LOGE("invalid param: connId=%u", connId);
        return IOTC_ERR_PARAM_INVALID;
    }

    // 遍历列表
    ListEntry *item;
    LIST_FOR_EACH_ITEM(item, GetSleConnDevListHead()) {
        SleConnDevList *node = CONTAINER_OF(item, SleConnDevList, list);
        if (node == NULL) {
            IOTC_LOGE("invalid node: node is NULL in list, connId=%u", connId);
            return IOTC_ERR_PARAM_INVALID;
        }

        if (node->info.connID == connId) {
            uint32_t devIdLen = strlen(node->info.devId);
            if (devIdLen >= (DEVICE_ID_MAX_STR_LEN + 1) ) {
                IOTC_LOGE("buffer overflow: devId length exceeds max limit, connId=%u", connId);
                return IOTC_ERR_PARAM_INVALID;
            }

            if (memcpy_s(devId, DEVICE_ID_MAX_STR_LEN + 1, node->info.devId, devIdLen) != EOK) {
                IOTC_LOGE("memcpy_s error: failed to copy devId, connId=%u", connId);
                return IOTC_ERR_PARAM_INVALID;
            }
            return IOTC_OK;
        }
    }

    IOTC_LOGW("no matching connId found: connId=%u", connId);
    return IOTC_ERR_PARAM_INVALID;
}

IotcConDeviceInfo* SleGetConnectionInfoByDevId(const char *devId)
{
    if (devId == NULL || strlen(devId) == 0) {
        IOTC_LOGE("invalid devId");
        return NULL;
    }

    ListEntry *listHead = GetSleConnDevListHead();
    if (listHead == NULL) {
        IOTC_LOGE("connection device list is null");
        return NULL;
    }

    uint32_t devIdLen = strlen(devId);

    ListEntry *item;
    LIST_FOR_EACH_ITEM(item, listHead) {
        SleConnDevList *node = CONTAINER_OF(item, SleConnDevList, list);
        if (node == NULL || node->info.devId[0] == '\0') {
            IOTC_LOGE("invalid node or devId in list");
            continue;
        }

        if (memcmp(node->info.devId, devId, devIdLen) == 0 && strlen(node->info.devId) == devIdLen) {
            return &node->info;
        }
    }
    IOTC_LOGW("no matching devId found: devId=%s", devId);
    return NULL;
}


IotcDeviceInfo* SleGetDeviceInfoByDevId(const char *devId)
{

    if (devId == NULL) {
        IOTC_LOGE("devId is NULL");
        return NULL;
    }

    ListEntry *item;
    LIST_FOR_EACH_ITEM(item, GetSleConnDevListHead()) {
        SleConnDevList *node = CONTAINER_OF(item, SleConnDevList, list);
        if (node == NULL) {
            IOTC_LOGE("Unexpected: node is NULL");
            continue;
        }

        if (strcmp(node->info.devId, devId) == 0) {
            return node->info.devInfo;
        }
    }

    IOTC_LOGD("Device ID not found: %s", devId);
    return NULL;
}

void SleDeleteConnDev(uint16_t connID)
{
    (void)UtilsGlobalMutexLock();
    ListEntry *item = NULL;
    ListEntry *next = NULL;

    ListEntry *head = GetSleConnDevListHead();
    if (!head) {
        UtilsGlobalMutexUnlock();
        return;
    }

    LIST_FOR_EACH_ITEM_SAFE(item, next, head) {
        SleConnDevList *node = CONTAINER_OF(item, SleConnDevList, list);
        if (node->info.connID == connID) {
            LIST_REMOVE(&node->list);
            IotcFree(node);
            IOTC_LOGI("delete connID:[%u]", connID);
            break;
        }
    }
    g_sleConnDevListNum--;
    UtilsGlobalMutexUnlock();
}

bool SleFindConnDevByDevId(const char *devID, uint16_t *connID)
{
    if (devID == NULL || connID == NULL) {
        IOTC_LOGE("Invalid input: devID or connID is NULL");
        return false;
    }

    uint32_t devIDLen = strlen(devID);
    if (devIDLen == 0) {
        IOTC_LOGE("Invalid input: devID is empty");
        return false;
    }

    ListEntry *item;
    ListEntry *next;
    ListEntry *listHead = GetSleConnDevListHead();
    if (listHead == NULL) {
        IOTC_LOGW("Connection device list is empty or not initialized");
        return false;
    }

    LIST_FOR_EACH_ITEM_SAFE(item, next, listHead) {
        SleConnDevList *node = CONTAINER_OF(item, SleConnDevList, list);
        if(node == NULL)
        {
            IOTC_LOGE("Unexpected: node is NULL");
            continue;
        }
        if (node->info.devId[0] == '\0' || strlen(node->info.devId) < devIDLen) {
            continue;
        }

        if (memcmp(node->info.devId, devID, devIDLen) == 0) {
            IOTC_LOGI("Found connID:[%u]", node->info.connID);
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
        if (node == NULL || node->info.devId[0] == '\0') {
            IOTC_LOGW("Invalid node or devAddr in list");
            continue;
        }

        char devAddrStr[18];
        snprintf(devAddrStr, sizeof(devAddrStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 node->info.devAddr[0], node->info.devAddr[1], node->info.devAddr[2],
                 node->info.devAddr[3], node->info.devAddr[4], node->info.devAddr[5]);
        IOTC_LOGI("connID:[%u], devAddr[%s]", node->info.connID, devAddrStr);
    }
}


void IotcOhSendCustomSecData(const char *devId, const char *service, const uint8_t *data, uint32_t len)
{
    uint16_t connId = 0;

    if (devId == NULL || service == NULL || data == NULL || len == 0) {
        IOTC_LOGE("Invalid input parameters: devId=%p, service=%p, data=%p, len=%u", devId, service, data, len);
        return;
    }

    if (!SleFindConnDevByDevId(devId, &connId)) {
        IOTC_LOGI("not find connID, devID[%s]", devId);
        return;
    }

    int result = SleLinkLayerReportSvcDataEnc(connId, service, data, len, SLE_OPTYPE_GET);
    if (result != IOTC_OK) {
        IOTC_LOGE("Failed to report service data, connID=%u, service=%s, error=%d", connId, service, result);
    }

}
