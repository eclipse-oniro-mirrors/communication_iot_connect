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
#ifndef BLE_ADV_PARAM_H
#define BLE_ADV_PARAM_H

#include <stdint.h>
#include "adapter_ble.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 获取广播参数
 *
 * @param advParam [OUT] 广播参数
 * @return 0 成功，非0 失败
 */
int32_t GetBleAdvParam(AdapterBleAdvParam *advParam);

#ifdef __cplusplus
}
#endif

#endif /* BLE_ADV_PARAM_H */
