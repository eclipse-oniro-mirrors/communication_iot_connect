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
#include "lan_search_svc.h"
#include "lan_search_ctx.h"
#include "iotc_errcode.h"
#include "iotc_svc.h"
#include "iotc_svc_lan_search.h"
#include "lan_search_peer_mngr.h"
#include "lan_search_coap_stack.h"
#include "utils_assert.h"
#include "event_bus.h"
#include "iotc_event.h"
#include "iotc_svc_conn.h"
#include "utils_common.h"

static const char *LAN_SEARCH_SERVICE_NAME = "LAN_SEARCH";
static const uint8_t LAN_SEARCH_DEFAULT_PEER_NUM = 2;
static const uint32_t LAN_SEARCH_DEFAULT_PEER_EXPIRE_TIME = UTILS_MIN_TO_MS(1);

static void LanSearchInitParamSetup(LanSearchContext *ctx, const LanSearchInitParam *init)
{
    if (init == NULL) {
        ctx->config.maxPeerNum = LAN_SEARCH_DEFAULT_PEER_NUM;
        ctx->config.peerExpireTime = LAN_SEARCH_DEFAULT_PEER_EXPIRE_TIME;
    } else {
        ctx->config.maxPeerNum = init->peerMaxNum;
        ctx->config.peerExpireTime = init->peerExpireTimeMs;
    }
    IOTC_LOGI("lan search init %u/%u", ctx->config.maxPeerNum, ctx->config.peerExpireTime);
}

static void EventSubForLanSearchCoapStackStart(uint32_t event, void *param, uint32_t len)
{
    NOT_USED(event);
    NOT_USED(len);
    NOT_USED(param);

    int32_t ret = LanSearchCoapStackStart(GetLanSearchCtx());
    if (ret != IOTC_OK) {
        IOTC_LOGW("start lan search coap server error %d", ret);
    } else {
        IOTC_LOGI("start lan search coap server");
    }
}

static void EventSubForLanSearchCoapStackStop(uint32_t event, void *param, uint32_t len)
{
    NOT_USED(event);
    NOT_USED(len);
    NOT_USED(param);

    LanSearchCoapStackStop(GetLanSearchCtx());
}

static void LanSearchServiceStopInner(LanSearchContext *ctx)
{
    LanSearchCoapStackDestroy(ctx);
    LanSearchPeerManagerDeinit(ctx);
}

/* param 为空使用默认配置 */
static int32_t LanSearchServiceStart(int32_t instanceId, ServiceFinishCallback onFinish, void *param)
{
    LanSearchContext *ctx = GetLanSearchCtx();
    CHECK_RETURN_LOGE(ctx != NULL, IOTC_ERR_CONTEXT_NULL, "lan search ctx null");

    LanSearchInitParamSetup(ctx, param);
    ctx->svcInfo.instanceId = instanceId;
    ctx->svcInfo.onFinish = onFinish;

    int32_t ret;
    do {
        ret = LanSearchPeerManagerInit(ctx);
        if (ret != IOTC_OK) {
            IOTC_LOGW("peer manager init error %d", ret);
            break;
        }

        ret = EventBusSubscribe(EventSubForLanSearchCoapStackStart, IOTC_SDK_AILIFE_EVENT_WIFI_NET_CONNECT);
        if (ret != IOTC_OK) {
            IOTC_LOGW("sub coap start event error %d", ret);
            break;
        }

        ret = EventBusSubscribe(EventSubForLanSearchCoapStackStop, IOTC_SDK_AILIFE_EVENT_WIFI_NET_DISCONNECT);
        if (ret != IOTC_OK) {
            IOTC_LOGW("sub coap stop event error %d", ret);
            break;
        }

        if (ConnSvcProxyIsNetConnected()) {
            ret = LanSearchCoapStackStart(ctx);
            if (ret != IOTC_OK) {
                IOTC_LOGW("start lan search coap server error %d", ret);
                break;
            }
        }
        IOTC_LOGI("start lan search service");
        return IOTC_OK;
    } while (0);
    LanSearchServiceStopInner(ctx);
    return ret;
}

static int32_t LanSearchServiceStop(int32_t instanceId, void *param)
{
    NOT_USED(param);
    LanSearchContext *ctx = GetLanSearchCtx();
    CHECK_RETURN_LOGE(ctx != NULL, IOTC_ERR_CONTEXT_NULL, "lan search ctx null");

    if (ctx->svcInfo.instanceId != instanceId) {
        IOTC_LOGW("invalid instance id %d/%d", instanceId, ctx->svcInfo.instanceId);
        return IOTC_ERR_PARAM_INVALID;
    }

    if (ctx->svcInfo.onFinish != NULL) {
        ctx->svcInfo.onFinish(instanceId);
    }

    LanSearchServiceStopInner(ctx);
    return IOTC_OK;
}

int32_t LanSearchServiceInit(void)
{
    int32_t ret = LanSearchContextInit();
    if (ret != IOTC_OK) {
        IOTC_LOGW("lan search init error %d", ret);
        return ret;
    }

    static const ServiceHandler lanSearchHandler = {
        .onStart = LanSearchServiceStart,
        .onStop = LanSearchServiceStop,
        .onReset = NULL,
    };

    ServiceInstance instance = {
        .serviceId = IOTC_SERVICE_ID_LAN_SEARCH,
        .name = LAN_SEARCH_SERVICE_NAME,
        .handler = &lanSearchHandler,
        .msgNum = 0,
        .msgIds = NULL,
        .apiHandler = NULL,
    };

    ret = ServiceManagerRegisterInstance(&instance);
    if (ret != IOTC_OK) {
        IOTC_LOGW("reg lan search instance error %d", ret);
        return ret;
    }

    return IOTC_OK;
}

void LanSearchServiceDeinit(void)
{
    LanSearchServiceStopInner(GetLanSearchCtx());
    ServiceManagerUnregisterInstance(IOTC_SERVICE_ID_LAN_SEARCH);
}