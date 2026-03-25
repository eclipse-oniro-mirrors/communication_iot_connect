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
#ifndef IOTC_OH_WIFI_H
#define IOTC_OH_WIFI_H

#include <stdint.h>
#include <stddef.h>
#include "iotc_def.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t (*IotcOhWifiGetCertCallback)(const char **ca[], uint32_t *num);

typedef int32_t (*IotcMakeOsWifiEnableCallback)(void);

IOTC_API_PUBLIC int32_t IotcOhWifiEnable(void);

IOTC_API_PUBLIC int32_t IotcOhWifiDisable(void);

#ifdef __cplusplus
}
#endif

#endif /* IOTC_OH_WIFI_H */
