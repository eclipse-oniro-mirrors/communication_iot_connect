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
#include "sle_disc_event.h"
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
    IotcAdptSleAnnounceSeekEvent annSeekEvent;
    SleScheduleEvent scheduleEvent;
    void (*msgFree)(void *msg);
} AnnounceSeekEventToScheduleEvent;

static void MsgFree(void *msg)
{
    IotcFree(msg);
}

static void SeekResultMsgFree(void *msg)
{
    if (msg == NULL) {
        IOTC_LOGW("invalid param");
        return;
    }
    IotcAdptSleAnnounceSeekEventParam *param = (IotcAdptSleAnnounceSeekEventParam *)msg;
    IotcFree(param->seekResult.data);
    param->seekResult.data = NULL;
    IotcFree(msg);
}

static const AnnounceSeekEventToScheduleEvent EVENT_COVERT_MAP[] = {
    {
        .annSeekEvent = IOTC_ADPT_SLE_ENABLE_EVENT,
        .scheduleEvent = SLE_EVENT_ENABLE,
        .msgFree = MsgFree
    },
    {
        .annSeekEvent = IOTC_ADPT_SLE_DISABLE_EVENT,
        .scheduleEvent = SLE_EVENT_DISABLE,
        .msgFree = MsgFree
    },
    {
        .annSeekEvent = IOTC_ADPT_SLE_ANNOUNCE_ENABLE_EVENT,
        .scheduleEvent = SLE_EVENT_ANNOUNCE_ENABLE,
        .msgFree = MsgFree
    },
    {
        .annSeekEvent = IOTC_ADPT_SLE_ANNOUNCE_DISABLE_EVENT,
        .scheduleEvent = SLE_EVENT_ANNOUNCE_DISABLE,
        .msgFree = MsgFree
    },
    {
        .annSeekEvent = IOTC_ADPT_SLE_ANNOUNCE_TERMINAL_EVENT,
        .scheduleEvent = SLE_EVENT_ANNOUNCE_TERMINAL,
        .msgFree = MsgFree
    },
    {
        .annSeekEvent = IOTC_ADPT_SLE_ANNOUNCE_REMOVE_EVENT,
        .scheduleEvent = SLE_EVENT_ANNOUNCE_REMOVE,
        .msgFree = MsgFree
    },
    {
        .annSeekEvent = IOTC_ADPT_SLE_SEEK_ENABLE_EVENT,
        .scheduleEvent = SLE_EVENT_SEEK_ENABLE,
        .msgFree = MsgFree
    },
    {
        .annSeekEvent = IOTC_ADPT_SLE_SEEK_DISABLE_EVENT,
        .scheduleEvent = SLE_EVENT_SEEK_DISABLE,
        .msgFree = MsgFree
    },
    {
        .annSeekEvent = IOTC_ADPT_SLE_SEEK_RESULT_EVENT,
        .scheduleEvent = SLE_EVENT_SEEK_RESULT,
        .msgFree = SeekResultMsgFree
    },
};

static int32_t SleAnnounceSeekEventHandler(
    IotcAdptSleAnnounceSeekEvent discEvent,
    const IotcAdptSleAnnounceSeekEventParam *param
)
{
    CHECK_RETURN_LOGW(param != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");
    for (uint32_t i = 0; i < ARRAY_SIZE(EVENT_COVERT_MAP); i++) {
        if (discEvent != EVENT_COVERT_MAP[i].annSeekEvent) {
            continue;
        }

        IotcAdptSleAnnounceSeekEventParam *eventParam =
            (IotcAdptSleAnnounceSeekEventParam *)UtilsMallocCopy(
                (const uint8_t *)param,
                sizeof(IotcAdptSleAnnounceSeekEventParam)
            );
        if (eventParam == NULL) {
            IOTC_LOGW("malloc error");
            return IOTC_ADAPTER_MEM_ERR_MALLOC;
        }

        SleSchedMsg msg;
        msg.event = EVENT_COVERT_MAP[i].scheduleEvent;
        msg.param = eventParam;
        msg.freeFunc = EVENT_COVERT_MAP[i].msgFree;
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

int32_t SleDiscEventInit(void)
{
    return IotcSleRegisterAnnounceSeekCallbacks(SleAnnounceSeekEventHandler);
}