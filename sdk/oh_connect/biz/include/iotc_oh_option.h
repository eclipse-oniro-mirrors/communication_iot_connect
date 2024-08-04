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
#ifndef IOTC_OH_OPTION_H
#define IOTC_OH_OPTION_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include "iotc_oh_sdk.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t (*IotcOhOptionSetFunc)(va_list args);

typedef struct {
    int32_t option;
    IotcOhOptionSetFunc setFunc;
} OptionItem;

int32_t IotcOhOptionRegister(const OptionItem *items, uint32_t len);

void IotcOhOptionUnregister(const OptionItem *items);

int32_t IotcOhSetOptionInner(int32_t option, va_list args);

#ifdef __cplusplus
}
#endif

#endif /* IOTC_OH_OPTION_H */
