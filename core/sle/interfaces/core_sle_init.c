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
#include "core_sle_init.h"
#include "fwk_init.h"
#include "iotc_log.h"
#include "utils_common.h"
#include "sle_svc.h"
#include "iotc_errcode.h"

static const FwkInitUnit CORE_SLE[] = {
    {FWK_INIT_LVL_BIZ, "sle_svc", SleConnectServiceInit, SleConnectServiceDeinit},
};

int32_t SleCoreRegisterInitUnit(void)
{
    int32_t ret = FwkRegInitUnits(CORE_SLE, ARRAY_SIZE(CORE_SLE), TO_STR(CORE_SLE));
    if (ret != IOTC_OK) {
        IOTC_LOGW("reg sle core init unit error %d", ret);
    }
    return ret;
}

void SleCoreUnregisterInitUnit(void)
{
    FwkUnregInitUnit(CORE_SLE);
}