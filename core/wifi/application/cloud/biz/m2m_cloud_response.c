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

typedef struct {
    const CoapOption *reqIdOpt;
    const CoapOption *devIdOpt;
    const CoapOption *userIdOpt;
    const CoapOption *seqIdOpt;
} ResponseOptions;

typedef struct {
    CoapEndpoint *endpoint;
    const CoapPacket *req;
    const SocketAddr *addr;
    const M2mCloudContext *ctx;
    IotcJson *respJson;
} ResponseContext;

static int32_t ExtractResponseOptions(const CoapPacket *req, ResponseOptions *opts)
{
    uint32_t seg = 0;
    opts->reqIdOpt = CoapUtilsFindOption(req, COAP_OPTION_TYPE_REQ_ID, &seg);
    if (opts->reqIdOpt == NULL || seg != 1 || opts->reqIdOpt->value.data == NULL || opts->reqIdOpt->value.len == 0) {
        return IOTC_CORE_WIFI_M2M_ERR_CLOUD_GET_OPT_REQ_ID;
    }
    opts->devIdOpt = CoapUtilsFindOption(req, COAP_OPTION_TYPE_DEV_ID, &seg);
    if (opts->devIdOpt == NULL || seg != 1 || opts->devIdOpt->value.data == NULL || opts->devIdOpt->value.len == 0) {
        return IOTC_CORE_WIFI_M2M_ERR_CLOUD_GET_OPT_DEV_ID;
    }
    opts->userIdOpt = CoapUtilsFindOption(req, COAP_OPTION_TYPE_USER_ID, &seg);
    if (opts->userIdOpt == NULL || seg != 1 || opts->userIdOpt->value.data == NULL || opts->userIdOpt->value.len == 0) {
        return IOTC_CORE_WIFI_M2M_ERR_CLOUD_GET_OPT_USER_ID;
    }
    opts->seqIdOpt = CoapUtilsFindOption(req, COAP_OPTION_TYPE_SEQ_NUM_ID, &seg);
    if (opts->seqIdOpt == NULL || seg != 1 || opts->seqIdOpt->value.data == NULL || opts->seqIdOpt->value.len == 0) {
        return IOTC_CORE_WIFI_M2M_ERR_CLOUD_GET_OPT_SEQ_NUM_ID;
    }
    return IOTC_OK;
}

static int32_t BuildAndSendResponse(const ResponseContext *ctx, const ResponseOptions *opts)
{
    const CoapOption options[] = {
        {COAP_OPTION_TYPE_ACCESS_TOKEN_ID, {(const uint8_t *)ctx->ctx->tokenInfo.access,
                                            strlen(ctx->ctx->tokenInfo.access)}},
        {COAP_OPTION_TYPE_REQ_ID, {(const uint8_t *)opts->reqIdOpt->value.data, opts->reqIdOpt->value.len}},
        {COAP_OPTION_TYPE_DEV_ID, {(const uint8_t *)opts->devIdOpt->value.data, opts->devIdOpt->value.len}},
        {COAP_OPTION_TYPE_USER_ID, {(const uint8_t *)opts->userIdOpt->value.data, opts->userIdOpt->value.len}},
        {COAP_OPTION_TYPE_SEQ_NUM_ID, {(const uint8_t *)opts->seqIdOpt->value.data, opts->seqIdOpt->value.len}},
    };
    CoapServerRespParam respParam = {
        .req = ctx->req,
        .type = COAP_MSG_TYPE_NCON,
        .code = COAP_RESPONSE_CODE_CONTENT,
        .opNum = ARRAY_SIZE(options),
        .options = options,
        .payload = NULL,
        .payloadBuilder = CoapUtilsBuildJsonPayloadFunc,
        .payloadUserData = ctx->respJson,
        .preSize = 0,
    };
    CoapPacket packet;
    int32_t ret = CoapServerSendResp(ctx->endpoint, &respParam, ctx->addr, &packet);
    if (ret != IOTC_OK) {
        IOTC_LOGW("send e2e ctrl resp msg error %d", ret);
    }
    return ret;
}

