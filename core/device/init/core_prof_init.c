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
#include "core_prof_init.h"
#include "iotc_errcode.h"
#include "fwk_init.h"
#include "dev_info.h"
#include "svc_info.h"
#include "utils_assert.h"
#include "profile_report.h"
#include "utils_common.h"
#include "security_key.h"
#include "dfx_watch_dog.h"
#include "iotc_dev_svc.h"
#include "config_revoke.h"
#include "config_register_info.h"
#include "product_adapter.h"
#include "security_random.h"

static int32_t RegkeyHook(void);
static int32_t WatchDogRebootInit(void);

static const FwkInitUnit CORE_PROFILE[] = {
    {FWK_INIT_LVL_DEP, "report", ProfileReportInit, ProfileReportDeinit},
    {FWK_INIT_LVL_DEP, "key_hook", RegkeyHook, NULL},
    {FWK_INIT_LVL_DEP, "reboot_hook", WatchDogRebootInit, NULL},
    {FWK_INIT_LVL_DEP, "reg_info", ConfigClearRegisterInfo, NULL},
    {FWK_INIT_LVL_FWK, "config_revoke", ConfigRevokeInit, ConfigRevokeDeinit},
    {FWK_INIT_LVL_FWK, "device_service", DeviceServiceInit, DeviceServiceDeinit},
};

static int32_t RegkeyHook(void)
{
    SecurityRegUdidCallback(ModelGetUdid);
    SecurityRegAcKeyCallback(ProductProfGetAcKey);
    uint32_t temp = 0;
    if (ProductDevTrng((uint8_t *)&temp, sizeof(temp)) == IOTC_OK) {
        SecurityRandomRegTrng(ProductDevTrng);
    }
    return IOTC_OK;
}

static void WatchDogTimeoutHandler(void)
{
    int32_t ret = ProductDevReboot(IOTC_REBOOT_WATCH_DOG_TIMEOUT);
    if (ret != IOTC_OK) {
        IOTC_LOGE("watch dog reboot error %d", ret);
    }
}

static int32_t WatchDogRebootInit(void)
{
    DfxWatchDogRegCommTimeoutHandler(WatchDogTimeoutHandler);
    return IOTC_OK;
}

int32_t CoreDeviceRegisterInitUnit(void)
{
    return FwkRegInitUnitArray(CORE_PROFILE);
}

void CoreDeviceUnregisterInitUnit(void)
{
    FwkUnregInitUnit(CORE_PROFILE);
}