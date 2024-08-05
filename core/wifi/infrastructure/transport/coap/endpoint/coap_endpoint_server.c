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
#include "coap_endpoint_priv.h"
#include "utils_assert.h"
#include "comm_def.h"
#include "securec.h"
#include "coap_codec_utils.h"
#include "iotc_errcode.h"

int32_t CoapEndpointServerInit(CoapEndpointServer *server)
{
    CHECK_RETURN(server != NULL, IOTC_ERR_PARAM_INVALID);
    LIST_INIT(&server->resList);
    return IOTC_OK;
}

void CoapEndpointServerDeinit(CoapEndpointServer *server)
{
    CHECK_V_RETURN(server != NULL);

    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &server->resList) {
        CoapResourceNode *node = CONTAINER_OF(item, CoapResourceNode, node);
        LIST_REMOVE(item);
        AdapterFree(node);
    }
}

int32_t CoapServerAddGlobalChecker(CoapEndpoint *endpoint, CoapServerReqChecker checker)
{
    CHECK_RETURN(endpoint != NULL && checker != NULL, IOTC_ERR_PARAM_INVALID);

    ENDPOINT_LOCK_RETURN(endpoint, IOTC_ERR_TIMEOUT);

    endpoint->server.globalChecker = checker;
    ENDPOINT_UNLOCK(endpoint);

    return IOTC_OK;
}

int32_t CoapServerAddDefaultReqHandler(CoapEndpoint *endpoint, CoapServerReqHandler handler)
{
    CHECK_RETURN(endpoint != NULL && handler != NULL, IOTC_ERR_PARAM_INVALID);

    ENDPOINT_LOCK_RETURN(endpoint, IOTC_ERR_TIMEOUT);

    endpoint->server.defaultHandler = handler;
    ENDPOINT_UNLOCK(endpoint);

    return IOTC_OK;
}

int32_t CoapServerAddResource(CoapEndpoint *endpoint, const CoapResource *res, uint32_t num)
{
    CHECK_RETURN(endpoint != NULL && res != NULL && num != 0, IOTC_ERR_PARAM_INVALID);

    CoapResourceNode *newNode = (CoapResourceNode *)AdapterMalloc(sizeof(CoapResourceNode));
    if (newNode == NULL) {
        IOTC_LOGW("malloc error");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(newNode, sizeof(CoapResourceNode), 0, sizeof(CoapResourceNode));

    if (!UtilsExMutexLock(endpoint->mutex)) {
        AdapterFree(newNode);
        return IOTC_ERR_TIMEOUT;
    }

    newNode->resource = res;
    newNode->num = num;

    LIST_INSERT_BEFORE(&newNode->node, &endpoint->server.resList);

    ENDPOINT_UNLOCK(endpoint);
    return IOTC_OK;
}

void CoapServerRemoveResource(CoapEndpoint *endpoint, const CoapResource *res)
{
    CHECK_V_RETURN(endpoint != NULL && res != NULL);

    ENDPOINT_LOCK_V_RETURN(endpoint);

    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &endpoint->server.resList) {
        CoapResourceNode *node = CONTAINER_OF(item, CoapResourceNode, node);
        if (node->resource != res) {
            continue;
        }
        LIST_REMOVE(item);
        AdapterFree(node);
    }

    ENDPOINT_UNLOCK(endpoint);
    return;
}

