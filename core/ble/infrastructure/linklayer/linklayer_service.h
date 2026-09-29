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
#ifndef BLE_LINKLAYER_SERVICE_H
#define BLE_LINKLAYER_SERVICE_H

#include <stdint.h>
#include "ble_linklayer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SVC_TYPE_LEN    1
#define SVC_TYPE_SHIFT  4    /* 类型/操作字节：类型占高 4 位 */
#define SVC_TYPE_MASK   0x0F /* 类型/操作字节：低 4 位掩码 */
#define SVC_LEN_LEN     1
#define SVC_PAYLOAD_LEN_LEN     2

int32_t LinkLayerProcessData(const uint8_t *buff, uint32_t len, LinkLayerEncryptType encryptType,
    uint8_t **outBuff, uint32_t *outLen);

int32_t DecodeCmdData(const uint8_t *buff, uint32_t len, BtCmdParam *cmdParam);

/**
 * @brief 向 buf 写入 svc 帧头（类型/操作字节 + 服务长度 + 服务名 + payload 长度2字节）
 *
 * @param buf [OUT] 目标缓冲
 * @param bufCap [IN] 缓冲容量，需不小于 SVC_TYPE_LEN+SVC_LEN_LEN+服务名长+SVC_PAYLOAD_LEN_LEN
 * @param cmdParam [IN] 命令参数（dataFormat/opType/service）
 * @param payloadLen [IN] payload 长度
 * @param headerLen [OUT] 实际写入的帧头长度
 * @return 0成功，非0失败
 */
int32_t LinkLayerWriteSvcHeader(uint8_t *buf, uint32_t bufCap, const BtCmdParam *cmdParam,
    uint32_t payloadLen, uint32_t *headerLen);

#ifdef __cplusplus
}
#endif

#endif /* BLE_LINKLAYER_SERVICE_H */