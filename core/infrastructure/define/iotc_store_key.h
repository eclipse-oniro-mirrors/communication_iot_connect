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
#ifndef IOTC_STORE_KEY_H
#define IOTC_STORE_KEY_H
#include <stdint.h>
#include "security_store.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STORE_KEY_BASE(module) ((module) << 16)

typedef enum {
    IOTC_STORE_KEY_INT_WIFI_CONN_CONFIG = STORE_KEY_BASE(IOTC_SUB_MODULE_CORE_WIFI),
    IOTC_STORE_KEY_INT_DEVICE_CONFIG = STORE_KEY_BASE(IOTC_SUB_MODULE_CORE_DEVICE),
} IotcOhStoreKeyInt;

#define IOTC_STORE_KEY_CHAR_WIFI_CONN_CONFIG "conn_data"
#define IOTC_STORE_KEY_CHAR_DEVICE_CONFIG "dev_data"

#ifdef __cplusplus
}
#endif

#endif /* IOTC_STORE_KEY_H */