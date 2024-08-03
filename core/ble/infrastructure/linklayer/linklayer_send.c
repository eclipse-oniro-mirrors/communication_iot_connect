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
#include "linklayer_send.h"
#include "linklayer_process.h"
#include "ble_linklayer.h"
#include "utils_common.h"
#include "utils_assert.h"
#include "adapter_mem.h"
#include "adapter_os.h"
#include "securec.h"
#include "iotc_errcode.h"

#define SINGLE_PKG_MAX_SIZE         160
#define SINGLE_PKG_DATA_MAX_SIZE    (SINGLE_PKG_MAX_SIZE - PKG_HEAD_LEN)
#define PKG_MAX_NUM                 16
#define SEND_PKG_INTERVAL           5

static uint8_t g_pkgMaxNum = PKG_MAX_NUM;

static LinkLayerSendBtData g_linkLayerSendBtDataCb = NULL;

int32_t LinkLayerSetSendMaxPkg(uint8_t maxPkg)
{
    CHECK_RETURN(maxPkg > 0, IOTC_ERR_PARAM_INVALID);
    IOTC_LOGI("ll set send max pkg:%u", maxPkg);
    g_pkgMaxNum = maxPkg;
    return IOTC_OK;
}

int32_t LinkLayerRegisterBtDataSendCb(LinkLayerSendBtData cb)
{
    CHECK_RETURN_LOGE(cb != NULL, IOTC_ERR_PARAM_INVALID, "ll reg bt data send cb NULL err");
    g_linkLayerSendBtDataCb = cb;
    return IOTC_OK;
}

int32_t LinkLayerBtDataSend(const uint8_t *buff, uint32_t len)
{
    CHECK_RETURN_LOGE(g_linkLayerSendBtDataCb != NULL, IOTC_ERR_CALLBACK_NULL,
        "ll bt data send cb NULL err");
    int32_t ret = g_linkLayerSendBtDataCb(buff, len);
    CHECK_RETURN_LOGE(ret == IOTC_OK, IOTC_CORE_BLE_LL_ERR_DATA_SEND,
        "ll bt data send err:%d", ret);
    return IOTC_OK;
}

int32_t LinkLayerSendBtPkg(const uint8_t *buff, uint32_t len)
{
    CHECK_RETURN((buff != NULL) && (len > PKG_HEAD_LEN), IOTC_ERR_PARAM_INVALID);

    uint32_t totalDataLen = len - PKG_HEAD_LEN;
    uint32_t pkgNum = totalDataLen / SINGLE_PKG_DATA_MAX_SIZE +
        (totalDataLen % SINGLE_PKG_DATA_MAX_SIZE == 0 ? 0 : 1);
    CHECK_RETURN_LOGE(pkgNum <= g_pkgMaxNum, IOTC_CORE_BLE_LL_ERR_PKGNUM,
        "send pkgNum:%u over limit:%u", pkgNum, g_pkgMaxNum);

    const uint8_t *curPtr = buff + PKG_HEAD_LEN;
    uint32_t leftLen = totalDataLen;
    uint8_t sendBuff[SINGLE_PKG_MAX_SIZE] = { 0 };
    int32_t ret = IOTC_OK;
    for (uint32_t i = 0; i < pkgNum; i++) {
        (void)memset_s(sendBuff, SINGLE_PKG_MAX_SIZE, 0, SINGLE_PKG_MAX_SIZE);
        uint32_t sendBuffLen = UTILS_MIN(SINGLE_PKG_MAX_SIZE, leftLen + PKG_HEAD_LEN);

        ret = memcpy_s(sendBuff, sendBuffLen, buff, PKG_HEAD_LEN);
        CHECK_RETURN(ret == EOK, IOTC_ERR_SECUREC_MEMCPY);
        sendBuff[PKG_HEAD_PKGNUM_IDX] = pkgNum;
        /* i < pkgNum <= g_pkgMaxNum, which is uint8_t */
        sendBuff[PKG_HEAD_INDEX_IDX] = (uint8_t)i;
        ret = memcpy_s(sendBuff + PKG_HEAD_LEN, SINGLE_PKG_DATA_MAX_SIZE, curPtr, sendBuffLen - PKG_HEAD_LEN);
        CHECK_RETURN(ret == EOK, IOTC_ERR_SECUREC_MEMCPY);

        curPtr += sendBuffLen - PKG_HEAD_LEN;
        leftLen -= sendBuffLen - PKG_HEAD_LEN;

        IOTC_LOGI("send bt data pkg(%u/%u), encType:%u, len:%u", sendBuff[PKG_HEAD_INDEX_IDX],
            sendBuff[PKG_HEAD_PKGNUM_IDX], sendBuff[PKG_HEAD_ENCRYPT_TYPE_IDX], sendBuffLen);
        ret = LinkLayerBtDataSend(sendBuff, sendBuffLen);
        CHECK_RETURN(ret == IOTC_OK, ret);

        if (leftLen > 0) {
            AdapterSleepMs(SEND_PKG_INTERVAL);
        }
    }

    return IOTC_OK;
}