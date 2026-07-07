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
#include "sched_main_loop.h"
#include "sched_event_loop.h"
#include "sched_timer.h"
#include "main_loop.h"
#include "comm_def.h"
#include "utils_common.h"
#include "iotc_log.h"
#include "dfx_watch_dog.h"
#include "iotc_errcode.h"

#define SCHED_MAIN_LOOP_TASK_NAME "SCHED_LOOP"

#ifdef IOTC_CONNECT_DEVICE_WATCH_DOG_SUPPORT
static void EventLoopFeedWatchDogCallback(void)
{
    int32_t ret = DfxFeedWatchDog();
    if (ret != IOTC_OK) {
        IOTC_LOGW("feed watch dog error %d", ret);
    }
}
#endif

static int32_t SchedMainLoopEntry(void *userData)
{
    NOT_USED(userData);
#ifdef IOTC_CONNECT_DEVICE_WATCH_DOG_SUPPORT
    int32_t ret = DfxAddWatchDog(IOTC_CONF_WATCH_DOG_TIMEOUT, NULL, SCHED_MAIN_LOOP_TASK_NAME);
    if (ret != IOTC_OK) {
        IOTC_LOGW("add watch dog error %d", ret);
    }

    EventLoopAddIdle(GetSchedEventLoop(), EventLoopFeedWatchDogCallback);
#endif
    EventLoopRun(GetSchedEventLoop());
#ifdef IOTC_CONNECT_DEVICE_WATCH_DOG_SUPPORT
    DfxDelWatchDog();
#endif
    return IOTC_OK;
}

static void SchedMainLoopExit(void *userData)
{
    NOT_USED(userData);
    EventLoopQuit(GetSchedEventLoop());
}

int32_t SchedMainLoopInit(void)
{
    static FwkLoop wifiSchedLoop = {
        .name = SCHED_MAIN_LOOP_TASK_NAME,
        .entry = SchedMainLoopEntry,
        .exit = SchedMainLoopExit,
        .userData = NULL,
    };
    int32_t ret = FwkMainLoopReg(&wifiSchedLoop);
    if (ret != IOTC_OK) {
        IOTC_LOGW("sched main loop reg error %d", ret);
        return ret;
    }
    return IOTC_OK;
}