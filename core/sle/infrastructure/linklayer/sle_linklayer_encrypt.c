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

#include "sle_linklayer_encrypt.h"
#include "sle_linklayer_encrypt_speke.h"
#include "sle_linklayer_encrypt_sesskey.h"
#include "sle_linklayer_service.h"
#include "sle_linklayer.h"
#include "utils_common.h"
#include "utils_assert.h"
#include "iotc_mem.h"
#include "securec.h"
#include "iotc_errcode.h"

typedef struct {
    SleLinkLayerEncryptType type;
    int32_t (*encryptCb)(uint32_t connId, const uint8_t *data, uint32_t dataLen,
        uint8_t **encData, uint32_t *encDataLen);
    int32_t (*decryptCb)(uint32_t connId, uint8_t *data, uint32_t *dataLen);
} SleEncryptHandler;

static int32_t UnencryptedBuffEnc(uint32_t connId,const uint8_t *data, uint32_t dataLen, uint8_t **outData, uint32_t *outDataLen)
{
    NOT_USED(connId);
    *outData = UtilsMallocCopy(data, dataLen);
    CHECK_RETURN(*outData != NULL, IOTC_ADAPTER_MEM_ERR_MALLOC);
    *outDataLen = dataLen;
    return IOTC_OK;
}

static int32_t UnencryptedBuffDec(uint32_t connId, uint8_t *data, uint32_t *dataLen)
{
    NOT_USED(connId);
    NOT_USED(data);
    NOT_USED(dataLen);
    return IOTC_OK;
}

static SleEncryptHandler g_encryptSleHandler[] = {
    { SLE_ENC_TYPE_UNENCRYPTED, UnencryptedBuffEnc, UnencryptedBuffDec },
    { SLE_ENC_TYPE_SPEKE, SleLinkLayerSpekeEncrypt, SleLinkLayerSpekeDecrypt },
};

static SleLinkLayerEncryptType g_encryptSleType = SLE_ENC_TYPE_SPEKE;


int32_t SleLinkLayerSetEncryptType(SleLinkLayerEncryptType encryptType)
{
    for (uint8_t i = 0; i < sizeof(g_encryptSleHandler) / sizeof(g_encryptSleHandler[0]); i++) {
        if (g_encryptSleHandler[i].type != encryptType) {
            continue;
        }
        g_encryptSleType = encryptType;
        return IOTC_OK;
    }

    IOTC_LOGE("ll set enc type:%d err", g_encryptSleType);
    return IOTC_CORE_SLE_LL_ERR_ENCRYPT_TYPE;
}

SleLinkLayerEncryptType SleLinkLayerGetEncryptType(void)
{
    if (SleLinkLayerSessKeyExist()) {
        return SLE_ENC_TYPE_SESSKEY;
    }
    return g_encryptSleType;
}

int32_t SleLinkLayerDecryptData(uint32_t connId, uint8_t *data, uint32_t *dataLen, SleLinkLayerEncryptType encryptType)
{
    CHECK_RETURN((data != NULL) && (dataLen != NULL), IOTC_ERR_PARAM_INVALID);

    for (uint8_t i = 0; i < sizeof(g_encryptSleHandler) / sizeof(g_encryptSleHandler[0]); i++) {
        if (g_encryptSleHandler[i].type != encryptType) {
            continue;
        }
        IOTC_LOGI("dec ll data[%u] with type:%d", *dataLen, encryptType);
        return g_encryptSleHandler[i].decryptCb(connId, data, dataLen);
    }

    IOTC_LOGE("ble link layer decrypt type:%d err", encryptType);
    return IOTC_CORE_SLE_LL_ERR_ENCRYPT_TYPE;
}

int32_t SleLinkLayerEncryptData(uint32_t connId, const SleLinkLayerEncryptParam *param)
{
    CHECK_RETURN(param != NULL, IOTC_ERR_PARAM_INVALID);
    CHECK_RETURN((param->data != NULL) && (param->dataLen > 0), IOTC_ERR_PARAM_INVALID);
    CHECK_RETURN((param->encData != NULL) && (param->encDataLen != NULL), IOTC_ERR_PARAM_INVALID);

    for (uint8_t i = 0; i < sizeof(g_encryptSleHandler) / sizeof(g_encryptSleHandler[0]); i++) {
        if (g_encryptSleHandler[i].type != param->encryptType) {
            continue;
        }
        IOTC_LOGI("enc ll data[%u] with type:%d", param->dataLen, param->encryptType);
        return g_encryptSleHandler[i].encryptCb(connId, param->data, param->dataLen,
            param->encData, param->encDataLen);
    }

    IOTC_LOGE("ble link layer encrypt type:%d err", param->encryptType);
    return IOTC_CORE_SLE_LL_ERR_ENCRYPT_TYPE;
}