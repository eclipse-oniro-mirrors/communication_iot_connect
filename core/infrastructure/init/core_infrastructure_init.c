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
#include "core_infrastructure_init.h"
#include "fwk_init.h"
#include "utils_mutex_global.h"
#include "security_random.h"
#include "iotc_log.h"
#include "dfx_watch_dog.h"
#include "main_loop.h"
#include "event_bus.h"
#include "utils_common.h"
#include "utils_time.h"
#include "netcfg_service.h"
#include "event_bus_public.h"
#include "sched_event_loop.h"
#include "sched_timer.h"
#include "sched_main_loop.h"
#include "sched_msg_queue.h"
#include "sched_executor.h"
#include "service_manager.h"

static const FwkInitUnit CORE_COMM[] = {
    {FWK_INIT_LVL_DEP, "glock", UtilsGlobalMutexInit, UtilsGlobalMutexDeinit},
    {FWK_INIT_LVL_DEP, "fwk_loop", FwkMainLoopInit, FwkMainLoopDeinit},
    {FWK_INIT_LVL_DEP, "fwk_event", EventBusInit, EventBusDeinit},
    {FWK_INIT_LVL_DEP, "event_pub", IotcPublicEventListenerInit, IotcPublicEventListenerDeinit},
    {FWK_INIT_LVL_DEP, "random", SecurityRandomInit, SecurityRandomDeinit},
    {FWK_INIT_LVL_DEP, "service", ServiceManagerInit, ServiceManagerDeinit},
    {FWK_INIT_LVL_DEP, "watch_dog", DfxWatchDogInit, DfxWatchDogDeinit},
    {FWK_INIT_LVL_DEP, "sched_executor", SchedAsyncExecutorInit, SchedAsyncExecutorDeinit},
    {FWK_INIT_LVL_DEP, "sched_event_loop", SchedEventLoopInit, SchedEventLoopDeinit},
    {FWK_INIT_LVL_DEP, "sched_timer", SchedTimerInit, SchedTimerDeinit},
    {FWK_INIT_LVL_DEP, "sched_main_loop", SchedMainLoopInit, NULL},
    {FWK_INIT_LVL_DEP, "sched_msg_queue", SchedMsgQueueInit, SchedMsgQueueDeinit},
    {FWK_INIT_LVL_DEP, "time", UtilsTimeInit, UtilsTimeDeinit},
};

const FwkInitUnit *CoreInfrastructureGetInitUnit(void)
{
    return CORE_COMM;
}

uint32_t CoreInfrastructureGetInitUnitSize(void)
{
    return ARRAY_SIZE(CORE_COMM);
}

const char *CoreInfrastructureGetInitName(void)
{
    return TO_STR(CORE_COMM);
}