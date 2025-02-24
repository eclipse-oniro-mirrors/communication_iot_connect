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
#ifndef SLE_ADV_CTRL_H
#define SLE_ADV_CTRL_H

#include <stdint.h>
#include "iotc_sle.h"
#include "iotc_sle_def.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t (*SleGetAdvInfoCallback)(IotcAdptSleAdvParam *advPara, IotcAdptSleAdvData *advData);
int32_t SleRegAdvAdvInfoCallback(SleGetAdvInfoCallback cb);
int32_t SleAdvCtrlStartSpecific(const IotcSleAdvParam *advPara, const IotcSleAdvData *advData, uint32_t ms);
int32_t SleAdvCtrlStart(uint32_t ms);
int32_t SleAdvCtrlStop(void);
int32_t SleAdvCtrlResume(void);
int32_t SleAdvCtrlUpdate(void);

#ifdef __cplusplus
}
#endif

#endif /* SLE_ADV_CTRL_H */
