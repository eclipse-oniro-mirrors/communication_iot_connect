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
#include "ble_adv.h"
#include "ble_adv_ctrl.h"
#include "utils_assert.h"
#include "comm_def.h"
#include "ble_adv_data.h"
#include "ble_adv_param.h"
#include "utils_common.h"
#include "event_bus.h"
#include "iotc_event.h"
#include "iotc_errcode.h"

static int32_t AiLifeBleGetAdvInfoCallback(AdapterBleAdvParam *advPara, AdapterBleAdvData *advData)
{
    CHECK_RETURN_LOGW(advPara != NULL && advData != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    int32_t ret = GetBleAdvParam(advPara);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get adv param error %d", ret);
        return ret;
    }

    ret = GetBleAdvData(advData);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get adv data error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static void AdvUpdateEventBusCallback(uint32_t event, void *param, uint32_t len)
{
    NOT_USED(event);
    NOT_USED(param);
    NOT_USED(len);
    int32_t ret = BleAdvCtrlUpdate();
    if (ret != IOTC_OK) {
        IOTC_LOGW("update adv error %d", ret);
    }
}

static bool AdvUpdateEventMatchFunc(uint32_t event)
{
    /* 注册、在线状态变化时更新广播参数 */
    static const uint32_t UPDATE_EVENT[] = {
        IOTC_SDK_AILIFE_EVENT_WIFI_UPLINK_REGISTERED,
        IOTC_SDK_AILIFE_EVENT_WIFI_UPLINK_UNREGISTER,
        IOTC_SDK_AILIFE_EVENT_WIFI_UPLINK_ONLINE,
        IOTC_SDK_AILIFE_EVENT_WIFI_UPLINK_OFFLINE,
    };
    for (uint32_t i = 0; i < ARRAY_SIZE(UPDATE_EVENT); ++i) {
        if (UPDATE_EVENT[i] == event) {
            return true;
        }
    }
    return false;
}

int32_t BleAdvInit(void)
{
    int32_t ret = EventBusSubscribeMatch(AdvUpdateEventBusCallback, AdvUpdateEventMatchFunc);
    if (ret != IOTC_OK) {
        IOTC_LOGW("sub adv update event error %d", ret);
        return ret;
    }
    return BleRegAdvAdvInfoCallback(AiLifeBleGetAdvInfoCallback);
}

