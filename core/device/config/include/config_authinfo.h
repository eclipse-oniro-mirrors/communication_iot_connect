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
#ifndef CONFIG_BLE_AUTHINFO_H
#define CONFIG_BLE_AUTHINFO_H

#include <stdbool.h>
#include "iotc_svc_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t ConfigSaveAuthInfo(const DevAuthInfo *info);

int32_t ConfigGetAuthInfo(DevAuthInfo *info);

int32_t ConfigClearAuthInfo(void);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_BLE_AUTHINFO_H */