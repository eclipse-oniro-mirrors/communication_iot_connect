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
#include "comm_def.h"
#include "utils_assert.h"
#include "utils_common.h"
#include "securec.h"
#include "adapter_os.h"
#include "sched_timer.h"
#include "iotc_errcode.h"

static const uint8_t TOKEN_MAX_REQ_TIME = 4;
/* right shift 5 bit = divide by 32 */
static const uint8_t TOKEN_REQ_TIME_INTERVAL_RATIO_RIGHT_SHIFT = 5;

static void TokenUpdateTimerCallback(int32_t id, void *userData)
{
    return;
}

static int32_t UpdateTokenRefreshTimer(M2mCloudContext *ctx)
{
    uint32_t expireTime = ctx->tokenInfo.timeout;
    uint32_t passTime = UtilsDeltaTime(AdapterGetSysTimeMs(), ctx->tokenInfo.updateTime);
    if (passTime >= expireTime || ctx->tokenInfo.cnt > TOKEN_MAX_REQ_TIME) {
        IOTC_LOGE("token timeout %u/%u/%u", expireTime, passTime, ctx->tokenInfo.cnt);
        return IOTC_SDK_AILIFE_WIFI_ERR_CLOUD_TOKEN_EXPIRE;
    }
    uint32_t refreshInterval = expireTime >> TOKEN_REQ_TIME_INTERVAL_RATIO_RIGHT_SHIFT;

    uint32_t nextReqTime = expireTime - refreshInterval * (TOKEN_MAX_REQ_TIME - ctx->tokenInfo.cnt);
    uint32_t waitTime = nextReqTime <= passTime ? 0 : nextReqTime - passTime;
    if (ctx->stateManager.tokenTimer >= 0) {
        SchedTimerUpdate(ctx->stateManager.tokenTimer, EVENT_SOURCE_TIMER_TYPE_ONCE, waitTime);
    } else {
        ctx->stateManager.tokenTimer = SchedTimerAdd(EVENT_SOURCE_TIMER_TYPE_ONCE,
            TokenUpdateTimerCallback, waitTime, ctx);
        if (ctx->stateManager.tokenTimer < 0) {
            IOTC_LOGE("add token update timer error %d", ctx->stateManager.tokenTimer);
            return ctx->stateManager.tokenTimer;
        }
    }
    return IOTC_OK;
}

int32_t UpdateCloudTokenInfo(M2mCloudContext *ctx, const CloudTokenInfo *tokenInfo)
{
    CHECK_RETURN_LOGW(ctx != NULL && tokenInfo != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    CHECK_RETURN_LOGE(!UtilsIsEmptyStr(tokenInfo->access),
        IOTC_SDK_AILIFE_WIFI_ERR_CLOUD_INVALID_ACCESS_TOKEN, "access token invalid");
    CHECK_RETURN_LOGE(!UtilsIsEmptyStr(tokenInfo->refresh),
        IOTC_SDK_AILIFE_WIFI_ERR_CLOUD_INVALID_REFRESH_TOKEN, "refresh token invalid");
    CHECK_RETURN_LOGE(tokenInfo->timeout != 0,
        IOTC_SDK_AILIFE_WIFI_ERR_CLOUD_INVALID_TOKEN_TIMEOUT, "token timeout zero");

    int32_t ret = strcpy_s(ctx->tokenInfo.access, sizeof(ctx->tokenInfo.access), tokenInfo->access);
    if (ret != EOK) {
        IOTC_LOGW("access strcpy error %d", ret);
        return IOTC_ERR_SECUREC_MEMCPY;
    };

    ret = strcpy_s(ctx->tokenInfo.refresh, sizeof(ctx->tokenInfo.refresh), tokenInfo->refresh);
    if (ret != EOK) {
        IOTC_LOGW("refresh strcpy error %d", ret);
        return IOTC_ERR_SECUREC_MEMCPY;
    };

    ctx->tokenInfo.updateTime = AdapterGetSysTimeMs();

    ret = UpdateTokenRefreshTimer(ctx);
    if (ret != IOTC_OK) {
        IOTC_LOGE("update token refresh timer error %d", ret);
        return ret;
    }
    return IOTC_OK;
}