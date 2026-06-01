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
#ifndef IOTC_OH_DEVICE_H
#define IOTC_OH_DEVICE_H

#include <stdint.h>
#include <stddef.h>
#include "iotc_prof_def.h"
#include "iotc_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IOTC_OH_SN_STR_MAX_LEN 39
#define IOTC_OH_PRO_ID_STR_LEN 5
#define IOTC_OH_DEV_TYPE_ID_STR_LEN 4
#define IOTC_OH_SUB_PRO_ID_STR_LEN 2
#define IOTC_OH_MODEL_STR_MAX_LEN 31
#define IOTC_OH_DEV_TYPE_NAME_STR_MAX_LEN 31
#define IOTC_OH_MANU_ID_STR_LEN 3
#define IOTC_OH_MANU_NAME_STR_MAX_LEN 31
#define IOTC_OH_DEV_NAME_STR_MAX_LEN 31
#define IOTC_OH_FIRMWARE_VER_STR_MAX_LEN 63
#define IOTC_OH_HARDWARE_VER_STR_MAX_LEN 63
#define IOTC_OH_SOFTWARE_VER_STR_MAX_LEN 63
#define IOTC_OH_SVC_TYPE_STR_MAX_LEN 31
#define IOTC_OH_SVC_ID_STR_MAX_LEN 63

typedef int32_t (*IotcDevProfPutCharState)(const IotcCharState state[], uint32_t num);
typedef int32_t (*IotcDevProfGetCharState)(const IotcCharState state[], char *out[], uint32_t len[], uint32_t num);
typedef int32_t (*IotcDevProfReportAll)(void);
typedef int32_t (*IotcDevProfGetPincode)(uint8_t *buf, uint32_t bufLen);
typedef int32_t (*IotcDevProfGetAcKey)(uint8_t *buf, uint32_t bufLen);
typedef void (*IotcDevProfFree)(void *ptr);
typedef int32_t (*IotcDevReboot)(int32_t res);
typedef int32_t (*IotcDevTrng)(uint8_t *buf, uint32_t len);
typedef int32_t (*IotcDevProfGetCloudRegisterState)(IotcOhCloudRegisterState *state);

IOTC_API_PUBLIC int32_t IotcOhDevInit(void);

IOTC_API_PUBLIC int32_t IotcOhDevDeinit(void);

IOTC_API_PUBLIC int32_t IotcOhDevReportCharState(const IotcCharState state[], uint32_t num);

#ifdef __cplusplus
}
#endif

#endif /* IOTC_OH_DEVICE_H */
