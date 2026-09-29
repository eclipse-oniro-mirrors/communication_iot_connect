/*
 * Copyright (c) 2024-2026 ShenZhen Kaihong Device Co., Ltd.
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

#ifndef SLE_CLIENT_EVENT_SERVICE_H
#define SLE_CLIENT_EVENT_SERVICE_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    IOTC_SLE_ADDRESS_TYPE_PUBLIC = 0,   /*!< @if Eng public address
                                                @else   公有地址 @endif */
    IOTC_SLE_ADDRESS_TYPE_RANDOM = 6,   /*!< @if Eng random address
                                                @else   随机地址 @endif */
} IotcSleAddrType;

int32_t ClientSleSpekeStartSession(uint16_t connId);

#ifdef __cplusplus
}
#endif

#endif