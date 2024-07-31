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
#ifndef EVENT_SOURCE_FD_H
#define EVENT_SOURCE_FD_H

#include "event_source.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EVENT_SOURCE_FD_MAX_WATCH_SIZE 16

typedef struct {
    int32_t fd[EVENT_SOURCE_FD_MAX_WATCH_SIZE];
    uint32_t num;
} EventSourceFdSet;

/* 有事件时返回>0，无事件返回=0，出错返回<0 */
typedef int32_t (*EventSourceFdPoll)(EventSourceFdSet *read, EventSourceFdSet *write,
    EventSourceFdSet *except, uint32_t timeout);

typedef bool (*EventSourceFdCallback)(int32_t fd, void *userData);

typedef enum {
    FD_EVENT_PRIORITY_MIN = 0,
    FD_EVENT_PRIORITY_MID,
    FD_EVENT_PRIORITY_MAX,
} FdEventPriority;

typedef struct {
    int32_t fd;
    FdEventPriority prio;
    EventSourceFdCallback recvEvent;
    EventSourceFdCallback sendEvent;
    EventSourceFdCallback exceEvent;
    void *userData;
} FdWatchParam;

EventSource *EventSourceFdNew(EventSourceFdPoll poll);

bool EventSourceFdWatch(EventSource *source, const FdWatchParam *watch);

bool EventSourceFdSuspend(EventSource *loop, int32_t fd);

bool EventSourceFdResume(EventSource *loop, int32_t fd);

void EventSourceFdRemove(EventSource *loop, int32_t fd);

#ifdef __cplusplus
}
#endif

#endif /* EVENT_SOURCE_FD_H */
