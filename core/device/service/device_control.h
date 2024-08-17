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
#ifndef DEVICE_CONTROL_H
#define DEVICE_CONTROL_H

#include <stdbool.h>
#include <stdint.h>
#include "iotc_json.h"
#include "iotc_svc_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t DeviceControlReportAll(DevReportType type);

int32_t DeviceControlGetCharStates(const IotcJson *inArray, IotcJson **outArray);

/* outArray为空则不会返回属性修改结果，无论是否为空，都会一次触发异步上报 */
int32_t DeviceControlPutCharStates(const IotcJson *inArray, IotcJson **outArray);

#ifdef __cplusplus
}
#endif

#endif /* DEVICE_CONTROL_H */