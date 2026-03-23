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

/* SLE SSAPC application-layer client stubs */

#include "sle_ssap_service.h"
#include "iotc_errcode.h"
#include "iotc_log.h"

int32_t SleScanServiceStop(void)
{
    return IOTC_OK;
}

int32_t SleAdvServiceStart(uint32_t ms)
{
    IOTC_LOGD("SleAdvServiceStart %d", ms);
    return IOTC_OK;
}

int32_t SleAdvServiceStop(void)
{
    return IOTC_OK;
}

void SleAdvSetType(SleSvcAdvDataType type)
{
    IOTC_LOGD("SleAdvSetType %d", type);
}
