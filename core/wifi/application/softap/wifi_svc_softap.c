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
#include "wifi_svc_softap.h"
#include "iotc_svc.h"
#include "iotc_svc_softap.h"
#include "iotc_svc_dev.h"
#include "iotc_log.h"
#include "sched_timer.h"
#include "utils_assert.h"
#include "securec.h"
#include "svc_softap_ssid.h"
#include "adapter_wifi.h"
#include "svc_softap_sess.h"
#include "utils_common.h"
#include "svc_softap_ctx.h"
#include "utils_bit_map.h"
#include "svc_softap_report.h"
#include "service_manager.h"
#include "event_bus.h"
#include "service_proxy.h"
#include "iotc_errcode.h"
#include "iotc_event.h"

static const char *SOFTAP_SERVICE_NAME = "SOFTAP";
/* 收到配网信息后释放softap配网的时间，预留100ms * 5的重传时间 */
#define RECV_NET_CFG_FINISH_TIME 500
#define SESS_CREATE_TIMER_INTERVAL UTILS_SEC_TO_MS(5)
#define SESS_CREATE_TIMER_FIRST (0)

static int32_t CreateWorkingSoftap(SoftapServiceContext *ctx)
{
    uint8_t ssid[ADAPTER_WIFI_SSID_MAX_LEN + 1] = {0};
    uint32_t len = ADAPTER_WIFI_SSID_MAX_LEN;

    int32_t ret;
    if (UTILS_IS_BIT_SET(ctx->initParam.bitMap, IOTC_WIFI_SERVICE_SOFTAP_CUSTOM_SSID)) {
        if (ctx->initParam.onBuildSsid == NULL) {
            IOTC_LOGW("custom ssid null");
            return IOTC_ERR_CALLBACK_NULL;
        }
        IOTC_LOGI("use custom ssid");
        ret = ctx->initParam.onBuildSsid(ssid, &len);
    } else {
        IOTC_LOGI("use hilink ssid");
        ret = BuildHiLinkSoftapSsid(ssid, &len);
    }
    if (ret != IOTC_OK) {
        IOTC_LOGW("build ssid error %d", ret);
        return ret;
    }

    ret = AdapterStartSoftAp(ssid, len, NULL, 0);
    if (ret != IOTC_OK) {
        IOTC_LOGW("start softap error %d", ret);
        return ret;
    }
    EventBusPublishAsync(IOTC_CORE_WIFI_EVENT_SOFTAP_START, NULL, 0, NULL);
    return IOTC_OK;
}

static void SoftapServiceExit(void)
{
    SoftapServiceContext *ctx = GetSoftapServiceContext();
    if (ctx == NULL) {
        return;
    }
    if (ctx->tmoTimerFd >= 0) {
        SchedTimerRemove(ctx->tmoTimerFd);
    }
    if (ctx->startUpTimerFd >= 0) {
        SchedTimerRemove(ctx->startUpTimerFd);
    }
    if (ctx->stopTimerFd >= 0) {
        SchedTimerRemove(ctx->stopTimerFd);
    }
    DestroySoftapSess(&ctx->sess);

    if (UTILS_IS_BIT_SET(ctx->bitMap, SOFTAP_CTX_BIT_MAP_SOFTAP_STARTED)) {
        int32_t ret = AdapterStopSoftAp();
        IOTC_LOGI("stop softap ret %d", ret);
        EventBusPublishAsync(IOTC_CORE_WIFI_EVENT_SOFTAP_STOP, NULL, 0, NULL);
    }
    if (ctx->onFinish != NULL) {
        ctx->onFinish(ctx->instanceId);
    }

    SoftapServiceContextDeinit();
}

static void SoftapTimeoutTimerCallback(int32_t id, void *userData)
{
    NOT_USED(id);
    NOT_USED(userData);

    SoftapServiceContext *ctx = GetSoftapServiceContext();
    if (ctx == NULL) {
        return;
    }

    IOTC_LOGN("softap service timeout %u", ctx->initParam.timeoutMs);
    SoftapServiceExit();
    return;
}

static void SessCreateTimerCallback(int32_t id, void *userData)
{
    NOT_USED(userData);

    SoftapServiceContext *ctx = GetSoftapServiceContext();
    if (ctx == NULL) {
        SchedTimerRemove(id);
        return;
    }
    SchedTimerUpdate(id, EVENT_SOURCE_TIMER_TYPE_REPEAT, SESS_CREATE_TIMER_INTERVAL);

    int32_t ret;
    if (!UTILS_IS_BIT_SET(ctx->bitMap, SOFTAP_CTX_BIT_MAP_SOFTAP_STARTED)) {
        ret = CreateWorkingSoftap(ctx);
        if (ret != IOTC_OK) {
            IOTC_LOGW("create softap error %d", ret);
            return;
        }
        UTILS_BIT_SET(ctx->bitMap, SOFTAP_CTX_BIT_MAP_SOFTAP_STARTED);
        IOTC_LOGN("softap start success");
    }

    if (!UTILS_IS_BIT_SET(ctx->bitMap, SOFTAP_CTX_BIT_MAP_SESS_CREATED)) {
        ret = CreateSoftapSess(&ctx->sess, &ctx->initParam);
        if (ret != IOTC_OK) {
            IOTC_LOGW("create sess error %d", ret);
            return;
        }
        UTILS_BIT_SET(ctx->bitMap, SOFTAP_CTX_BIT_MAP_SESS_CREATED);
        IOTC_LOGN("softap sess create success");
    }

    SchedTimerRemove(ctx->startUpTimerFd);
    ctx->startUpTimerFd = EVENT_SOURCE_INVALID_TIMER_FD;
    return;
}

