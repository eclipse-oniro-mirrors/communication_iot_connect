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
#include "linklayer_encrypt.h"
#include "linklayer_encrypt_speke.h"
#include "linklayer_encrypt_sesskey.h"
#include "linklayer_service.h"
#include "ble_linklayer.h"
#include "utils_common.h"
#include "utils_assert.h"
#include "adapter_mem.h"
#include "securec.h"
#include "iotc_errcode.h"

typedef struct {
    LinkLayerEncryptType type;
    int32_t (*encryptCb)(const uint8_t *data, uint32_t dataLen, uint8_t **encData, uint32_t *encDataLen);
    int32_t (*decryptCb)(uint8_t *data, uint32_t *dataLen);
} EncryptHandler;

static int32_t UnencryptedBuffEnc(const uint8_t *data, uint32_t dataLen, uint8_t **outData, uint32_t *outDataLen);
static int32_t UnencryptedBuffDec(uint8_t *data, uint32_t *dataLen);

static EncryptHandler g_encryptHandler[] = {
    { ENC_TYPE_UNENCRYPTED, UnencryptedBuffEnc, UnencryptedBuffDec },
    { ENC_TYPE_SPEKE, LinkLayerSpekeEncrypt, LinkLayerSpekeDecrypt },
    { ENC_TYPE_SESSKEY, LinkLayerSessKeyEncrypt, LinkLayerSessKeyDecrypt },
};

static LinkLayerEncryptType g_encryptType = ENC_TYPE_UNENCRYPTED;

static int32_t UnencryptedBuffEnc(const uint8_t *data, uint32_t dataLen, uint8_t **outData, uint32_t *outDataLen)
{
    *outData = UtilsMallocCopy(data, dataLen);
    CHECK_RETURN(*outData != NULL, IOTC_ADAPTER_MEM_ERR_MALLOC);
    *outDataLen = dataLen;
    return IOTC_OK;
}

static int32_t UnencryptedBuffDec(uint8_t *data, uint32_t *dataLen)
{
    NOT_USED(data);
    NOT_USED(dataLen);
    return IOTC_OK;
}

int32_t LinkLayerSetEncryptType(LinkLayerEncryptType encryptType)
{
    for (uint8_t i = 0; i < sizeof(g_encryptHandler) / sizeof(g_encryptHandler[0]); i++) {
        if (g_encryptHandler[i].type != encryptType) {
            continue;
        }
        g_encryptType = encryptType;
        return IOTC_OK;
    }

    IOTC_LOGE("ll set enc type:%d err", encryptType);
    return IOTC_CORE_BLE_LL_ERR_ENCRYPT_TYPE;
}

LinkLayerEncryptType LinkLayerGetEncryptType(void)
{
    if (LinkLayerSessKeyExist()) {
        return ENC_TYPE_SESSKEY;
    }
    return g_encryptType;
}

int32_t LinkLayerDecryptData(uint8_t *data, uint32_t *dataLen, LinkLayerEncryptType encryptType)
{
    CHECK_RETURN((data != NULL) && (dataLen != NULL), IOTC_ERR_PARAM_INVALID);

    for (uint8_t i = 0; i < sizeof(g_encryptHandler) / sizeof(g_encryptHandler[0]); i++) {
        if (g_encryptHandler[i].type != encryptType) {
            continue;
        }
        IOTC_LOGI("dec ll data[%u] with type:%d", *dataLen, encryptType);
        return g_encryptHandler[i].decryptCb(data, dataLen);
    }

    IOTC_LOGE("ble link layer decrypt type:%d err", encryptType);
    return IOTC_CORE_BLE_LL_ERR_ENCRYPT_TYPE;
}

int32_t LinkLayerEncryptData(const uint8_t *data, uint32_t dataLen, LinkLayerEncryptType encryptType,
    uint8_t **encData, uint32_t *encDataLen)
{
    CHECK_RETURN((data != NULL) && (dataLen > 0), IOTC_ERR_PARAM_INVALID);
    CHECK_RETURN((encData != NULL) && (encDataLen != NULL), IOTC_ERR_PARAM_INVALID);

    for (uint8_t i = 0; i < sizeof(g_encryptHandler) / sizeof(g_encryptHandler[0]); i++) {
        if (g_encryptHandler[i].type != encryptType) {
            continue;
        }
        IOTC_LOGI("enc ll data[%u] with type:%d", dataLen, encryptType);
        return g_encryptHandler[i].encryptCb(data, dataLen, encData, encDataLen);
    }

    IOTC_LOGE("ble link layer encrypt type:%d err", encryptType);
    return IOTC_CORE_BLE_LL_ERR_ENCRYPT_TYPE;
}