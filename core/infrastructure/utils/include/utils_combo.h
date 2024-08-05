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
#ifndef UTILS_COMBO_H
#define UTILS_COMBO_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    COMBO_TYPE_INVALID = -1,
    COMBO_TYPE_COMBO = 0,
    COMBO_TYPE_WIFI_ONLY,
    COMBO_TYPE_BLE_ONLY,
} ComboType;

void UtilsComboSetBleFlag(bool flag);

void UtilsComboSetWifiFlag(bool flag);

ComboType UtilsGetComboType(void);

#ifdef __cplusplus
}
#endif
#endif /* UTILS_COMBO_H */