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
#include "sle_speke_session.h"
#include "utils_common.h"
#include "utils_assert.h"
#include "security_speke.h"
#include "event_bus_sub.h"
#include "event_bus_pub.h"
#include "sle_linklayer.h"
#include "ble_linklayer.h"
#include "iotc_errcode.h"
#include "iotc_event.h"
#include "iotc_svc_dev.h"
#include "iotc_svc.h"
#include "product_adapter.h"
#include "utils_list.h"
#include "sle_linklayer_encrypt_speke.h"
#include "utils_mutex_global.h"
#include "sle_comm_status.h"

#define SLE_SESSION_NUM_LIMIT    64
typedef struct {
    SpekeSession *sleSpekeSess;
    uint16_t connSessionId;
    uint8_t use; // 0: not used, 1: used
    ListEntry node;
} SleSessionNode;


static ListEntry g_sleSpekeSessList = LIST_DECLARE_INIT(&g_sleSpekeSessList);
static int32_t g_sleSpekeErrCode = IOTC_OK;

static int32_t GetSleSessionConnId(SpekeSession *sleSpekeSess, uint16_t *connId)
{
    ListEntry *item;

    if (UtilsGlobalMutexLock() != IOTC_OK) {
        IOTC_LOGE("Failed to acquire mutex lock");
        return IOTC_ERROR;
    }

    bool found = false;
    LIST_FOR_EACH_ITEM(item, &g_sleSpekeSessList) {
        SleSessionNode *spekeNode = CONTAINER_OF(item, SleSessionNode, node);
        if (spekeNode->sleSpekeSess == sleSpekeSess) {
            *connId = spekeNode->connSessionId;
            found = true;
            break;
        }
    }

    UtilsGlobalMutexUnlock();

    if (found) {
        return IOTC_OK;
    } else {
        IOTC_LOGE("No matching session found in the list");
        return IOTC_ERROR;
    }
}

static SpekeSession *GetSleSessionNode(uint8_t connSessionId)
{
    ListEntry *item;
    (void)UtilsGlobalMutexLock();
    LIST_FOR_EACH_ITEM(item, &g_sleSpekeSessList) {
        SleSessionNode *spekeNode = CONTAINER_OF(item, SleSessionNode, node);
        if(spekeNode == NULL)
        {
            IOTC_LOGE("spekeNode is null");
            return NULL;
        }

        if (spekeNode->connSessionId == connSessionId) {
            spekeNode->use = 1;
            UtilsGlobalMutexUnlock();
            return spekeNode->sleSpekeSess;
        }
    }
    UtilsGlobalMutexUnlock();
    return NULL;
}

static void SleSessionNodeRelease(void)
{
    ListEntry *item;
    ListEntry *next;
    (void)UtilsGlobalMutexLock();
    LIST_FOR_EACH_ITEM_SAFE(item, next, &g_sleSpekeSessList) {
        SleSessionNode *spekeNode = CONTAINER_OF(item, SleSessionNode, node);
        if(spekeNode == NULL)
        {
            IOTC_LOGE("spekeNode is null");
            return;
        }
        LIST_REMOVE(&spekeNode->node);
        SpekeFreeSession(spekeNode->sleSpekeSess);
    }
    UtilsGlobalMutexUnlock();
}


static int32_t SleSessionNodeRegister(SpekeSession *sessNode, uint32_t connSessionId)
{
    IOTC_LOGI("register sle session session id:%x", sessNode);
    if (sessNode == NULL) {
        IOTC_LOGE("create speke session is null");
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_CREATE;
    }
    if (connSessionId >= SLE_SESSION_NUM_LIMIT) {
        IOTC_LOGE("sle session key:%u out of range", connSessionId);
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_CREATE;
    }

    if (GetSleSessionNode(connSessionId) != NULL) {
        IOTC_LOGE("sle session key:%u exist", connSessionId);
        return IOTC_OK;
    }

    SleSessionNode *sleSessionNode = (SleSessionNode *)IotcCalloc(1, sizeof(SleSessionNode));
    CHECK_RETURN_LOGE(sleSessionNode != NULL, IOTC_ADAPTER_MEM_ERR_CALLOC, "calloc sleSessionNode err");
    sleSessionNode->sleSpekeSess = sessNode;
    (void)UtilsGlobalMutexLock();
    LIST_INSERT_BEFORE(&sleSessionNode->node, &g_sleSpekeSessList);
    UtilsGlobalMutexUnlock();

    return IOTC_OK;
}


static int32_t GetPinCode(SpekeSession *session, void *user, uint8_t *pinCode, uint32_t *len)
{
    NOT_USED(session);
    NOT_USED(user);

    *len = IOTC_PINCODE_LEN;
    return ProductProfGetPincode(pinCode, IOTC_PINCODE_LEN);
}

static int32_t NotifySpekeFinished(SpekeSession *session, void *user, int32_t errorCode)
{
    NOT_USED(user);
    g_sleSpekeErrCode = errorCode;
    IOTC_LOGN("speke errcode:%d", errorCode);

    uint16_t connSessionId = 0;
    if(GetSleSessionConnId(session, &connSessionId)!= IOTC_OK)
    {
        IOTC_LOGE("get sle session connId failed");
        return IOTC_ERROR;
    }
    NotifyFinishedStatus status = {0};
    status.errorCode = errorCode;
    status.connSessionId = connSessionId;

    EventBusPublishSync(IOTC_CORE_SLE_EVENT_SPEKE_FINISHED, (void *)&status, sizeof(NotifyFinishedStatus));
    return IOTC_OK;
}

SpekeSession *GetSleSpekeSess(uint32_t connSessionId)
{
    return GetSleSessionNode(connSessionId);
}

static void SleSpekeSessClear(uint32_t event, void *param, uint32_t len)
{
    NOT_USED(event);
    NOT_USED(param);
    NOT_USED(len);
    DestroySleSpekeSess();
}

int32_t CreateSleSpekeSess(SpekeType type, uint32_t connId)
{
    if (GetSleSessionNode(connId) != NULL) {
        return IOTC_OK;
    }

    int32_t ret = EventBusSubscribe(SleSpekeSessClear, IOTC_CORE_SLE_EVENT_SSAP_DISCONNECT);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "subscribe ssap disconn err:%d", ret);
    ret = EventBusSubscribe(SleSpekeSessClear, IOTC_CORE_COMM_EVENT_MAIN_RESET);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "subscribe sdk reset err:%d", ret);
    ret = EventBusSubscribe(SleSpekeSessClear, IOTC_CORE_COMM_EVENT_MAIN_QUIT);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "subscribe sdk quit err:%d", ret);

    SpekeCallback cb = {
        .getPinCode = GetPinCode,
        .notifySpekeFinished = NotifySpekeFinished,
    };
    SpekeSession *sessNode = SpekeInitSession(type, &cb, NULL);
    if (sessNode == NULL) {
        IOTC_LOGE("create speke session");
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_CREATE;
    }

    if (IOTC_OK != SleSessionNodeRegister(sessNode, connId)) {
        IOTC_LOGE("create register speke session");
        SpekeFreeSession(sessNode);
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_CREATE;
    }

    return SleLinkLayerRegisterSpekeSessionGetCb(GetSleSpekeSess);
}

void DestroySleSpekeSess(void)
{
    SleSessionNodeRelease();
}

int32_t GetSleSpekeErrCode(void)
{
    return g_sleSpekeErrCode;
}