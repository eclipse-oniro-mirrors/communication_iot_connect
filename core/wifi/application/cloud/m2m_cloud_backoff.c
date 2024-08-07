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
#include "m2m_cloud_backoff.h"
#include "securec.h"
#include "utils_assert.h"
#include "adapter_os.h"
#include "utils_common.h"
#include "config_login_info.h"
#include "security_random.h"
#include "utils_bit_map.h"

#define GET_BACKOFF_CTX(_cloudCtx) (&((_cloudCtx)->backoff))

bool IsM2mCloudBackoffTime(M2mCloudContext *ctx)
{
    CHECK_RETURN_LOGW(ctx != NULL, false, "param invalid");

    if (!UTILS_IS_BIT_SET(ctx->bitMap, M2M_CLOUD_CTX_BIT_ENABLE_BACKOFF)) {
        return false;
    }

    if (ctx->backoffInfo.cnt == 0) {
        return false;
    }

    if (UtilsDeltaTime(AdapterGetSysTimeMs(), ctx->backoffInfo.before) >= ctx->backoffInfo.interval) {
        return false;
    }
    return true;
}

void M2mCloudBackoffInit(M2mCloudContext *ctx)
{
    CHECK_V_RETURN_LOGW(ctx != NULL, "param invalid");

    /* backoff only use for login, avoid surges caused by regional centralized power on */
    if (UTILS_IS_BIT_SET(ctx->bitMap, M2M_CLOUD_CTX_BIT_REGISTER)) {
        return;
    }
    UTILS_BIT_SET(ctx->bitMap, M2M_CLOUD_CTX_BIT_ENABLE_BACKOFF);
    ctx->backoffInfo.cnt = 0;
    return;
}

void M2mCloudBackoffUpdate(M2mCloudContext *ctx)
{
    CHECK_V_RETURN_LOGW(ctx != NULL, "param invalid");
    if (!UTILS_IS_BIT_SET(ctx->bitMap, M2M_CLOUD_CTX_BIT_ENABLE_BACKOFF)) {
        return;
    }

    const uint8_t CLOUD_BACKOFF_MAX_CNT = 5;
    const uint8_t CLOUD_BACKOFF_EXP_TIMES = 3;
    const uint32_t CLOUD_BACKOFF_FIRST_MAX_TIME = UTILS_SEC_TO_MS(5);
    const uint32_t CLOUD_BACKOFF_MAX_TIME = UTILS_MIN_TO_MS(10);

    uint32_t min = 0;
    uint32_t max = CLOUD_BACKOFF_FIRST_MAX_TIME;

    /* Every login failure will increase waiting time range */
    for (uint8_t i = 0; i < ctx->backoffInfo.cnt; ++i) {
        min = max;
        max = max * CLOUD_BACKOFF_EXP_TIMES;
    }
    if (max > CLOUD_BACKOFF_MAX_TIME) {
        max = UTILS_MAX(min, CLOUD_BACKOFF_MAX_TIME);
    }

    if (ctx->backoffInfo.cnt < CLOUD_BACKOFF_MAX_CNT) {
        ++ctx->backoffInfo.cnt;
    }

    uint32_t random = SecurityRandomUint32();
    ctx->backoffInfo.interval = (random % (max - min)) + min;
    ctx->backoffInfo.before = AdapterGetSysTimeMs();
    IOTC_LOGI("cloud backoff time update %u/%u/%u",
        ctx->backoffInfo.cnt, ctx->backoffInfo.interval, ctx->backoffInfo.before);
}