static int32_t M2mCloudCtrlResponseMsg(CoapEndpoint *endpoint, const CoapPacket *req, const SocketAddr *addr,
                                       const M2mCloudContext *ctx, IotcJson *respJson)
{
    ResponseOptions opts = {0};
    int32_t ret = ExtractResponseOptions(req, &opts);
    if (ret != IOTC_OK) {
        IotcJsonDelete(respJson);
        return ret;
    }

    ResponseContext respCtx = {
        .endpoint = endpoint,
        .req = req,
        .addr = addr,
        .ctx = ctx,
        .respJson = respJson,
    };
    ret = BuildAndSendResponse(&respCtx, &opts);
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

static void M2mCloudDestroyNode(CoapResponeNode *node)
{
    if (node == NULL) {
        return;
    }
    if (node->req != NULL) {
        IotcFree((void *)node->req);
    }
    if (node->addr != NULL) {
        IotcFree((void *)node->addr);
    }
    if (node->devId != NULL) {
        IotcFree((void *)node->devId);
    }
    if (node->msgId != NULL) {
        IotcFree((void *)node->msgId);
    }
    IotcFree(node);
}

static int32_t M2mCloudExtractMsgIdAndDevId(const CoapPacket *req_pkt, char **msgId, char **devId)
{
    uint32_t seg = 0;
    const CoapOption *reqIdOpt = CoapUtilsFindOption(req_pkt, COAP_OPTION_TYPE_REQ_ID, &seg);
    if (reqIdOpt == NULL || seg != 1 || reqIdOpt->value.data == NULL || reqIdOpt->value.len == 0) {
        return IOTC_ERR_INVALID_PARAM;
    }
    const CoapOption *devIdOpt = CoapUtilsFindOption(req_pkt, COAP_OPTION_TYPE_DEV_ID, &seg);
    if (devIdOpt == NULL || seg != 1 || devIdOpt->value.data == NULL || devIdOpt->value.len == 0) {
        return IOTC_ERR_INVALID_PARAM;
    }

    *devId = IotcMalloc(devIdOpt->value.len + 1);
    if (*devId == NULL) {
        IOTC_LOGE("%s: devId strdup failed", __func__);
        return IOTC_ERR_NO_MEMORY;
    }
    (void)memcpy_s(*devId, devIdOpt->value.len + 1, devIdOpt->value.data, devIdOpt->value.len);
    ((char *)*devId)[devIdOpt->value.len] = '\0';

    *msgId = IotcMalloc(reqIdOpt->value.len + 1);
    if (*msgId == NULL) {
        IOTC_LOGE("%s: msgId strdup failed", __func__);
        IotcFree(*devId);
        *devId = NULL;
        return IOTC_ERR_NO_MEMORY;
    }
    (void)memcpy_s(*msgId, reqIdOpt->value.len + 1, reqIdOpt->value.data, reqIdOpt->value.len);
    ((char *)*msgId)[reqIdOpt->value.len] = '\0';

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
        M2mCloudDestroyNode(node);
        return NULL;
    }
    (void)memcpy_s((void *)node->req, sizeof(CoapPacket), req_pkt, sizeof(CoapPacket));

    node->addr = IotcMalloc(sizeof(SocketAddr));
    if (node->addr == NULL) {
        IOTC_LOGE("%s:addr malloc failed", __func__);
        M2mCloudDestroyNode(node);
        return NULL;
    }
    (void)memcpy_s((void *)node->addr, sizeof(SocketAddr), sock_addr, sizeof(SocketAddr));

    node->endpoint = ep;
    node->ctx = cloud_ctx;

    if (M2mCloudExtractMsgIdAndDevId(req_pkt, (char **)&node->msgId, (char **)&node->devId) != IOTC_OK) {
        M2mCloudDestroyNode(node);
        return NULL;
    }

    LIST_INSERT_BEFORE(&node->list, &g_m2mCloudResponseList);
    return node;
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
