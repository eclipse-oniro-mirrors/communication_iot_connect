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
#ifndef EVENT_SOURCE_TIMER_H
#define EVENT_SOURCE_TIMER_H

#include "event_source.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EVENT_SOURCE_INVALID_TIMER_FD (-1)

typedef enum {
    EVENT_SOURCE_TIMER_STATUS_INVALID = 0,
    EVENT_SOURCE_TIMER_STATUS_RUNNING,
    EVENT_SOURCE_TIMER_STATUS_FINISH,
} EventSourceTimerStatus;

typedef enum {
    EVENT_SOURCE_TIMER_TYPE_ONCE = 0,
    EVENT_SOURCE_TIMER_TYPE_REPEAT,
} EventSourceTimerType;

typedef void (*EventSourceTimerCallback)(int32_t id, void *userData);

EventSource *EventSourceTimerNew(const char *name);

int32_t EventSourceTimerAdd(EventSource *source, EventSourceTimerType type,
    EventSourceTimerCallback cb, uint32_t ms, void *userData);

void EventSourceTimerRemove(EventSource *loop, int32_t id);

void EventSourceTimerUpdate(EventSource *loop, int32_t id,
    EventSourceTimerType type, uint32_t ms);

EventSourceTimerStatus EventSourceTimerGetStatus(EventSource *loop, int32_t id);

#ifdef __cplusplus
}
#endif

#endif /* EVENT_SOURCE_TIMER_H */