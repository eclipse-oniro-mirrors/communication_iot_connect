

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
#include "svc_conn_action.h"
#include "svc_conn_ctx.h"
#include "sched_timer.h"
#include "svc_conn_conf.h"
#include "adapter_wifi.h"
#include "event_bus.h"
#include "utils_bit_map.h"
#include "utils_common.h"
#include "wifi_net_info.h"
#include "utils_assert.h"
#include "iotc_errcode.h"
#include "iotc_event.h"
#include "fwk_main.h"

#define WIFI_VERIFY_CONNECT_TIMEOUT_MS UTILS_SEC_TO_MS(60)
#define WIFI_STATUS_CHECK_PERIOD UTILS_SEC_TO_MS(1)

static void WifiVerifyFailedProcess(void)
{
    int32_t ret = AdapterDeleteWifiInfo();
    if (ret != IOTC_OK) {
        IOTC_LOGF("del wifi info error %d", ret);
    }
    ret = AdapterRestartWifi();
    if (ret != IOTC_OK) {
        IOTC_LOGW("restart wifi error %d", ret);
    }

    ret = ConfigClearNetInfoFlag();
    if (ret != IOTC_OK) {
        IOTC_LOGF("clear net info flag error %d", ret);
    }

    /* 复位回退到待配网 */
    IotcFwkNotifyReset();
}

static void WifiVerifyConnectTimeoutTimerCallback(int32_t id, void *userData)
{
    NOT_USED(id);
    NOT_USED(userData);
    ConnectServiceContext *ctx = GetConnSvcCtx();
    if (WifiIsNetConnected()) {
        SchedTimerRemove(ctx->verifyTimerFd);
        ctx->verifyTimerFd = EVENT_SOURCE_INVALID_TIMER_FD;
        return;
    }

    EventBusPublishSync(IOTC_CORE_WIFI_EVENT_CONNECT_FAIL, NULL, 0);
    WifiVerifyFailedProcess();
    SchedTimerRemove(ctx->verifyTimerFd);
    ctx->verifyTimerFd = EVENT_SOURCE_INVALID_TIMER_FD;
}

int32_t ConnSvcActionConnectWifi(void)
{
    ConnectServiceContext *ctx = GetConnSvcCtx();
    if (ctx->verifyTimerFd >= 0) {
        SchedTimerRemove(ctx->verifyTimerFd);
        ctx->verifyTimerFd = EVENT_SOURCE_INVALID_TIMER_FD;
    }

    IOTC_LOGN("wifi connecting");
    EventBusPublishSync(IOTC_CORE_WIFI_EVENT_CONNECTING, NULL, 0);
    int32_t ret = AdapterConnectWifi();
    if (ret != IOTC_OK) {
        IOTC_LOGW("connect wifi error %d", ret);
        EventBusPublishSync(IOTC_CORE_WIFI_EVENT_CONNECT_FAIL, NULL, 0);
    }

    if (!ConfigIsNetInfoNeedVerify()) {
        return ret;
    }

    if (ret != IOTC_OK) {
        WifiVerifyFailedProcess();
        return ret;
    }

    ctx->verifyTimerFd = SchedTimerAdd(EVENT_SOURCE_TIMER_TYPE_ONCE, WifiVerifyConnectTimeoutTimerCallback,
        WIFI_VERIFY_CONNECT_TIMEOUT_MS, NULL);
    if (ctx->verifyTimerFd < 0) {
        IOTC_LOGW("add verify timer error %d", ctx->verifyTimerFd);
        return ctx->verifyTimerFd;
    }
    return IOTC_OK;
}

static void WifiStatusCheckTimerCallback(int32_t id, void *userData)
{
    NOT_USED(id);
    NOT_USED(userData);

    ConnectServiceContext *ctx = GetConnSvcCtx();
    bool wifiStatusBef = UTILS_IS_BIT_SET(ctx->bitMap, CONN_SVC_BIT_MAP_WIFI_CONNECTED);
    bool wifiStatusCur = WifiIsNetConnected();
    if (wifiStatusCur == wifiStatusBef) {
        return;
    }

    if (!wifiStatusCur) {
        IOTC_LOGI("wifi net disconnect");
        UTILS_BIT_RESET(ctx->bitMap, CONN_SVC_BIT_MAP_WIFI_CONNECTED);
        EventBusPublishSync(IOTC_SDK_AILIFE_EVENT_WIFI_NET_DISCONNECT, NULL, 0);
        return;
    }

    IOTC_LOGI("wifi net connect");
    UTILS_BIT_SET(ctx->bitMap, CONN_SVC_BIT_MAP_WIFI_CONNECTED);
    EventBusPublishSync(IOTC_SDK_AILIFE_EVENT_WIFI_NET_CONNECT, NULL, 0);

    if (ConfigIsNetInfoChecked()) {
        return;
    }

    int32_t ret = ConfigSetNetInfoChecked();
    if (ret != IOTC_OK) {
        IOTC_LOGW("set config info checked error %d", ret);
    }
}

int32_t ConnSvcStartNetCheckTimer(void)
{
    ConnectServiceContext *ctx = GetConnSvcCtx();
    if (ctx->netCheckTimerFd >= 0) {
        SchedTimerRemove(ctx->netCheckTimerFd);
        ctx->netCheckTimerFd = EVENT_SOURCE_INVALID_TIMER_FD;
    }
    ctx->netCheckTimerFd = SchedTimerAdd(EVENT_SOURCE_TIMER_TYPE_REPEAT, WifiStatusCheckTimerCallback,
        WIFI_STATUS_CHECK_PERIOD, NULL);
    if (ctx->netCheckTimerFd < 0) {
        IOTC_LOGW("add wifi check timer error %d", ctx->netCheckTimerFd);
        return ctx->netCheckTimerFd;
    }
    return IOTC_OK;
}