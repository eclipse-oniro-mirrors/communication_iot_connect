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
#ifndef SLE_ADV_H
#define SLE_ADV_H

#include <stdint.h>
#include "iotc_svc_sle.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t SleAdvInit(void);

void SleSetAdvType(SleSvcAdvDataType type);

typedef int32_t (*CustomAdvDataCb)(IotcAdptSleAnnounceData *advData);
int32_t RegSleCustomAdvDataCb(CustomAdvDataCb cb);

#ifdef __cplusplus
}
#endif

#endif /* SLE_ADV_H */
