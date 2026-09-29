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
#ifndef BLE_LINKLAYER_ENCRYPT_SPEKE_H
#define BLE_LINKLAYER_ENCRYPT_SPEKE_H

#include <stdint.h>
#include "linklayer_encrypt.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t LinkLayerSpekeEncrypt(const uint8_t *data, uint32_t dataLen, uint8_t **outData, uint32_t *outDataLen);

int32_t LinkLayerSpekeDecrypt(uint8_t *data, uint32_t *dataLen);

/**
 * @brief SPEKE 加密直写到调用方提供的缓冲（无中转分配）
 *
 * @param data [IN] 明文数据
 * @param dataLen [IN] 明文长度
 * @param out [IN/OUT] 目标缓冲（buff/buffCap），实际长度经 *buffLen 返回
 * @return 0成功，非0失败
 */
int32_t LinkLayerSpekeEncryptInto(const uint8_t *data, uint32_t dataLen, const LinkLayerEncryptOut *out);

#ifdef __cplusplus
}
#endif

#endif /* BLE_LINKLAYER_ENCRYPT_SPEKE_H */