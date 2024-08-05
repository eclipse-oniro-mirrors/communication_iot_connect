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
#include "iotc_oh_wifi.h"
#include <stdarg.h>
#include "core_wifi_init.h"
#include "iotc_errcode.h"
#include "utils_assert.h"
#include "netcfg_service.h"
#include "utils_combo.h"
#include "fwk_main.h"
#include "config_login_info.h"
#include "wifi_net_info.h"
#include "utils_bit_map.h"
#include "utils_common.h"
#include "event_bus.h"
#include "iotc_oh_option.h"
#include "iotc_oh_sdk.h"
#include "trans_buffer.h"
#include "product_adapter.h"
#include "iotc_svc_softap.h"
#include "iotc_svc.h"
#include "iotc_oh_softap_name.h"
#include "fwk_init.h"
#include "iotc_event_inner.h"
#include "iotc_svc_conn.h"
#include "service_proxy.h"

static int32_t StartWifiSoftapSvc(void)
{
    bool isBinded = DevSvcProxyGetBindStatus() == DEV_BIND_STATUS_BIND;
    bool isNoSoftapMode = (GetNetCfgMode() != IOTC_NET_CONFIG_MODE_SOFTAP);
    bool isConnect = ConnSvcProxyIsNetConnected();
    bool isWifiInfoExits = ConnSvcProxyIsWifiInfoExits();
    if (isBinded || isNoSoftapMode || isConnect || isWifiInfoExits) {
        IOTC_LOGI("skip softap service %d/%d/%d/%d", isBinded, isNoSoftapMode, isConnect, isWifiInfoExits);
        return IOTC_OK;
    }

    SoftapSvcInitParam param = {
        .bitMap = UTILS_BIT(IOTC_WIFI_SERVICE_SOFTAP_NETCFG) | UTILS_BIT(IOTC_WIFI_SERVICE_SOFTAP_E2E_CTRL) |
            UTILS_BIT(IOTC_WIFI_SERVICE_SOFTAP_CUSTOM_SSID) | UTILS_BIT(IOTC_WIFI_SERVICE_SOFTAP_TIMEOUT),
        .timeoutMs = GetNetCfgTimeout(),
        .onBuildSsid = IotcOhGetSoftApName,
    };

#if IOTC_CONF_AILIFE_SUPPORT
    UTILS_BIT_RESET(param.bitMap, IOTC_WIFI_SERVICE_SOFTAP_CUSTOM_SSID);
    param.onBuildSsid = NULL;
#endif

    int32_t ret = ServiceProxyStartService(IOTC_SERVICE_ID_SOFTAP, &param);
    if (ret != IOTC_OK) {
        IOTC_LOGE("start softap service error");
        return ret;
    }
    return IOTC_OK;
}

static void EventBusStartWifiSvcCallback(uint32_t event, void *param, uint32_t len)
{
    NOT_USED(event);
    NOT_USED(param);
    NOT_USED(len);

    int32_t ret = ServiceProxyStartService(IOTC_SERVICE_ID_WIFI_CONNECT, NULL);
    if (ret != IOTC_OK) {
        IOTC_LOGE("start connect service error %d", ret);
        return;
    }

    ret = StartWifiSoftapSvc();
    if (ret != IOTC_OK) {
        return;
    }

    ret = ServiceProxyStartService(IOTC_SERVICE_ID_LOCAL_CONTROL, NULL);
    if (ret != IOTC_OK) {
        IOTC_LOGE("start local control service error %d", ret);
        return;
    }

    ret = ServiceProxyStartService(IOTC_SERVICE_ID_LAN_SEARCH, NULL);
    if (ret != IOTC_OK) {
        IOTC_LOGE("start lan search error %d", ret);
        return;
    }

    return;
}

