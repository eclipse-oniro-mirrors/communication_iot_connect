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
#ifndef PROFILE_REPORT_H
#define PROFILE_REPORT_H

#include <stdint.h>
#include <stddef.h>
#include "iotc_prof_def.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t (*UplinkReportFunc)(const IotcCharState state[], uint32_t num);

int32_t ProfileReportInit(void);

void ProfileReportDeinit(void);

int32_t ProfileReportRegUplink(UplinkReportFunc func, const char *name);

void ProfileReportUnregUplink(const UplinkReportFunc uplink);

int32_t ProfileReportCharState(const IotcCharState state[], uint32_t num);

#ifdef __cplusplus
}
#endif

#endif /* PROFILE_REPORT_H */
