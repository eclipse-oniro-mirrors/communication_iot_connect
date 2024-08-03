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
#include "ble_gatt_event.h"
#include "ble_gatt_mgt.h"
#include "adapter_ble.h"
#include "ble_sched_event.h"
#include "sched_msg_queue.h"
#include "securec.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "adapter_mem.h"
#include "utils_assert.h"
#include "utils_common.h"

typedef struct {
    AdapterBleGattEvent gattEvent;
    BleScheduleEvent scheduleEvent;
    void (*msgFree)(void *msg);
} GattEventToScheduleEvent;

static void MsgFree(void *msg);
static void ReqWriteMsgFree(void *msg);

static const GattEventToScheduleEvent EVENT_COVERT_MAP[] = {
    {
        .gattEvent = ADAPTER_BLE_GATT_EVENT_CONNECT,
        .scheduleEvent = BLE_EVENT_CONNECT,
        .msgFree = MsgFree
    },
    {
        .gattEvent = ADAPTER_BLE_GATT_EVENT_DISCONNECT,
        .scheduleEvent = BLE_EVENT_DISCONNECT,
        .msgFree = MsgFree
    },
    {
        .gattEvent = ADAPTER_BLE_GATT_EVENT_START_SVC_RESULT,
        .scheduleEvent = BLE_EVENT_START_SVC_RESULT,
        .msgFree = MsgFree
    },
    {
        .gattEvent = ADAPTER_BLE_GATT_EVENT_STOP_SVC_RESULT,
        .scheduleEvent = BLE_EVENT_STOP_SVC_RESULT,
        .msgFree = MsgFree
    },
    {
        .gattEvent = ADAPTER_BLE_GATT_EVENT_INDICATE_CONF,
        .scheduleEvent = BLE_EVENT_INDICATE_CONF,
        .msgFree = MsgFree
    },
    {
        .gattEvent = ADAPTER_BLE_GATT_EVENT_SET_MTU_RESULT,
        .scheduleEvent = BLE_EVENT_SET_MTU_RESULT,
        .msgFree = MsgFree
    },
    {
        .gattEvent = ADAPTER_BLE_GATT_EVENT_START_ADV_RESULT,
        .scheduleEvent = BLE_EVENT_START_ADV_RESULT,
        .msgFree = MsgFree
    },
    {
        .gattEvent = ADAPTER_BLE_GATT_EVENT_STOP_ADV_RESULT,
        .scheduleEvent = BLE_EVENT_STOP_ADV_RESULT,
        .msgFree = MsgFree
    },
    {
        .gattEvent = ADAPTER_BLE_GATT_EVENT_REQ_READ,
        .scheduleEvent = BLE_EVENT_REQ_READ,
        .msgFree = MsgFree
    },
    {
        .gattEvent = ADAPTER_BLE_GATT_EVENT_REQ_WRITE,
        .scheduleEvent = BLE_EVENT_REQ_WRITE,
        .msgFree = ReqWriteMsgFree
    },
};

static void MsgFree(void *msg)
{
    /* AdapterFree may be macro when debug */
    AdapterFree(msg);
}

static void ReqWriteMsgFree(void *msg)
{
    AdapterBleGattEventParam *eventParam = (AdapterBleGattEventParam *)msg;
    AdapterFree(eventParam->reqWrite.value);
    eventParam->reqWrite.value = NULL;
    AdapterFree(msg);
}

static int32_t BleGattEventHandler(AdapterBleGattEvent gattEvent, const AdapterBleGattEventParam *param)
{
    CHECK_RETURN_LOGW(param != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");
    for (uint32_t i = 0; i < ARRAY_SIZE(EVENT_COVERT_MAP); i++) {
        if (gattEvent != EVENT_COVERT_MAP[i].gattEvent) {
            continue;
        }

        AdapterBleGattEventParam *eventParam =
            (AdapterBleGattEventParam *)UtilsMallocCopy((const uint8_t *)param, sizeof(AdapterBleGattEventParam));
        if (eventParam == NULL) {
            IOTC_LOGW("malloc error");
            return IOTC_ADAPTER_MEM_ERR_MALLOC;
        }

        BleSchedMsg msg;
        msg.event = EVENT_COVERT_MAP[i].scheduleEvent;
        msg.param = eventParam;
        msg.free = EVENT_COVERT_MAP[i].msgFree;
        int32_t ret = BleSchedMsgQueueSend(&msg, 0);
        if (ret != IOTC_OK) {
            AdapterFree(eventParam);
            IOTC_LOGW("send msg error %d", ret);
            return ret;
        }
        return IOTC_OK;
    }
    IOTC_LOGW("invalid event %d", gattEvent);
    return IOTC_CORE_BLE_INVALID_GATT_EVENT;
}

static void BleSendIndicateDataFree(void *param)
{
    CHECK_V_RETURN(param != NULL);
    BleIndicateParam *indParam = (BleIndicateParam *)param;
    if (indParam->value != NULL) {
        AdapterFree(indParam->value);
        indParam->value = NULL;
    }
    AdapterFree(indParam);
}

int32_t IotcBleSendIndicateData(const char *svcUuid, const char *charUuid,
    const uint8_t *value, uint32_t valueLen)
{
    CHECK_RETURN_LOGW((svcUuid != NULL) && (charUuid != NULL) &&  (value != NULL) && (valueLen != 0),
        IOTC_ERR_PARAM_INVALID, "invalid param");

    BleIndicateParam *param = AdapterCalloc(1, sizeof(BleIndicateParam));
    if (param == NULL) {
        IOTC_LOGE("malloc err");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    param->value = UtilsMallocCopy(value, valueLen);
    if (param->value == NULL) {
        AdapterFree(param);
        IOTC_LOGE("malloc err");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    param->valueLen = valueLen;
    param->svcUuid = svcUuid;
    param->charUuid = charUuid;

    BleSchedMsg msg;
    msg.event = BLE_EVENT_SEND_INDICATE;
    msg.param = param;
    msg.free = BleSendIndicateDataFree;
    int32_t ret = BleSchedMsgQueueSend(&msg, 0);
    if (ret != IOTC_OK) {
        AdapterFree(param->value);
        AdapterFree(param);
        IOTC_LOGE("send msg err ret=%d", ret);
        return ret;
    }
    IOTC_LOGN("send indicate msg success valueLen=%u", param->valueLen);
    return IOTC_OK;
}

int32_t BleGattEventInit(void)
{
    return AdapterBleRegisterGattCb(BleGattEventHandler);
}