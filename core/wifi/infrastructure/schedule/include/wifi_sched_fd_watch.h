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
#ifndef WIFI_SCHED_FD_WATCH_H
#define WIFI_SCHED_FD_WATCH_H
#include <stdint.h>
#include "event_source_fd.h"
#include "trans_link.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t WifiSchedLinkRecvWatch(TransLink *link);

int32_t WifiSchedFdWatch(const FdWatchParam *watch);

int32_t WifiSchedFdSuspend(int32_t fd);

int32_t WifiSchedFdResume(int32_t fd);

void WifiSchedFdRemove(int32_t fd);

int32_t WifiSchedFdWatchInit(void);

void WifiSchedFdWatchDeinit(void);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_SCHED_FD_WATCH_H */