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
#ifndef EVENT_SOURCE_MSG_QUEUE_H
#define EVENT_SOURCE_MSG_QUEUE_H

#include "event_source.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*EventSourceMsgHandler)(const uint8_t *msg, uint32_t len);

EventSource *EventSourceMsgQueueNew(uint32_t cap, uint32_t maxMsgLen, const char *name);

int32_t EventSourceMsgQueueSend(EventSource *source, const uint8_t *msg, uint32_t len,
    EventSourceMsgHandler handler, uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* EVENT_SOURCE_MSG_QUEUE_H */
