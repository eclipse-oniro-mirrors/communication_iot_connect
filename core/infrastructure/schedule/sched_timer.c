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
#include "sched_timer.h"
#include "sched_event_loop.h"
#include "comm_def.h"
#include "event_source_timer.h"
#include "utils_assert.h"
#include "utils_common.h"
#include "iotc_errcode.h"

#define MIN_TIMER_SCHED_INTERVAL_MS 10
#define SCHED_EVENT_SOURCE_TIMER_NAME "SCHED_TIMER"

static EventSource *g_timerSource = NULL;

static EventSource *GetSchedEventSourceTimer(void)
{
    return g_timerSource;
}

int32_t SchedTimerAdd(EventSourceTimerType type, EventSourceTimerCallback cb, uint32_t ms, void *userData)
{
    CHECK_RETURN_LOGW(cb != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");
    return EventSourceTimerAdd(GetSchedEventSourceTimer(), type, cb, ms, userData);
}

void SchedTimerRemove(int32_t id)
{
    CHECK_V_RETURN_LOGW(id >= 0, "invalid param");
    EventSourceTimerRemove(GetSchedEventSourceTimer(), id);
}

void SchedTimerUpdate(int32_t id, EventSourceTimerType type, uint32_t ms)
{
    CHECK_V_RETURN_LOGW(id >= 0, "invalid param");
    EventSourceTimerUpdate(GetSchedEventSourceTimer(), id, type, ms);
}

EventSourceTimerStatus SchedTimerGetStatus(int32_t id)
{
    CHECK_RETURN_LOGW(id >= 0, EVENT_SOURCE_TIMER_STATUS_INVALID, "invalid param");
    return EventSourceTimerGetStatus(GetSchedEventSourceTimer(), id);
}

int32_t SchedTimerInit(void)
{
    if (GetSchedEventLoop() == NULL) {
        return IOTC_ERR_NOT_INIT;
    }

    g_timerSource = EventSourceTimerNew(SCHED_EVENT_SOURCE_TIMER_NAME);
    if (g_timerSource == NULL) {
        IOTC_LOGE("init timer source error");
        return IOTC_CORE_COMM_FWK_ERR_SOURCE_NEW;
    }
    int32_t ret = EventLoopAddSource(GetSchedEventLoop(), g_timerSource);
    if (ret != IOTC_OK) {
        EventSourceFree(g_timerSource);
        g_timerSource = NULL;
        IOTC_LOGW("add source error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

void SchedTimerDeinit(void)
{
    EventLoopDelSource(GetSchedEventLoop(), g_timerSource);
    g_timerSource = NULL;
}

uint32_t SchedGetDefaultTimerInterval(void)
{
    uint32_t minTime = 0;
    uint32_t maxTime = 0;
    EventLoopGetPollTime(GetSchedEventLoop(), &minTime, &maxTime);
    return UTILS_MAX(MIN_TIMER_SCHED_INTERVAL_MS, maxTime);
}