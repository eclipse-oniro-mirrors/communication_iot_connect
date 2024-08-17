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
#include "ble_profile.h"
#include "ble_svc_net_cfg_ver.h"
#include "ble_svc_device_info.h"
#include "ble_svc_auth_setup.h"
#include "ble_svc_clear_dev_reg_info.h"
#include "ble_svc_speke.h"
#include "ble_svc_netcfg.h"
#include "ble_svc_create_session.h"
#include "ble_svc_custom_sec_data.h"
#include "securec.h"
#include "utils_assert.h"
#include "adapter_json.h"
#include "utils_common.h"
#include "ble_linklayer.h"
#include "iotc_errcode.h"

#define BLE_SVC_NET_CFG_VER_IDX 80
#define BLE_SVC_DEVICE_INFO_IDX 81
#define BLE_SVC_AUTH_SETUP_IDX 83
#define BLE_SVC_CLEAR_REGINFO_IDX 85
#define BLE_SVC_SPEKE_IDX 67
#define BLE_SVC_NETCFG_IDX 68
#define BLE_SVC_CREATE_SESSION_IDX 84
#define BLE_SVC_CUSTOM_SEC_DATA_IDX 79

static const BtSvcInfo g_svcInfoTab[] = {
    {.svcIdx = BLE_SVC_NET_CFG_VER_IDX, .service = BLE_SVC_NET_CFG_VER, .suppEncType = ENC_SUPP_PLAIN,
        .getFunc = GetBleSvcNetCfgVer, .putFunc = NULL},
    {.svcIdx = BLE_SVC_DEVICE_INFO_IDX, .service = BLE_SVC_DEVICE_INFO, .suppEncType = ENC_SUPP_SPEKE_SESSKEY,
        .getFunc = GetBleSvcDeviceInfo, .putFunc = NULL},
    {.svcIdx = BLE_SVC_AUTH_SETUP_IDX, .service = BLE_SVC_AUTH_SETUP, .suppEncType = ENC_SUPP_SPEKE,
        .getFunc = GetBleSvcAuthSetup, .putFunc = NULL},
    {.svcIdx = BLE_SVC_CLEAR_REGINFO_IDX, .service = BLE_SVC_CLEAR_REGINFO, .suppEncType = ENC_SUPP_SPEKE_SESSKEY,
        .getFunc = NULL, .putFunc = PutBleSvcClearDevRegInfo},
    {.svcIdx = BLE_SVC_SPEKE_IDX, .service = BLE_SVC_SPEKE, .suppEncType = ENC_SUPP_PLAIN,
        .getFunc = NULL, .putFunc = PutBleSvcSpeke},
    {.svcIdx = BLE_SVC_NETCFG_IDX, .service = BLE_SVC_NETCFG, .suppEncType = ENC_SUPP_SPEKE,
        .getFunc = NULL, .putFunc = PutBleSvcNetCfg},
    {.svcIdx = BLE_SVC_CREATE_SESSION_IDX, .service = BLE_SVC_CREATE_SESSION, .suppEncType = ENC_SUPP_PLAIN,
        .getFunc = GetBleSvcCreateSession, .putFunc = NULL},
    {.svcIdx = BLE_SVC_CUSTOM_SEC_DATA_IDX, .service = BLE_SVC_CUSTOM_SEC_DATA, .suppEncType = ENC_SUPP_SPEKE_SESSKEY,
        .getFunc = NULL, .putFunc = PutBleSvcCustomSecData},
};

int32_t BleProfileInit(void)
{
    int32_t ret = LinkLayerServiceRegister(g_svcInfoTab, ARRAY_SIZE(g_svcInfoTab));
    if (ret != IOTC_OK) {
        IOTC_LOGW("reg linklayer svc error %d", ret);
    }
    return ret;
}

void BleProfileDeinit(void)
{
    LinkLayerServiceRelease();
}