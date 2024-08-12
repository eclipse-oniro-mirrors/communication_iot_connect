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
#include "wifi_svc_conn.h"
#include "iotc_svc.h"
#include "iotc_log.h"
#include "sched_timer.h"
#include "utils_assert.h"
#include "svc_conn_ctx.h"
#include "service_proxy.h"
#include "svc_conn_net_info.h"
#include "svc_conn_action.h"
#include "utils_common.h"
#include "svc_conn_conf.h"
#include "iotc_svc_conn.h"
#include "wifi_net_info.h"
#include "iotc_errcode.h"
#include "event_bus.h"
#include "iotc_event.h"
#include "adapter_wifi.h"
#include "fwk_main.h"

static const char *NET_CONNECT_SERVICE_NAME = "CONNECT";

static void RevokeEventWifiInfoCallback(uint32_t event, void *param, uint32_t len)
{
    NOT_USED(event);
    NOT_USED(param);
    NOT_USED(len);
    if (!ConfigIsNetInfoNeedVerify() && !ConfigIsNetInfoChecked()) {
        IOTC_LOGI("no wifi info need clear");
        return;
    }

    int32_t ret = AdapterDeleteWifiInfo();
    if (ret != IOTC_OK) {
        IOTC_LOGW("del wifi info error %d", ret);
    } else {
        IOTC_LOGI("clear wifi info ok");
    }

    ret = AdapterRestartWifi();
    if (ret != IOTC_OK) {
        IOTC_LOGW("restart wifi error %d", ret);
    }

    ret = ConfigClearNetInfoFlag();
    if (ret != IOTC_OK) {
        IOTC_LOGF("clear net info flag error %d", ret);
    }

    /* ¸´Î»»ØÍËµ½´ýÅäÍø */
    IotcFwkNotifyReset();
}

static int32_t StopWifiConnectService(int32_t instanceId, void *param)
{
    NOT_USED(param);
    ConnectServiceContext *ctx = GetConnSvcCtx();
    if (ctx->instanceId != instanceId) {
        IOTC_LOGW("invalid instance id %d/%d", instanceId, ctx->instanceId);
        return IOTC_ERR_PARAM_INVALID;
    }

    if (ctx->onFinish != NULL) {
        ctx->onFinish(instanceId);
    }
    ConnSvcCtxDeinit();
    ConnectConfigInfoDeinit();
    (void)EventBusUnsubscribe(RevokeEventWifiInfoCallback);
    return IOTC_OK;
}

static int32_t StartWifiConnectService(int32_t instanceId, ServiceFinishCallback onFinish, void *param)
{
    CHECK_RETURN_LOGW(onFinish != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    ConnectServiceContext *ctx = GetConnSvcCtx();

    ctx->instanceId = instanceId;
    ctx->onFinish = onFinish;

    int32_t ret = ConnectConfigInfoInit();
    if (ret != IOTC_OK) {
        IOTC_LOGW("connect config init error %d", ret);
        return ret;
    }

    if (ConfigIsNetInfoNeedVerify() || ConfigIsNetInfoChecked()) {
        ret = ConnSvcActionConnectWifi();
        if (ret != IOTC_OK) {
            IOTC_LOGW("conn svc connect wifi error %d", ret);
            return ret;
        }
    }

    ret = EventBusSubscribe(RevokeEventWifiInfoCallback, IOTC_CORE_COMM_EVENT_MAIN_RESTORE);
    if (ret != IOTC_OK) {
        IOTC_LOGW("sub restore event error %d", ret);
        return ret;
    }

    ret = ConnSvcStartNetCheckTimer();
    if (ret != IOTC_OK) {
        IOTC_LOGW("start net check timer error %d", ret);
        return ret;
    }

    return IOTC_OK;
}

int32_t WifiServiceConnectInit(void)
{
    static const ServiceHandler CONNECT_SERVICE_HANDLER = {
        .onStart = StartWifiConnectService,
        .onStop = StopWifiConnectService,
        .onReset = NULL,
    };

    static const ConnSvcApi CONN_SVC_API = {
        .isNetConnected = WifiIsNetConnected,
        .isWifiExist = WifiIsNetInfoExit,
        .onSetNetInfo = SvcConnSetNetInfo,
    };

    ServiceInstance instance = {
        .serviceId = IOTC_SERVICE_ID_WIFI_CONNECT,
        .name = NET_CONNECT_SERVICE_NAME,
        .handler = &CONNECT_SERVICE_HANDLER,
        .msgNum = 0,
        .msgIds = NULL,
        .apiHandler = &CONN_SVC_API,
    };

    int32_t ret = ServiceManagerRegisterInstance(&instance);
    if (ret != IOTC_OK) {
        IOTC_LOGW("reg connect service instance error %d", ret);
        return ret;
    }

    ret = ConnSvcCtxInit();
    if (ret != IOTC_OK) {
        IOTC_LOGW("ctx init error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

void WifiServiceConnectDeinit(void)
{
    ConnectConfigInfoDeinit();
    ConnSvcCtxDeinit();
    ServiceManagerUnregisterInstance(IOTC_SERVICE_ID_WIFI_CONNECT);
}