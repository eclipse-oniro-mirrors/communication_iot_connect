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
#ifndef IOT_CONNECT_CORE_SLE_INFRASTRUCTURE_STACK_SSAPC_INCLUDE_SLE_SSAPC_CTRL_H_
#define IOT_CONNECT_CORE_SLE_INFRASTRUCTURE_STACK_SSAPC_INCLUDE_SLE_SSAPC_CTRL_H_

#include <stdint.h>
#include "iotc_sle_client.h"
#include "iotc_sle_def.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t SleCtrlSsapcRegister(sleUUID *appUuid, uint8_t *clientId);
int32_t SleCtrlSsapcRegisterUnregister(uint8_t clientId);
int32_t SleCtrlSsapcFindStructure(uint8_t clientId, uint16_t connId, IotcAdptSsapcFindStructureParam *param);
int32_t SleCtrlSsapcReadReq(uint8_t clientId, uint16_t connId, uint16_t handle, uint8_t type);
int32_t SleCtrlSsapcWriteReq(uint8_t clientId, uint16_t connId, IotcAdptSsapcWriteParam *param);
int32_t SleCtrlSsapcExchangeInfoReq(uint8_t clientId, uint16_t connId, IotcAdptSsapExchangeInfo* param);

#ifdef __cplusplus
}
#endif

#endif /* SLE_ADV_CTRL_H */
