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
#ifndef ADAPTER_NETWORK_H
#define ADAPTER_NETWORK_H

#include <stdint.h>
#include "adapter_wifi_def.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ADAPTER_NETWORK_NOT_CONNECTED = 0,
    ADAPTER_NETWORK_CONNECTED,
} AdapterNetworkState;

int32_t AdapterGetLocalIp(char *buf, uint32_t len);

int32_t AdapterGetMacAddr(uint8_t *buf, uint32_t len);

int32_t AdapterGetBroadcastAddr(char *buf, uint32_t len);

AdapterNetworkState AdapterGetNetworkState(void);

#ifdef __cplusplus
}
#endif

#endif /* ADAPTER_NETWORK_H */