int32_t CoapServerSendResp(CoapEndpoint *endpoint, const CoapServerRespParam *param, const SocketAddr *addr,
    CoapPacket *packetBuf)
{
    CHECK_RETURN(endpoint != NULL && param != NULL && param->req != NULL && packetBuf != NULL, IOTC_ERR_PARAM_INVALID);

    CoapBuildPacket buildPkt;
    (void)memset_s(&buildPkt, sizeof(buildPkt), 0, sizeof(buildPkt));
    (void)memset_s(packetBuf, sizeof(CoapPacket), 0, sizeof(CoapPacket));

    /* 使用请求报文中的msgid与token */
    buildPkt.header.msgId = param->req->header.msgId;
    buildPkt.header.type = (param->req->header.type == COAP_MSG_TYPE_CON) ? COAP_MSG_TYPE_ACK : param->type;
    buildPkt.header.code = param->code;
    buildPkt.header.tkl = param->req->header.tkl;
    buildPkt.opNum =  param->opNum;
    buildPkt.options =  param->options;
    buildPkt.payload = param->payload;
    buildPkt.buildFunc = param->payloadBuilder;
    buildPkt.userData = param->payloadUserData;
    int32_t ret = memcpy_s(buildPkt.token, sizeof(buildPkt.token), param->req->token, param->req->header.tkl);
    if (ret != EOK) {
        IOTC_LOGW("memcpy error %u", param->req->header.tkl);
        return IOTC_ERR_SECUREC_MEMCPY;
    }

    IOTC_LOGD("endpoint server send resp packet");
    ret = CoapEndpointSendPacket(endpoint, &buildPkt, packetBuf, param->preSize, addr);
    (void)memset_s(packetBuf, sizeof(CoapPacket), 0, sizeof(CoapPacket));
    if (ret != IOTC_OK) {
        IOTC_LOGW("send resp error %d", ret);
        return ret;
    }

    return IOTC_OK;
}

static bool CoapReqChecker(CoapEndpoint *endpoint, const CoapPacket *pkt, const SocketAddr *addr,
    CoapServerReqChecker globalChecker, CoapServerReqChecker sourceChecker)
{
    bool pass = true;
    /* 全局检查 */
    if (globalChecker != NULL) {
        pass = globalChecker(endpoint, pkt, addr, endpoint->userData);
    }
    /* resource检查 */
    if (pass && sourceChecker != NULL) {
        pass = sourceChecker(endpoint, pkt, addr, endpoint->userData);
    }
    return pass;
}

static void CoapReqHandler(CoapEndpoint *endpoint, const CoapPacket *pkt, const SocketAddr *addr,
    CoapServerReqHandler defaultHandler, CoapServerReqHandler sourceHandler)
{
    /* 优先分发至resource处理 */
    if (sourceHandler != NULL) {
        sourceHandler(endpoint, pkt, addr, endpoint->userData);
        return;
    }
    if (defaultHandler != NULL) {
        defaultHandler(endpoint, pkt, addr, endpoint->userData);
        return;
    }
    IOTC_LOGW("recv req no handler");
    return;
}

int32_t CoapEndpointServerRecvReqPacket(CoapEndpoint *endpoint, const CoapPacket *pkt, const SocketAddr *addr)
{
    CHECK_RETURN(endpoint != NULL && pkt != NULL, IOTC_ERR_PARAM_INVALID);

    char uri[COAP_URI_MAX_LEN + 1] = {0};
    int32_t ret = CoapUtilsGetUri(pkt, uri, COAP_URI_MAX_LEN);
    if (ret != IOTC_OK) {
        return ret;
    }

    CoapServerReqChecker sourceChecker = NULL;
    CoapServerReqHandler sourceHandler = NULL;

    ENDPOINT_LOCK_RETURN(endpoint, IOTC_ERR_TIMEOUT);
    CoapServerReqChecker globalChecker = endpoint->server.globalChecker;
    CoapServerReqHandler defaultHandler = endpoint->server.defaultHandler;
    bool find = false;
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &endpoint->server.resList) {
        CoapResourceNode *node = CONTAINER_OF(item, CoapResourceNode, node);
        if (node->num == 0 || node->resource == NULL) {
            continue;
        }
        for (uint32_t i = 0; i < node->num; ++i) {
            if (strcmp(node->resource[i].uri, uri) != 0) {
                continue;
            }
            find = true;
            sourceChecker = node->resource[i].checker;
            sourceHandler = node->resource[i].handler;
            break;
        }
        if (find) {
            break;
        }
    }
    ENDPOINT_UNLOCK(endpoint);
    if (CoapReqChecker(endpoint, pkt, addr, globalChecker, sourceChecker)) {
        CoapReqHandler(endpoint, pkt, addr, defaultHandler, sourceHandler);
    }
    return IOTC_OK;
}