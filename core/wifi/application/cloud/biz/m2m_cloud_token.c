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
#include "m2m_cloud_token.h"
#include "m2m_cloud_errcode.h"
#include "comm_def.h"
#include "utils_assert.h"
#include "utils_common.h"
#include "utils_bit_map.h"
#include "utils_time.h"
#include "securec.h"
#include "adapter_os.h"
#include "sched_timer.h"
#include "event_bus.h"
#include "iotc_errcode.h"
#include "iotc_event.h"
#include "fwk_main.h"

#define M2M_CLOUD_DEV_EXPIRED 3
#define M2M_CLOUD_DEV_SECRET_ERR 5
#define M2M_CLOUD_DEV_DELETED 6


static const uint8_t TOKEN_MAX_REQ_TIME = 4;
/* right shift 5 bit = divide by 32 */
static const uint8_t TOKEN_REQ_TIME_INTERVAL_RATIO_RIGHT_SHIFT = 5;

static const CloudOption *M2mCloudGetTokenRereshOption(void)
{
    static const char *SYS_TOKEN[] = {STR_URI_PATH_SYS, STR_URI_PATH_TOKEN};
    static const CloudOption TOKEN_OPTION = {
        .uri = SYS_TOKEN,
        .num = ARRAY_SIZE(SYS_TOKEN),
        .opBitMap = UTILS_BIT(CLOUD_OPTION_BIT_SEQ_NUM_ID) | UTILS_BIT(CLOUD_OPTION_BIT_ACCESS_TOKEN_ID),
    };
    return &TOKEN_OPTION;
}

static IotcJson *M2mCloudBuildTokenRefreshRequest(M2mCloudContext *ctx)
{
    CHECK_RETURN_LOGW(ctx != NULL, NULL, "param invalid");

    IotcJson *rootObj = IotcJsonCreate();
    CHECK_RETURN_LOGW(rootObj != NULL, NULL, "param invalid");

    int32_t ret = IotcJsonAddStr2Obj(rootObj, STR_JSON_REFRESH_TOKEN, ctx->tokenInfo.refresh);
    if (ret != IOTC_OK) {
        IotcJsonDelete(rootObj);
        return NULL;
    }
    return rootObj;
}

int32_t DealErrCodeRsp(int32_t errcode)
{
    if ((errcode == CLOUD_ERRCODE_CODE_DEV_DELETED) || (errcode == CLOUD_ERRCODE_CODE_EXPIRED) ||
        (errcode == CLOUD_ERRCODE_CODE_SECRET_ERR)) {
        int32_t ret = DevSvcProxyCleanLoginInfo();
        if (ret != IOTC_OK) {
            IOTC_LOGW("clean loginInfo error");
            return ret;
        }
        ret = IotcFwkRestore();
        if (ret != IOTC_OK) {
            IOTC_LOGF("restore error %d", ret);
            return ret;
        }
        return IOTC_OK;
    }
    return errcode;
}

static void CloudTokenRefreshRespHandler(const CoapPacket *resp, const SocketAddr *addr, void *userData, bool timeout)
{
    NOT_USED(addr);
    if (timeout) {
        IOTC_LOGW("Token refresh wait resp timeout");
        return;
    }

    M2mCloudContext *ctx = (M2mCloudContext *)userData;
    CHECK_V_RETURN_LOGW(ctx != NULL, "param invalid");
    CHECK_V_RETURN_LOGW(resp != NULL, "param invalid");

    IotcJson *jsonObj = IotcJsonParseWithLen((const char *)resp->payload.data, resp->payload.len);
    if (jsonObj == NULL) {
        IOTC_LOGE("json parse error %u", resp->payload.len);
        return;
    }

    int32_t errcode;
    int32_t ret = UtilsJsonGetNum(jsonObj, STR_ERRCODE, &errcode);
    if (ret != IOTC_OK) {
        IOTC_LOGE("get errcode error %d", ret);
        IotcJsonDelete(jsonObj);
        return;
    }
    ret = DealErrCodeRsp(errcode);
    if (ret == IOTC_OK) {
        IotcJsonDelete(jsonObj);
        return;
    }
    if (errcode != CLOUD_ERRCODE_OK) {
        IOTC_LOGE("get errcode %d", errcode);
        IotcJsonDelete(jsonObj);
        return;
    }

    ret = ParseTokenInfo(ctx, jsonObj);
    IotcJsonDelete(jsonObj);
    if (ret != IOTC_OK) {
        IOTC_LOGW("parse token error %d", ret);
        return;
    }
}

static int32_t CalcWaitTime(const M2mCloudContext *ctx, uint32_t *waitTime)
{
    uint32_t expireTime = ctx->tokenInfo.timeout;
    uint32_t passTime = UtilsDeltaTime(IotcGetSysTimeMs(), ctx->tokenInfo.updateTime);
    if (passTime >= expireTime || ctx->tokenInfo.cnt > TOKEN_MAX_REQ_TIME) {
        IOTC_LOGE("token timeout %u/%u/%u", expireTime, passTime, ctx->tokenInfo.cnt);
        return IOTC_SDK_AILIFE_WIFI_ERR_CLOUD_TOKEN_EXPIRE;
    }
    uint32_t refreshInterval = expireTime >> TOKEN_REQ_TIME_INTERVAL_RATIO_RIGHT_SHIFT;

    uint32_t nextReqTime = expireTime - refreshInterval * (TOKEN_MAX_REQ_TIME - ctx->tokenInfo.cnt);
    *waitTime = nextReqTime <= passTime ? 0 : nextReqTime - passTime;

    return IOTC_OK;
}

