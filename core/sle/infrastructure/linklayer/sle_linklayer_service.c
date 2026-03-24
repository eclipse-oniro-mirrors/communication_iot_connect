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
#include <stdbool.h>
#include "sle_linklayer_service.h"
#include "sle_linklayer_process.h"
#include "sle_linklayer_encrypt.h"
#include "utils_list.h"
#include "utils_assert.h"
#include "iotc_mem.h"
#include "securec.h"
#include "iotc_errcode.h"

#define SLE_SERVICE_NUM_LIMIT    64
#define BITS_PER_BYTE 8

typedef struct {
    SleSvcInfo svcInfo;
    ListEntry node;
} SleSvcInfoNode;

static ListEntry g_serviceList = LIST_DECLARE_INIT(&g_serviceList);
static int32_t g_serviceNum = 0;

static SleSvcInfo *GetSleSvcInfoBySvcIdx(uint8_t svcIdx)
{
    ListEntry *item;
    LIST_FOR_EACH_ITEM(item, &g_serviceList) {
        SleSvcInfoNode *svcInfoNode = CONTAINER_OF(item, SleSvcInfoNode, node);
        if (svcInfoNode->svcInfo.svcIdx == svcIdx) {
            return &(svcInfoNode->svcInfo);
        }
    }
    return NULL;
}

static SleSvcInfo *GetSleSvcInfoByService(const char *service)
{
    ListEntry *item;
    LIST_FOR_EACH_ITEM(item, &g_serviceList) {
        SleSvcInfoNode *svcInfoNode = CONTAINER_OF(item, SleSvcInfoNode, node);
        if (strcmp(svcInfoNode->svcInfo.service, service) == 0) {
            return &(svcInfoNode->svcInfo);
        }
    }
    return NULL;
}

void SleLinkLayerServiceRelease(void)
{
    ListEntry *item;
    ListEntry *next;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &g_serviceList) {
        SleSvcInfoNode *svcInfoNode = CONTAINER_OF(item, SleSvcInfoNode, node);
        LIST_REMOVE(&svcInfoNode->node);
        IotcFree(svcInfoNode);
        g_serviceNum--;
    }
}

int32_t SleLinkLayerServiceRegister(const SleSvcInfo *svcInfo, uint32_t svcNum)
{
    CHECK_RETURN_LOGE((svcInfo != NULL) && (svcNum > 0) && (svcNum <= SLE_SERVICE_NUM_LIMIT),
        IOTC_ERR_PARAM_INVALID,
        "register bt svc param err, svcNum:%u, limit:%d", svcNum, SLE_SERVICE_NUM_LIMIT);
    CHECK_RETURN_LOGE(svcNum + g_serviceNum <= SLE_SERVICE_NUM_LIMIT, IOTC_CORE_SLE_LL_ERR_SVC_NUM,
        "register bt svc:%u over limit:%d, cur:%u", svcNum, SLE_SERVICE_NUM_LIMIT, g_serviceNum);

    for (uint32_t i = 0; i < svcNum; i++) {
        if (GetSleSvcInfoByService(svcInfo[i].service) != NULL) {
            IOTC_LOGE("svc:%s exist", svcInfo[i].service);
            continue;
        }
        if (GetSleSvcInfoBySvcIdx(svcInfo[i].svcIdx) != NULL) {
            IOTC_LOGE("svc:%u exist", svcInfo[i].svcIdx);
            continue;
        }

        SleSvcInfoNode *svcInfoNode = (SleSvcInfoNode *)IotcCalloc(1, sizeof(SleSvcInfoNode));
        CHECK_RETURN_LOGE(svcInfoNode != NULL, IOTC_ADAPTER_MEM_ERR_CALLOC, "calloc btSvcInfoNode err");

        int32_t ret = memcpy_s(&svcInfoNode->svcInfo, sizeof(SleSvcInfo), &(svcInfo[i]), sizeof(SleSvcInfo));
        if (ret != EOK) {
            IotcFree(svcInfoNode);
            return IOTC_ERR_SECUREC_MEMCPY;
        }
        LIST_INSERT_BEFORE(&svcInfoNode->node, &g_serviceList);
        g_serviceNum++;
    }
    return IOTC_OK;
}

