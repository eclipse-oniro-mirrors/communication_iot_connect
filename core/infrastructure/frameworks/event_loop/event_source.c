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
#include "event_source.h"
#include "iotc_log.h"
#include "adapter_mem.h"
#include "securec.h"
#include "utils_assert.h"
#include "utils_common.h"

EventSource *EventSourceNew(const EventSourceOps *ops, uint32_t size, const char *name, void *userData)
{
    CHECK_RETURN(ops != NULL, NULL);
    if ((size < sizeof(EventSource)) || (size > EVENT_SOURCE_MAX_SIZE)) {
        IOTC_LOGW("check source size failed, size[%u]", size);
        return NULL;
    }

    EventSource *source = (EventSource *)IotcMalloc(size);
    if (source == NULL) {
        IOTC_LOGW("malloc source failed, size[%u]", size);
        return NULL;
    }
    (void)memset_s(source, size, 0, size);

    source->name = name;
    source->ops = ops;
    source->userData = userData;
    return source;
}

void EventSourceFree(EventSource *source)
{
    CHECK_V_RETURN(source != NULL);

    if ((source->ops != NULL) && (source->ops->finalize != NULL)) {
        source->ops->finalize(source);
    }
    IotcFree(source);
}

bool EventSourcePrepare(EventSource *source, uint32_t *timeout)
{
    CHECK_RETURN((source != NULL) && (timeout != NULL), false);
    if ((source->ops != NULL) && (source->ops->prepare != NULL)) {
        return source->ops->prepare(source, timeout);
    }
    return false;
}

bool EventSourceCheck(EventSource *source)
{
    CHECK_RETURN(source != NULL, false);
    if ((source->ops != NULL) && (source->ops->check != NULL)) {
        return source->ops->check(source);
    }
    return false;
}

bool EventSourceDispatch(EventSource *source)
{
    CHECK_RETURN(source != NULL, false);
    if ((source->ops != NULL) && (source->ops->check != NULL)) {
        return source->ops->dispatch(source);
    }
    return false;
}

bool EventSourcePoll(EventSource *source, uint32_t timeout)
{
    CHECK_RETURN(source != NULL, false);
    if ((source->ops != NULL) && (source->ops->poll != NULL)) {
        return source->ops->poll(source, timeout);
    }
    return false;
}

bool EventSourceIsPoll(EventSource *source)
{
    CHECK_RETURN(source != NULL, false);
    return source->ops != NULL && source->ops->poll != NULL;
}