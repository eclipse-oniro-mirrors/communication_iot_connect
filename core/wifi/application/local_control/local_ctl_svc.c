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
#include "local_ctl_svc.h"
#include "iotc_errcode.h"
#include "iotc_svc.h"
#include "local_ctl_ctx.h"
#include "utils_assert.h"
#include "iotc_svc_local_ctl.h"
#include "utils_common.h"
#include "local_ctl_coap_stack.h"
#include "local_ctl_cli_mngr.h"
#include "event_bus.h"
#include "iotc_event.h"
#include "iotc_svc_dev.h"
#include "iotc_svc_conn.h"
#include "service_proxy.h"
#include "local_ctl_report.h"

#define LOCAL_CONTROL_SERVICE_NAME "LOCAL_CTL"
#define LOCAL_CONTROL_DEFAULT_CLIENT_NUM 10
#define LOCAL_CONTROL_DEFAULT_CLIENT_EXPIRE_TIME UTILS_HOUR_TO_MS(15 * 24)

static int32_t LocalControlConfigSetup(const LocalCtlInitParam *conf, LocalControlContext *ctx)
{
    if (conf == NULL) {
        ctx->config.maxClientNum = LOCAL_CONTROL_DEFAULT_CLIENT_NUM;
        ctx->config.clientExpireTime = LOCAL_CONTROL_DEFAULT_CLIENT_EXPIRE_TIME;
    } else {
        ctx->config.maxClientNum = conf->clientMaxNum;
        ctx->config.clientExpireTime = conf->clientExpireTimeMs;
    }

    IOTC_LOGI("local control conf %u/%u", ctx->config.maxClientNum, ctx->config.clientExpireTime);
    return IOTC_OK;
}

