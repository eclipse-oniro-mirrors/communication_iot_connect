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
#include "linklayer_encrypt_speke.h"
#include "ble_linklayer.h"
#include "utils_assert.h"
#include "iotc_mem.h"
#include "securec.h"
#include "iotc_errcode.h"

static LinkLayerGetSpekeSession g_spekeSessionGetCb = NULL;

static SpekeSession *GetLinkLayerSpekeSession(void)
{
    CHECK_RETURN(g_spekeSessionGetCb != NULL, NULL);
    return g_spekeSessionGetCb();
}

int32_t LinkLayerRegisterSpekeSessionGetCb(LinkLayerGetSpekeSession cb)
{
    g_spekeSessionGetCb = cb;
    return IOTC_OK;
}

int32_t LinkLayerSpekeEncrypt(const uint8_t *data, uint32_t dataLen, uint8_t **outData, uint32_t *outDataLen)
{
    CHECK_RETURN((data != NULL) && (dataLen > 0), IOTC_ERR_PARAM_INVALID);
    CHECK_RETURN((outData != NULL) && (outDataLen != NULL), IOTC_ERR_PARAM_INVALID);
    SpekeSession *session = GetLinkLayerSpekeSession();
    CHECK_RETURN_LOGE(session != NULL, IOTC_CORE_BLE_LL_ERR_SPEKE_NULL, "ll speke session null");
    return SpekeEncryptData(session, data, dataLen, outData, outDataLen);
}

int32_t LinkLayerSpekeDecrypt(uint8_t *data, uint32_t *dataLen)
{
    CHECK_RETURN((data != NULL) && (dataLen != NULL), IOTC_ERR_PARAM_INVALID);
    SpekeSession *session = GetLinkLayerSpekeSession();
    CHECK_RETURN_LOGE(session != NULL, IOTC_CORE_BLE_LL_ERR_SPEKE_NULL, "ll speke session null");

    uint32_t decDataLen = 0;
    int32_t ret = SpekeDecryptDataInPlace(session, data, *dataLen, &decDataLen);
    CHECK_RETURN(ret == IOTC_OK, ret);

    if (decDataLen < *dataLen) {
        data[decDataLen] = 0;
    }
    *dataLen = decDataLen;
    return IOTC_OK;
}

/* SPEKE 加密直写到 out->buff，省 stage 路径的 encData 中转分配 */
int32_t LinkLayerSpekeEncryptInto(const uint8_t *data, uint32_t dataLen, const LinkLayerEncryptOut *out)
{
    CHECK_RETURN((data != NULL) && (dataLen > 0) && (out != NULL) && (out->buff != NULL) &&
        (out->buffLen != NULL), IOTC_ERR_PARAM_INVALID);
    SpekeSession *session = GetLinkLayerSpekeSession();
    CHECK_RETURN_LOGE(session != NULL, IOTC_CORE_BLE_LL_ERR_SPEKE_NULL, "ll speke session null");

    uint32_t encLen = 0;
    SpekeDataBuf dst = { out->buff, out->buffCap, &encLen };
    int32_t ret = SpekeEncryptDataInto(session, data, dataLen, &dst);
    if (ret != IOTC_OK) {
        return ret;
    }
    *out->buffLen = encLen;
    return IOTC_OK;
}