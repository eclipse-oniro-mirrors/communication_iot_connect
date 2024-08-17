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
#include "event_source_msg_queue.h"
#include "utils_msg_queue.h"
#include "comm_def.h"
#include "adapter_mem.h"
#include "utils_assert.h"
#include "utils_buffer.h"
#include "utils_common.h"
#include "securec.h"
#include "iotc_errcode.h"

#define MSG_QUEUE_EVENT_SOURCE_NAME "MSG"

typedef struct {
    EventSourceMsgHandler handler;
    uint32_t len;
    uint8_t msg[];
} EventSourceMsg;

typedef struct {
    EventSource base;
    UtilsMsgQueue *msgQueue;
    uint32_t maxMsgLen;
    uint32_t msgInfoLen;
    EventSourceMsg *msgInfo;
} EventSourceMsgQueue;

static void MsgInfoHandler(EventSourceMsg **msgInfo, uint32_t *msgInfoLen)
{
    if (*msgInfo == NULL) {
        return;
    }

    if (*msgInfoLen != (*msgInfo)->len + sizeof(EventSourceMsg)) {
        IOTC_LOGF("invalid msg %u/%u", *msgInfoLen, (*msgInfo)->len);
    }

    if ((*msgInfo)->handler != NULL) {
        (*msgInfo)->handler((*msgInfo)->msg, (*msgInfo)->len);
    }
    UTILS_FREE_2_NULL(*msgInfo);
    *msgInfoLen = 0;
}

static bool MsgQueueSourcePrepare(EventSource *self, uint32_t *timeout)
{
    CHECK_RETURN(self != NULL && timeout != NULL, false);
    /* 实际受 event loop 的 pool time max 与 pool time min 约束 */
    *timeout = UINT32_MAX;
    return true;
}

static bool MsgQueueSourcePoll(EventSource *self, uint32_t timeout)
{
    CHECK_RETURN(self != NULL, false);
    EventSourceMsgQueue *mqSource = (EventSourceMsgQueue *)self;

    /* make sure msg has already process */
    MsgInfoHandler(&mqSource->msgInfo, &mqSource->msgInfoLen);

    int32_t ret = UtilsMsgQueuePopMem(mqSource->msgQueue, (void **)&mqSource->msgInfo, &mqSource->msgInfoLen, timeout);
    if (ret == IOTC_OK) {
        /* resv queue msg */
        return true;
    }
    if (ret == IOTC_CORE_COMM_UTILS_ERR_MSG_QUEUE_EMPTY || ret == IOTC_ERR_TIMEOUT) {
        /* not recv queue msg */
        return false;
    }
    IOTC_LOGW("msg queue pop error %d", ret);
    return false;
}

static bool MsgQueueSourceCheck(EventSource *self)
{
    CHECK_RETURN(self != NULL, false);
    EventSourceMsgQueue *mqSource = (EventSourceMsgQueue *)self;
    return mqSource->msgInfo != NULL;
}

static bool MsgQueueSourceDispatch(EventSource *self)
{
    CHECK_RETURN(self != NULL, false);
    EventSourceMsgQueue *mqSource = (EventSourceMsgQueue *)self;
    MsgInfoHandler(&mqSource->msgInfo, &mqSource->msgInfoLen);
    return true;
}

static void MsgQueueSourceFinalize(EventSource *self)
{
    CHECK_V_RETURN(self != NULL);
    EventSourceMsgQueue *mqSource = (EventSourceMsgQueue *)self;

    MsgInfoHandler(&mqSource->msgInfo, &mqSource->msgInfoLen);
    if (mqSource->msgQueue != NULL) {
        (void)UtilsMsgQueueSuspend(mqSource->msgQueue);
        /* 清空消息 */
        while (UtilsMsgQueuePopMem(mqSource->msgQueue, (void **)&mqSource->msgInfo,
            &mqSource->msgInfoLen, 0) == IOTC_OK) {
            MsgInfoHandler(&mqSource->msgInfo, &mqSource->msgInfoLen);
        }
        UtilsMsgQueueDestroy(&mqSource->msgQueue);
    }
}

EventSource *EventSourceMsgQueueNew(uint32_t cap, uint32_t maxMsgLen, const char *name)
{
    CHECK_RETURN_LOGE(maxMsgLen > 0 && cap > 0, NULL, "param invalid");

    static EventSourceOps msgQueueSourceOps = {
        .prepare = MsgQueueSourcePrepare,
        .poll = MsgQueueSourcePoll,
        .check = MsgQueueSourceCheck,
        .dispatch = MsgQueueSourceDispatch,
        .finalize = MsgQueueSourceFinalize,
    };

    EventSource *source = EventSourceNew(&msgQueueSourceOps, sizeof(EventSourceMsgQueue),
        name == NULL ? MSG_QUEUE_EVENT_SOURCE_NAME : name, NULL);
    if (source == NULL) {
        return NULL;
    }

    EventSourceMsgQueue *mqSource = (EventSourceMsgQueue *)source;
    do {
        mqSource->msgQueue = UtilsMsgQueueCreate(cap, NULL);
        if (mqSource->msgQueue == NULL) {
            IOTC_LOGW("create msg queue error");
            break;
        }

        mqSource->maxMsgLen = maxMsgLen;
        return source;
    } while (0);

    EventSourceFree(source);
    return NULL;
}

int32_t EventSourceMsgQueueSend(EventSource *source, const uint8_t *msg, uint32_t len,
    EventSourceMsgHandler handler, uint32_t timeout)
{
    CHECK_RETURN_LOGE(source != NULL && msg != NULL && len != 0 && handler != NULL, IOTC_ERR_PARAM_INVALID,
        "param invalid");

    EventSourceMsgQueue *mqSource = (EventSourceMsgQueue *)source;
    if (len > mqSource->maxMsgLen) {
        IOTC_LOGW("invalid msg len %u", len);
        return IOTC_CORE_COMM_FWK_ERR_SOURCE_QUEUE_MSG_TOO_LONG;
    }

    uint32_t msgInfoLen = len + sizeof(EventSourceMsg);
    EventSourceMsg *msgInfo = (EventSourceMsg *)IotcMalloc(msgInfoLen);
    if (msgInfo == NULL) {
        IOTC_LOGW("malloc error %u", msgInfoLen);
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }

    /* 已全量赋值，不再memset初始化 */
    msgInfo->handler = handler;
    msgInfo->len = len;
    int32_t ret = memcpy_s(msgInfo->msg, len, msg, len);
    if (ret != EOK) {
        IotcFree(msgInfo);
        return IOTC_ERR_SECUREC_MEMCPY;
    }

    ret = UtilsMsgQueuePushMem(mqSource->msgQueue, (void **)&msgInfo, msgInfoLen, timeout);
    if (ret != IOTC_OK) {
        IotcFree(msgInfo);
        IOTC_LOGW("msg push queue error %d/%u", ret, timeout);
        return ret;
    }
    return IOTC_OK;
}