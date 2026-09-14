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
#ifndef BLE_LINKLAYER_ENCRYPT_H
#define BLE_LINKLAYER_ENCRYPT_H

#include <stdint.h>
#include <stdbool.h>
#include "security_speke.h"
#include "ble_linklayer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 设置加密方式
 *
 * @param encryptType [IN] 加密方式
 * @return 0成功，非0失败
 */
int32_t LinkLayerSetEncryptType(LinkLayerEncryptType encryptType);

/**
 * @brief 获取加密方式
 *
 * @return 加密方式
 */
LinkLayerEncryptType LinkLayerGetEncryptType(void);

/**
 * @brief 解密数据
 *
 * @param data [IN/OUT] 业务数据
 * @param dataLen [IN/OUT] 数据长度
 * @param encryptType [IN] 解密方式
 * @return 0成功，非0失败
 */
int32_t LinkLayerDecryptData(uint8_t *data, uint32_t *dataLen, LinkLayerEncryptType encryptType);

/**
 * @brief 加密数据
 *
 * @param data [IN] 业务数据
 * @param dataLen [IN] 数据长度
 * @param encryptType [IN] 加密方式
 * @param encData [OUT] 加密数据
 * @param encDataLen [OUT] 加密数据长度
 * @return 0成功，非0失败
 * @attention caller frees encData
 */
int32_t LinkLayerEncryptData(const uint8_t *data, uint32_t dataLen, LinkLayerEncryptType encryptType,
    uint8_t **encData, uint32_t *encDataLen);

/**
 * @brief encrypt into caller-provided buffer, no intermediate encBuff
 */
/* Extra bytes added by encryption, used to size the send buffer before encrypting */
#define SPEKE_ENC_DATA_MAX_OVERHEAD 29  /* SPEKE frame: ver 1 + iv 12 + tag 16 */
#define SESS_ENC_DATA_MAX_OVERHEAD  92  /* sesskey frame: iv 12 + tag 16 + sessId 32 + hmac 32 */
#define ENC_DATA_MAX_OVERHEAD       (SESS_ENC_DATA_MAX_OVERHEAD > SPEKE_ENC_DATA_MAX_OVERHEAD ? \
                                     SESS_ENC_DATA_MAX_OVERHEAD : SPEKE_ENC_DATA_MAX_OVERHEAD)

/* Destination buffer for the *Into encrypt APIs: the API writes ciphertext into
 * buff and reports the actual length through *buffLen */
typedef struct {
    uint8_t *buff;       /* ciphertext destination */
    uint32_t buffCap;    /* capacity of buff */
    uint32_t *buffLen;   /* [OUT] actual encrypted length */
} LinkLayerEncryptOut;

int32_t LinkLayerEncryptDataInto(const uint8_t *data, uint32_t dataLen,
    LinkLayerEncryptType encryptType, const LinkLayerEncryptOut *out);

#ifdef __cplusplus
}
#endif

#endif /* BLE_LINKLAYER_ENCRYPT_H */