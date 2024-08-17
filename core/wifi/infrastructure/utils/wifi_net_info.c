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
#include <stdint.h>
#include <string.h>
#include "wifi_net_info.h"
#include "adapter_wifi.h"
#include "adapter_network.h"
#include "utils_common.h"
#include "securec.h"
#include "iotc_errcode.h"
#include "adapter_wifi_def.h"
#include "utils_assert.h"

#define INADDR_IP_STR "0.0.0.0"

bool IsNetworkConnected(void)
{
    if (IotcGetNetworkState() != IOTC_NETWORK_CONNECTED) {
        return false;
    }

    IotcWifiMode mode = IotcGetWifiMode();
    if (mode != IOTC_WIFI_MODE_STATION) {
        return false;
    }

    char ip[IOTC_IP_STR_MAX_LEN + 1] = {0};
    int32_t ret = IotcGetLocalIp(ip, sizeof(ip));
    if (ret != IOTC_OK) {
        return false;
    }
    if (strcmp(ip, INADDR_IP_STR) == 0) {
        return false;
    }

    return true;
}

bool IsWifiNetInfoExit(void)
{
    uint8_t ssid[IOTC_WIFI_SSID_MAX_LEN + 1] = {0};
    uint8_t pwd[IOTC_WIFI_PWD_MAX_LEN + 1] = {0};
    uint32_t ssidLen = IOTC_WIFI_SSID_MAX_LEN;
    uint32_t pwdLen = IOTC_WIFI_PWD_MAX_LEN;
    int32_t ret = IotcGetWifiInfo(ssid, &ssidLen, pwd, &pwdLen);
    if (ret != IOTC_OK) {
        return false;
    }

    bool valid = UtilsIsEmptyStr((const char *)ssid);
    (void)memset_s(ssid, sizeof(ssid), 0, sizeof(ssid));
    (void)memset_s(pwd, sizeof(pwd), 0, sizeof(pwd));
    return valid;
}

int32_t GetWifiMacAddrStr(char *buf, uint32_t len)
{
    CHECK_RETURN_LOGW(buf != NULL && len > MAC_ADDR_STR_LEN, IOTC_ERR_PARAM_INVALID, "param invalid");

    uint8_t mac[IOTC_MAC_ADDRESS_LEN] = {0};
    int32_t ret = IotcGetMacAddr(mac, sizeof(mac));
    if (ret != IOTC_OK) {
        IOTC_LOGW("get mac error %d", ret);
        return ret;
    }

    /* CI NOTE: 012345为mac的前6字节序号 */
    ret = sprintf_s(buf, len, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    if (ret <= 0) {
        IOTC_LOGW("sprintf error %d", ret);
        return IOTC_ERR_SECUREC_SPRINTF;
    }
    return IOTC_OK;
}