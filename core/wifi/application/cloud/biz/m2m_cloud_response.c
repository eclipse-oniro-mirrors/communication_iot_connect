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

#include "m2m_cloud_response.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "coap_codec_utils.h"
#include "utils_json.h"
#include "coap_endpoint_server.h"
#include "utils_assert.h"
#include "securec.h"
#include <string.h>

ListEntry g_m2mCloudResponseList = LIST_DECLARE_INIT(&g_m2mCloudResponseList);

static int32_t M2mCloudCtrlResponseMsg(CoapEndpoint *endpoint, const CoapPacket *req, const SocketAddr *addr,
                                       const M2mCloudContext *ctx, IotcJson *respJson)
{
    int32_t ret = IOTC_OK;
    do {
        uint32_t seg = 0;
        const CoapOption *reqIdOpt = CoapUtilsFindOption(req, COAP_OPTION_TYPE_REQ_ID, &seg);
        if (reqIdOpt == NULL || seg != 1 || reqIdOpt->value.data == NULL || reqIdOpt->value.len == 0) {
            ret = IOTC_CORE_WIFI_M2M_ERR_CLOUD_GET_OPT_REQ_ID;
            break;
        }
        const CoapOption *devIdOpt = CoapUtilsFindOption(req, COAP_OPTION_TYPE_DEV_ID, &seg);
        if (devIdOpt == NULL || seg != 1 || devIdOpt->value.data == NULL || devIdOpt->value.len == 0) {
            ret = IOTC_CORE_WIFI_M2M_ERR_CLOUD_GET_OPT_DEV_ID;
            break;
        }
        const CoapOption *uerIdOpt = CoapUtilsFindOption(req, COAP_OPTION_TYPE_USER_ID, &seg);
        if (uerIdOpt == NULL || seg != 1 || uerIdOpt->value.data == NULL || uerIdOpt->value.len == 0) {
            ret = IOTC_CORE_WIFI_M2M_ERR_CLOUD_GET_OPT_USER_ID;
            break;
        }
        const CoapOption *seqIdOpt = CoapUtilsFindOption(req, COAP_OPTION_TYPE_SEQ_NUM_ID, &seg);
        if (uerIdOpt == NULL || seg != 1 || seqIdOpt->value.data == NULL || seqIdOpt->value.len == 0) {
            ret = IOTC_CORE_WIFI_M2M_ERR_CLOUD_GET_OPT_SEQ_NUM_ID;
            break;
        }
        const CoapOption options[] = {
            {COAP_OPTION_TYPE_ACCESS_TOKEN_ID, {(const uint8_t *)ctx->tokenInfo.access, strlen(ctx->tokenInfo.access)}},
            {COAP_OPTION_TYPE_REQ_ID, {(const uint8_t *)reqIdOpt->value.data, reqIdOpt->value.len}},
            {COAP_OPTION_TYPE_DEV_ID, {(const uint8_t *)devIdOpt->value.data, devIdOpt->value.len}},
            {COAP_OPTION_TYPE_USER_ID, {(const uint8_t *)uerIdOpt->value.data, uerIdOpt->value.len}},
            {COAP_OPTION_TYPE_SEQ_NUM_ID, {(const uint8_t *)seqIdOpt->value.data, seqIdOpt->value.len}},
        };
        CoapServerRespParam respParam = {
            .req = req,
            .type = COAP_MSG_TYPE_NCON,
            .code = COAP_RESPONSE_CODE_CONTENT,
            .opNum = ARRAY_SIZE(options),
            .options = options,
            .payload = NULL,
            .payloadBuilder = CoapUtilsBuildJsonPayloadFunc,
            .payloadUserData = respJson,
            .preSize = 0,
        };
        CoapPacket packet;
        ret = CoapServerSendResp(endpoint, &respParam, addr, &packet);
        if (ret != IOTC_OK) {
            IOTC_LOGW("send e2e ctrl resp msg error %d", ret);
        }
    } while (false);
    IotcJsonDelete(respJson);
    return ret;
}

static int32_t GetResponeDevMsgId(const IotcJson *msg, IotcJson **savedDevid, IotcJson **saveMsgId)
{
    if (msg == NULL || savedDevid == NULL || saveMsgId == NULL) {
        return IOTC_ERR_INVALID_PARAM;
    }

    uint32_t arraySize = 0;
    IotcJsonGetArraySize(msg, &arraySize);

    for (int32_t i = 0; i < arraySize; i++) {
        IotcJson *item = IotcJsonGetArrayItem(msg, i);
        if (item == NULL) {
            continue;
        }

        // 提取并保存 devid
        if (IotcJsonHasObj(item, STR_JSON_DEVID)) {
            if (*savedDevid == NULL && (strcmp(IotcJsonGetObj(item, STR_JSON_DEVID), "0") != 0)) {
                *savedDevid = IotcDuplicateJson(IotcJsonGetObj(item, STR_JSON_DEVID), true);
            }
            IotcJsonDeleteItem(item, STR_JSON_DEVID);
        }

        // 提取并保存 msg_id
        if (IotcJsonHasObj(item, STR_JSON_MSG_ID)) {
            if (*saveMsgId == NULL && (strcmp(IotcJsonGetObj(item, STR_JSON_MSG_ID), "0") != 0)) {
                *saveMsgId = IotcDuplicateJson(IotcJsonGetObj(item, STR_JSON_MSG_ID), true);
            }
            IotcJsonDeleteItem(item, STR_JSON_MSG_ID);
        }
    }

    return IOTC_OK;
}

