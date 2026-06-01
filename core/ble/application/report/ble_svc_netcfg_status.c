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
#include <stddef.h>
#include "ble_svc_netcfg_status.h"
#include "utils_assert.h"
#include "utils_common.h"
#include "utils_json.h"
#include "event_bus_sub.h"
#include "ble_linklayer.h"
#include "ble_profile.h"
#include "iotc_event.h"
#include "comm_def.h"
#include "iotc_errcode.h"
#include <string.h>

/* 蓝牙配网上报状态 */
enum {
    NETCFG_STATUS_WIFI_CONNECTED = 100,
    NETCFG_STATUS_WIFI_WRONG_KEY = 101,
    NETCFG_STATUS_WIFI_CONNECT_FAIL = 102,
    NETCFG_STATUS_WIFI_CONNECTING = 104,
    NETCFG_STATUS_REG_IOT_OK = 200,
    NETCFG_STATUS_REG_IOT_FAIL = 202,
    NETCFG_STATUS_DEINIT_BLE_STACK = 350,
    NETCFG_STATUS_ERR = 999,
};

static int32_t GenNetCfgKeyValueJsonStr(char *data, char **out, uint32_t *outLen)
{
 	if ((data == NULL) || (out == NULL) || (outLen == NULL)) {
 	    IOTC_LOGE("invalid param");
 	    return IOTC_ERR_PARAM_INVALID;
 	}
 	char *key = "service";
 	char *value = "netCfg";
 	IotcJson *obj = IotcJsonParse((const char *)data);
 	if (obj == NULL) {
 	    IOTC_LOGE("json parse");
 	    return IOTC_ADAPTER_JSON_ERR_PARSE;
 	}
 	int32_t ret = IotcJsonAddStr2Obj(obj, key, value);
 	if (ret != IOTC_OK) {
 	    IOTC_LOGW("add num to obj err %d", ret);
 	    IotcJsonDelete(obj);
 	    return IOTC_ADAPTER_JSON_ERR_ADD;
 	}
 	*out = UtilsJsonPrintByMalloc(obj);
 	IotcJsonDelete(obj);
 	if (*out == NULL) {
 	    IOTC_LOGW("json print err");
 	    return IOTC_CORE_COMM_UTILS_ERR_JSON_MALLOC_PRINT;
 	}
 	*outLen = strlen(*out);
 	return IOTC_OK;
}

static void BleNetcfgRptStatus(uint32_t event, void *param, uint32_t len)
{
    NOT_USED(param);
    NOT_USED(len);
    char *out = NULL;
    uint32_t outLen = 0;
    int32_t status = 0;
    char *netCfgOut = NULL;
    uint32_t netCfgOutLen = 0;
    if (event == IOTC_CORE_WIFI_EVENT_CONNECTING) {
        status = NETCFG_STATUS_WIFI_CONNECTING;
    } else if (event == IOTC_CORE_WIFI_EVENT_CONNECTED) {
        status = NETCFG_STATUS_WIFI_CONNECTED;
    } else if (event == IOTC_CORE_WIFI_EVENT_CONNECT_FAIL) {
        status = NETCFG_STATUS_WIFI_CONNECT_FAIL;
    } else if (event == IOTC_SDK_AILIFE_EVENT_WIFI_UPLINK_ONLINE) {
        status = NETCFG_STATUS_REG_IOT_OK;
    }else if (event == IOTC_SDK_AILIFE_EVENT_WIFI_UPLINK_REGISTER_FAILED) {
        status = NETCFG_STATUS_REG_IOT_FAIL;
    }    

    int32_t ret = UtilsGenKeyIntValueJsonStr(STR_JSON_STATUS, status, &out, &outLen);
    if ((ret != IOTC_OK) || (out == NULL) || (outLen == 0)) {
        IOTC_LOGE("ret=%d", ret);
        return;
    }
    // WIFI连接状态（json格式），例：{"status":100, "service":"netCfg"}
 	ret = GenNetCfgKeyValueJsonStr(out, &netCfgOut, &netCfgOutLen);
 	if ((ret != IOTC_OK) || (netCfgOut == NULL) || (netCfgOutLen == 0)) {
 	    IotcFree(out);
 	    IOTC_LOGE("ret=%d", ret);
 	    return;
 	}
 	ret = LinkLayerReportSvcDataEnc(BLE_SVC_NETCFG, (const uint8_t *)netCfgOut, netCfgOutLen);
 	IOTC_LOGN("ret=%d, netCfgOut=%s", ret, netCfgOut);
    IotcFree(out);
    IotcFree(netCfgOut);
}

int32_t BleSvcNetCfgInit(void)
{
    const uint32_t EVENT_SUBS[] = {
        IOTC_CORE_WIFI_EVENT_CONNECTING,
        IOTC_CORE_WIFI_EVENT_CONNECTED,
        IOTC_CORE_WIFI_EVENT_CONNECT_FAIL,
        IOTC_SDK_AILIFE_EVENT_WIFI_UPLINK_REGISTER_FAILED,
        IOTC_SDK_AILIFE_EVENT_WIFI_UPLINK_ONLINE,
    };
    int32_t ret;
    for (uint32_t i = 0 ; i < ARRAY_SIZE(EVENT_SUBS); ++i) {
        ret = EventBusSubscribe(BleNetcfgRptStatus, EVENT_SUBS[i]);
        if (ret != IOTC_OK) {
            IOTC_LOGW("sub event error %d", ret);
            return ret;
        }
    }

    return IOTC_OK;
}

void BleSvcNetCfgDeinit(void)
{
    EventBusUnsubscribe(BleNetcfgRptStatus);
}