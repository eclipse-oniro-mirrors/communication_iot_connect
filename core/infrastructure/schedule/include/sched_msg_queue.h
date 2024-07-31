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
#ifndef SCHEDULE_MSG_QUEUE_H
#define SCHEDULE_MSG_QUEUE_H
#include <stdint.h>
#include "event_source_msg_queue.h"
#include "iotc_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

#if IOTC_CONF_DEVICE_LEVEL == IOTC_DEVICE_LEVEL_MINI
#define SCHED_MSG_QUEUE_MAX_MSG_LEN 2048
#define SCHED_MSG_QUEUE_MAX_CAP 32
#else
#define SCHED_MSG_QUEUE_MAX_MSG_LEN (1024 * 64)
#define SCHED_MSG_QUEUE_MAX_CAP 128
#endif

int32_t SchedMsgQueueSend(const uint8_t *msg, uint32_t len, EventSourceMsgHandler handler, uint32_t timeout);

int32_t SchedMsgQueueInit(void);

void SchedMsgQueueDeinit(void);

#ifdef __cplusplus
}
#endif

#endif /* SCHEDULE_MSG_QUEUE_H */