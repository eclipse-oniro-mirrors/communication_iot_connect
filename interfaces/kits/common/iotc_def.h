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
#ifndef IOTC_DEFINE_H
#define IOTC_DEFINE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IOTC_SUB_MODULE_PUBLIC = 1,
    IOTC_SUB_MODULE_ADAPTER,
    IOTC_SUB_MODULE_CORE_INFRASTRUCTURE,
    IOTC_SUB_MODULE_CORE_PROFILE,
    IOTC_SUB_MODULE_CORE_WIFI,
    IOTC_SUB_MODULE_CORE_BLE,
    IOTC_SUB_MODULE_CORE_SLE,
    IOTC_SUB_MODULE_CORE_DEVICE,
    IOTC_SUB_MODULE_SDK,
    IOTC_SUB_MODULE_SDK_AILIFE,
    IOTC_SUB_MODULE_SDK_OH_CONNECT,
    IOTC_SUB_MODULE_MAX,
} IotcSubModule;

typedef enum {
    IOTC_REG_STATUS_NOT_REG = 0,
    IOTC_REG_STATUS_REG,
} IotcRegStatus;

typedef enum {
    IOTC_UPLINK_ONLINE = 0,
    IOTC_UPLINK_OFFLINE,
} IotcUplinkStatus;

typedef enum {
    /* 无需配网 */
    IOTC_NET_CONFIG_MODE_NONE = 0,
    /* 使用softap配网 */
    IOTC_NET_CONFIG_MODE_SOFTAP,
    /* ble辅助配网 */
    IOTC_NET_CONFIG_MODE_BLE_SUP,
    /* ble代理注册 */
    IOTC_NET_CONFIG_MODE_BLE_AGT,
    IOTC_NET_CONFIG_MODE_MAX,
} IotcNetConfigMode;

#define  IOTC_LOG_LEVEL_MIN    0
#define  IOTC_LOG_LEVEL_FATAL  1
#define  IOTC_LOG_LEVEL_ERROR  2
#define  IOTC_LOG_LEVEL_WARN   3
#define  IOTC_LOG_LEVEL_NOTICE 4
#define  IOTC_LOG_LEVEL_INFO   5
#define  IOTC_LOG_LEVEL_DEBUG  6
#define  IOTC_LOG_LEVEL_MAX    7

#define IOTC_DEVICE_LEVEL_MINI 0
#define IOTC_DEVICE_LEVEL_SMALL 1
#define IOTC_DEVICE_LEVEL_STANDARD 2

#define IOTC_BUILD_TYPE_RELEASE 0
#define IOTC_BUILD_TYPE_DEBUG 1

typedef enum {
    IOTC_NETWORK_NOT_AVAILABLE = 0,
    IOTC_NETWORK_AVAILABLE
} IotcNetworkStatus;

typedef enum {
    IOTC_REBOOT_WATCH_DOG_TIMEOUT = 0,
    IOTC_REBOOT_RESTORE,
} IotcRebootReason;

#define IOTC_SN_STR_MAX_LEN 39
#define IOTC_PRO_ID_STR_LEN 4
#define IOTC_SUB_PRO_ID_STR_LEN 2
#define IOTC_MODEL_STR_MAX_LEN 31
#define IOTC_DEV_TYPE_ID_STR_LEN 3
#define IOTC_DEV_TYPE_NAME_STR_MAX_LEN 31
#define IOTC_MANU_ID_STR_LEN 3
#define IOTC_MANU_NAME_STR_MAX_LEN 31
#define IOTC_FIRMWARE_VER_STR_MAX_LEN 63
#define IOTC_HARDWARE_VER_STR_MAX_LEN 63
#define IOTC_SOFTWARE_VER_STR_MAX_LEN 63

#define IOTC_SVC_TYPE_STR_MAX_LEN 31
#define IOTC_SVC_ID_STR_MAX_LEN 63

#define IOTC_PINCODE_LEN 8
#define IOTC_AC_KEY_LEN 48

#define IOTC_BLE_UUID_MAX_LEN 16

#define IOTC_API_PUBLIC __attribute__ ((visibility ("default")))

#ifdef __cplusplus
}
#endif

#endif /* IOTC_DEFINE_H */