static int32_t LocalCtlDeviceReportMessageHandler(const ServiceMessage *req, ServiceResponseInfo *resp)
{
    NOT_USED(resp);
    CHECK_RETURN_LOGW(req != NULL && req->msg != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    int32_t ret = LocalCtlReportToAllClient(req->msg, GetLocalCtlCtx());
    if (ret != IOTC_OK) {
        IOTC_LOGW("local ctl report error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static int32_t LocalCtlServiceMsgSub(LocalControlContext *ctx)
{
    int32_t ret = ServiceProxySubscribeMessage(ctx->svcInfo.instanceId, IOTC_SERVICE_ID_DEVICE,
        DEVICE_SERVICE_MSG_ID_REPORT, LocalCtlDeviceReportMessageHandler);
    if (ret != IOTC_OK) {
        IOTC_LOGW("sub device service report error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static void LocalControlServiceStopInner(LocalControlContext *ctx)
{
    LocalControlCoapStackDestroy(ctx);
    LocalCtlClientManagerDestroy(ctx);
}

static void EventSubForLocalCoapServerStart(uint32_t event, void *param, uint32_t len)
{
    NOT_USED(event);
    NOT_USED(param);
    NOT_USED(len);

    DevBindStatus bindStatus = DevSvcProxyGetBindStatus();
    bool isWifiConnected = ConnSvcProxyIsNetConnected();
    if (bindStatus != DEV_BIND_STATUS_BIND || !isWifiConnected) {
        return;
    }

    int32_t ret = LocalControlCoapStackStart(GetLocalCtlCtx());
    if (ret != IOTC_OK) {
        IOTC_LOGW("local coap server start error %d", ret);
    }
}

static void EventSubForLocalCoapServerStop(uint32_t event, void *param, uint32_t len)
{
    NOT_USED(event);
    NOT_USED(param);
    NOT_USED(len);

    LocalControlCoapStackStop(GetLocalCtlCtx());
}

static void EventSubForStopCoapServerAndClearAllClient(uint32_t event, void *param, uint32_t len)
{
    NOT_USED(event);
    NOT_USED(param);
    NOT_USED(len);

    LocalControlContext *ctx = GetLocalCtlCtx();
    LocalControlCoapStackStop(ctx);
    ClearAllLocalControlClient(ctx);
}

static bool LocalCoapStartEventMatch(uint32_t event)
{
    const int32_t startEvents[] = {
        IOTC_SDK_AILIFE_EVENT_WIFI_UPLINK_REGISTERED,
        IOTC_SDK_AILIFE_EVENT_WIFI_NET_CONNECT,
    };
    for (uint32_t i = 0 ; i < ARRAY_SIZE(startEvents); ++i) {
        if (event == startEvents[i]) {
            return true;
        }
    }
    return false;
}

static int32_t LocalControlEventSub(void)
{
    /* 本地控通道在满足绑定且LAN连接的情况下才会启动，当前不满足则监听对应事件启动 */
    int32_t ret = EventBusSubscribeMatch(EventSubForLocalCoapServerStart, LocalCoapStartEventMatch);
    if (ret != IOTC_OK) {
        IOTC_LOGW("sub coap start event error %d", ret);
        return ret;
    }

    ret = EventBusSubscribe(EventSubForLocalCoapServerStop, IOTC_SDK_AILIFE_EVENT_WIFI_NET_DISCONNECT);
    if (ret != IOTC_OK) {
        IOTC_LOGW("sub coap stop event error %d", ret);
        return ret;
    }

    ret = EventBusSubscribe(EventSubForStopCoapServerAndClearAllClient,
        IOTC_SDK_AILIFE_EVENT_WIFI_UPLINK_UNREGISTER);
    if (ret != IOTC_OK) {
        IOTC_LOGW("sub clear client event error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

/* param可以为空，为空时使用默认配置 */
static int32_t LocalControlServiceStart(int32_t instanceId, ServiceFinishCallback onFinish, void *param)
{
    CHECK_RETURN_LOGW(onFinish != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    LocalControlContext *ctx = GetLocalCtlCtx();
    CHECK_RETURN_LOGE(ctx != NULL, IOTC_ERR_CONTEXT_NULL, "local ctl ctx null");

    int32_t ret = LocalControlConfigSetup(param, ctx);
    if (ret != IOTC_OK) {
        IOTC_LOGW("conf setup error %d", ret);
        return ret;
    }

    ctx->svcInfo.instanceId = instanceId;
    ctx->svcInfo.onFinish = onFinish;

    do {
        ret = LocalCtlClientManagerInit(ctx);
        if (ret != IOTC_OK) {
            IOTC_LOGW("client manager init error %d", ret);
            break;
        }

        ret = LocalControlEventSub();
        if (ret != IOTC_OK) {
            break;
        }

        DevBindStatus bindStatus = DevSvcProxyGetBindStatus();
        bool isWifiConnected = ConnSvcProxyIsNetConnected();
        if (bindStatus == DEV_BIND_STATUS_BIND && isWifiConnected) {
            ret = LocalControlCoapStackStart(ctx);
            if (ret != IOTC_OK) {
                IOTC_LOGW("start local coap server error %d", ret);
                break;
            }
        }

        ret = LocalCtlServiceMsgSub(ctx);
        if (ret != IOTC_OK) {
            IOTC_LOGW("sub report msg error %d", ret);
            break;
        }
        IOTC_LOGI("start local control service");
        return IOTC_OK;
    } while (0);
    LocalControlServiceStopInner(ctx);
    return ret;
}

static int32_t LocalControlServiceStop(int32_t instanceId, void *param)
{
    NOT_USED(param);
    LocalControlContext *ctx = GetLocalCtlCtx();
    CHECK_RETURN_LOGE(ctx != NULL, IOTC_ERR_CONTEXT_NULL, "local ctl ctx null");

    if (ctx->svcInfo.instanceId != instanceId) {
        IOTC_LOGW("invalid instance id %d/%d", instanceId, ctx->svcInfo.instanceId);
        return IOTC_ERR_PARAM_INVALID;
    }

    if (ctx->svcInfo.onFinish != NULL) {
        ctx->svcInfo.onFinish(instanceId);
    }

    LocalControlServiceStopInner(ctx);
    return IOTC_OK;
}

int32_t LocalControlServiceInit(void)
{
    int32_t ret = LocalCtlCtxInit();
    if (ret != IOTC_OK) {
        IOTC_LOGW("local control context init error %d", ret);
        return ret;
    }

    static const ServiceHandler localCtlHandler = {
        .onStart = LocalControlServiceStart,
        .onStop = LocalControlServiceStop,
        .onReset = NULL,
    };

    ServiceInstance instance = {
        .serviceId = IOTC_SERVICE_ID_LOCAL_CONTROL,
        .name = LOCAL_CONTROL_SERVICE_NAME,
        .handler = &localCtlHandler,
        .msgNum = 0,
        .msgIds = NULL,
        .apiHandler = NULL,
    };

    ret = ServiceManagerRegisterInstance(&instance);
    if (ret != IOTC_OK) {
        IOTC_LOGW("reg local control service instance error %d", ret);
        return ret;
    }

    return IOTC_OK;
}

void LocalControlServiceDeinit(void)
{
    LocalControlServiceStopInner(GetLocalCtlCtx());
    ServiceManagerUnregisterInstance(IOTC_SERVICE_ID_LOCAL_CONTROL);
}