static int32_t DeviceReportMessageHandler(const ServiceMessage *req, ServiceResponseInfo *resp)
{
    NOT_USED(resp);
    CHECK_RETURN_LOGW(req != NULL && req->msg != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    int32_t ret = SoftapServiceReportToAllPeer(req->msg);
    if (ret != IOTC_OK) {
        IOTC_LOGW("softap report error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static int32_t SoftapServiceMsgSub(SoftapServiceContext *ctx)
{
    if (!UTILS_IS_BIT_SET(ctx->initParam.bitMap, IOTC_WIFI_SERVICE_SOFTAP_E2E_CTRL)) {
        return IOTC_OK;
    }
    int32_t ret = ServiceProxySubscribeMessage(ctx->instanceId, IOTC_SERVICE_ID_DEVICE, DEVICE_SERVICE_MSG_ID_REPORT,
        DeviceReportMessageHandler);
    if (ret != IOTC_OK) {
        IOTC_LOGW("sub device service report error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static int32_t StartSoftapService(int32_t instanceId, ServiceFinishCallback onFinish, void *param)
{
    CHECK_RETURN_LOGW(onFinish != NULL && param != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    if (GetSoftapServiceContext() != NULL) {
        SoftapServiceExit();
    }

    int32_t ret;
    do {
        /* 1. 初始化上下文 */
        ret = SoftapServiceContextInit();
        if (ret != IOTC_OK) {
            break;
        }
        SoftapServiceContext *ctx = GetSoftapServiceContext();
        if (ctx == NULL) {
            ret = IOTC_CORE_WIFI_NETCFG_ERR_SOFTAP_INVALID_CTX;
            break;
        }
        ctx->onFinish = onFinish;
        ctx->instanceId = instanceId;

        ret = memcpy_s(&ctx->initParam, sizeof(ctx->initParam), param, sizeof(SoftapSvcInitParam));
        if (ret != EOK) {
            ret = IOTC_ERR_SECUREC_MEMCPY;
            break;
        }

        /* 2. 启动定时器，延时启动softap与会话监听 */
        ctx->startUpTimerFd = SchedTimerAdd(EVENT_SOURCE_TIMER_TYPE_REPEAT, SessCreateTimerCallback,
            SESS_CREATE_TIMER_FIRST, NULL);
        if (ctx->startUpTimerFd < 0) {
            ret = ctx->startUpTimerFd;
            break;
        }
        /* 3. 启动超时定时器 */
        if (UTILS_IS_BIT_SET(ctx->initParam.bitMap, IOTC_WIFI_SERVICE_SOFTAP_TIMEOUT)) {
            ctx->tmoTimerFd = SchedTimerAdd(EVENT_SOURCE_TIMER_TYPE_ONCE, SoftapTimeoutTimerCallback,
                ctx->initParam.timeoutMs, NULL);
            if (ctx->tmoTimerFd < 0) {
                ret = ctx->tmoTimerFd;
                break;
            }
        }
        /* 4. 订阅上报消息 */
        ret = SoftapServiceMsgSub(ctx);
        if (ret != IOTC_OK) {
            break;
        }

        IOTC_LOGN("softap service start success");
        return IOTC_OK;
    } while (0);

    /* 异常处理 */
    SoftapServiceExit();
    return ret;
}

static int32_t StopSoftapService(int32_t instanceId, void *param)
{
    IOTC_LOGN("softap service stop");
    SoftapServiceContext *ctx = GetSoftapServiceContext();
    if (ctx == NULL) {
        return IOTC_CORE_WIFI_NETCFG_ERR_SOFTAP_INVALID_CTX;
    }
    if (instanceId != ctx->instanceId) {
        IOTC_LOGW("invalid instance id to stop %d", instanceId);
        return IOTC_ERR_PARAM_INVALID;
    }

    SoftapServiceExit();
    return IOTC_OK;
}

int32_t WifiServiceSoftapInit(void)
{
    static const ServiceHandler SOFTAP_SERVICE_HANDLER = {
        .onStart = StartSoftapService,
        .onStop = StopSoftapService,
        .onReset = NULL,
    };

    int32_t msgs[SOFTAP_SERVICE_MSG_ID_MAX] = {0};
    for (uint32_t i = 0 ; i < SOFTAP_SERVICE_MSG_ID_MAX; ++i) {
        msgs[i] = i;
    }

    ServiceInstance instance = {
        .serviceId = IOTC_SERVICE_ID_SOFTAP,
        .name = SOFTAP_SERVICE_NAME,
        .handler = &SOFTAP_SERVICE_HANDLER,
        .msgNum = SOFTAP_SERVICE_MSG_ID_MAX,
        .msgIds = msgs,
    };

    int32_t ret = ServiceManagerRegisterInstance(&instance);
    if (ret != IOTC_OK) {
        IOTC_LOGE("reg softap service instance error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

void WifiServiceSoftapDeinit(void)
{
    (void)SoftapServiceExit();
    ServiceManagerUnregisterInstance(IOTC_SERVICE_ID_SOFTAP);
}
