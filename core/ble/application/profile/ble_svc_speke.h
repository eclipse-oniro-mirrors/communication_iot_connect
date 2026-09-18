/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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
#ifndef BLE_SVC_SPEKE_H
#define BLE_SVC_SPEKE_H

#include <stdint.h>
#include "ble_linklayer.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t PutBleSvcSpeke(const BtCmdParam *param, uint8_t **out, uint32_t *outLen);

/* 帧化回调——响应直接在 [PKG_HEAD_LEN 预留 + svc 帧] 内构造，
   SERVER_RSP 手写序列化直写帧内偏移（零中转拷贝），*outLen 为 svc 帧长度 */
int32_t PutBleSvcSpekeFramed(const BtCmdParam *param, uint8_t **out, uint32_t *outLen);

#ifdef __cplusplus
}
#endif

#endif /* BLE_SVC_SPEKE_H */