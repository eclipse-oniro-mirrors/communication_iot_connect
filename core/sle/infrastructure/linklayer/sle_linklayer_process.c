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
#include "sle_linklayer_process.h"
#include "sle_linklayer_encrypt.h"
#include "sle_linklayer_recv.h"
#include "sle_linklayer_send.h"
#include "sle_linklayer_service.h"
#include "utils_assert.h"
#include "iotc_mem.h"
#include "securec.h"
#include "iotc_errcode.h"

#define BIT_OFFSET_FOUR     4
#define PKG_HEAD_VERSION    0
#define LL_RET_ERR          1

static uint8_t g_sendToken = 0;

typedef struct {
    const uint8_t *reqBuff;
    uint8_t token;
    uint8_t encryptType;
    uint8_t **outData;
    uint32_t *outDataLen;
} SleGetCompleteDataParam;

typedef enum {
    CMD_TYPE_REQUEST    = 0b0000,
    CMD_TYPE_RESPONSE   = 0b0001,
    CMD_TYPE_REPORT     = 0b0010,
} CmdType;

typedef struct {
    uint8_t version;
    CmdType cmdType;
    uint8_t token;
    uint8_t pkgNum;
    uint8_t pkgIdx;
    uint8_t reserved;
    uint8_t encryptType;
    uint8_t ret;
} PkgHead;

static void SleParsePkgHead(const uint8_t *buff, uint32_t len, PkgHead *pkgHead)
{
    /* 第0字节的高4位 */
    pkgHead->version = buff[SLE_PKG_HEAD_TYPE_IDX] >> BIT_OFFSET_FOUR;
    /* 第0字节的低4位 */
    pkgHead->cmdType = buff[SLE_PKG_HEAD_TYPE_IDX] & 0x0F;
    pkgHead->token = buff[SLE_PKG_HEAD_TOKEN_IDX];
    pkgHead->pkgNum = buff[SLE_PKG_HEAD_PKGNUM_IDX];
    pkgHead->pkgIdx = buff[SLE_PKG_HEAD_INDEX_IDX];
    pkgHead->reserved = buff[SLE_PKG_HEAD_RESERVED_IDX];
    pkgHead->encryptType = buff[SLE_PKG_HEAD_ENCRYPT_TYPE_IDX];
    pkgHead->ret = buff[SLE_PKG_HEAD_RET_IDX];

    if (pkgHead->ret != IOTC_OK) {
        IOTC_LOGW("ll parse pkg ret:%u, len = %d", pkgHead->ret, len);
    }
}

static void SleSetPkgHeadVersion(uint8_t head[SLE_PKG_HEAD_LEN], uint8_t version)
{
    /* 报头第0字节的高4位表示版本信息 */
    uint8_t ver = version << 4;
    head[SLE_PKG_HEAD_TYPE_IDX] &= 0x0F;
    head[SLE_PKG_HEAD_TYPE_IDX] |= ver;
}

static void SleSetPkgHeadCmdType(uint8_t head[SLE_PKG_HEAD_LEN], CmdType type)
{
    switch (type) {
        case CMD_TYPE_REQUEST:
            head[SLE_PKG_HEAD_TYPE_IDX] &= 0xF0;
            break;
        case CMD_TYPE_RESPONSE:
            head[SLE_PKG_HEAD_TYPE_IDX] &= 0xF0;
            head[SLE_PKG_HEAD_TYPE_IDX] |= 0x01;
            break;
        case CMD_TYPE_REPORT:
            head[SLE_PKG_HEAD_TYPE_IDX] &= 0xF0;
            head[SLE_PKG_HEAD_TYPE_IDX] |= 0x02;
            break;
        default:
            return;
    }
}

static void SleLinkLayerRspExceptionData(uint32_t connId, const uint8_t *reqBuff, uint8_t retVal)
{
    uint8_t sendBuff[SLE_PKG_HEAD_LEN] = { 0 };
    int32_t ret = memcpy_s(sendBuff, SLE_PKG_HEAD_LEN, reqBuff, SLE_PKG_HEAD_LEN);
    CHECK_V_RETURN(ret == EOK);
    SleSetPkgHeadVersion(sendBuff, PKG_HEAD_VERSION);
    SleSetPkgHeadCmdType(sendBuff, CMD_TYPE_RESPONSE);
    sendBuff[SLE_PKG_HEAD_PKGNUM_IDX] = 1;
    sendBuff[SLE_PKG_HEAD_INDEX_IDX] = 0;
    sendBuff[SLE_PKG_HEAD_RET_IDX] = retVal;

    (void)LinkLayerSleDataSend(connId, sendBuff, SLE_PKG_HEAD_LEN);
}