static int32_t IotcOhWifiMainInit(void)
{
    int32_t ret = EventBusSubscribe(EventBusStartWifiSvcCallback, IOTC_EVENT_INNER_DEVICE_SVC_START);
    if (ret != IOTC_OK) {
        IOTC_LOGW("sub dev svc error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static void IotcOhWifiMainDeinit(void)
{
    (void)EventBusUnsubscribe(EventBusStartWifiSvcCallback);
    (void)ServiceProxyStopService(IOTC_SERVICE_ID_SOFTAP, NULL);
    (void)ServiceProxyStopService(IOTC_SERVICE_ID_WIFI_CONNECT, NULL);
    (void)ServiceProxyStopService(IOTC_SERVICE_ID_LOCAL_CONTROL, NULL);
    (void)ServiceProxyStopService(IOTC_SERVICE_ID_LAN_SEARCH, NULL);
}

static const FwkInitUnit OH_WIFI[] = {
    { FWK_INIT_LVL_BIZ, "wifi_main", IotcOhWifiMainInit, IotcOhWifiMainDeinit },
};

static int32_t OptionSetWifiSendBufferSize(va_list args)
{
    uint32_t resSize = va_arg(args, uint32_t);
    uint32_t maxSize = va_arg(args, uint32_t);
    if (maxSize ==0 || maxSize < resSize) {
        IOTC_LOGW("buffer size error %u/%u", maxSize, resSize);
        return IOTC_ERR_PARAM_INVALID;
    }

    TransSetSendBufferSize(resSize, maxSize);
    IOTC_LOGN("set send buffer size %u/%u", resSize, maxSize);
    return IOTC_OK;
}

static int32_t OptionSetWifiRecvBufferSize(va_list args)
{
    uint32_t resSize = va_arg(args, uint32_t);
    uint32_t maxSize = va_arg(args, uint32_t);
    if (maxSize ==0 || maxSize < resSize) {
        IOTC_LOGW("buffer size error %u/%u", maxSize, resSize);
        return IOTC_ERR_PARAM_INVALID;
    }

    TransSetRecvBufferSize(resSize, maxSize);
    IOTC_LOGN("set recv buffer size %u/%u", resSize, maxSize);
    return IOTC_OK;
}

static int32_t OptionSetWifiNetcfgMode(va_list args)
{
    int32_t mode = va_arg(args, int32_t);
    SetNetCfgMode((IotcNetConfigMode)mode);
    IOTC_LOGN("set netcfg mode %d", mode);
    return IOTC_OK;
}

static int32_t OptionSetWifiNetcfgTimeout(va_list args)
{
    uint32_t timeout = va_arg(args, uint32_t);
    SetNetCfgTimeout(timeout);
    IOTC_LOGN("set netcfg timeout %u", timeout);
    return IOTC_OK;
}

static int32_t OptionSetGetCertCallback(va_list args)
{
    IotcOhWifiGetCertCallback cb = va_arg(args, IotcOhWifiGetCertCallback);
    CHECK_RETURN_LOGE(cb != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    ProductHooks hooks = {0};
    hooks.onGetRootCaCert = cb;
    int32_t ret = ProductRegisterHooks(&hooks, PROD_HOOK_REG_POLICY_COVER_NON_NULL);
    if (ret != IOTC_OK) {
        IOTC_LOGW("set cert cb error %d", ret);
        return ret;
    }

    IOTC_LOGN("set root cert cb");
    return IOTC_OK;
}

static const OptionItem WIFI_OPTION_TABLE[] = {
    { IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, OptionSetWifiSendBufferSize },
    { IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, OptionSetWifiRecvBufferSize },
    { IOTC_OH_OPTION_WIFI_NETCFG_MODE, OptionSetWifiNetcfgMode },
    { IOTC_OH_OPTION_WIFI_NETCFG_TIMEOUT, OptionSetWifiNetcfgTimeout },
    { IOTC_OH_OPTION_WIFI_GET_CERT_CALLBACK, OptionSetGetCertCallback },
};

int32_t IotcOhWifiEnable(void)
{
    IOTC_LOGN("iotc oh wifi enable");
    CHECK_MAIN_RUNNING_RETURN();

    int32_t ret = IotcOhOptionRegister(WIFI_OPTION_TABLE, ARRAY_SIZE(WIFI_OPTION_TABLE));
    if (ret != IOTC_OK) {
        IOTC_LOGE("reg wifi option error %d", ret);
        return ret;
    }

    ret = FwkRegInitUnitArray(OH_WIFI);
    if (ret != IOTC_OK) {
        IOTC_LOGE("reg oh wifi init unit error %d", ret);
        return ret;
    }

    /* depend on wifi core */
    ret = CoreWifiRegisterInitItem();
    if (ret != IOTC_OK) {
        IOTC_LOGE("wifi core init error %d", ret);
        return ret;
    }
    UtilsComboSetWifiFlag(true);
    return IOTC_OK;
}

int32_t IotcOhWifiDisable(void)
{
    IOTC_LOGN("iotc oh wifi disable");
    CHECK_MAIN_RUNNING_RETURN();

    FwkUnregInitUnit(OH_WIFI);
    CoreWifiUnregisterInitItem();
    IotcOhOptionUnregister(WIFI_OPTION_TABLE);
    UtilsComboSetWifiFlag(false);
    return IOTC_OK;
}