CoapResponeNode* M2mCloudCreateCoapNode(CoapEndpoint *ep, const CoapPacket *req_pkt,
     const SocketAddr *sock_addr, const M2mCloudContext *cloud_ctx)
{
    CoapResponeNode *node = IotcMalloc(sizeof(CoapResponeNode));
    if (node == NULL) {
        IOTC_LOGE("%s: malloc failed", __func__);
        return NULL;
    }
    (void)memset_s(node, sizeof(CoapResponeNode), 0, sizeof(CoapResponeNode));
    LIST_INIT(&node->list);

    node->req = IotcMalloc(sizeof(CoapPacket));
    if (node->req == NULL) {
        IOTC_LOGE("%s:req malloc failed", __func__);
        goto ERROR;
    }
    (void)memcpy_s((void *)node->req, sizeof(CoapPacket), req_pkt, sizeof(CoapPacket));

    node->addr = IotcMalloc(sizeof(SocketAddr));
    if (node->addr == NULL) {
        IOTC_LOGE("%s:addr malloc failed", __func__);
        goto ERROR;
    }
    (void)memcpy_s((void *)node->addr, sizeof(SocketAddr), sock_addr, sizeof(SocketAddr));

    node->endpoint = ep;
    node->ctx = cloud_ctx;

    do { // get msgId and devId
        uint32_t seg = 0;
        const CoapOption *reqIdOpt = CoapUtilsFindOption(req_pkt, COAP_OPTION_TYPE_REQ_ID, &seg);
        if (reqIdOpt == NULL || seg != 1 || reqIdOpt->value.data == NULL || reqIdOpt->value.len == 0) {
            goto ERROR;
        }
        const CoapOption *devIdOpt = CoapUtilsFindOption(req_pkt, COAP_OPTION_TYPE_DEV_ID, &seg);
        if (devIdOpt == NULL || seg != 1 || devIdOpt->value.data == NULL || devIdOpt->value.len == 0) {
            goto ERROR;
        }

        node->devId = IotcMalloc(devIdOpt->value.len + 1);
        if (node->devId == NULL) {
            IOTC_LOGE("%s: devId strdup failed", __func__);
            goto ERROR;
        }
        (void)memcpy_s(node->devId, devIdOpt->value.len + 1, devIdOpt->value.data, devIdOpt->value.len);
        ((char *)node->devId)[devIdOpt->value.len] = '\0';

        node->msgId = IotcMalloc(reqIdOpt->value.len + 1);
        if (node->msgId == NULL) {
            IOTC_LOGE("%s: msgId strdup failed", __func__);
            goto ERROR;
        }
        (void)memcpy_s(node->msgId, reqIdOpt->value.len + 1, reqIdOpt->value.data, reqIdOpt->value.len);
        ((char *)node->msgId)[reqIdOpt->value.len] = '\0';
    } while (0);

    LIST_INSERT_BEFORE(&node->list, &g_m2mCloudResponseList);
    return node;

ERROR:
    if (node != NULL) {
        if (node->req != NULL) {
            IotcFree((void *)node->req);
            node->req = NULL;
        }
        if (node->addr != NULL) {
            IotcFree((void *)node->addr);
            node->addr = NULL;
        }
        if (node->devId != NULL) {
            IotcFree((void *)node->devId);
            node->devId = NULL;
        }
        if (node->msgId != NULL) {
            IotcFree((void *)node->msgId);
            node->msgId = NULL;
        }
        IotcFree(node);
    }
    return NULL;
}

CoapResponeNode *M2mCloudFindNodeByMsgId(const char *msgId, const char *devId)
{
    if (msgId == NULL) {
        return NULL;
    }

    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &g_m2mCloudResponseList)
    {
        CoapResponeNode *resp = CONTAINER_OF(item, CoapResponeNode, list);
        if ((strcmp(msgId, resp->msgId) == 0) && (strcmp(devId, resp->devId) == 0)) {
            return resp;
        }
    }
    return NULL;
}

void M2mCloudRemoveNode(CoapResponeNode *node)
{
    if (node == NULL) {
        return;
    }

    LIST_REMOVE(&node->list);

    IotcFree((void *)node->req);
    IotcFree((void *)node->addr);
    IotcFree((void *)node->devId);
    IotcFree((void *)node->msgId);

    // 最后释放 node 自身
    IotcFree(node);
}

int32_t M2mCloudResponseMessage(const IotcJson *dataArray)
{
    CHECK_RETURN_LOGW(dataArray != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    // 获取设备id 和msgId并且删除掉
    IotcJson *devId = NULL;
    IotcJson *msgId = NULL;
    GetResponeDevMsgId(dataArray, &devId, &msgId);
    if (devId == NULL || msgId == NULL) {
        IOTC_LOGE("devId or msgId is null");
        return IOTC_ERROR;
    }
    CoapResponeNode *coapResponse = M2mCloudFindNodeByMsgId(IotcJsonGetStr(msgId), IotcJsonGetStr(devId));
    if (coapResponse == NULL) {
        IOTC_LOGE("Failed to find the response node by message ID: %s", msgId);
        return IOTC_ERROR;
    }

    IotcJson *dataArrayRespone = IotcDuplicateJson(dataArray, true);
    // send response
    if (M2mCloudCtrlResponseMsg(coapResponse->endpoint, coapResponse->req, coapResponse->addr, coapResponse->ctx,
                                dataArrayRespone) != IOTC_OK) {
        IOTC_LOGE("Failed to send response message to the client");
    }
    //  remove node
    M2mCloudRemoveNode(coapResponse);
    IotcFree(dataArrayRespone);

    return IOTC_OK;
}
