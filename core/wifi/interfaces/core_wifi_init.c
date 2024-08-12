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
#include "core_wifi_init.h"
#include "fwk_init.h"
#include "wifi_sched_fd_watch.h"
#include "trans_buffer_inner.h"
#include "utils_common.h"
#include "wifi_svc_softap.h"
#include "wifi_svc_conn.h"
#include "utils_assert.h"
#include "iotc_errcode.h"
#include "lan_search_svc.h"
#include "local_ctl_svc.h"
#include "m2m_cloud_svc.h"

static const FwkInitUnit CORE_WIFI[] = {
    /* 链路收发缓冲区组件 */
    {FWK_INIT_LVL_COMP, "wifi_buffer", TransBufferInit, TransBufferDeinit},
    /* 套接字监听及时间分发框架 */
    {FWK_INIT_LVL_FWK, "wifi_fd_watch", WifiSchedFdWatchInit, WifiSchedFdWatchDeinit},
    /* softap配网模式 */
    {FWK_INIT_LVL_BIZ, "softap", WifiServiceSoftapInit, WifiServiceSoftapDeinit},
    {FWK_INIT_LVL_BIZ, "wifi_connect", WifiServiceConnectInit, WifiServiceConnectDeinit},
    {FWK_INIT_LVL_BIZ, "lan_search", LanSearchServiceInit, LanSearchServiceDeinit},
    {FWK_INIT_LVL_BIZ, "local_control", LocalControlServiceInit, LocalControlServiceDeinit},
    {FWK_INIT_LVL_BIZ, "cloud", M2mCloudServiceInit, M2mCloudServiceDeinit},
};

int32_t CoreWifiRegisterInitItem(void)
{
    int32_t ret = FwkRegInitUnitArray(CORE_WIFI);
    if (ret != IOTC_OK) {
        IOTC_LOGE("reg wifi core unit error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

void CoreWifiUnregisterInitItem(void)
{
    FwkUnregInitUnit(CORE_WIFI);
}