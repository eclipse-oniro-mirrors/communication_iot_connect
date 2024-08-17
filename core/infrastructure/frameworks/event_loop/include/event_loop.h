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
#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include "event_source.h"
#include "iotc_os.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct EventLoop EventLoop;

typedef void (*EventLoopIdleCallback)(void);

#define EVENT_LOOP_DEFAULT_POLL_MAX_TIME_MS 150
#define EVENT_LOOP_DEFAULT_POLL_MIN_TIME_MS 1

/**
 * @brief New a EventLoop context
 *
 * @return NULL if failed
 */
EventLoop *EventLoopNew(const char *name);

/**
 * @brief Free a EventLoop context
 *
 * @param loop [IN] EventLoop context
 * @attention Can only be called when the loop is not running
 */
void EventLoopFree(EventLoop *loop);

/**
 * @brief Reset a EventLoop context
 *
 * @param loop [IN] EventLoop context
 * @attention Can only be called when the loop is not running
 */
void EventLoopReset(EventLoop *loop);

/**
 * @brief Suspend a EventLoop for a specified time
 *
 * @param loop [IN] EventLoop context
 * @param timeoutMs [IN] suspend time in ms
 */
void EventLoopSuspend(EventLoop *loop, uint32_t timeoutMs);

/**
 * @brief Resume a suspend Eventloop
 *
 * @param loop [IN] EventLoop context
 */
int32_t EventLoopResume(EventLoop *loop);

/**
 * @brief Block run a Eventloop
 *
 * @param [IN] EventLoop context
 */
void EventLoopRun(EventLoop *loop);

/**
 * @brief EventLoop quit
 *
 * @param loop [IN] EventLoop context
 * @attention This interface only sets the quit flag, and the loop quit by itself
 */
void EventLoopQuit(EventLoop *loop);

/**
 * @brief Add a idle func for Eventloop, this func will be executed in the Eventloop thread every loop
 *
 * @param loop [IN] EventLoop context
 * @param cb [IN] Idle func
 */
void EventLoopAddIdle(EventLoop *loop, EventLoopIdleCallback cb);

/**
 * @brief Configuring polling time in ms
 *
 * @param loop [IN] EventLoop context
 * @param minTime [IN] min polling time
 * @param maxTime [IN] max polling time
 */
void EventLoopSetPollTime(EventLoop *loop, uint32_t minTime, uint32_t maxTime);

/**
 * @brief Get polling time in ms
 *
 * @param loop [IN] EventLoop context
 * @param minTime [IN] min polling time
 * @param maxTime [IN] max polling time
 */
void EventLoopGetPollTime(EventLoop *loop, uint32_t *minTime, uint32_t *maxTime);

/**
 * @brief Add a source for EventLoop
 *
 * @param loop [IN] EventLoop context
 * @param source [IN] Source context
 * @attention Source added to EventLoop can only be deleted use EventLoopDelSource
 */
int32_t EventLoopAddSource(EventLoop *loop, EventSource *source);

/**
 * @brief Delete a source from EventLoop
 *
 * @param loop [IN] EventLoop context
 * @param source [IN] Source context
 */
void EventLoopDelSource(EventLoop *loop, EventSource *source);

IotcTaskId *EventLoopGetTaskId(EventLoop *loop);

#ifdef __cplusplus
}
#endif

#endif /* EVENT_LOOP_H */