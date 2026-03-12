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
#ifndef SLE_LINKLAYER_RECV_H
#define SLE_LINKLAYER_RECV_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t LinkLayerRecvPkgInsert(uint8_t token, uint8_t pkgNum, uint8_t pkgIdx, const uint8_t *data, uint32_t dataLen);

int32_t LinkLayerRecvCompleteCheck(uint8_t token, bool *isComplete);

int32_t LinkLayerRecvMergePkgs(uint8_t token, uint8_t **outData, uint32_t *outDataLen);

#ifdef __cplusplus
}
#endif

#endif /* SLE_LINKLAYER_RECV_H */