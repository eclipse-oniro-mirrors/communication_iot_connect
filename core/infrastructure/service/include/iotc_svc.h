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
#ifndef IOTC_SERVICE_H
#define IOTC_SERVICE_H
#include <stdint.h>
#include "iotc_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IOTC_SERVICE_ID_BASE(module) ((module) << 8)

typedef enum {
    IOTC_SERVICE_ID_SOFTAP = IOTC_SERVICE_ID_BASE(IOTC_SUB_MODULE_CORE_WIFI),
    IOTC_SERVICE_ID_WIFI_CONNECT,
    IOTC_SERVICE_ID_LOCAL_CONTROL,
    IOTC_SERVICE_ID_LAN_SEARCH,
    IOTC_SERVICE_ID_M2M_CLOUD,

    IOTC_SERVICE_ID_BLE = IOTC_SERVICE_ID_BASE(IOTC_SUB_MODULE_CORE_BLE),
    
    IOTC_SERVICE_ID_SLE = IOTC_SERVICE_ID_BASE(IOTC_SUB_MODULE_CORE_SLE),

    IOTC_SERVICE_ID_DEVICE = IOTC_SERVICE_ID_BASE(IOTC_SUB_MODULE_CORE_DEVICE),
} IotcServiceId;

#ifdef __cplusplus
}
#endif
#endif /* IOTC_SERVICE_H */