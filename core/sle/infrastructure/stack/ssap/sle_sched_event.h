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
#ifndef SLE_SCHEDULE_EVENT_H
#define SLE_SCHEDULE_EVENT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SLE_EVENT_START = 0,
    SLE_EVENT_CONNECT,
    SLE_EVENT_SEND_INDICATE,
    SLE_EVENT_INDICATE_CONF,
    SLE_EVENT_DISCONNECT,
    SLE_EVENT_START_SVC_RESULT,
    SLE_EVENT_STOP_SVC_RESULT,
    SLE_EVENT_SET_MTU_RESULT,
    SLE_EVENT_START_ADV_RESULT,
    SLE_EVENT_STOP_ADV_RESULT,
    SLE_EVENT_REQ_READ,
    SLE_EVENT_REQ_WRITE,
} SleScheduleEvent;

typedef struct {
    int32_t event;
    void *param;
    void (*free)(void *param);
} SleSchedMsg;

int32_t SleSchedMsgQueueSend(const SleSchedMsg *msg, uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* SLE_SCHEDULE_EVENT_H */