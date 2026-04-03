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
#ifndef COMMON_DEF_H
#define COMMON_DEF_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STR_ERRCODE "errcode"

typedef struct {
    uint8_t *buffer;
    uint32_t len;
    uint32_t size;
} CommBuffer;

typedef struct {
    const uint8_t *data;
    uint32_t len;
} CommData;

#define GET_STR_TAIL(s, len) ((s) + strlen(s) - (len))

#define DEVICE_ID_MAX_STR_LEN 40
#define CLOUD_SECRET_MAX_STR_LEN 40
#define CLOUD_ACTIVATE_CODE_MAX_STR_LEN 40
#define CLOUD_MAX_URL_LEN 64
#define CLOUD_PSK_MAX_LEN 16
#define CLOUD_PSK_HEX_MAX_STR_LEN (CLOUD_PSK_MAX_LEN * 2)
#define CLOUD_REFRESH_TOKEN_STR_LEN 40
#define CLOUD_ACCESS_TOKEN_STR_LEN 40
#define CLOUD_TOKEN_LEN 2

#define BLE_UID_HASH_LEN            64
#define BLE_AUTHCODE_ID_LEN         32
#define BLE_AUTHCODE_LEN            16

#define PUUID_MAX_STR_LEN 40
#define LOCAL_CONTROL_PORT 5686
#define LAN_SEARCH_PORT 5683
#define WIFI_SOFTAP_UDP_PORT 5683
#define LOCAL_CONTROL_REPORT_PORT 5683
#define COAP_DEFAULT_PORT 5683
#define PORT_BYTE_LENGTH 2
#define BITS_PER_BYTE 8

#define STR_URI_LOCAL_CONTROL_SESS_MNGR ".sys/sessMngr"
#define STR_URI_LOCAL_CONTROL_SEARCH ".well-known/core?st=ohLocalControl"
#define STR_URI_LAN_SEARCH ".well-known/core?st=ohCloudSetup"
#define STR_URI_SPEKE "SPEKE"
#define STR_URI_SPKEK_V2 "SPEKE/v2"
#define STR_URI_CLOUD_SETUP_V2 "CloudSetup/v2"
#define STR_URI_PATH_SYS ".sys"
#define STR_URI_PATH_ACTIVATE "activate"
#define STR_URI_PATH_LOGIN "login"
#define STR_URI_PATH_SYNC "devInfoSync"
#define STR_URI_PATH_STATUS_SYNC "statusSync"
#define STR_URI_PATH_DEVCONTROL "devControl"
#define STR_URI_PATH_TOKEN "refresh"
#define STR_URI_PATH_HB "heartbeat"
#define STR_URI_PATH_DEV_DEL ".sys/delDevice"
#define STR_URI_PATH_REVOKE "revoke"
#define STR_URI_PATH_PSK "psk"
#define STR_URI_PATH_AUTHCODE "authCode"

#define WIFI_CLOUD_TLS_PORT 5683
#define WIFI_CLOUD_TCP_PORT 5685
#define STA_CLOUD_TCP_PORT 5893
#ifdef HI3863_SDK_CONFIG_PATH
#define SDK_CONFIG_PATH "iotc"
#define HI3863_SDK_CONFIG_JSON_PATH "authInfo"
#else
#define SDK_CONFIG_PATH "/data/app/iotc"
#endif

#define STR_NETINFO_SSID "ssid"
#define STR_NETINFO_PASSWORD "password"
#define STR_NETINFO_DEVICE_ID "devId"
#define STR_NETINFO_PSK "psk"
#define STR_NETINFO_CODE "code"
#define STR_NETINFO_URL "cloudPrimaryUrl"
#define STR_NETINFO_BACKUP_URL "cloudStandbyUrl"

#define STR_JSON_ENCRYPT "encryptMode"
#define STR_JSON_PLAINTEXT "plaintext"
#define STR_JSON_DEVID          "devId"
#define STR_JSON_GATEWAY_ID     "gateWayId"
#define STR_JSON_AUTHCODE       "authCode"
#define STR_JSON_UIDHASH        "uidHash"
#define STR_JSON_AUTHCODE_ID    "authCodeId"
#define STR_JSON_SEQ_NUM        "seq"
#define STR_JSON_UUID           "uuid"
#define STR_JSON_SN1            "sn1"
#define STR_JSON_SN2            "sn2"
#define STR_JSON_SESS_ID        "sessId"
#define STR_JSON_MODE_SUPPORT   "modeSupport"
#define STR_JSON_MODE_RESP      "modeResp"
#define STR_JSON_VER "ver"
#define STR_JSON_SEQ "seq"
#define STR_JSON_MSG_ID      "msgId"
#define STR_JSON_DEVICE_INFO "deviceInfo"
#define STR_JSON_MAC "mac"
#define STR_JSON_BLE_MAC "bleMac"
#define STR_JSON_SLE_MAC "sleMac"
#define STR_JSON_HIV "hiv"
#define STR_JSON_VENDOR "vendor"
#define STR_JSON_SID "sid"
#define STR_JSON_DATA "data"
#define STR_JSON_ALL_SERVICE "allservices"
#define STR_JSON_CODE STR_NETINFO_CODE
#define STR_JSON_DEV_INFO "devInfo"
#define STR_JSON_SERVICES "services"
#define STR_JSON_DEV_ID STR_NETINFO_DEVICE_ID
#define STR_JSON_SECRET "secret"
#define STR_JSON_TIMESTAMP "timestamp"
#define STR_JSON_TIMEZONE "timezone"
#define STR_JSON_ACCESS_TOKEN "accessToken"
#define STR_JSON_REFRESH_TOKEN "refreshToken"
#define STR_JSON_TIMEOUT "timeout"
#define STR_JSON_STATUS "status"
#define STR_JSON_SN "sn"
#define STR_JSON_PRODUCT_ID "productId"
#define STR_JSON_PROD_ID "prodId"
#define STR_JSON_SUB_PROD_ID "subProdId"
#define STR_JSON_PROT_TYPE "protType"
#define STR_JSON_DEV_TYPE "devType"
#define STR_JSON_DEV_TYPE_NAME "devTypeName"
#define STR_JSON_DISC_TYPE "discType"
#define STR_JSON_MODEL "model"
#define STR_JSON_MANU "manu"
#define STR_JSON_DEV_NAME "devName"
#define STR_JSON_MANU_NAME "manuName"
#define STR_JSON_FWV "fwv"
#define STR_JSON_SWV "swv"
#define STR_JSON_HWV "hwv"
#define STR_JSON_SVC_TYPE "st"
#define STR_JSON_SVC_ID "sid"
#define STR_JSON_DEVICE_DELAY "devicedelay"
#define STR_E2E_DATA_CHANGE "e2eDataChange"
#define STR_E2E_CONTROL "e2eCtrl"
#define STR_JSON_ERR_CODE "errcode"
#define DEVICE_STATUS_ONLINE   "online"
#define DEVICE_STATUS_OFFLINE  "offline"
#define DEVICE_STATUS_INBOX    "inbox"

#define HILINK_VERSION "1.0"

#ifdef __cplusplus
}
#endif
#endif /* COMMON_DEF_H */