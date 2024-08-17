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
#ifndef CHAR_STATE_MODEL_H
#define CHAR_STATE_MODEL_H

#include <stdint.h>
#include <stddef.h>
#include "adapter_json.h"
#include "iotc_prof_def.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char **data;
    uint32_t *len;
} GetCharStatesData;

int32_t MdlPutJsonArrayToCharStates(const IotcJson *json, IotcCharState **states, uint32_t *size);

int32_t MdlGetJsonArrayToCharStates(const IotcJson *json, IotcCharState **states, uint32_t *size);

void MdlCharStatesFree(IotcCharState **states, uint32_t size);

int32_t MdlInitGetCharStatesData(uint32_t size, GetCharStatesData *charData);

void MdlFreeGetCharStatesData(GetCharStatesData *charData);

int32_t MdlCharStatesToJson(const IotcCharState state[], uint32_t num, IotcJson **array);

int32_t MdlUpdateCharStates(IotcCharState states[], const GetCharStatesData *charData, uint32_t num);

#ifdef __cplusplus
}
#endif

#endif /* CHAR_STATE_MODEL_H */
