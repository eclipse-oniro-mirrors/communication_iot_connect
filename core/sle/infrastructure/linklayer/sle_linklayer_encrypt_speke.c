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

#include "sle_linklayer_encrypt_speke.h"
#include "utils_assert.h"
#include "iotc_mem.h"
#include "securec.h"
#include "iotc_errcode.h"

static SleLinkLayerGetSpekeSession g_sleSpekeSessionGetCb = NULL;

static SpekeSession *SleGetLinkLayerSpekeSession(uint32_t connId)
{
    CHECK_RETURN(g_sleSpekeSessionGetCb != NULL, NULL);
    return g_sleSpekeSessionGetCb(connId);
}

int32_t SleLinkLayerRegisterSpekeSessionGetCb(SleLinkLayerGetSpekeSession cb)
{
    g_sleSpekeSessionGetCb = cb;
    return IOTC_OK;
}

int32_t SleLinkLayerSpekeEncrypt(uint32_t connId, const uint8_t *data, uint32_t dataLen,
    uint8_t **outData, uint32_t *outDataLen)
{
    CHECK_RETURN((data != NULL) && (dataLen > 0), IOTC_ERR_PARAM_INVALID);
    CHECK_RETURN((outData != NULL) && (outDataLen != NULL), IOTC_ERR_PARAM_INVALID);
    SpekeSession *session = SleGetLinkLayerSpekeSession(connId);
    CHECK_RETURN_LOGE(session != NULL, IOTC_CORE_SLE_LL_ERR_SPEKE_NULL, "ll speke session null");
    return SpekeEncryptData(session, data, dataLen, outData, outDataLen);
}

int32_t SleLinkLayerSpekeDecrypt(uint32_t connId, uint8_t *data, uint32_t *dataLen)
{
    CHECK_RETURN((data != NULL) && (dataLen != NULL), IOTC_ERR_PARAM_INVALID);
    SpekeSession *session = SleGetLinkLayerSpekeSession(connId);
    CHECK_RETURN_LOGE(session != NULL, IOTC_CORE_SLE_LL_ERR_SPEKE_NULL, "ll speke session null");

    uint8_t *decData = NULL;
    uint32_t decDataLen = 0;
    int32_t ret = SpekeDecryptData(session, data, *dataLen, &decData, &decDataLen);
    CHECK_RETURN(ret == IOTC_OK, ret);

    ret = memcpy_s(data, *dataLen, decData, decDataLen);
    if (ret != EOK) {
        IOTC_LOGE("speke memcpy buffLen:%u, decDataLen:%u err", *dataLen, decDataLen);
        IotcFree(decData);
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    IotcFree(decData);
    if (decDataLen < *dataLen) {
        data[decDataLen] = 0;
    }
    *dataLen = decDataLen;
    return IOTC_OK;
}