static int32_t SleGetCompleteData(uint32_t connId, const SleGetCompleteDataParam *p)
{
    uint8_t *mergedBuff = NULL;
    uint32_t mergedBuffLen = 0;
    int32_t ret = SleLinkLayerRecvMergePkgs(p->token, &mergedBuff, &mergedBuffLen);
    CHECK_RETURN((ret == IOTC_OK) && (mergedBuff != NULL) && (mergedBuffLen > 0), ret);

    ret = SleLinkLayerDecryptData(connId, mergedBuff, &mergedBuffLen, p->encryptType);
    if (ret != IOTC_OK) {
        SleLinkLayerRspExceptionData(connId, p->reqBuff, LL_RET_ERR);
        IotcFree(mergedBuff);
        return ret;
    }

    *p->outData = mergedBuff;
    *p->outDataLen = mergedBuffLen;
    return IOTC_OK;
}

static int32_t SleSendRspData(uint32_t connId, const uint8_t *reqBuff, uint8_t encryptType,
    uint8_t *rspData, uint32_t rspDataLen)
{
    uint8_t *encBuff = NULL;
    uint32_t encBuffLen = 0;
    SleLinkLayerEncryptParam encParam = {
        .data = rspData,
        .dataLen = rspDataLen,
        .encryptType = encryptType,
        .encData = &encBuff,
        .encDataLen = &encBuffLen
    };
    int32_t ret = SleLinkLayerEncryptData(connId, &encParam);
    if ((ret != IOTC_OK) || (encBuff == NULL) || (encBuffLen == 0)) {
        SleLinkLayerRspExceptionData(connId, reqBuff, LL_RET_ERR);
        return ret;
    }

    uint32_t sendBuffLen = SLE_PKG_HEAD_LEN + encBuffLen;
    uint8_t *sendBuff = (uint8_t *)IotcCalloc(sendBuffLen, sizeof(uint8_t));
    if (sendBuff == NULL) {
        IOTC_LOGE("ll rsp data Calloc:%u err", sendBuffLen);
        IotcFree(encBuff);
        return IOTC_ADAPTER_MEM_ERR_CALLOC;
    }

    ret  = memcpy_s(sendBuff + SLE_PKG_HEAD_LEN, sendBuffLen - SLE_PKG_HEAD_LEN, encBuff, encBuffLen);
    if (ret != EOK) {
        IotcFree(encBuff);
        IotcFree(sendBuff);
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    IotcFree(encBuff);

    ret = memcpy_s(sendBuff, sendBuffLen, reqBuff, SLE_PKG_HEAD_LEN);
    if (ret != EOK) {
        IotcFree(sendBuff);
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    SleSetPkgHeadCmdType(sendBuff, CMD_TYPE_RESPONSE);
    sendBuff[SLE_PKG_HEAD_RET_IDX] = IOTC_OK;

    ret = SleLinkLayerSendPkg(connId, sendBuff, sendBuffLen);
    IotcFree(sendBuff);
    return ret;
}

int32_t SleLinkLayerProcessData(uint32_t connId, const uint8_t *buff, uint32_t len)
{
    CHECK_RETURN_LOGE((buff != NULL) && (len > SLE_PKG_HEAD_LEN), IOTC_ERR_PARAM_INVALID,
        "ll process bt data param invalid, len:%u", len);

    PkgHead pkgHead = { 0 };
    SleParsePkgHead(buff, len, &pkgHead);
    IOTC_LOGI("recv sle data pkg(%u/%u), encType:%u", pkgHead.pkgIdx, pkgHead.pkgNum, pkgHead.encryptType);

    int32_t ret = SleLinkLayerRecvPkgInsert(pkgHead.token, pkgHead.pkgNum, pkgHead.pkgIdx,
        buff + SLE_PKG_HEAD_LEN, len - SLE_PKG_HEAD_LEN);
    CHECK_RETURN(ret == IOTC_OK, ret);

    ret = SleLinkLayerSetEncryptType(pkgHead.encryptType);
    CHECK_RETURN(ret == IOTC_OK, ret);
    bool isComplete = false;
    ret = SleLinkLayerRecvCompleteCheck(pkgHead.token, &isComplete);
    CHECK_RETURN((ret == IOTC_OK) && isComplete, ret);

    uint8_t *completeBuff = NULL;
    uint32_t completeBuffLen = 0;
    SleGetCompleteDataParam completeParam = {
        .reqBuff = buff,
        .token = pkgHead.token,
        .encryptType = pkgHead.encryptType,
        .outData = &completeBuff,
        .outDataLen = &completeBuffLen
    };
    ret = SleGetCompleteData(connId, &completeParam);
    CHECK_RETURN((ret == IOTC_OK) && (completeBuff != NULL) && (completeBuffLen > 0), ret);

    uint8_t *rspBuff = NULL;
    uint32_t rspBuffLen = 0;
    SleLinkLayerProcessRspParam rspParam = {
        .buff = completeBuff,
        .len = completeBuffLen,
        .encryptType = pkgHead.encryptType,
        .outBuff = &rspBuff,
        .outLen = &rspBuffLen
    };
    ret = SleLinkLayerProcessRspData(connId, &rspParam);
    if ((ret != IOTC_OK) || (rspBuff == NULL) || (rspBuffLen == 0)) {
        SleLinkLayerRspExceptionData(connId, buff, LL_RET_ERR);
        IotcFree(completeBuff);
        return ret;
    }
    IotcFree(completeBuff);

    ret = SleSendRspData(connId, buff, pkgHead.encryptType, rspBuff, rspBuffLen);
    IotcFree(rspBuff);
    return ret;
}

int32_t SleLinkLayerReportEncryptCmdData(uint32_t connId, const uint8_t *buff, uint32_t len)
{
    CHECK_RETURN_LOGE((buff != NULL) && (len > 0), IOTC_ERR_PARAM_INVALID,
        "ll process bt data param invalid, len:%u", len);

        SleLinkLayerEncryptType encType = SleLinkLayerGetEncryptType();

    uint8_t *encBuff = NULL;
    uint32_t encBuffLen = 0;
    SleLinkLayerEncryptParam encParam = {
        .data = buff,
        .dataLen = len,
        .encryptType = encType,
        .encData = &encBuff,
        .encDataLen = &encBuffLen
    };
    int32_t ret = SleLinkLayerEncryptData(connId, &encParam);
    CHECK_RETURN((ret == IOTC_OK) && (encBuff != NULL) && (encBuffLen > 0), ret);

    uint32_t sendBuffLen = SLE_PKG_HEAD_LEN + encBuffLen;
    uint8_t *sendBuff = (uint8_t *)IotcCalloc(sendBuffLen, sizeof(uint8_t));
    if (sendBuff == NULL) {
        IOTC_LOGE("ll rpt data calloc:%u err", sendBuffLen);
        IotcFree(encBuff);
        return IOTC_ADAPTER_MEM_ERR_CALLOC;
    }

    ret  = memcpy_s(sendBuff + SLE_PKG_HEAD_LEN, sendBuffLen - SLE_PKG_HEAD_LEN, encBuff, encBuffLen);
    if (ret != EOK) {
        IotcFree(encBuff);
        IotcFree(sendBuff);
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    IotcFree(encBuff);

    SleSetPkgHeadVersion(sendBuff, PKG_HEAD_VERSION);
    SleSetPkgHeadCmdType(sendBuff, CMD_TYPE_REPORT);
    sendBuff[SLE_PKG_HEAD_TOKEN_IDX] = g_sendToken++;
    sendBuff[SLE_PKG_HEAD_ENCRYPT_TYPE_IDX] = encType;
    sendBuff[SLE_PKG_HEAD_RESERVED_IDX] = 0;
    sendBuff[SLE_PKG_HEAD_RET_IDX] = IOTC_OK;

    ret = SleLinkLayerSendPkg(connId, sendBuff, sendBuffLen);
    IotcFree(sendBuff);
    return ret;
}

int32_t SleLinkLayerReportCmdData(uint32_t connId, const uint8_t *buff, uint32_t len)
{
    CHECK_RETURN_LOGE((buff != NULL) && (len > 0), IOTC_ERR_PARAM_INVALID,
        "ll process bt data param invalid, len:%u", len);

    uint32_t sendBuffLen = SLE_PKG_HEAD_LEN + len;
    uint8_t *sendBuff = (uint8_t *)IotcCalloc(sendBuffLen, sizeof(uint8_t));
    if (sendBuff == NULL) {
        IOTC_LOGE("ll rpt data calloc:%u err", sendBuffLen);
        return IOTC_ADAPTER_MEM_ERR_CALLOC;
    }

    int32_t ret  = memcpy_s(sendBuff + SLE_PKG_HEAD_LEN, sendBuffLen - SLE_PKG_HEAD_LEN, buff, len);
    if (ret != EOK) {
        IotcFree(sendBuff);
        return IOTC_ERR_SECUREC_MEMCPY;
    }

    SleSetPkgHeadVersion(sendBuff, PKG_HEAD_VERSION);
    SleSetPkgHeadCmdType(sendBuff, CMD_TYPE_REPORT);
    sendBuff[SLE_PKG_HEAD_TOKEN_IDX] = g_sendToken++;
    sendBuff[SLE_PKG_HEAD_ENCRYPT_TYPE_IDX] = SLE_ENC_TYPE_UNENCRYPTED;
    sendBuff[SLE_PKG_HEAD_RESERVED_IDX] = 0;
    sendBuff[SLE_PKG_HEAD_RET_IDX] = IOTC_OK;

    ret = SleLinkLayerSendPkg(connId, sendBuff, sendBuffLen);
    IotcFree(sendBuff);
    return ret;
}