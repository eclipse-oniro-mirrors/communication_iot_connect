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
#include "sle_disc_ctrl.h"
#include "iotc_sle_server.h"
#include "securec.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "iotc_os.h"
#include "utils_common.h"


int32_t SleSeekCtrlParamSet(const IotcAdptSleSeekParam *param)
{
    return IotcSleSetSeekParam(param);
}

int32_t SleSeekCtrlStart(void)
{
    return IotcSleStartSeek();
}

int32_t SleSeekCtrlStop(void)
{
    return IotcSleStoptSeek();
}