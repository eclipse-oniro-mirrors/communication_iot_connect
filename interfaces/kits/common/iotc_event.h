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
#ifndef IOTC_EVENT_H
#define IOTC_EVENT_H

#include <stdint.h>
#include "iotc_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IOTC_EVENT_ID_BASE(module) ((module) << 16)

typedef enum {
    /**< 设备重启 */
    IOTC_CORE_COMM_EVENT_MAIN_REBOOT = IOTC_EVENT_ID_BASE(IOTC_SUB_MODULE_PUBLIC),
    /**< 设备恢复出厂 */
    IOTC_CORE_COMM_EVENT_MAIN_RESTORE,
    /**< SDK完成初始化 */
    IOTC_CORE_COMM_EVENT_MAIN_INITIALIZED,
    /**< SDK复位 */
    IOTC_CORE_COMM_EVENT_MAIN_RESET,
    /**< SDK退出 */
    IOTC_CORE_COMM_EVENT_MAIN_QUIT,
    /**< 服务启动 */
    IOTC_CORE_COMM_BLE_SVC_START,
    /**< GATT建链 */
    IOTC_CORE_BLE_EVENT_GATT_CONNECT,
    IOTC_CORE_BLE_EVENT_GATT_DISCONNECT,

    /**< SSAP建链 */
    IOTC_CORE_SLE_EVENT_SSAP_CONNECT,
    IOTC_CORE_SLE_EVENT_SSAP_DISCONNECT,

    IOTC_CORE_SLE_EVENT_ENABLED,
    IOTC_CORE_SLE_EVENT_DISABLED,
    IOTC_CORE_SLE_EVENT_ANNOUNCE_ENABLED,
    IOTC_CORE_SLE_EVENT_ANNOUNCE_DISABLED,
    IOTC_CORE_SLE_EVENT_ANNOUNCE_TERMINAL,
    IOTC_CORE_SLE_EVENT_ANNOUNCE_REMOVE,
    IOTC_CORE_SLE_EVENT_SEEK_ENABLE,
    IOTC_CORE_SLE_EVENT_SEEK_DISABLE,
    IOTC_CORE_SLE_EVENT_SEEK_RESULT,
    IOTC_CORE_SLE_EVENT_CONNECT_STATE_CHANGED,
    IOTC_CORE_SLE_EVENT_CONNECT_PARAM_UPDATE_REQ,
    IOTC_CORE_SLE_EVENT_CONNECT_PARAM_UPDATE,
    IOTC_CORE_SLE_EVENT_AUTH_COMPLETE,
    IOTC_CORE_SLE_EVENT_PAIR_COMPLETE,
    IOTC_CORE_SLE_EVENT_READ_RSSI,
    IOTC_CORE_SLE_EVENT_LOW_LATENCY,
    IOTC_CORE_SLE_EVENT_SET_PHY_EVENT,

    /**< WIFI状态 */
    IOTC_CORE_WIFI_EVENT_CONNECTING,
    IOTC_CORE_WIFI_EVENT_CONNECTED,
    IOTC_CORE_WIFI_EVENT_CONNECT_FAIL,
    IOTC_CORE_WIFI_EVENT_SOFTAP_START,
    IOTC_CORE_WIFI_EVENT_SOFTAP_STOP,

    IOTC_SDK_AILIFE_EVENT_WIFI_UPLINK_REGISTER_FAILED,
    /**< 上行已注册 */
    IOTC_SDK_AILIFE_EVENT_WIFI_UPLINK_REGISTERED,
    /**< 上行解绑 */
    IOTC_SDK_AILIFE_EVENT_WIFI_UPLINK_UNREGISTER,
    /**< 上行在线 */
    IOTC_SDK_AILIFE_EVENT_WIFI_UPLINK_ONLINE,
    /**< 上行离线 */
    IOTC_SDK_AILIFE_EVENT_WIFI_UPLINK_OFFLINE,
    IOTC_SDK_AILIFE_EVENT_WIFI_NET_CONNECT,
    IOTC_SDK_AILIFE_EVENT_WIFI_NET_DISCONNECT,
} IotcEvent;

#ifdef __cplusplus
}
#endif

#endif /* IOTC_EVENT_H */