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
#include "sle_conn_event.h"
#include "sle_ssap_mgt.h"
#include "sle_sched_event.h"
#include "sched_msg_queue.h"
#include "securec.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "iotc_mem.h"
#include "utils_assert.h"
#include "utils_common.h"

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
    int32_t ret = SleSsapServiceEventInit(); //init server or client event
    if (ret != IOTC_OK) { 
        IOTC_LOGE("sle ssap init err ret=%d", ret);
        return ret;
    }

    ret = SleConnectionEventInit();
    if (ret != IOTC_OK) {
        IOTC_LOGE("sle conn init err ret=%d", ret);
        return ret;
    }
    return IOTC_OK;
}