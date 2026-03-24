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
#ifndef IOTC_OH_SLE_H
#define IOTC_OH_SLE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "iotc_def.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t (*IotcSleRecvNetCfgInfoCallback)(const char *netInfo, uint32_t len);

typedef int32_t (*IotcSleRecvCustomSecDataCallback)(const uint8_t *data, uint32_t len);

typedef int32_t (*IotcSleProfReportByDevIdCallback)(const char *devId);

IOTC_API_PUBLIC int32_t IotcOhSleEnable(void);

IOTC_API_PUBLIC int32_t IotcOhSleStartSeek(void);

IOTC_API_PUBLIC int32_t IotcOhSleDisable(void);

IOTC_API_PUBLIC int32_t IotcOhSleRelease(void);

IOTC_API_PUBLIC int32_t IotcOhSleStartAdv(uint32_t ms);

IOTC_API_PUBLIC int32_t IotcOhSleStopAdv(void);

IOTC_API_PUBLIC int32_t IotcOhSleSendCustomSecData(const char *devId, uint8_t *data, uint32_t len);

IOTC_API_PUBLIC int32_t IotcOhSleSendIndicateData(const char *svcUuid, const char *charUuid,
    const uint8_t *value, uint32_t valueLen);

IOTC_API_PUBLIC int32_t IotcOhFindDeviceInfo(const char *devId, void **info);

#ifdef __cplusplus
}
#endif

#endif /* IOTC_OH_SLE_H */
