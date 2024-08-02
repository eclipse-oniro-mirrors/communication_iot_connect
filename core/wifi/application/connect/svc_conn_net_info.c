

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
#include "svc_conn_net_info.h"
#include "utils_json.h"
#include "adapter_wifi.h"
#include "iotc_errcode.h"
#include "comm_def.h"
#include "utils_assert.h"
#include "utils_common.h"
#include "svc_conn_conf.h"
#include "svc_conn_action.h"
#include "securec.h"
#include "event_bus.h"
#include "iotc_event.h"

int32_t ParseAndSaveNetCfgWifiInfo(const AdapterJson *jsonObj)
{
    CHECK_RETURN_LOGW(jsonObj != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    char ssid[ADAPTER_WIFI_SSID_MAX_LEN + 1] = {0};
    char pwd[ADAPTER_WIFI_PWD_MAX_LEN + 1] = {0};
    int32_t ret = UtilsJsonGetString(jsonObj, STR_NETINFO_SSID, ssid, sizeof(ssid));
    if (ret != IOTC_OK) {
        IOTC_LOGE("get ssid error %d", ret);
        return ret;
    }

    ret = UtilsJsonGetString(jsonObj, STR_NETINFO_PASSWORD, pwd, sizeof(pwd));
    if (ret != IOTC_OK) {
        (void)memset_s(ssid, sizeof(ssid), 0, sizeof(ssid));
        IOTC_LOGE("get pwd error %d", ret);
        return ret;
    }

    ret = AdapterSetWifiInfo((const uint8_t *)ssid, strlen(ssid), (const uint8_t *)pwd, strlen(pwd));
    (void)memset_s(ssid, sizeof(ssid), 0, sizeof(ssid));
    (void)memset_s(pwd, sizeof(pwd), 0, sizeof(pwd));
    if (ret != IOTC_OK) {
        IOTC_LOGW("set wifi info error %d", ret);
        return ret;
    }

    IOTC_LOGI("save net info ok");
    return IOTC_OK;
}

static void SoftapStopEventCallback(uint32_t event, void *param, uint32_t len)
{
    NOT_USED(event);
    NOT_USED(param);
    NOT_USED(len);

    int32_t ret = ConnSvcActionConnectWifi();
    if (ret != IOTC_OK) {
        IOTC_LOGW("wifi connect error %d", ret);
        return;
    }
}

int32_t SvcConnSetNetInfo(const AdapterJson *json)
{
    CHECK_RETURN_LOGW(json != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    int32_t ret = ParseAndSaveNetCfgWifiInfo(json);
    if (ret != IOTC_OK) {
        IOTC_LOGW("save net info error %d", ret);
        return ret;
    }

    ret = ConfigSetNetInfoNeedVerify();
    if (ret != IOTC_OK) {
        IOTC_LOGW("set net info verify flag error %d", ret);
        /* 不退出仅告警 因为verify flag不影响wifi连接，且此处退出会有wifi数据残留 */
    }

    if (AdapterGetWifiMode() != ADAPTER_WIFI_MODE_SOFTAP) {
        ret = ConnSvcActionConnectWifi();
    } else {
        ret = EventBusSubscribe(SoftapStopEventCallback, IOTC_CORE_WIFI_EVENT_SOFTAP_STOP);
    }
    if (ret != IOTC_OK) {
        return ret;
    }
    return IOTC_OK;
}