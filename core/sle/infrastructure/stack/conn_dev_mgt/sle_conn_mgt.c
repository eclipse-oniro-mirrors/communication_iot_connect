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

#include "sle_conn_mgt.h"
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

static void ReleaseStrings(uint8_t **strs)
{
    if (strs == NULL) {
        return;
    }
    int i = 0;
    while (strs[i] != NULL) {
        free(strs[i]);
        strs[i] = NULL;
        ++i;
    }
    free(strs);
    strs = NULL;
    return;
}

static ListEntry *GetSleConnDevListHead(void)
{
    return &g_sleConnDevList;
}

int32_t SleConnDevMgt(const SleConnRetDeviceInfo *retDev)
{
    int32_t ret = IOTC_ERROR;
    if(retDev->status != IOTC_CONN_SLE_STATE_CONNECTED && retDev->status != IOTC_CONN_SLE_STATE_DISCONNECTED) {
        IOTC_LOGE("conn status:[%u]", retDev->status);
        return IOTC_ERR_PARAM_INVALID;
    }
    
    if(retDev->status == IOTC_CONN_SLE_STATE_DISCONNECTED) {
        SleDeleteConnDev(retDev->connID);
        IOTC_LOGI("delete connID:[%u]", retDev->connID);
        return IOTC_OK;
    }

    ret = SleAddConnDev(retDev);
    if(ret != IOTC_OK) {
        return ret;
    }
    
    return IOTC_OK;
}

bool IsExitSleConnDev(const uint16_t connID)
{
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &g_sleConnDevList) {
        SleConnDevList *node = CONTAINER_OF(item, SleConnDevList, list);
        if(node->info.connID == connID) {
            return true;
        }
    }
    return false;
}

int32_t SleAddConnDev(const SleConnRetDeviceInfo *retDev)
{
    CHECK_RETURN_LOGW(retDev != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");

    if(true == IsExitSleConnDev(retDev->connID)) {
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

void SleDeleteConnDev(uint16_t connID)
{
    (void)UtilsGlobalMutexLock();
    ListEntry *item;
    ListEntry*next;
    LIST_FOR_EACH_ITEM_SAFE(item, next, GetSleConnDevListHead()) {
        SleConnDevList *node = CONTAINER_OF(item, SleConnDevList, list);
        if(node->info.connID == connID) {
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
    ListEntry*next;
    LIST_FOR_EACH_ITEM_SAFE(item, next, GetSleConnDevListHead()) {
        SleConnDevList *node = CONTAINER_OF(item, SleConnDevList, list);
        if(memcmp(node->info.devId, devID, strlen(devID)) == 0) {
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

void PrintSleConnDevList()
{
    (void) UtilsGlobalMutexLock();
    ListEntry *item;
    LIST_FOR_EACH_ITEM(item, GetSleConnDevListHead()) {
        SleConnDevList *node = CONTAINER_OF(item, SleConnDevList, list);
        IOTC_LOGI("connID:[%u], devAddr[%s]", node->info.connID, node->info.devAddr);
    }
}

void IotcOhSleFindDeviceInfo(const char *devId, SleConnDeviceInfo *deviceInfo)
{
    if(deviceInfo == NULL) {
        return;
    }

    uint32_t len = 0;
    uint8_t **msg = (uint8_t **)IotcCalloc(1, sizeof(uint8_t *));
    if(msg == NULL) {
        return;
    }

    GetSleSvcDeviceInfo(NULL, msg, &len);
    IotcJson *root = IotcJsonParse((char *)*msg);

    IotcJson *vendorJson = IotcJsonGetObj(root, STR_JSON_VENDOR);
    if(vendorJson == NULL) {
        return;
    }

    IotcJson *devInfoJson = IotcJsonGetObj(vendorJson, STR_JSON_DEV_INFO);
    if(devInfoJson == NULL) {
        return;
    }

    IotcJson *snJson = IotcJsonGetObj(devInfoJson, STR_JSON_SN);
    const char *sn = IotcJsonGetStr(snJson);
    if(sn != NULL) {
        memcpy_s(deviceInfo->sn, sizeof(deviceInfo->sn), sn, strlen(sn) + 1);
    }

    IotcJson *modelJson = IotcJsonGetObj(root, STR_JSON_MODEL);
    const char *model = IotcJsonGetStr(modelJson);
    if(model != NULL) {
        memcpy_s(deviceInfo->model, sizeof(deviceInfo->model), model, strlen(model) + 1);
    }

    IotcJson *devTypeJson = IotcJsonGetObj(root, STR_JSON_DEV_TYPE);
    const char *devType = IotcJsonGetStr(devTypeJson);
    if(devType != NULL) {
        memcpy_s(deviceInfo->devType, sizeof(deviceInfo->devType), devType, strlen(devType) + 1);
    }

    IotcJson *manuNameJson = IotcJsonGetObj(root, STR_JSON_MANU);
    const char *manuName = IotcJsonGetStr(manuNameJson);
    if(manuName != NULL) {
        memcpy_s(deviceInfo->manu, sizeof(deviceInfo->manu), manuName, strlen(manuName) + 1);
    }

    IotcJson *prodIdJson = IotcJsonGetObj(root, STR_JSON_PROD_ID);
    const char *prodId = IotcJsonGetStr(prodIdJson);
    if(prodId != NULL) {
        memcpy_s(deviceInfo->prodId, sizeof(deviceInfo->prodId), prodId, strlen(prodId) + 1);
    }

    ReleaseStrings(msg);
    return;
}

void IotcOhSendCustomSecData(const char *devId, const char *service, const uint8_t *data, uint32_t len)
{
    uint16_t *connId = (uint16_t *)malloc(sizeof(uint16_t));
    if(connId == NULL) {
        return;
    }

    if(!SleFindConnDevByDevId(devId, connId)) {
        IOTC_LOGI("not find connID, devID[%s]", devId);
        free(connId);
        return;
    }
    SleLinkLayerReportSvcDataEnc(*connId, service, data, len);
    free(connId);
}
