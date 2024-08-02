
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
#include "lan_search_ctx.h"
#include "sched_timer.h"
#include "iotc_errcode.h"
#include "securec.h"

LanSearchContext *GetLanSearchCtx(void)
{
    static LanSearchContext ctx;
    return &ctx;
}

int32_t LanSearchContextInit(void)
{
    LanSearchContext *ctx = GetLanSearchCtx();
    (void)memset_s(ctx, sizeof(LanSearchContext), 0, sizeof(LanSearchContext));
    ctx->peerManager.expireCheckTimer = EVENT_SOURCE_INVALID_TIMER_FD;

    return IOTC_OK;
}