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
#include "linklayer_process.h"
#include "linklayer_encrypt.h"
#include "linklayer_recv.h"
#include "linklayer_send.h"
#include "linklayer_service.h"
#include "utils_assert.h"
#include "iotc_mem.h"
#include "securec.h"
#include "iotc_errcode.h"

#define BIT_OFFSET_FOUR     4
#define PKG_HEAD_VERSION    0
#define LL_RET_ERR          1

static uint8_t g_sendToken = 0;

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

static void ParsePkgHead(const uint8_t *buff, uint32_t len, PkgHead *pkgHead)
{
    /* 第0字节的高4位 */
    pkgHead->version = buff[PKG_HEAD_TYPE_IDX] >> BIT_OFFSET_FOUR;
    /* 第0字节的低4位 */
    pkgHead->cmdType = buff[PKG_HEAD_TYPE_IDX] & 0x0F;
    pkgHead->token = buff[PKG_HEAD_TOKEN_IDX];
    pkgHead->pkgNum = buff[PKG_HEAD_PKGNUM_IDX];
    pkgHead->pkgIdx = buff[PKG_HEAD_INDEX_IDX];
    pkgHead->reserved = buff[PKG_HEAD_RESERVED_IDX];
    pkgHead->encryptType = buff[PKG_HEAD_ENCRYPT_TYPE_IDX];
    pkgHead->ret = buff[PKG_HEAD_RET_IDX];

    if (pkgHead->ret != IOTC_OK) {
        IOTC_LOGW("ll parse pkg ret:%u", pkgHead->ret);
    }
}

static void SetPkgHeadVersion(uint8_t head[PKG_HEAD_LEN], uint8_t version)
{
    /* 报头第0字节的高4位表示版本信息 */
    uint8_t ver = version << 4;
    head[PKG_HEAD_TYPE_IDX] &= 0x0F;
    head[PKG_HEAD_TYPE_IDX] |= ver;
}

static void SetPkgHeadCmdType(uint8_t head[PKG_HEAD_LEN], CmdType type)
{
    switch (type) {
        case CMD_TYPE_REQUEST:
            head[PKG_HEAD_TYPE_IDX] &= 0xF0;
            break;
        case CMD_TYPE_RESPONSE:
            head[PKG_HEAD_TYPE_IDX] &= 0xF0;
            head[PKG_HEAD_TYPE_IDX] |= 0x01;
            break;
        case CMD_TYPE_REPORT:
            head[PKG_HEAD_TYPE_IDX] &= 0xF0;
            head[PKG_HEAD_TYPE_IDX] |= 0x02;
            break;
        default:
            return;
    }
}

static void LinkLayerRspExceptionData(const uint8_t *reqBuff, uint8_t retVal)
{
    uint8_t sendBuff[PKG_HEAD_LEN] = { 0 };
    int32_t ret = memcpy_s(sendBuff, PKG_HEAD_LEN, reqBuff, PKG_HEAD_LEN);
    CHECK_V_RETURN(ret == EOK);
    SetPkgHeadVersion(sendBuff, PKG_HEAD_VERSION);
    SetPkgHeadCmdType(sendBuff, CMD_TYPE_RESPONSE);
    sendBuff[PKG_HEAD_PKGNUM_IDX] = 1;
    sendBuff[PKG_HEAD_INDEX_IDX] = 0;
    sendBuff[PKG_HEAD_RET_IDX] = retVal;

    (void)LinkLayerBtDataSend(sendBuff, PKG_HEAD_LEN);
}

static int32_t GetCompleteData(const uint8_t *reqBuff, uint8_t token, uint8_t encryptType,
    uint8_t **outData, uint32_t *outDataLen)
{
    uint8_t *mergedBuff = NULL;
    uint32_t mergedBuffLen = 0;
    int32_t ret = LinkLayerRecvMergePkgs(token, &mergedBuff, &mergedBuffLen);
    CHECK_RETURN((ret == IOTC_OK) && (mergedBuff != NULL) && (mergedBuffLen > 0), ret);

    ret = LinkLayerDecryptData(mergedBuff, &mergedBuffLen, encryptType);
    if (ret != IOTC_OK) {
        LinkLayerRspExceptionData(reqBuff, LL_RET_ERR);
        IotcFree(mergedBuff);
        return ret;
    }

    *outData = mergedBuff;
    *outDataLen = mergedBuffLen;
    return IOTC_OK;
}

