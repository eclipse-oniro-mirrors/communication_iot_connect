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
#include "sle_conn_event.h"
#include "iotc_sle_server.h"
#include "sle_sched_event.h"
#include "sched_msg_queue.h"
#include "securec.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "iotc_mem.h"
#include "utils_assert.h"
#include "utils_common.h"

typedef struct {
    IotcAdptSleConnectionEvent connectionEvent;
    SleScheduleEvent scheduleEvent;
    void (*msgFree)(void *msg);
} ConnectionEventToScheduleEvent;

static void MsgFree(void *msg)
{
    IotcFree(msg);
}

static const ConnectionEventToScheduleEvent EVENT_COVERT_MAP[] = {
    {
        .connectionEvent = IOTC_ADPT_SLE_CONNECT_STATE_CHANGED_EVENT,
        .scheduleEvent = SLE_EVENT_CONNECT_STATE_CHANGED,
        .msgFree = MsgFree
    },
    {
        .connectionEvent = IOTC_ADPT_SLE_CONNECT_PARAM_UPDATE_REQ_EVENT,
        .scheduleEvent = SLE_EVENT_CONNECT_PARAM_UPDATE_REQ,
        .msgFree = MsgFree
    },
    {
        .connectionEvent = IOTC_ADPT_SLE_CONNECT_PARAM_UPDATE_EVENT,
        .scheduleEvent = SLE_EVENT_CONNECT_PARAM_UPDATE,
        .msgFree = MsgFree
    },
    {
        .connectionEvent = IOTC_ADPT_SLE_AUTH_COMPLETE_EVENT,
        .scheduleEvent = SLE_EVENT_AUTH_COMPLETE,
        .msgFree = MsgFree
    },
    {
        .connectionEvent = IOTC_ADPT_SLE_PAIR_COMPLETE_EVENT,
        .scheduleEvent = SLE_EVENT_PAIR_COMPLETE,
        .msgFree = MsgFree
    },
    {
        .connectionEvent = IOTC_ADPT_SLE_READ_RSSI_EVENT,
        .scheduleEvent = SLE_EVENT_READ_RSSI,
        .msgFree = MsgFree
    },
    {
        .connectionEvent = IOTC_ADPT_SLE_LOW_LATENCY_EVENT,
        .scheduleEvent = SLE_EVENT_LOW_LATENCY,
        .msgFree = MsgFree
    },
    {
        .connectionEvent = IOTC_ADPT_SLE_SET_PHY_EVENT,
        .scheduleEvent = SLE_EVENT_SET_PHY_EVENT,
        .msgFree = MsgFree
    },
};

static int32_t SleConnectionEventHandler(
    IotcAdptSleConnectionEvent discEvent,
    const IotcAdptSleConnectionEventParam *param
)
{
    CHECK_RETURN_LOGW(param != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");
    for (uint32_t i = 0; i < ARRAY_SIZE(EVENT_COVERT_MAP); i++) {
        if (discEvent != EVENT_COVERT_MAP[i].connectionEvent) {
            continue;
        }

        IotcAdptSleConnectionEventParam *eventParam =
            (IotcAdptSleConnectionEventParam *)UtilsMallocCopy(
                (const uint8_t *)param,
                sizeof(IotcAdptSleConnectionEventParam)
            );
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

int32_t SleConnectionEventInit(void)
{
    return IotcSleRegisterConnectionCallbacks(SleConnectionEventHandler);
}