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
#ifndef SCHEDULE_TIMER_H
#define SCHEDULE_TIMER_H
#include <stdint.h>
#include "event_source_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t SchedTimerAdd(EventSourceTimerType type, EventSourceTimerCallback cb, uint32_t ms, void *userData);

void SchedTimerRemove(int32_t id);

void SchedTimerUpdate(int32_t id, EventSourceTimerType type, uint32_t ms);

EventSourceTimerStatus SchedTimerGetStatus(int32_t id);

uint32_t SchedGetDefaultTimerInterval(void);

int32_t SchedTimerInit(void);

void SchedTimerDeinit(void);

#ifdef __cplusplus
}
#endif

#endif /* SCHEDULE_TIMER_H */