static void TokenUpdateTimerCallback(int32_t id, void *userData)
{
    NOT_USED(id);
    M2mCloudContext *ctx = (M2mCloudContext *)userData;
    CHECK_V_RETURN_LOGW(ctx != NULL, "param invalid");

    int32_t ret = M2mCloudSendRequest(ctx, CloudTokenRefreshRespHandler,
        M2mCloudBuildTokenRefreshRequest, M2mCloudGetTokenRereshOption());
    if (ret != IOTC_OK) {
        IOTC_LOGW("dev token refresh error %d", ret);
        return;
    }

    uint32_t waitTime = 0;
    ret = CalcWaitTime((const M2mCloudContext *)ctx, &waitTime);
    if (ret != IOTC_OK) {
        IOTC_LOGW("calc waitTime error %d", ret);
        return;
    }
}

static int32_t UpdateTokenRefreshTimer(M2mCloudContext *ctx)
{
    if (ctx->stateManager.tokenTimer < 0) {
        uint32_t waitTime = 0;
        int32_t ret = CalcWaitTime((const M2mCloudContext *)ctx, &waitTime);
        if (ret != IOTC_OK) {
            IOTC_LOGW("calc waitTime error %d", ret);
            return ret;
        }
        ctx->stateManager.tokenTimer = SchedTimerAdd(EVENT_SOURCE_TIMER_TYPE_REPEAT, TokenUpdateTimerCallback, waitTime, ctx);
        if (ctx->stateManager.tokenTimer < 0) {
            IOTC_LOGE("add token update timer error %d", ctx->stateManager.tokenTimer);
            return ctx->stateManager.tokenTimer;
        }
    }

    return IOTC_OK;
}

static int32_t UpdateCloudTokenInfo(M2mCloudContext *ctx, const CloudTokenInfo *token)
{
    CHECK_RETURN_LOGW(ctx != NULL && token != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    CHECK_RETURN_LOGE(!UtilsIsEmptyStr(token->access),
        IOTC_SDK_AILIFE_WIFI_ERR_CLOUD_INVALID_ACCESS_TOKEN, "access token invalid");
    CHECK_RETURN_LOGE(!UtilsIsEmptyStr(token->refresh),
        IOTC_SDK_AILIFE_WIFI_ERR_CLOUD_INVALID_REFRESH_TOKEN, "refresh token invalid");
    CHECK_RETURN_LOGE(token->timeout != 0,
        IOTC_SDK_AILIFE_WIFI_ERR_CLOUD_INVALID_TOKEN_TIMEOUT, "token timeout zero");

    int32_t ret = strcpy_s(ctx->tokenInfo.access, sizeof(ctx->tokenInfo.access), token->access);
    if (ret != EOK) {
        IOTC_LOGW("access strcpy error %d", ret);
        return IOTC_ERR_SECUREC_MEMCPY;
    };

    ret = strcpy_s(ctx->tokenInfo.refresh, sizeof(ctx->tokenInfo.refresh), token->refresh);
    if (ret != EOK) {
        IOTC_LOGW("refresh strcpy error %d", ret);
        return IOTC_ERR_SECUREC_MEMCPY;
    };

    ctx->tokenInfo.updateTime = IotcGetSysTimeMs();
    ctx->tokenInfo.timeout = token->timeout;

    ret = UpdateTokenRefreshTimer(ctx);
    if (ret != IOTC_OK) {
        IOTC_LOGE("update token refresh timer error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static int32_t UpdateTimeStamp(const IotcJson *jsonObj)
{
    const char *timestamp = IotcJsonGetStr(IotcJsonGetObj(jsonObj, STR_JSON_TIMESTAMP));
    const char *timezone = IotcJsonGetStr(IotcJsonGetObj(jsonObj, STR_JSON_TIMEZONE));
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

static int32_t GetTokenInfo(const IotcJson *jsonObj, CloudTokenInfo *token)
{
    uint32_t timeout = 0;

    int32_t ret = UtilsJsonGetString(jsonObj, STR_JSON_ACCESS_TOKEN, token->access, sizeof(token->access));
    if (ret != IOTC_OK) {
        IOTC_LOGE("get access token error %d", ret);
        return ret;
    }
    ret = UtilsJsonGetString(jsonObj, STR_JSON_REFRESH_TOKEN, token->refresh, sizeof(token->refresh));
    if (ret != IOTC_OK) {
        IOTC_LOGE("get refresh token error %d", ret);
        return ret;
    }
    ret = UtilsJsonGetUint(jsonObj, STR_JSON_TIMEOUT, &timeout);
    if (ret != IOTC_OK) {
        IOTC_LOGE("get refresh timeout error %d", ret);
        return ret;
    }
    if (timeout > UINT32_MAX / UTILS_MS_PER_SECOND) {
        IOTC_LOGE("get refresh timeout invalid %u", timeout);
        return IOTC_SDK_AILIFE_WIFI_ERR_CLOUD_TOKEN_TIMEOUT_INVALID;
    }
    token->timeout = timeout * UTILS_MS_PER_SECOND;
    return IOTC_OK;
}

int32_t ParseTokenInfo(M2mCloudContext *ctx, IotcJson *jsonObj)
{
    CHECK_RETURN_LOGW(ctx != NULL && jsonObj != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    int32_t ret = UpdateTimeStamp((const IotcJson *)jsonObj);
    if (ret != IOTC_OK) {
        IOTC_LOGE("update timestamp error %d", ret);
        return ret;
    }

    CloudTokenInfo token = {0};
    ret = GetTokenInfo((const IotcJson *)jsonObj, &token);
    if (ret != IOTC_OK) {
        IOTC_LOGE("get tokeninfo error %d", ret);
        return ret;
    }

    return UpdateCloudTokenInfo(ctx, &token);
}