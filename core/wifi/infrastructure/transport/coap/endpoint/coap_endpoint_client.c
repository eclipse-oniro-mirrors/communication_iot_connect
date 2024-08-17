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
#include <string.h>
#include "coap_endpoint_priv.h"
#include "comm_def.h"
#include "utils_assert.h"
#include "adapter_os.h"
#include "security_random.h"
#include "securec.h"
#include "iotc_errcode.h"

static const uint8_t COAP_ENDPOINT_CLIENT_REQ_MAX_CNT = UINT8_MAX;

int32_t CoapEndpointClientInit(CoapEndpointClient *cli)
{
    if (cli == NULL) {
        return IOTC_ERR_PARAM_INVALID;
    }

    LIST_INIT(&cli->reqList);
    cli->timeout = COAP_CLIENT_DEFAULT_RESP_TIMEOUT;
    cli->tkl = COAP_TOKEN_MAX_LEN;

    int32_t ret = SecurityRandom((uint8_t *)&cli->msgId, sizeof(cli->msgId));
    if (ret != IOTC_OK) {
        CoapEndpointClientDeinit(cli);
        IOTC_LOGW("msgid init error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

void CoapEndpointClientDeinit(CoapEndpointClient *cli)
{
    CHECK_V_RETURN(cli);
    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &cli->reqList) {
        CoapReqNode *node = CONTAINER_OF(item, CoapReqNode, node);
        LIST_REMOVE(item);
        IotcFree(node);
    }
    return;
}

int32_t CoapClientSetTokenLen(CoapEndpoint *endpoint, uint8_t tkl)
{
    CHECK_RETURN(endpoint != NULL && tkl <= COAP_TOKEN_MAX_LEN, IOTC_ERR_PARAM_INVALID);

    ENDPOINT_LOCK_RETURN(endpoint, IOTC_ERR_TIMEOUT);

    endpoint->client.tkl = tkl;
    ENDPOINT_UNLOCK(endpoint);

    return IOTC_OK;
}

static CoapClientRespHandler GetRespHandler(CoapEndpoint *endpoint, const CoapPacket *pkt)
{
    ENDPOINT_LOCK_RETURN(endpoint, NULL);

    CoapClientRespHandler ret = endpoint->client.defaultHandler;
    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &endpoint->client.reqList) {
        CoapReqNode *node = CONTAINER_OF(item, CoapReqNode, node);

        if (pkt->header.msgId == node->msgId && /* 匹配msgid */
            (endpoint->client.tkl == 0 || /* token长度不为0则匹配token */
            (endpoint->client.tkl == pkt->header.tkl && memcmp(pkt->token, node->token, pkt->header.tkl) == 0))) {
            /* 移除匹配到的节点 */
            ret = node->respHandler;
            LIST_REMOVE(item);
            IotcFree(node);
            endpoint->client.reqCnt = endpoint->client.reqCnt > 0 ? endpoint->client.reqCnt - 1 : 0;
            break;
        }
    }
    ENDPOINT_UNLOCK(endpoint);
    return ret;
}

static CoapClientRespHandler GetTimeoutRespHandler(CoapEndpoint *endpoint, uint32_t cur)
{
    ENDPOINT_LOCK_RETURN(endpoint, NULL);

    CoapClientRespHandler ret = NULL;
    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &endpoint->client.reqList) {
        CoapReqNode *node = CONTAINER_OF(item, CoapReqNode, node);
        if (UtilsDeltaTime(cur, node->time) > endpoint->client.timeout) {
            /* 移除超时节点 */
            ret = node->respHandler;
            LIST_REMOVE(item);
            IotcFree(node);
            endpoint->client.reqCnt = endpoint->client.reqCnt > 0 ? endpoint->client.reqCnt - 1 : 0;
        }
    }
    ENDPOINT_UNLOCK(endpoint);
    return ret;
}

int32_t CoapEndpointClientRecvRespPacket(CoapEndpoint *endpoint, const CoapPacket *pkt, const SocketAddr *addr)
{
    CHECK_RETURN(endpoint != NULL && pkt != NULL, IOTC_ERR_PARAM_INVALID);

    CoapClientRespHandler handler = GetRespHandler(endpoint, pkt);
    if (handler != NULL) {
        handler(pkt, addr, endpoint->userData, false);
    } else {
        IOTC_LOGD("resp packet no invalid handler");
    }
    CoapClientRespTimeoutCheck(endpoint);
    return IOTC_OK;
}

void CoapClientRespTimeoutCheck(CoapEndpoint *endpoint)
{
    CHECK_V_RETURN_LOGW(endpoint, "param invalid");
    uint32_t cur = IotcGetSysTimeMs();

    CoapClientRespHandler handler = NULL;
    do {
        handler = GetTimeoutRespHandler(endpoint, cur);
        if (handler != NULL) {
            handler(NULL, NULL, endpoint->userData, true);
        }
    } while (handler != NULL);
    return;
}

int32_t CoapClientSetRespTimeout(CoapEndpoint *endpoint, uint32_t ms)
{
    CHECK_RETURN(endpoint != NULL, IOTC_ERR_PARAM_INVALID);

    ENDPOINT_LOCK_RETURN(endpoint, IOTC_ERR_TIMEOUT);

    endpoint->client.timeout = ms;
    ENDPOINT_UNLOCK(endpoint);

    return IOTC_OK;
}

static uint16_t GetMsgId(CoapEndpoint *endpoint)
{
    uint16_t msgId = 0;
    ENDPOINT_LOCK_RETURN(endpoint, msgId);
    msgId = endpoint->client.msgId++;
    ENDPOINT_UNLOCK(endpoint);
    return msgId;
}

