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
#ifndef IOTC_PROFILE_H
#define IOTC_PROFILE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IOTC_PROT_TYPE_INVALID = 0,
    IOTC_PROT_TYPE_WIFI,
    IOTC_PROT_TYPE_Z_WAVE,
    IOTC_PROT_TYPE_ZIGBEE,
    IOTC_PROT_TYPE_BLE,
    IOTC_PROT_TYPE_PLC,
    IOTC_PROT_TYPE_BLE_MESH,
    IOTC_PROT_TYPE_NFC,
    IOTC_PROT_TYPE_ETHERNET,
    IOTC_PROT_TYPE_MOBILE,
    IOTC_PROT_TYPE_USB,
    IOTC_PROT_TYPE_PLC_AND_WIFI,
    IOTC_PROT_TYPE_BLE_AND_WIFI,
    IOTC_PROT_TYPE_HIBEACON,
    IOTC_PROT_TYPE_VIRTUAL = 100,
} IotcProtType;

typedef struct {
    const char *sn;
    const char *prodId;
    const char *subProdId;
    const char *model;
    const char *devTypeId;
    const char *devTypeName;
    const char *manuId;
    const char *manuName;
    const char *fwv;
    const char *hwv;
    const char *swv;
    int8_t protType;
} IotcDeviceInfo;

typedef struct {
    const char *svcType;
    const char *svcId;
} IotcServiceInfo;

typedef struct {
    const char *svcId;
    const char *data;
    uint32_t len;
    const char *devId;
    const char *msgId;
} IotcCharState;

#ifdef __cplusplus
}
#endif

#endif /* IOTC_PROFILE_H */
