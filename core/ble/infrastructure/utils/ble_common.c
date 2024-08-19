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
#include "iotc_ble.h"
#include "securec.h"
#include "iotc_log.h"
#include "utils_assert.h"
#include "iotc_errcode.h"
#include "iotc_svc_dev.h"
#include "utils_common.h"
#include "dev_info.h"

#define PROT_TYPE_STR_BUF_SIZE 10
#define SUB_PROID_LEN 2

#define MAC_INDEX_0 0
#define MAC_INDEX_1 1
#define MAC_INDEX_2 2
#define MAC_INDEX_3 3
#define MAC_INDEX_4 4
#define MAC_INDEX_5 5

static uint32_t g_startUpAdvTimeout = 0;

void SetBleStartUpAdvTimeout(uint32_t ms)
{
    g_startUpAdvTimeout = ms;
}

uint32_t GetBleStartUpAdvTimeout(void)
{
    return g_startUpAdvTimeout;
}

char *GetBleMacStr(void)
{
    static char macStr[MAC_STR_MAX_LEN];
    (void)memset_s(macStr, sizeof(macStr), 0, sizeof(macStr));
    uint8_t mac[IOTC_ADPT_BLE_ADDR_LEN] = {0};
    if (IotcBleGetBleMac(mac, IOTC_ADPT_BLE_ADDR_LEN) != IOTC_OK) {
        IOTC_LOGE("get mac");
        (void)memset_s(macStr, sizeof(macStr), 0, sizeof(macStr));
        return macStr;
    }
    if (sprintf_s(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
        mac[MAC_INDEX_0], mac[MAC_INDEX_1], mac[MAC_INDEX_2],
        mac[MAC_INDEX_3], mac[MAC_INDEX_4], mac[MAC_INDEX_5]) <= 0) {
        (void)memset_s(macStr, sizeof(macStr), 0, sizeof(macStr));
        IOTC_LOGE("sprintf_s");
        return macStr;
    }
    return macStr;
}

uint8_t GetSubProIdInt(void)
{
    const char *subProId = ModelGetDevSubProId();
    uint8_t proId = 0;
    if (subProId == NULL || strlen(subProId) != HEXIFY_LEN(sizeof(uint8_t)) ||
        !UtilsUnhexify(subProId, HEXIFY_LEN(sizeof(uint8_t)), &proId, sizeof(uint8_t))) {
        IOTC_LOGI("sub pro id invalid");
    }

    return proId;
}