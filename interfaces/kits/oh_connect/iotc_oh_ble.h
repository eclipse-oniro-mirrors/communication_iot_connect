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
#ifndef IOTC_OH_BLE_H
#define IOTC_OH_BLE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "iotc_ble_def.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t (*IotcRecvNetCfgInfoCallback)(const char *netInfo, uint32_t len);

typedef int32_t (*IotcRecvCustomSecDataCallback)(const uint8_t *data, uint32_t len);

IOTC_API_PUBLIC int32_t IotcOhBleEnable(void);

IOTC_API_PUBLIC int32_t IotcOhBleDisable(void);

IOTC_API_PUBLIC int32_t IotcOhBleRelease(void);

IOTC_API_PUBLIC int32_t IotcOhBleStartAdv(uint32_t ms);

IOTC_API_PUBLIC int32_t IotcOhBleStopAdv(void);

IOTC_API_PUBLIC int32_t IotcOhBleSendCustomSecData(const uint8_t *data, uint32_t len);

IOTC_API_PUBLIC int32_t IotcOhBleSendIndicateData(const char *svcUuid, const char *charUuid,
    const uint8_t *value, uint32_t valueLen);

#ifdef __cplusplus
}
#endif

#endif /* IOTC_OH_BLE_H */