int32_t DecodeCmdSleData(const uint8_t *buff, uint32_t len, SleCmdParam *cmdParam)
{
    CHECK_RETURN((buff != NULL) && (cmdParam != NULL), IOTC_ERR_PARAM_INVALID);

    uint32_t pos = 0;
    CHECK_RETURN_LOGE(pos < len, IOTC_CORE_SLE_LL_ERR_CMD_DECODE, "decode len:%u err", len);
    /* 高4位为数据类型, 低4位为操作类型 */
    cmdParam->dataFormat = (buff[pos] & 0xF0) >> 4;
    cmdParam->opType = buff[pos] & 0x0F;
    pos++;

    /* 当前只支持json方式 */
    CHECK_RETURN_LOGE(cmdParam->dataFormat == SLE_CMD_DATA_MODE_JSON, IOTC_CORE_SLE_LL_ERR_DATAMODE,
        "dataFormat:%d invalid", cmdParam->dataFormat);
    CHECK_RETURN_LOGE(pos < len, IOTC_CORE_SLE_LL_ERR_CMD_DECODE, "decode len:%u err", len);
    uint8_t svcLen = buff[pos];
    pos++;
    CHECK_RETURN_LOGE((pos + svcLen <= len) && (svcLen < SLE_SVC_LEN), IOTC_CORE_SLE_LL_ERR_CMD_DECODE,
        "decode svcLen:%u, len:%u err", svcLen, len);
    int32_t ret = strncpy_s(cmdParam->service, SLE_SVC_LEN, (char *)(buff + pos), svcLen);
    CHECK_RETURN(ret == EOK, IOTC_ERR_SECUREC_STRCPY);
    pos += svcLen;

    CHECK_RETURN_LOGE(pos + sizeof(uint16_t) <= len, IOTC_CORE_SLE_LL_ERR_CMD_DECODE,
        "decode pos:%u, len:%u err", pos, len);
    /* 长度占2字节, 小端序 */
    cmdParam->requestLen = (buff[pos + 1] << BITS_PER_BYTE) | buff[pos];
    pos += sizeof(uint16_t);
    CHECK_RETURN_LOGE(pos + cmdParam->requestLen == len, IOTC_CORE_SLE_LL_ERR_CMD_DECODE,
        "decode requestLen:%u, len:%u err", cmdParam->requestLen, len);
    cmdParam->request = (cmdParam->requestLen == 0 ? NULL : buff + pos);

    return IOTC_OK;
}

