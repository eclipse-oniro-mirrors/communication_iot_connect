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
#include "sle_linklayer.h"
#include "ble_linklayer.h"
#include "iotc_errcode.h"
#include "iotc_event.h"
#include "iotc_svc_dev.h"
#include "iotc_svc.h"
#include "product_adapter.h"
#include "utils_list.h"
#include "sle_linklayer_encrypt_speke.h"

#define SLE_SESSION_NUM_LIMIT    64
typedef struct {
    SpekeSession *sleSpekeSess;
    uint32_t connSessionId;
    ListEntry node;
} SleSessionNode;


static ListEntry g_sleSpekeSessList = LIST_DECLARE_INIT(&g_sleSpekeSessList);
static int32_t g_sleSpekeErrCode = IOTC_OK;


static SpekeSession *GetSleSessionNode(uint8_t connSessionId)
{
    ListEntry *item;
    LIST_FOR_EACH_ITEM(item, &g_sleSpekeSessList) {
        SleSessionNode *spekeNode = CONTAINER_OF(item, SleSessionNode, node);
        if (spekeNode->connSessionId == connSessionId) {
            return spekeNode->sleSpekeSess;
        }
    }
    return NULL;
}

static void SleSessionNodeRelease(void)
{
    ListEntry *item;
    ListEntry *next;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &g_sleSpekeSessList) {
        SleSessionNode *spekeNode = CONTAINER_OF(item, SleSessionNode, node);
        LIST_REMOVE(&spekeNode->node);
        SpekeFreeSession(spekeNode->sleSpekeSess);
    }
}


static int32_t SleSessionNodeRegister(SpekeSession *sessNode, uint32_t connSessionId)
{
    CHECK_RETURN_LOGE((sessNode != NULL) && (connSessionId > 0) && (connSessionId <= SLE_SESSION_NUM_LIMIT),
        IOTC_ERR_PARAM_INVALID,
        "register sle param err, connSessionId:%u, limit:%d", connSessionId, SLE_SESSION_NUM_LIMIT);
    CHECK_RETURN_LOGE(connSessionId <= SLE_SESSION_NUM_LIMIT, IOTC_CORE_BLE_LL_ERR_SVC_NUM,
        "register sle key:%u over limit:%d", connSessionId, SLE_SESSION_NUM_LIMIT);

    if (GetSleSessionNode(connSessionId) != NULL) {
        IOTC_LOGE("sle session key:%u exist", connSessionId);
        return IOTC_OK;
    }

    SleSessionNode *sleSessionNode = (SleSessionNode *)IotcCalloc(1, sizeof(SleSessionNode));
    CHECK_RETURN_LOGE(sleSessionNode != NULL, IOTC_ADAPTER_MEM_ERR_CALLOC, "calloc sleSessionNode err");
    sleSessionNode->sleSpekeSess = sessNode;
    LIST_INSERT_BEFORE(&sleSessionNode->node, &g_sleSpekeSessList);

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
    NOT_USED(session);
    NOT_USED(user);
    g_sleSpekeErrCode = errorCode;
    IOTC_LOGN("speke errcode:%d", errorCode);
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