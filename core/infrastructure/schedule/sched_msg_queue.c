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
#include "sched_msg_queue.h"
#include "sched_event_loop.h"
#include "comm_def.h"
#include "utils_assert.h"
#include "utils_common.h"
#include "iotc_errcode.h"

#define SCHED_EVENT_SOURCE_MSG_QUEUE_NAME "SCHED_MSG"

static EventSource *g_msgQueueSource = NULL;

int32_t SchedMsgQueueSend(const uint8_t *msg, uint32_t len, EventSourceMsgHandler handler, uint32_t timeout)
{
    return EventSourceMsgQueueSend(g_msgQueueSource, msg, len, handler, timeout);
}

int32_t SchedMsgQueueInit(void)
{
    if (GetSchedEventLoop() == NULL) {
        return IOTC_ERR_NOT_INIT;
    }

    g_msgQueueSource = EventSourceMsgQueueNew(SCHED_MSG_QUEUE_MAX_CAP, SCHED_MSG_QUEUE_MAX_MSG_LEN,
        SCHED_EVENT_SOURCE_MSG_QUEUE_NAME);
    if (g_msgQueueSource == NULL) {
        IOTC_LOGE("init msg source source error");
        return IOTC_CORE_COMM_FWK_ERR_SOURCE_NEW;
    }
    int32_t ret = EventLoopAddSource(GetSchedEventLoop(), g_msgQueueSource);
    if (ret != IOTC_OK) {
        EventSourceFree(g_msgQueueSource);
        g_msgQueueSource = NULL;
        IOTC_LOGW("add source error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

void SchedMsgQueueDeinit(void)
{
    EventLoopDelSource(GetSchedEventLoop(), g_msgQueueSource);
    g_msgQueueSource = NULL;
}
