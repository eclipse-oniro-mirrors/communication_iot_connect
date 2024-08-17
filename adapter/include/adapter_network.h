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
#ifndef IOTC_NETWORK_H
#define IOTC_NETWORK_H

#include <stdint.h>
#include "adapter_wifi_def.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IOTC_NETWORK_NOT_CONNECTED = 0,
    IOTC_NETWORK_CONNECTED,
} IotcNetworkState;

int32_t IotcGetLocalIp(char *buf, uint32_t len);

int32_t IotcGetSoftApIp(char *buf, uint32_t len);

int32_t IotcGetMacAddr(uint8_t *buf, uint32_t len);

int32_t IotcGetBroadcastAddr(char *buf, uint32_t len);

IotcNetworkState IotcGetNetworkState(void);

#ifdef __cplusplus
}
#endif

#endif /* IOTC_NETWORK_H */
