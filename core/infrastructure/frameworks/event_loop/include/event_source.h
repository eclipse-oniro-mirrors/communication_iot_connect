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
#ifndef EVENT_SOURCE_H
#define EVENT_SOURCE_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EVENT_SOURCE_MAX_SIZE 1024

typedef struct EventSourceOps EventSourceOps;

/* EventSource public struct, as custom source struct header */
typedef struct {
    const EventSourceOps *ops;
    void *userData;
    const char *name;
} EventSource;

struct EventSourceOps {
    /**
     * @brief EventSource prepare, get timeout
     *
     * @param self [IN] EventSource context
     * @param timeout [OUT] timeout in ms
     * @return true-OK, false-Failed
     */
    bool (*prepare)(EventSource *self, uint32_t *timeout);
    /**
     * @brief EventSource polling for io event, a EventLoop recommends only one pollable EventSource
     *
     * @param self [IN] EventSource context
     * @param timeout [OUT] polling timeout in ms
     * @return true-OK, false-Failed
     */
    bool (*poll)(EventSource *self, uint32_t timeout);
    /**
     * @brief EventSource check if any events occurred
     *
     * @param self [IN] EventSource context
     * @return true-Event occurred，false-No event
     */
    bool (*check)(EventSource *self);
    /**
     * @brief EventSource dispatch event, EventLoop will del source if dispatch failed
     *
     * @param self [IN] EventSource context
     * @return true-OK, false-Failed
     */
    bool (*dispatch)(EventSource *self);
    /**
     * @brief Free custom resource
     *
     * @param self [IN] EventSource context
     */
    void (*finalize)(EventSource *self);
};

/**
 * @brief New a EventSource
 *
 * @param ops [IN] EventSource operation Function, should be const static
 * @param name [IN] EventSource name, should be const static
 * @param size [IN] EventSource size, limited by EVENT_SOURCE_MAX_SIZE
 * @param userData [IN] User data
 * @return NULL-Failed
 */
EventSource *EventSourceNew(const EventSourceOps *ops, uint32_t size, const char *name, void *userData);

/**
 * @brief Free EventSource
 *
 * @param source [IN] EventSource context
 * @attention Source added to EventLoop can only be deleted use EventLoopDelSource
 */
void EventSourceFree(EventSource *source);

#ifdef __cplusplus
}
#endif

#endif /* EVENT_SOURCE_H */