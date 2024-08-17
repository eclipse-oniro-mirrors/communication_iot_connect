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
#include "svc_softap_ctx.h"
#include "utils_common.h"
#include "iotc_log.h"
#include "securec.h"
#include "sched_timer.h"
#include "iotc_errcode.h"

static SoftapServiceContext *g_softapCtx = NULL;

SoftapServiceContext *GetSoftapServiceContext()
{
    return g_softapCtx;
}

int32_t SoftapServiceContextInit(void)
{
    g_softapCtx = (SoftapServiceContext *)IotcMalloc(sizeof(SoftapServiceContext));
    if (g_softapCtx == NULL) {
        IOTC_LOGW("malloc error");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(g_softapCtx, sizeof(SoftapServiceContext), 0, sizeof(SoftapServiceContext));
    g_softapCtx->tmoTimerFd = EVENT_SOURCE_INVALID_TIMER_FD;
    g_softapCtx->startUpTimerFd = EVENT_SOURCE_INVALID_TIMER_FD;
    g_softapCtx->stopTimerFd = EVENT_SOURCE_INVALID_TIMER_FD;
    return IOTC_OK;
}

void SoftapServiceContextDeinit(void)
{
    UTILS_FREE_2_NULL(g_softapCtx);
}