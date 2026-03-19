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
#include "sle_ssapc_event.h"
#include "iotc_sle_client.h"
#include "sle_sched_event.h"
#include "sched_msg_queue.h"
#include "securec.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "iotc_mem.h"
#include "utils_assert.h"
#include "utils_common.h"

typedef struct {
    IotcAdptSleSsapClientEvent connectionEvent;
    SleScheduleEvent scheduleEvent;
    void (*msgFree)(void *msg);
} SsapClientEventToScheduleEvent;

static void MsgFree(void *msg)
{
    IotcFree(msg);
}

static void FindPropertyMsgFree(void *msg)
{
    if (msg == NULL) {
        IOTC_LOGW("invalid param");
        return;
    }
    IotcAdptSleSsapClientEventParam *param = (IotcAdptSleSsapClientEventParam *)msg;
    if (param->ssapcFindPropertyResult.property.descriptorsType != NULL) {
        IotcFree(param->ssapcFindPropertyResult.property.descriptorsType);
        param->ssapcFindPropertyResult.property.descriptorsType = NULL;
    }
    IotcFree(msg);
}

static void ReadCfmMsgFree(void *msg)
{
    if (msg == NULL) {
        IOTC_LOGW("invalid param");
        return;
    }
    IotcAdptSleSsapClientEventParam *param = (IotcAdptSleSsapClientEventParam *)msg;
    if (param->ssapcHandleValue.readData.data != NULL) {
        IotcFree(param->ssapcHandleValue.readData.data);
        param->ssapcHandleValue.readData.data = NULL;
    }
    IotcFree(msg);
}

static void NotificationMsgFree(void *msg)
{
    if (msg == NULL) {
        IOTC_LOGW("invalid param");
        return;
    }
    IotcAdptSleSsapClientEventParam *param = (IotcAdptSleSsapClientEventParam *)msg;
    if (param->ssapcNotification.data.data != NULL) {
        IotcFree(param->ssapcNotification.data.data);
        param->ssapcNotification.data.data = NULL;
    }
    IotcFree(msg);
}

static void IndicationFree(void *msg)
{
    if (msg == NULL) {
        IOTC_LOGW("invalid param");
        return;
    }
    IotcAdptSleSsapClientEventParam *param = (IotcAdptSleSsapClientEventParam *)msg;
    if (param->ssapcIndication.data.data != NULL) {
        IotcFree(param->ssapcIndication.data.data);
        param->ssapcIndication.data.data = NULL;
    }
    IotcFree(msg);
}

static const SsapClientEventToScheduleEvent EVENT_COVERT_MAP[] = {
    {
        .connectionEvent = IOTC_ADPT_SLE_SSAPC_FIND_STRUCTURE_EVENT,
        .scheduleEvent = SLE_EVENT_SSAPC_FIND_STRUCTURE,
        .msgFree = MsgFree
    },
    {
        .connectionEvent = IOTC_ADPT_SLE_SSAPC_FIND_PROPERTY_EVENT,
        .scheduleEvent = SLE_EVENT_SSAPC_FIND_PROPERTY,
        .msgFree = FindPropertyMsgFree
    },
    {
        .connectionEvent = IOTC_ADPT_SLE_SSAPC_FIND_STRUCTURE_COMPLETE_EVENT,
        .scheduleEvent = SLE_EVENT_SSAPC_FIND_STRUCTURE_COMPLETE,
        .msgFree = MsgFree
    },
    {
        .connectionEvent = IOTC_ADPT_SLE_SSAPC_READ_CFM_EVENT,
        .scheduleEvent = SLE_EVENT_SSAPC_READ_CFM,
        .msgFree = ReadCfmMsgFree
    },
    {
        .connectionEvent = IOTC_ADPT_SSAPC_READ_BY_UUID_COMPLETE_EVENT,
        .scheduleEvent = SLE_EVENT_SSAPC_READ_BY_UUID_COMPLETE,
        .msgFree = MsgFree
    },
    {
        .connectionEvent = IOTC_ADPT_SSAPC_WRITE_CFM_EVENT,
        .scheduleEvent = SLE_EVENT_SSAPC_WRITE_CFM,
        .msgFree = MsgFree
    },
    {
        .connectionEvent = IOTC_ADPT_SSAPC_EXCHANGE_INFO_EVENT,
        .scheduleEvent = SLE_EVENT_SSAPC_EXCHANGE_INFO,
        .msgFree = MsgFree
    },
    {
        .connectionEvent = IOTC_ADPT_SSAPC_NOTIFICATION_EVENT,
        .scheduleEvent = SLE_EVENT_SSAPC_NOTIFICATION,
        .msgFree = NotificationMsgFree
    },
    {
        .connectionEvent = IOTC_ADPT_SSAPC_INDICATION_EVENT,
        .scheduleEvent = SLE_EVENT_SSAPC_INDICATION,
        .msgFree = IndicationFree
    },
};

static int32_t SleSsapClientEventHandler(IotcAdptSleSsapClientEvent discEvent,
    const IotcAdptSleSsapClientEventParam *param)
{
    CHECK_RETURN_LOGW(param != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");
    for (uint32_t i = 0; i < ARRAY_SIZE(EVENT_COVERT_MAP); i++) {
        if (discEvent != EVENT_COVERT_MAP[i].connectionEvent) {
            continue;
        }

        IotcAdptSleSsapClientEventParam *eventParam = (IotcAdptSleSsapClientEventParam *)
            UtilsMallocCopy((const uint8_t *)param,
            sizeof(IotcAdptSleSsapClientEventParam));
        if (eventParam == NULL) {
            IOTC_LOGW("malloc error");
            return IOTC_ADAPTER_MEM_ERR_MALLOC;
        }

        SleSchedMsg msg;
        msg.event = EVENT_COVERT_MAP[i].scheduleEvent;
        msg.param = eventParam;
        msg.free = EVENT_COVERT_MAP[i].msgFree;
        int32_t ret = SleSchedMsgQueueSend(&msg, 0);
        if (ret != IOTC_OK) {
            IotcFree(eventParam);
            IOTC_LOGW("send msg error %d", ret);
            return ret;
        }
        return IOTC_OK;
    }
    IOTC_LOGW("invalid event %d", discEvent);
    return IOTC_CORE_BLE_INVALID_GATT_EVENT;
}

int32_t SleSsapClinetEventInit(void)
{
    return IotcSleRegisterSsapClientCallbacks(SleSsapClientEventHandler);
}