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
#include "svc_conn_ctx.h"
#include "securec.h"
#include "iotc_errcode.h"
#include "sched_timer.h"

int32_t ConnSvcCtxInit(void)
{
    ConnectServiceContext *ctx = GetConnSvcCtx();
    if (ctx == NULL) {
        return IOTC_ERR_PARAM_INVALID;
    }
    (void)memset_s(ctx, sizeof(ConnectServiceContext), 0, sizeof(ConnectServiceContext));
    ctx->netCheckTimerFd = EVENT_SOURCE_INVALID_TIMER_FD;
    ctx->verifyTimerFd = EVENT_SOURCE_INVALID_TIMER_FD;

    return IOTC_OK;
}

void ConnSvcCtxDeinit(void)
{
    ConnectServiceContext *ctx = GetConnSvcCtx();
    if (ctx == NULL) {
        return;
    }
    if (ctx->netCheckTimerFd >= 0) {
        SchedTimerRemove(ctx->netCheckTimerFd);
        ctx->netCheckTimerFd = EVENT_SOURCE_INVALID_TIMER_FD;
    }
    if (ctx->verifyTimerFd >= 0) {
        SchedTimerRemove(ctx->verifyTimerFd);
        ctx->verifyTimerFd = EVENT_SOURCE_INVALID_TIMER_FD;
    }
}

ConnectServiceContext *GetConnSvcCtx(void)
{
    static ConnectServiceContext ctx;
    return &ctx;
}