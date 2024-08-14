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
#include "m2m_cloud_heartbeat.h"
#include "securec.h"
#include "sched_timer.h"
#include "utils_assert.h"
#include "utils_time.h"
#include "utils_bit_map.h"
#include "iotc_errcode.h"

#define STR_JSON_HB_CNT "cnt"
#define CLOUD_HB_INTERVAL (50 * 1000)
#define CLOUD_HB_TIMEOUT_CNT 3

static const CloudOption *M2mCloudGetHeartbeatOption(void)
{
    static const char *SYS_HB[] = {STR_URI_PATH_SYS, STR_URI_PATH_HB};
    static const CloudOption HB_OPTION = {
        .uri = SYS_HB,
        .num = ARRAY_SIZE(SYS_HB),
        .opBitMap = UTILS_BIT(CLOUD_OPTION_BIT_SEQ_NUM_ID) | UTILS_BIT(CLOUD_OPTION_BIT_ACCESS_TOKEN_ID),
    };
    return &HB_OPTION;
}

static AdapterJson *M2mCloudBuildHeartbeatRequest(M2mCloudContext *ctx)
{
    CHECK_RETURN_LOGW(ctx != NULL, NULL, "param invalid");

    AdapterJson *rootObj = AdapterCreateJson();
    CHECK_RETURN_LOGW(rootObj != NULL, NULL, "param invalid");

    int32_t ret = AdapterJsonAddNum2Obj(rootObj, STR_JSON_HB_CNT, ctx->heartbeatInfo.sentCnt);
    if (ret != IOTC_OK) {
        IOTC_LOGW("Json Add sentCnt error, ret=%d", ret);
        AdapterJsonDelete(rootObj);
        return NULL;
    }
    ctx->heartbeatInfo.sentCnt++;
    IOTC_LOGN("send heartbeat sentCnt:%u", ctx->heartbeatInfo.sentCnt);

    return rootObj;
}

static int32_t ParseHeartbeatInfo(M2mCloudContext *ctx, const AdapterJson *jsonObj)
{
    const char *timestamp = AdapterJsonGetStr(AdapterJsonGetObj(jsonObj, STR_JSON_TIMESTAMP));
    const char *timezone = AdapterJsonGetStr(AdapterJsonGetObj(jsonObj, STR_JSON_TIMEZONE));
    IOTC_LOGI("get time info %s/%s", NON_NULL_STR(timestamp), NON_NULL_STR(timezone));

    if (timestamp != NULL && timezone != NULL) {
        uint64_t ts = atoll(timestamp);
        if (ts == 0) {
            IOTC_LOGW("timestamp trans to num error");
            return IOTC_SDK_AILIFE_WIFI_ERR_CLOUD_INVALID_TIMESTAMP;
        }
        int32_t ret = UtilsSetUtcTimeStamp(ts);
        if (ret != IOTC_OK) {
            IOTC_LOGW("set timestamp error %d", ret);
            return ret;
        }
    }

    return IOTC_OK;
}

static void M2mCloudHeartbbeatRespHandler(const CoapPacket *resp, const SocketAddr *addr, void *userData, bool timeout)
{
    NOT_USED(addr);

    M2mCloudContext *ctx = (M2mCloudContext *)userData;
    CHECK_V_RETURN_LOGW(ctx != NULL, "param invalid");

    if (timeout) {
        if (ctx->heartbeatInfo.sentCnt >= CLOUD_HB_TIMEOUT_CNT) {
            IOTC_LOGW("cloud mode heartbbeat reach max times!");
            M2mCloudDisableHeartbeat(ctx);
            UtilsFsmSwitch(ctx->stateManager.fsmCtx, M2M_CLOUD_FSM_STATE_CONNECT);
        }
        return;
    }

    CHECK_V_RETURN_LOGW(resp != NULL, "param invalid");

    AdapterJson *jsonObj = AdapterJsonParseWithLen((const char *)resp->payload.data, resp->payload.len);
    if (jsonObj == NULL) {
        IOTC_LOGE("json parse error %u", resp->payload.len);
        return;
    }

    int32_t ret = ParseHeartbeatInfo(ctx, (const AdapterJson *)jsonObj);
    AdapterJsonDelete(jsonObj);
    CHECK_V_RETURN_LOGW(ret == IOTC_OK, "parse hb Info error");
    ctx->heartbeatInfo.sentCnt = 0;
    IOTC_LOGN("rcv heartbeat clean sentCnt");
}

static void CloudHeartbeatTimerCallback(int32_t id, void *userData)
{
    NOT_USED(id);
    M2mCloudContext *ctx = (M2mCloudContext *)userData;
    CHECK_V_RETURN_LOGW(ctx != NULL, "param invalid");

    int32_t ret = M2mCloudSendRequest(ctx, M2mCloudHeartbbeatRespHandler,
        M2mCloudBuildHeartbeatRequest, M2mCloudGetHeartbeatOption());
    if (ret != IOTC_OK) {
        IOTC_LOGW("send hb error %d", ret);
    }
}

int32_t M2mCloudEnableHeartbeat(M2mCloudContext *ctx)
{
    CHECK_RETURN_LOGW(ctx != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    
    (void)memset_s(&ctx->heartbeatInfo, sizeof(ctx->heartbeatInfo), 0, sizeof(ctx->heartbeatInfo));
    ctx->heartbeatInfo.interval = CLOUD_HB_INTERVAL;

    IOTC_LOGI("start Heartbeat Timer[%u]", ctx->heartbeatInfo.interval);
    if (ctx->stateManager.hbTimer < 0) {
        ctx->stateManager.hbTimer = SchedTimerAdd(EVENT_SOURCE_TIMER_TYPE_REPEAT,
            CloudHeartbeatTimerCallback, ctx->heartbeatInfo.interval, ctx);
        if (ctx->stateManager.hbTimer < 0) {
            IOTC_LOGE("add heartbeat timer error %d", ctx->stateManager.hbTimer);
            return ctx->stateManager.hbTimer;
        }
    }
    return IOTC_OK;
}

int32_t M2mCloudDisableHeartbeat(M2mCloudContext *ctx)
{
    CHECK_RETURN_LOGW(ctx != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    IOTC_LOGI("stop Heartbeat Timer[%u]", ctx->heartbeatInfo.interval);
    (void)memset_s(&ctx->heartbeatInfo, sizeof(ctx->heartbeatInfo), 0, sizeof(ctx->heartbeatInfo));
    if (ctx->stateManager.hbTimer >= 0) {
        SchedTimerRemove(ctx->stateManager.hbTimer);
        ctx->stateManager.hbTimer = EVENT_SOURCE_INVALID_TIMER_FD;
    }
    return IOTC_OK;
}