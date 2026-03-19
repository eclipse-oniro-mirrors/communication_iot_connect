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

#ifndef IOT_CONNECT_ADAPTER_INCLUDE_IOTC_SLE_CLIENT_H_
#define IOT_CONNECT_ADAPTER_INCLUDE_IOTC_SLE_CLIENT_H_

#include "iotc_sle_host.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t IotcSleConnectRemoteDevice(const IotcAdptSleDeviceAddr *addr);
int32_t SleCtrlDisconnectRemoteDevice(const IotcAdptSleDeviceAddr *addr);
int32_t SleCtrlDefaultConnectionParamSet(const IotcAdptSleDefaultConnectParam *param);
int32_t IotcSleSetConnectParam(const IotcAdptSleConnectParam *param);
int32_t IotcSleDisconnectSsap(const uint8_t *bdAddr, uint32_t addrLen);
int32_t IotcSleSetSeekParam(const IotcAdptSleSeekParam *param);
int32_t IotcSleStartSeek(void);
int32_t IotcSleStoptSeek(void);
int32_t IotcSleDisconnectRemoteDevice(const IotcAdptSleDeviceAddr *addr);
int32_t IotcSleDefaultConnectionParamSet(const IotcAdptSleDefaultConnectParam *param);


#ifdef __cplusplus
}
#endif

#endif