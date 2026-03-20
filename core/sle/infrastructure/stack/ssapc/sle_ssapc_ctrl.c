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
#include "sle_ssapc_ctrl.h"
#include "iotc_sle_client.h"
#include "securec.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "iotc_os.h"
#include "utils_common.h"

int32_t SleCtrlSsapcRegister(SleUuid *appUuid, uint8_t *clientId)
{
    return IotcSleSsapcRegister(appUuid, clientId);
}

int32_t SleCtrlSsapcRegisterUnregister(uint8_t clientId)
{
    return IotcSleSsapcRegisterUnregister(clientId);
}

int32_t SleCtrlSsapcFindStructure(uint8_t clientId, uint16_t connId, IotcAdptSsapcFindStructureParam *param)
{
    return IotcSleSsapcFindStructure(clientId, connId, param);
}

int32_t SleCtrlSsapcReadReq(uint8_t clientId, uint16_t connId, uint16_t handle, uint8_t type)
{
    return IotcSleSsapcReadReq(clientId, connId, handle, type);
}

int32_t SleCtrlSsapcWriteReq(uint8_t clientId, uint16_t connId, IotcAdptSsapcWriteParam *param)
{
    return IotcSleSsapcWriteReq(clientId, connId, param);
}

int32_t SleCtrlSsapcExchangeInfoReq(uint8_t clientId, uint16_t connId, IotcAdptSsapExchangeInfo* param)
{
    return IotcSleSsapcExchangeInfoReq(clientId, connId, param);
}