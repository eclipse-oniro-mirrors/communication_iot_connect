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
#include "sle_ssap_event.h"
#include "sle_ssap_mgt.h"
#include "iotc_sle.h"
#include "sle_sched_event.h"
#include "sched_msg_queue.h"
#include "securec.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "iotc_mem.h"
#include "utils_assert.h"
#include "utils_common.h"

typedef struct {
    IotcAdptSleSsapEvent ssapEvent;
    SleScheduleEvent scheduleEvent;
    void (*msgFree)(void *msg);
} SsapEventToScheduleEvent;

static void MsgFree(void *msg);
static void ReqWriteMsgFree(void *msg);

static const SsapEventToScheduleEvent EVENT_COVERT_MAP[] = {
    {
        .ssapEvent = IOTC_ADPT_SLE_SSAP_EVENT_CONNECT,
        .scheduleEvent = SLE_EVENT_CONNECT,
        .msgFree = MsgFree
    },
    {
        .ssapEvent = IOTC_ADPT_SLE_SSAP_EVENT_DISCONNECT,
        .scheduleEvent = SLE_EVENT_DISCONNECT,
        .msgFree = MsgFree
    },
    {
        .ssapEvent = IOTC_ADPT_SLE_SSAP_EVENT_START_SVC_RESULT,
        .scheduleEvent = SLE_EVENT_START_SVC_RESULT,
        .msgFree = MsgFree
    },
    {
        .ssapEvent = IOTC_ADPT_SLE_SSAP_EVENT_STOP_SVC_RESULT,
        .scheduleEvent = SLE_EVENT_STOP_SVC_RESULT,
        .msgFree = MsgFree
    },
    {
        .ssapEvent = IOTC_ADPT_SLE_SSAP_EVENT_INDICATE_CONF,
        .scheduleEvent = SLE_EVENT_INDICATE_CONF,
        .msgFree = MsgFree
    },
    {
        .ssapEvent = IOTC_ADPT_SLE_SSAP_EVENT_SET_MTU_RESULT,
        .scheduleEvent = SLE_EVENT_SET_MTU_RESULT,
        .msgFree = MsgFree
    },
    {
        .ssapEvent = IOTC_ADPT_SLE_SSAP_EVENT_START_ADV_RESULT,
        .scheduleEvent = SLE_EVENT_START_ADV_RESULT,
        .msgFree = MsgFree
    },
    {
        .ssapEvent = IOTC_ADPT_SLE_SSAP_EVENT_STOP_ADV_RESULT,
        .scheduleEvent = SLE_EVENT_STOP_ADV_RESULT,
        .msgFree = MsgFree
    },
    {
        .ssapEvent = IOTC_ADPT_SLE_SSAP_EVENT_REQ_READ,
        .scheduleEvent = SLE_EVENT_REQ_READ,
        .msgFree = MsgFree
    },
    {
        .ssapEvent = IOTC_ADPT_SLE_SSAP_EVENT_REQ_WRITE,
        .scheduleEvent = SLE_EVENT_REQ_WRITE,
        .msgFree = ReqWriteMsgFree
    },
};

static void MsgFree(void *msg)
{
    /* IotcFree may be macro when debug */
    IotcFree(msg);
}

static void ReqWriteMsgFree(void *msg)
{
    IotcAdptSleSsapEventParam *eventParam = (IotcAdptSleSsapEventParam *)msg;
    IotcFree(eventParam->reqWrite.value);
    eventParam->reqWrite.value = NULL;
    IotcFree(msg);
}

static int32_t SleSsapEventHandler(IotcAdptSleSsapEvent ssapEvent, const IotcAdptSleSsapEventParam *param)
{
    CHECK_RETURN_LOGW(param != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");
    for (uint32_t i = 0; i < ARRAY_SIZE(EVENT_COVERT_MAP); i++) {
        if (ssapEvent != EVENT_COVERT_MAP[i].ssapEvent) {
            continue;
        }

        IotcAdptSleSsapEventParam *eventParam =
            (IotcAdptSleSsapEventParam *)UtilsMallocCopy((const uint8_t *)param, sizeof(IotcAdptSleSsapEventParam));
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
    IOTC_LOGW("invalid event %d", ssapEvent);
    return IOTC_CORE_SLE_INVALID_SSAP_EVENT;
}

static void SleSendIndicateDataFree(void *param)
{
    CHECK_V_RETURN(param != NULL);
    SleIndicateParam *indParam = (SleIndicateParam *)param;
    if (indParam->value != NULL) {
        IotcFree(indParam->value);
        indParam->value = NULL;
    }
    IotcFree(indParam);
}

int32_t IotcSleSendIndicateData(const char *svcUuid, const char *charUuid,
    const uint8_t *value, uint32_t valueLen)
{
    CHECK_RETURN_LOGW((svcUuid != NULL) && (charUuid != NULL) &&  (value != NULL) && (valueLen != 0),
        IOTC_ERR_PARAM_INVALID, "invalid param");

    SleIndicateParam *param = IotcCalloc(1, sizeof(SleIndicateParam));
    if (param == NULL) {
        IOTC_LOGE("malloc err");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    param->value = UtilsMallocCopy(value, valueLen);
    if (param->value == NULL) {
        IotcFree(param);
        IOTC_LOGE("malloc err");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    param->valueLen = valueLen;
    param->svcUuid = svcUuid;
    param->charUuid = charUuid;

    SleSchedMsg msg;
    msg.event = SLE_EVENT_SEND_INDICATE;
    msg.param = param;
    msg.freeFunc = SleSendIndicateDataFree;
    int32_t ret = SleSchedMsgQueueSend(&msg, 0);
    if (ret != IOTC_OK) {
        IotcFree(param->value);
        IotcFree(param);
        IOTC_LOGE("send msg err ret=%d", ret);
        return ret;
    }
    IOTC_LOGN("send indicate msg success valueLen=%u", param->valueLen);
    return IOTC_OK;
}

int32_t SleSsapEventInit(void)
{
    return IotcSleRegisterSsapCb(SleSsapEventHandler);
}