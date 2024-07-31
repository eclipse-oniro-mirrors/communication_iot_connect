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
#ifndef IOTC_SERVICE_SOFTAP_H
#define IOTC_SERVICE_SOFTAP_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SOFTAP_SERVICE_MSG_DEVICE_E2E_CONTROL = 0,
    SOFTAP_SERVICE_MSG_DEVICE_CLOUD_SETUP,
    SOFTAP_SERVICE_MSG_ID_MAX,
} SoftapSvcMsgId;

typedef enum {
    IOTC_WIFI_SERVICE_SOFTAP_NETCFG = 0,
    IOTC_WIFI_SERVICE_SOFTAP_E2E_CTRL,
    IOTC_WIFI_SERVICE_SOFTAP_CUSTOM_SSID,
    IOTC_WIFI_SERVICE_SOFTAP_TIMEOUT,
} SoftapSvcBitMap;

typedef int32_t (*SoftapCustomBuildSsidCallback)(uint8_t *ssid, uint32_t *len);

typedef struct {
    uint32_t bitMap;
    uint32_t timeoutMs;
    SoftapCustomBuildSsidCallback onBuildSsid;
} SoftapSvcInitParam;

#ifdef __cplusplus
}
#endif
#endif /* IOTC_SERVICE_SOFTAP_H */