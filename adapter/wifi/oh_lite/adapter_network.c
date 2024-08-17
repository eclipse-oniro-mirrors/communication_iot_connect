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
#include "adapter_network.h"
#include <stdbool.h>
#include <stddef.h>
#include "securec.h"
#include "wifi_device.h"
#include "wifi_event.h"
#include "wifi_device_config.h"
#include "iotc_errcode.h"
#include "adapter_socket.h"
#include "iotc_conf.h"
#include "adapter_log.h"

int32_t IotcGetSoftApIp(char *buf, uint32_t len)
{
    return IotcGetLocalIp(buf, len);
}

int32_t IotcGetLocalIp(char *buf, uint32_t len)
{
    if ((buf == NULL) || (len == 0)) {
        ADAPTER_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    static bool isPrint = false;
    WifiLinkedInfo info;
    (void)memset_s(&info, sizeof(WifiLinkedInfo), 0, sizeof(WifiLinkedInfo));
    int32_t ret = GetLinkedInfo(&info);
    if (ret != WIFI_SUCCESS) {
        /* 该接口高频调用，避免日志过多 */
        if (!isPrint) {
            ADAPTER_LOGE("Get wifi linked info fail %d", ret);
            isPrint = true;
        }
        return IOTC_ADAPTER_NETWORK_ERR_GET_IP;
    }
    isPrint = false;

    const char *ip = IotcInetNtoa(info.ipAddress, buf, len);
    if ((ip == NULL) || (ip[0] == '\0')) {
        return IOTC_ADAPTER_NETWORK_ERR_GET_IP;
    }
    return IOTC_OK;
}

int32_t IotcGetMacAddr(uint8_t *buf, uint32_t len)
{
    if ((buf == NULL) || (len < IOTC_MAC_ADDRESS_LEN)) {
        ADAPTER_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    int32_t ret = GetDeviceMacAddress(buf);
    if (ret != WIFI_SUCCESS) {
        ADAPTER_LOGE("get mac error %d", ret);
        return IOTC_ADAPTER_NETWORK_ERR_GET_MAC;
    }
    return IOTC_OK;
}

IotcNetworkState IotcGetNetworkState(void)
{
    if (IsWifiActive() != WIFI_STA_ACTIVE) {
        return IOTC_NETWORK_NOT_CONNECTED;
    }
    WifiLinkedInfo info;
    (void)memset_s(&info, sizeof(WifiLinkedInfo), 0, sizeof(WifiLinkedInfo));
    if (GetLinkedInfo(&info) != WIFI_SUCCESS) {
        return IOTC_NETWORK_NOT_CONNECTED;
    }
    return (IotcNetworkState)info.connState;
}

int32_t IotcGetBroadcastAddr(char *buf, uint32_t len)
{
    if ((buf == NULL) || (len == 0)) {
        ADAPTER_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
#if IOTC_CONF_BROAD_CAST_SUPPORT
    IpInfo ipInfo = {0};
    WifiErrorCode ret = GetIpInfo(&ipInfo);
    if (ret != WIFI_SUCCESS) {
        ADAPTER_LOGE("get ip info error %d", ret);
        return IOTC_IOTC_NETWORK_ERR_GET_BROADCAST_IP;
    }
    uint32_t broadcastIp = (ipInfo.ipAddress & ipInfo.netMask) | (~ipInfo.netMask);
    const char *ip = IotcInetNtoa(broadcastIp, buf, len);
    if ((ip == NULL) || (ip[0] == '\0')) {
        return IOTC_IOTC_NETWORK_ERR_GET_BROADCAST_IP;
    }
#endif
    return IOTC_OK;
}