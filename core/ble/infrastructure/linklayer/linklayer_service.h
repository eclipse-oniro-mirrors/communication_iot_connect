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
#ifndef BLE_LINKLAYER_SERVICE_H
#define BLE_LINKLAYER_SERVICE_H

#include <stdint.h>
#include "ble_linklayer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SVC_TYPE_LEN    1
#define SVC_LEN_LEN     1
#define SVC_PAYLOAD_LEN_LEN     2

int32_t LinkLayerProcessData(const uint8_t *buff, uint32_t len, LinkLayerEncryptType encryptType,
    uint8_t **outBuff, uint32_t *outLen);

int32_t DecodeCmdData(const uint8_t *buff, uint32_t len, BtCmdParam *cmdParam);

#ifdef __cplusplus
}
#endif

#endif /* BLE_LINKLAYER_SERVICE_H */