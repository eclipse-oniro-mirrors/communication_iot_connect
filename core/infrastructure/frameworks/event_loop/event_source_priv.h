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
#ifndef EVENT_SOURCE_PRIV_H
#define EVENT_SOURCE_PRIV_H

#include "event_source.h"

#ifdef __cplusplus
extern "C" {
#endif

bool EventSourcePrepare(EventSource *source, uint32_t *timeout);
bool EventSourceCheck(EventSource *source);
bool EventSourceDispatch(EventSource *source);
bool EventSourcePoll(EventSource *source, uint32_t timeout);
bool EventSourceIsPoll(EventSource *source);

#ifdef __cplusplus
}
#endif

#endif /* EVENT_SOURCE_PRIV_H */