static int32_t SendRspData(const uint8_t *reqBuff, uint8_t encryptType,
    uint8_t *rspData, uint32_t rspDataLen)
{
    uint8_t *encBuff = NULL;
    uint32_t encBuffLen = 0;
    int32_t ret = LinkLayerEncryptData(rspData, rspDataLen, encryptType, &encBuff, &encBuffLen);
    if ((ret != IOTC_OK) || (encBuff == NULL) || (encBuffLen == 0)) {
        LinkLayerRspExceptionData(reqBuff, LL_RET_ERR);
        return ret;
    }

    uint32_t sendBuffLen = PKG_HEAD_LEN + encBuffLen;
    uint8_t *sendBuff = (uint8_t *)IotcCalloc(sendBuffLen, sizeof(uint8_t));
    if (sendBuff == NULL) {
        IOTC_LOGE("ll rsp data Calloc:%u err", sendBuffLen);
        IotcFree(encBuff);
        return IOTC_ADAPTER_MEM_ERR_CALLOC;
    }

    ret  = memcpy_s(sendBuff + PKG_HEAD_LEN, sendBuffLen - PKG_HEAD_LEN, encBuff, encBuffLen);
    if (ret != EOK) {
        IotcFree(encBuff);
        IotcFree(sendBuff);
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    IotcFree(encBuff);

    ret = memcpy_s(sendBuff, sendBuffLen, reqBuff, PKG_HEAD_LEN);
    if (ret != EOK) {
        IotcFree(sendBuff);
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    SetPkgHeadCmdType(sendBuff, CMD_TYPE_RESPONSE);
    sendBuff[PKG_HEAD_RET_IDX] = IOTC_OK;

    ret = LinkLayerSendBtPkg(sendBuff, sendBuffLen);
    IotcFree(sendBuff);
    return ret;
}

int32_t LinkLayerProcessBtData(const uint8_t *buff, uint32_t len)
{
    CHECK_RETURN_LOGE((buff != NULL) && (len > PKG_HEAD_LEN), IOTC_ERR_PARAM_INVALID,
        "ll process bt data param invalid, len:%u", len);

    PkgHead pkgHead = { 0 };
    ParsePkgHead(buff, len, &pkgHead);
    IOTC_LOGI("recv bt data pkg(%u/%u), encType:%u", pkgHead.pkgIdx, pkgHead.pkgNum, pkgHead.encryptType);

    int32_t ret = LinkLayerRecvPkgInsert(pkgHead.token, pkgHead.pkgNum, pkgHead.pkgIdx,
        buff + PKG_HEAD_LEN, len - PKG_HEAD_LEN);
    CHECK_RETURN(ret == IOTC_OK, ret);

    ret = LinkLayerSetEncryptType(pkgHead.encryptType);
    CHECK_RETURN(ret == IOTC_OK, ret);
    bool isComplete = false;
    ret = LinkLayerRecvCompleteCheck(pkgHead.token, &isComplete);
    CHECK_RETURN((ret == IOTC_OK) && isComplete, ret);

    uint8_t *completeBuff = NULL;
    uint32_t completeBuffLen = 0;
    ret = GetCompleteData(buff, pkgHead.token, pkgHead.encryptType, &completeBuff, &completeBuffLen);
    CHECK_RETURN((ret == IOTC_OK) && (completeBuff != NULL) && (completeBuffLen > 0), ret);

    uint8_t *rspBuff = NULL;
    uint32_t rspBuffLen = 0;
    ret = LinkLayerProcessData(completeBuff, completeBuffLen, pkgHead.encryptType, &rspBuff, &rspBuffLen);
    if ((ret != IOTC_OK) || (rspBuff == NULL) || (rspBuffLen == 0)) {
        LinkLayerRspExceptionData(buff, LL_RET_ERR);
        IotcFree(completeBuff);
        return ret;
    }
    IotcFree(completeBuff);

    ret = SendRspData(buff, pkgHead.encryptType, rspBuff, rspBuffLen);
    IotcFree(rspBuff);
    return ret;
}

int32_t LinkLayerReportEncryptCmdData(const uint8_t *buff, uint32_t len)
{
    CHECK_RETURN_LOGE((buff != NULL) && (len > 0), IOTC_ERR_PARAM_INVALID,
        "ll process bt data param invalid, len:%u", len);

    LinkLayerEncryptType encType = LinkLayerGetEncryptType();

    uint8_t *encBuff = NULL;
    uint32_t encBuffLen = 0;
    int32_t ret = LinkLayerEncryptData(buff, len, encType, &encBuff, &encBuffLen);
    CHECK_RETURN((ret == IOTC_OK) && (encBuff != NULL) && (encBuffLen > 0), ret);

    uint32_t sendBuffLen = PKG_HEAD_LEN + encBuffLen;
    uint8_t *sendBuff = (uint8_t *)IotcCalloc(sendBuffLen, sizeof(uint8_t));
    if (sendBuff == NULL) {
        IOTC_LOGE("ll rpt data calloc:%u err", sendBuffLen);
        IotcFree(encBuff);
        return IOTC_ADAPTER_MEM_ERR_CALLOC;
    }

    ret  = memcpy_s(sendBuff + PKG_HEAD_LEN, sendBuffLen - PKG_HEAD_LEN, encBuff, encBuffLen);
    if (ret != EOK) {
        IotcFree(encBuff);
        IotcFree(sendBuff);
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    IotcFree(encBuff);

    SetPkgHeadVersion(sendBuff, PKG_HEAD_VERSION);
    SetPkgHeadCmdType(sendBuff, CMD_TYPE_REPORT);
    sendBuff[PKG_HEAD_TOKEN_IDX] = g_sendToken++;
    sendBuff[PKG_HEAD_ENCRYPT_TYPE_IDX] = encType;
    sendBuff[PKG_HEAD_RESERVED_IDX] = 0;
    sendBuff[PKG_HEAD_RET_IDX] = IOTC_OK;

    ret = LinkLayerSendBtPkg(sendBuff, sendBuffLen);
    IotcFree(sendBuff);
    return ret;
}

int32_t LinkLayerReportCmdData(const uint8_t *buff, uint32_t len)
{
    CHECK_RETURN_LOGE((buff != NULL) && (len > 0), IOTC_ERR_PARAM_INVALID,
        "ll process bt data param invalid, len:%u", len);

    uint32_t sendBuffLen = PKG_HEAD_LEN + len;
    uint8_t *sendBuff = (uint8_t *)IotcCalloc(sendBuffLen, sizeof(uint8_t));
    if (sendBuff == NULL) {
        IOTC_LOGE("ll rpt data calloc:%u err", sendBuffLen);
        return IOTC_ADAPTER_MEM_ERR_CALLOC;
    }

    int32_t ret  = memcpy_s(sendBuff + PKG_HEAD_LEN, sendBuffLen - PKG_HEAD_LEN, buff, len);
    if (ret != EOK) {
        IotcFree(sendBuff);
        return IOTC_ERR_SECUREC_MEMCPY;
    }

    SetPkgHeadVersion(sendBuff, PKG_HEAD_VERSION);
    SetPkgHeadCmdType(sendBuff, CMD_TYPE_REPORT);
    sendBuff[PKG_HEAD_TOKEN_IDX] = g_sendToken++;
    sendBuff[PKG_HEAD_ENCRYPT_TYPE_IDX] = ENC_TYPE_UNENCRYPTED;
    sendBuff[PKG_HEAD_RESERVED_IDX] = 0;
    sendBuff[PKG_HEAD_RET_IDX] = IOTC_OK;

    ret = LinkLayerSendBtPkg(sendBuff, sendBuffLen);
    IotcFree(sendBuff);
    return ret;
}