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
#ifndef AILIFE_LOGIN_INFO_H
#define AILIFE_LOGIN_INFO_H

#include <stdint.h>
#include <stdbool.h>
#include "iotc_svc_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t ConfigSaveLoginInfo(const DevLoginInfo *info);

int32_t ConfigGetLoginInfo(DevLoginInfo *info);

bool IsDeviceBinded(void);

int32_t ConfigClearLoginInfo(void);

#ifdef __cplusplus
}
#endif

#endif /* AILIFE_LOGIN_INFO_H */
