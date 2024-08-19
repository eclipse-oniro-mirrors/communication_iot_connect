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
#ifndef IOTC_SERVICE_BLE_H
#define IOTC_SERVICE_BLE_H
#include <stdint.h>
#include "iotc_ble.h"
#include "iotc_ble_def.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BLE_SERVICE_MSG_ID_DEVICE_CONTROL = 0,
    BLE_SERVICE_MSG_ID_CLOUD_SETUP,
    BLE_SERVICE_MSG_ID_AUTH_SETUP,
    BLE_SERVICE_MSG_ID_MAX,
} BleSvcMsgId;

typedef enum {
    IOTC_BLE_ADV_TYPE_NEARBY_HALF_MODAL = 0, /* 靠近发现拉半模态 */
    IOTC_BLE_ADV_TYPE_NEARBY_FA,             /* 靠近发现拉FA */
    IOTC_BLE_ADV_TYPE_ONEHOP_HALF_MODAL,     /* 碰一碰拉半模态 */
    IOTC_BLE_ADV_TYPE_ONEHOP_FA,             /* 碰一碰拉FA */
    IOTC_BLE_ADV_TYPE_ONLY_NAME,             /* 只广播蓝牙名称 */
    IOTC_BLE_ADV_TYPE_CUSTOM,                /* 用户自定义广播 */
} BleSvcAdvDataType;

typedef int32_t (*BleSvcCustomAdvDataCallback)(IotcAdptBleAdvData *advData);
typedef int32_t (*BleStartAdv)(uint32_t ms);
typedef int32_t (*BleStopAdv)(void);
typedef void (*BleSetAdvDataType)(BleSvcAdvDataType type);
typedef int32_t (*BleRecvNetCfgInfo)(const char *netInfo, uint32_t len);
typedef int32_t (*BleRecvCustomSecData)(const uint8_t *data, uint32_t len);
typedef int32_t (*BleSendCustomSecData)(const uint8_t *data, uint32_t len);
typedef int32_t (*BleSendIndicateData)(const char *svcUuid, const char *charUuid,
    const uint8_t *value, uint32_t valueLen);

typedef struct {
    BleStartAdv onStartAdv;
    BleStopAdv onStopAdv;
    BleSetAdvDataType onSetAdvType;
    BleSendCustomSecData onSendCustomSecData;
    BleSendIndicateData onSendIndicateData;
} BleSvcApi;

typedef struct {
    BleSvcAdvDataType advType;
    BleSvcCustomAdvDataCallback onCustomAdv;
} BleSvcInitParam;

int32_t BleSvcProxyStartAdv(uint32_t ms);
int32_t BleSvcProxyStopAdv(void);
int32_t BleSvcProxySendCustomSecData(const uint8_t *data, uint32_t len);
int32_t BleSvcProxySendIndicateData(const char *svcUuid, const char *charUuid,
    const uint8_t *value, uint32_t valueLen);

#ifdef __cplusplus
}
#endif
#endif /* IOTC_SERVICE_BLE_H */