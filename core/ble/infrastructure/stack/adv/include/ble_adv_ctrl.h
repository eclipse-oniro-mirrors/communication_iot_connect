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
#ifndef BLE_ADV_CTRL_H
#define BLE_ADV_CTRL_H

#include <stdint.h>
#include "adapter_ble.h"
#include "iotc_ble_def.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t (*BleGetAdvInfoCallback)(AdapterBleAdvParam *advPara, AdapterBleAdvData *advData);
int32_t BleRegAdvAdvInfoCallback(BleGetAdvInfoCallback cb);
int32_t BleAdvCtrlStartSpecific(const IotcBleAdvParam *advPara, const IotcBleAdvData *advData, uint32_t ms);
int32_t BleAdvCtrlStart(uint32_t ms);
int32_t BleAdvCtrlStop(void);
int32_t BleAdvCtrlResume(void);
int32_t BleAdvCtrlUpdate(void);

#ifdef __cplusplus
}
#endif

#endif /* BLE_ADV_CTRL_H */