static int32_t EncodeCmdSleData(const SleCmdParam *cmdParam, const uint8_t *payload, uint32_t payloadLen,
    uint8_t **outBuff, uint32_t *outLen)
{
    CHECK_RETURN_LOGE(payload != NULL && payloadLen > 0, IOTC_CORE_SLE_LL_ERR_BODY,
        "encode body err, len:%u", payloadLen);
    uint32_t len = SVC_TYPE_LEN + SVC_LEN_LEN + strlen(cmdParam->service) +
        SVC_PAYLOAD_LEN_LEN + payloadLen;
    uint8_t *out = (uint8_t *)IotcCalloc(len, sizeof(uint8_t));
    CHECK_RETURN_LOGE(out != NULL, IOTC_ADAPTER_MEM_ERR_CALLOC, "calloc cmd data rsp:%u err", len);

    uint32_t pos = 0;
    /* 高4位为数据类型, 低4位为操作类型 */
    out[pos++] = (((uint8_t)cmdParam->dataFormat & 0x0F) << 4) | ((uint8_t)cmdParam->opType & 0x0F);
    /* 服务长度 */
    out[pos++] = strlen(cmdParam->service);
    /* 服务 */
    int32_t ret = memcpy_s(out + pos, len - pos, cmdParam->service, strlen(cmdParam->service));
    if (ret != EOK) {
        IotcFree(out);
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    pos += strlen(cmdParam->service);
    /* payload长度, 2字节 */
    out[pos++] = payloadLen & 0xFF;
    out[pos++] = (payloadLen >> BITS_PER_BYTE) & 0xFF;
    /* payload */
    ret = memcpy_s(out + pos, len - pos, payload, payloadLen);
    if (ret != EOK) {
        IotcFree(out);
        return IOTC_ERR_SECUREC_MEMCPY;
    }

    IOTC_LOGI("send sle cmd optype:%u, svc:%s, len:%u", cmdParam->opType, cmdParam->service, payloadLen);
    *outBuff = out;
    *outLen = len;
    return IOTC_OK;
}

static int32_t SvcCheckEncType(const SleSvcInfo *svcInfo, SleLinkLayerEncryptType encryptType)
{
    IOTC_LOGI("svc:%s, enc type:%d", svcInfo->service, encryptType);
    CHECK_RETURN_LOGE(encryptType < sizeof(svcInfo->suppEncType) * BITS_PER_BYTE,
        IOTC_CORE_SLE_LL_ERR_ENCRYPT_TYPE, "enc type too large:%d", encryptType);

    CHECK_RETURN_LOGE((svcInfo->suppEncType & (1 << encryptType)) > 0,
        IOTC_CORE_SLE_LL_ERR_SVC_NOT_SUPP_ENCTYPE, "svc:%s not supp enc type:%d", svcInfo->service, encryptType);
    return IOTC_OK;
}

int32_t SleLinkLayerProcessRspData(uint32_t connId, const SleLinkLayerProcessRspParam *param)
{
    CHECK_RETURN_LOGE(param != NULL, IOTC_ERR_PARAM_INVALID, "ll process bt cmd null param");
    CHECK_RETURN_LOGE((param->buff != NULL) && (param->len > 0) &&
        (param->outBuff != NULL) && (param->outLen != NULL),
        IOTC_ERR_PARAM_INVALID, "ll process bt cmd invalid param, len:%u", param->len);

    SleCmdParam cmdParam = { 0 };
    cmdParam.connId = connId;
    int32_t ret = DecodeCmdSleData(param->buff, param->len, &cmdParam);
    CHECK_RETURN(ret == IOTC_OK, ret);

    SleSvcInfo *svcInfo = GetSleSvcInfoByService(cmdParam.service);
    CHECK_RETURN_LOGE(svcInfo != NULL, IOTC_CORE_SLE_LL_ERR_SVC_NOT_FOUND, "svc:%s not found", cmdParam.service);
    ret = SvcCheckEncType(svcInfo, param->encryptType);
    CHECK_RETURN(ret == IOTC_OK, ret);
    IOTC_LOGI("recv sle cmd optype:%u, svc:%s, len:%u", cmdParam.opType, cmdParam.service, cmdParam.requestLen);

    uint8_t *response = NULL;
    uint32_t responseLen = 0;
    if (cmdParam.opType == SLE_OPTYPE_PUT) {
        CHECK_RETURN_LOGE(svcInfo->putSleFunc != NULL, IOTC_ERR_CALLBACK_NULL,
            "svc:%s put cb NULL", cmdParam.service);
        ret = svcInfo->putSleFunc(&cmdParam, &response, &responseLen);
    } else if (cmdParam.opType == SLE_OPTYPE_GET) {
        CHECK_RETURN_LOGE(svcInfo->getSleFunc != NULL, IOTC_ERR_CALLBACK_NULL,
            "svc:%s get cb NULL", cmdParam.service);
        ret = svcInfo->getSleFunc(&cmdParam, &response, &responseLen);
    } else {
        IOTC_LOGE("sle svc opType err:%d", cmdParam.opType);
        ret = IOTC_CORE_SLE_LL_ERR_SVC_OPTYPE;
    }
    CHECK_RETURN(ret == IOTC_OK, ret);

    if(strcmp(cmdParam.service ,  "speke") != 0) //不是speke类型的查找服务从这里过的都属于响应给客户端
    {
        cmdParam.opType = SLE_OPTYPE_RPT;
    }

    ret = EncodeCmdSleData(&cmdParam, response, responseLen, outBuff, outLen);
    if (response != NULL) {
        IotcFree(response);
    }
    return ret;
}

typedef struct {
    int32_t connId;
    const char *service;
    const uint8_t *data;
    uint32_t len;
    bool encrypt;
    SleOpType opType;
} SleRptCmdParam;

static int32_t SleCreateAndSendRptCmdData(const SleRptCmdParam *rptParam)
{
    SleSvcInfo *svcInfo = GetSleSvcInfoByService(rptParam->service);
    CHECK_RETURN_LOGE(svcInfo != NULL, IOTC_CORE_SLE_LL_ERR_SVC_NOT_FOUND, "svc:%s not found", rptParam->service);
    SleLinkLayerEncryptType encType = rptParam->encrypt ? SleLinkLayerGetEncryptType() : SLE_ENC_TYPE_UNENCRYPTED;
    int32_t ret = SvcCheckEncType(svcInfo, encType);
    CHECK_RETURN(ret == IOTC_OK, ret);

    SleCmdParam cmdParam = { 0 };
    cmdParam.dataFormat = SLE_CMD_DATA_MODE_JSON;
    cmdParam.opType = rptParam->opType;
    ret = strcpy_s(cmdParam.service, SLE_SVC_LEN, rptParam->service);
    CHECK_RETURN_LOGE(ret == EOK, IOTC_ERR_SECUREC_STRCPY, "svc strcpy err:%d", ret);

    uint8_t *rptData = NULL;
    uint32_t rptDataLen = 0;
    ret = EncodeCmdSleData(&cmdParam, rptParam->data, rptParam->len, &rptData, &rptDataLen);
    CHECK_RETURN(ret == IOTC_OK, ret);
    if (rptParam->encrypt) {
        ret = SleLinkLayerReportEncryptCmdData(rptParam->connId, rptData, rptDataLen);
    } else {
        ret = SleLinkLayerReportCmdData(rptParam->connId, rptData, rptDataLen);
    }
    IotcFree(rptData);
    return ret;
}

int32_t SleLinkLayerReportSvcData(int32_t connId, const char *service, const uint8_t *data, uint32_t len,
    SleOpType opType)
{
    CHECK_RETURN_LOGE((service != NULL) && (data != NULL) && (len > 0),
        IOTC_ERR_PARAM_INVALID, "ll rpt cmd invalid param, len:%u", len);

    SleRptCmdParam rptParam = { connId, service, data, len, false, opType };
    return SleCreateAndSendRptCmdData(&rptParam);
}

int32_t SleLinkLayerReportSvcDataEnc(int32_t connId, const char *service, const uint8_t *data, uint32_t len,
    SleOpType opType)
{
    CHECK_RETURN_LOGE((service != NULL) && (data != NULL) && (len > 0),
        IOTC_ERR_PARAM_INVALID, "ll rpt cmd enc invalid param, len:%u", len);

    SleRptCmdParam rptParam = { connId, service, data, len, true, opType };
    return SleCreateAndSendRptCmdData(&rptParam);
}