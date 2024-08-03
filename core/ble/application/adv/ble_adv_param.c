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
#include "ble_adv_param.h"
#include "securec.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "utils_assert.h"

static const AdapterBleAdvParam g_advParam = {
    .advType = ADAPTER_BLE_ADV_TYPE_IND,
    .advMinInt = 0x20,
    .advMaxInt = 0x40,
    .ownerAddrType = ADAPTER_BLE_ADV_ADDR_PUBLIC,
    .directAddrType = ADAPTER_BLE_ADV_ADDR_PUBLIC,
    .directAddr = NULL,
    .channelMap = ADAPTER_BLE_CHNL_ALL,
};

int32_t GetBleAdvParam(AdapterBleAdvParam *advParam)
{
    CHECK_RETURN_LOGW(advParam != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");
    if (memcpy_s(advParam, sizeof(AdapterBleAdvParam), &g_advParam, sizeof(AdapterBleAdvParam)) != EOK) {
        IOTC_LOGE("copy");
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    return IOTC_OK;
}