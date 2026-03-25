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
    const CoapOption *reqId;
    const CoapOption *devId;
    const CoapOption *userId;
    const CoapOption *seqId;
} CoapCtrlOptions;

static int32_t ExtractCtrlOptions(const CoapPacket *req, CoapCtrlOptions *opts)
{
    uint32_t seg = 0;
    opts->reqId = CoapUtilsFindOption(req, COAP_OPTION_TYPE_REQ_ID, &seg);
    if (opts->reqId == NULL || seg != 1 || opts->reqId->value.data == NULL || opts->reqId->value.len == 0) {
        return IOTC_CORE_WIFI_M2M_ERR_CLOUD_GET_OPT_REQ_ID;
    }
    opts->devId = CoapUtilsFindOption(req, COAP_OPTION_TYPE_DEV_ID, &seg);
    if (opts->devId == NULL || seg != 1 || opts->devId->value.data == NULL || opts->devId->value.len == 0) {
        return IOTC_CORE_WIFI_M2M_ERR_CLOUD_GET_OPT_DEV_ID;
    }
    opts->userId = CoapUtilsFindOption(req, COAP_OPTION_TYPE_USER_ID, &seg);
    if (opts->userId == NULL || seg != 1 || opts->userId->value.data == NULL || opts->userId->value.len == 0) {
        return IOTC_CORE_WIFI_M2M_ERR_CLOUD_GET_OPT_USER_ID;
    }
    opts->seqId = CoapUtilsFindOption(req, COAP_OPTION_TYPE_SEQ_NUM_ID, &seg);
    if (opts->seqId == NULL || seg != 1 || opts->seqId->value.data == NULL || opts->seqId->value.len == 0) {
        return IOTC_CORE_WIFI_M2M_ERR_CLOUD_GET_OPT_SEQ_NUM_ID;
    }
    return IOTC_OK;
}

static int32_t M2mCloudCtrlResponseMsg(CoapEndpoint *endpoint, const CoapPacket *req,
    const SocketAddr *addr, const M2mCloudContext *ctx, IotcJson *respJson)
{
    CoapCtrlOptions ctrlOpts = {0};
    int32_t ret = ExtractCtrlOptions(req, &ctrlOpts);
    if (ret != IOTC_OK) {
        IotcJsonDelete(respJson);
        return ret;
    }
    const CoapOption options[] = {
        {COAP_OPTION_TYPE_ACCESS_TOKEN_ID,
            {(const uint8_t *)ctx->tokenInfo.access, strlen(ctx->tokenInfo.access)}},
        {COAP_OPTION_TYPE_REQ_ID, {(const uint8_t *)ctrlOpts.reqId->value.data, ctrlOpts.reqId->value.len}},
        {COAP_OPTION_TYPE_DEV_ID, {(const uint8_t *)ctrlOpts.devId->value.data, ctrlOpts.devId->value.len}},
        {COAP_OPTION_TYPE_USER_ID, {(const uint8_t *)ctrlOpts.userId->value.data, ctrlOpts.userId->value.len}},
        {COAP_OPTION_TYPE_SEQ_NUM_ID, {(const uint8_t *)ctrlOpts.seqId->value.data, ctrlOpts.seqId->value.len}},
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
    IotcJsonDelete(respJson);
    return ret;
}

static int32_t M2mCloudJsonGetString(const IotcJson *json, const char *key, const char **outStr)
{
    CHECK_RETURN(json != NULL &&
        key != NULL &&
        outStr != NULL,
        IOTC_ERR_PARAM_INVALID);

    const char *src = IotcJsonGetStr(IotcJsonGetObj(json, key));
    if (src == NULL) {
        IOTC_LOGW("get json str error %s", key);
        return IOTC_ADAPTER_JSON_ERR_GET_STRING;
    }

    if (*outStr != NULL && strcmp(*outStr, src) == 0) {
        IOTC_LOGD("string content is the same, no need to reallocate");
        return IOTC_OK;
    }

    if (*outStr != NULL) {
        IotcFree((char *)*outStr);
        IOTC_LOGW("free old string %s", key);
    }

    char *dup = strdup(src);
    if (dup == NULL) {
        IOTC_LOGE("strdup(%s) failed", key);
        *outStr = NULL;
        return IOTC_ERROR;
    }

    *outStr = dup;
    return IOTC_OK;
}

static int32_t FillNodeIdsFromOptions(CoapResponeNode *node, const CoapPacket *req_pkt)
{
    uint32_t seg = 0;
    const CoapOption *reqIdOpt = CoapUtilsFindOption(req_pkt, COAP_OPTION_TYPE_REQ_ID, &seg);
    if (reqIdOpt == NULL || seg != 1 || reqIdOpt->value.data == NULL || reqIdOpt->value.len == 0) {
        return IOTC_ERROR;
    }
    const CoapOption *devIdOpt = CoapUtilsFindOption(req_pkt, COAP_OPTION_TYPE_DEV_ID, &seg);
    if (devIdOpt == NULL || seg != 1 || devIdOpt->value.data == NULL || devIdOpt->value.len == 0) {
        return IOTC_ERROR;
    }

    node->devId = IotcMalloc(devIdOpt->value.len + 1);
    if (node->devId == NULL) {
        IOTC_LOGE("%s: devId strdup failed", __func__);
        return IOTC_ERROR;
    }
    (void)memcpy_s(node->devId, devIdOpt->value.len + 1, devIdOpt->value.data, devIdOpt->value.len);
    ((char *)node->devId)[devIdOpt->value.len] = '\0';

    node->msgId = IotcMalloc(reqIdOpt->value.len + 1);
    if (node->msgId == NULL) {
        IOTC_LOGE("%s: msgId strdup failed", __func__);
        return IOTC_ERROR;
    }
    (void)memcpy_s(node->msgId, reqIdOpt->value.len + 1, reqIdOpt->value.data, reqIdOpt->value.len);
    ((char *)node->msgId)[reqIdOpt->value.len] = '\0';
    return IOTC_OK;
}

CoapResponeNode *M2mCloudCreateCoapNode(CoapEndpoint *ep, const CoapPacket *req_pkt,
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

    if (FillNodeIdsFromOptions(node, req_pkt) != IOTC_OK) {
        goto ERROR;
    }

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
    LIST_FOR_EACH_ITEM(item, &g_m2mCloudResponseList) {
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
    IotcJson *respJson = IotcJsonGetObj(dataArray, STR_JSON_VENDOR);

    const char *devId = "0";
    const char *msgId = "0";
    int32_t ret = M2mCloudJsonGetString(dataArray, STR_JSON_DEVID, &devId);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Failed to get device ID from JSON, error code: %d", ret);
        return ret;
    }
    ret = M2mCloudJsonGetString(dataArray, STR_JSON_MSG_ID, &msgId);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Failed to get message ID from JSON, error code: %d", ret);
        return ret;
    }
    CoapResponeNode *coapResponse = M2mCloudFindNodeByMsgId(msgId, devId);
    if (coapResponse == NULL) {
        IOTC_LOGE("Failed to find the response node by message ID: %s", msgId);
        return IOTC_ERROR;
    }

    // send response
    if (M2mCloudCtrlResponseMsg(coapResponse->endpoint, coapResponse->req,
        coapResponse->addr, coapResponse->ctx, respJson) != IOTC_OK) {
        IOTC_LOGE("Failed to send response message to the client");
    }
    // remove node
    M2mCloudRemoveNode(coapResponse);

    return IOTC_OK;
}