static int32_t CoapClientInsertReq(CoapEndpoint *endpoint, uint16_t msgId, const uint8_t *token,
    uint8_t tkl, CoapClientRespHandler handler)
{
    /* remove timeout node first */
    CoapClientRespTimeoutCheck(endpoint);
    ENDPOINT_LOCK_RETURN(endpoint, IOTC_ERR_TIMEOUT);
    if (endpoint->client.reqCnt >= COAP_ENDPOINT_CLIENT_REQ_MAX_CNT) {
        IOTC_LOGW("record req too much %u/%u", endpoint->client.reqCnt, COAP_ENDPOINT_CLIENT_REQ_MAX_CNT);
        ENDPOINT_UNLOCK(endpoint);
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_ENDPOINT_REQ_LIST_FULL;
    }

    CoapReqNode *newNode = (CoapReqNode *)IotcMalloc(sizeof(CoapReqNode));
    if (newNode == NULL) {
        IOTC_LOGW("calloc error");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(newNode, sizeof(CoapReqNode), 0, sizeof(CoapReqNode));
    newNode->msgId = msgId;
    if (tkl != 0) {
        int32_t ret = memcpy_s(newNode->token, sizeof(newNode->token), token, tkl);
        if (ret != EOK) {
            IotcFree(newNode);
            return IOTC_ERR_SECUREC_MEMCPY;
        }
    }

    newNode->respHandler = handler;
    newNode->time = IotcGetSysTimeMs();

    LIST_INSERT_BEFORE(&newNode->node, &endpoint->client.reqList);
    endpoint->client.reqCnt++;
    ENDPOINT_UNLOCK(endpoint);
    return IOTC_OK;
}

int32_t CoapClientSendReq(CoapEndpoint *endpoint, const CoapClientReqParam *param,
    const SocketAddr *addr, CoapPacket *packetBuf)
{
    CHECK_RETURN(endpoint != NULL && param != NULL && packetBuf != NULL, IOTC_ERR_PARAM_INVALID);

    CoapBuildPacket buildPkt;
    (void)memset_s(&buildPkt, sizeof(buildPkt), 0, sizeof(buildPkt));
    (void)memset_s(packetBuf, sizeof(CoapPacket), 0, sizeof(CoapPacket));

    buildPkt.header.msgId = GetMsgId(endpoint);
    buildPkt.header.type = param->type;
    buildPkt.header.code = param->code;
    buildPkt.header.tkl = endpoint->client.tkl;
    if (buildPkt.header.tkl != 0) {
        (void)SecurityRandom(buildPkt.token, buildPkt.header.tkl);
    }
    buildPkt.opNum =  param->opNum;
    buildPkt.options =  param->options;
    buildPkt.payload = param->payload;
    buildPkt.buildFunc = param->payloadBuilder;
    buildPkt.userData = param->payloadUserData;

    IOTC_LOGI("endpoint client send req packet");
    int32_t ret = CoapEndpointSendPacket(endpoint, &buildPkt, packetBuf, param->preSize, addr);
    if (ret != IOTC_OK) {
        (void)memset_s(packetBuf, sizeof(CoapPacket), 0, sizeof(CoapPacket));
        IOTC_LOGW("send req error %d", ret);
        return ret;
    }

    if (param->respHandler != NULL) {
        ret = CoapClientInsertReq(endpoint, packetBuf->header.msgId, packetBuf->token,
            packetBuf->header.tkl, param->respHandler);
        if (ret != IOTC_OK) {
            IOTC_LOGW("insert req list error %d/%u", ret, buildPkt.header.msgId);
        }
    }
    (void)memset_s(packetBuf, sizeof(CoapPacket), 0, sizeof(CoapPacket));
    return IOTC_OK;
}

int32_t CoapClientAddDefaultRespHandler(CoapEndpoint *endpoint, CoapClientRespHandler handler)
{
    CHECK_RETURN(endpoint != NULL && handler != NULL, IOTC_ERR_PARAM_INVALID);

    ENDPOINT_LOCK_RETURN(endpoint, IOTC_ERR_TIMEOUT);

    endpoint->client.defaultHandler = handler;
    ENDPOINT_UNLOCK(endpoint);

    return IOTC_OK;
}

void CoapClientReqPrepare(CoapEndpoint *endpoint, uint32_t *timeout)
{
    CHECK_V_RETURN(endpoint != NULL && timeout != NULL);
    if (LIST_EMPTY(&endpoint->client.reqList)) {
        return;
    }

    uint32_t now = IotcGetSysTimeMs();
    ENDPOINT_LOCK_V_RETURN(endpoint);
    ListEntry *item = LIST_HEAD(&endpoint->client.reqList);
    if (item != NULL) {
        CoapReqNode *firstNode = CONTAINER_OF(item, CoapReqNode, node);
        if (now > firstNode->time) {
            *timeout = 0;
        } else {
            *timeout = UtilsDeltaTime(now, firstNode->time);
        }
    }
    ENDPOINT_UNLOCK(endpoint);
    return;
}

bool CoapEndpointReqCheck(CoapEndpoint *endpoint, uint32_t cur)
{
    CHECK_RETURN(endpoint != NULL && cur != 0, IOTC_ERR_PARAM_INVALID);

    if (LIST_EMPTY(&endpoint->client.reqList)) {
        return false;
    }
    bool ready = false;
    ENDPOINT_LOCK_RETURN(endpoint, false);
    ListEntry *item = LIST_HEAD(&endpoint->client.reqList);
    if (item != NULL) {
        CoapReqNode *firstNode = CONTAINER_OF(item, CoapReqNode, node);
        if (cur > firstNode->time || UtilsDeltaTime(cur, firstNode->time) > endpoint->client.timeout) {
            ready = true;
        }
    }
    ENDPOINT_UNLOCK(endpoint);
    return ready;
}