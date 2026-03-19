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
#ifndef IOTC_SERVICE_SLE_H
#define IOTC_SERVICE_SLE_H
#include <stdint.h>
#include "iotc_sle_announce.h"
#include "iotc_sle_server.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    SLE_SERVICE_MSG_ID_DEVICE_CONTROL = 0,
    SLE_SERVICE_MSG_ID_CLOUD_SETUP,
    SLE_SERVICE_MSG_ID_AUTH_SETUP,
    SLE_SERVICE_MSG_ID_MAX,
} SleSvcMsgId;

typedef enum {
    IOTC_SLE_ADV_TYPE_NEARBY_HALF_MODAL = 0, /* 靠近发现拉半模态 */
    IOTC_SLE_ADV_TYPE_NEARBY_FA,             /* 靠近发现拉FA */
    IOTC_SLE_ADV_TYPE_ONEHOP_HALF_MODAL,     /* 碰一碰拉半模态 */
    IOTC_SLE_ADV_TYPE_ONEHOP_FA,             /* 碰一碰拉FA */
    IOTC_SLE_ADV_TYPE_ONLY_NAME,             /* 只广播蓝牙名称 */
    IOTC_SLE_ADV_TYPE_CUSTOM,                /* 用户自定义广播 */
} SleSvcAdvDataType;

typedef int32_t (*SleSvcCustomAdvDataCallback)(IotcAdptSleAnnounceData *advData);
typedef int32_t (*SleStartAdv)(uint32_t ms);
typedef int32_t (*SleStopAdv)(void);
typedef void (*SleSetAdvDataType)(SleSvcAdvDataType type);
typedef int32_t (*SleScanStart)(void);
typedef int32_t (*SleScanStop)(void);
typedef int32_t (*SleScanParamSet)(const IotcAdptSleSeekParam *param);
typedef int32_t (*SleConnectDevice)(const IotcAdptSleDeviceAddr *addr);
typedef int32_t (*SleDisconnectDevice)(const IotcAdptSleDeviceAddr *addr);
typedef int32_t (*SleDefConnectionParamSet)(const IotcAdptSleDefaultConnectParam *param);
typedef int32_t (*SleRecvNetCfgInfo)(const char *netInfo, uint32_t len);
typedef int32_t (*SleRecvCustomSecData)(const uint8_t *data, uint32_t len);
typedef int32_t (*SleSendCustomSecData)(const uint8_t *data, uint32_t len);
typedef int32_t (*SleSendIndicateData)(const char *svcUuid, const char *charUuid,
    const uint8_t *value, uint32_t valueLen);

typedef struct {
    SleStartAdv onStartAdv;
    SleStopAdv onStopAdv;
    SleSetAdvDataType onSetAdvType;
    SleScanStart onStartScan;
    SleScanStop onStopScan;
    SleScanParamSet onScanParamSet;
    SleConnectDevice onConnectDeviec;
    SleDisconnectDevice onDisconnectDevice;
    SleDefConnectionParamSet onConnectionParamSet;
    SleSendCustomSecData onSendCustomSecData;
    SleSendIndicateData onSendIndicateData;
} SleSvcApi;

typedef struct {
    SleSvcAdvDataType advType;
    SleSvcCustomAdvDataCallback onCustomAdv;
} SleSvcInitParam;

int32_t SleSvcProxyStartAdv(uint32_t ms);
int32_t SleSvcProxyStopAdv(void);
int32_t SleSvcProxySendCustomSecData(const uint8_t *data, uint32_t len);
int32_t SleSvcProxySendIndicateData(const char *svcUuid, const char *charUuid, const uint8_t *value, uint32_t valueLen);

#ifdef __cplusplus
}
#endif
#endif /* IOTC_SERVICE_SLE_H */