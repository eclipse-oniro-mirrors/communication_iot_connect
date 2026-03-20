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
#include "sle_profile.h"
#include "sle_svc_net_cfg_ver.h"
#include "sle_svc_device_info.h"
#include "sle_svc_auth_setup.h"
#include "sle_svc_clear_dev_reg_info.h"
#include "sle_svc_speke.h"
#include "sle_svc_netcfg.h"
#include "sle_svc_create_session.h"
#include "sle_svc_custom_sec_data.h"
#include "securec.h"
#include "utils_assert.h"
#include "iotc_json.h"
#include "utils_common.h"
#include "sle_linklayer.h"
#include "iotc_errcode.h"

#define SLE_SVC_NET_CFG_VER_IDX 80
#define SLE_SVC_DEVICE_INFO_IDX 81
#define SLE_SVC_AUTH_SETUP_IDX 83
#define SLE_SVC_CLEAR_REGINFO_IDX 85
#define SLE_SVC_SPEKE_IDX 67
#define SLE_SVC_NETCFG_IDX 68
#define SLE_SVC_CREATE_SESSION_IDX 84
#define SLE_SVC_CUSTOM_SEC_DATA_IDX 79

static const SleSvcInfo g_svcInfoTab[] = {
    {.svcIdx = SLE_SVC_NET_CFG_VER_IDX, .service = SLE_SVC_NET_CFG_VER, .suppEncType = SLE_ENC_SUPP_PLAIN,
        .getSleFunc = GetSleSvcNetCfgVer, .putSleFunc = NULL},
    {.svcIdx = SLE_SVC_DEVICE_INFO_IDX, .service = SLE_SVC_DEVICE_INFO, .suppEncType = SLE_ENC_SUPP_SPEKE_SESSKEY,
        .getSleFunc = GetSleSvcDeviceInfo, .putSleFunc = NULL},
    {.svcIdx = SLE_SVC_AUTH_SETUP_IDX, .service = SLE_SVC_AUTH_SETUP, .suppEncType = SLE_ENC_SUPP_SPEKE,
        .getSleFunc = GetSleSvcAuthSetup, .putSleFunc = NULL},
    {.svcIdx = SLE_SVC_CLEAR_REGINFO_IDX, .service = SLE_SVC_CLEAR_REGINFO, .suppEncType = SLE_ENC_SUPP_SPEKE_SESSKEY,
        .getSleFunc = NULL, .putSleFunc = PutSleSvcClearDevRegInfo},
    {.svcIdx = SLE_SVC_SPEKE_IDX, .service = SLE_SVC_SPEKE, .suppEncType = SLE_ENC_SUPP_PLAIN,
        .getSleFunc = NULL, .putSleFunc = PutSleSvcSpeke},
    {.svcIdx = SLE_SVC_NETCFG_IDX, .service = SLE_SVC_NETCFG, .suppEncType = SLE_ENC_SUPP_SPEKE,
        .getSleFunc = NULL, .putSleFunc = PutSleSvcNetCfg},
    {.svcIdx = SLE_SVC_CREATE_SESSION_IDX, .service = SLE_SVC_CREATE_SESSION, .suppEncType = SLE_ENC_SUPP_PLAIN,
        .getSleFunc = GetSleSvcCreateSession, .putSleFunc = NULL},
    {.svcIdx = SLE_SVC_CUSTOM_SEC_DATA_IDX, .service = SLE_SVC_CUSTOM_SEC_DATA,
        .suppEncType = SLE_ENC_SUPP_SPEKE_SESSKEY,
        .getSleFunc = NULL, .putSleFunc = PutSleSvcCustomSecData},
};

int32_t SleProfileInit(void)
{
    int32_t ret = SleLinkLayerServiceRegister(g_svcInfoTab, ARRAY_SIZE(g_svcInfoTab));
    if (ret != IOTC_OK) {
        IOTC_LOGW("reg linklayer svc error %d", ret);
    }
    return ret;
}

void SleProfileDeinit(void)
{
    SleLinkLayerServiceRelease();
}