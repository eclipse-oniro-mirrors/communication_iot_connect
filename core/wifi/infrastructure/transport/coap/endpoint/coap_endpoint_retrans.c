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
#include "utils_common.h"
#include "securec.h"
#include "iotc_errcode.h"

#define COAP_DEFAULT_COAP_RETRANS_CNT 5
#define COAP_DEFAULT_RETRANS_INTERVAL UTILS_SEC_TO_MS(1)

int32_t CoapEndpointRetransEnable(CoapEndpoint *endpoint, CoapRetransCheckFunc func, uint32_t maxBufSize)
{
    CHECK_RETURN_LOGW(endpoint != NULL && func != NULL && maxBufSize != 0, IOTC_ERR_PARAM_INVALID,
        "param invalid");

    ENDPOINT_LOCK_RETURN(endpoint, IOTC_ERR_TIMEOUT);

    endpoint->retrans.retransCheck = func;
    endpoint->retrans.maxBufSize = maxBufSize;
    ENDPOINT_UNLOCK(endpoint);

    return IOTC_OK;
}

int32_t CoapEndpointRetransInit(CoapEndpointRetrans *retrans)
{
    CHECK_RETURN_LOGW(retrans != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    LIST_INIT(&retrans->retransList);
    return IOTC_OK;
}

void CoapEndpointRetransDeinit(CoapEndpointRetrans *retrans)
{
    CHECK_V_RETURN(retrans != NULL);
    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &retrans->retransList) {
        CoapRetransNode *node = CONTAINER_OF(item, CoapRetransNode, node);
        UTILS_FREE_2_NULL(node->raw.data);
        LIST_REMOVE(item);
        IotcFree(node);
    }
}

static CoapRetransNode *RetransNodeNew(const CoapPacket *pkt, const CoapBuffer *buf, const SocketAddr *addr)
{
    CoapRetransNode *newNode = IotcMalloc(sizeof(CoapRetransNode));
    if (newNode == NULL) {
        IOTC_LOGW("malloc error");
        return NULL;
    }
    (void)memset_s(newNode, sizeof(CoapRetransNode), 0, sizeof(CoapRetransNode));

    do {
        newNode->param.msgId = pkt->header.msgId;
        newNode->param.tkl = pkt->header.tkl;
        int32_t ret = memcpy_s(&newNode->param.addr, sizeof(newNode->param.addr), addr, sizeof(SocketAddr));
        if (ret != EOK) {
            break;
        }

        if (pkt->header.tkl != 0) {
            ret = memcpy_s(&newNode->param.token, sizeof(newNode->param.token), pkt->token, pkt->header.tkl);
            if (ret != EOK) {
                break;
            }
        }

        newNode->raw.data = UtilsMallocCopy(buf->buffer, buf->len);
        if (newNode->raw.data == NULL)  {
            IOTC_LOGW("malloc copy error %u", buf->len);
            break;
        }
        newNode->raw.len = buf->len;
        return newNode;
    } while (0);

    UTILS_FREE_2_NULL(newNode->raw.data);
    IotcFree(newNode);
    return NULL;
}

int32_t CoapRetransAddPacket(CoapEndpoint *endpoint, const CoapPacket *pkt,
    const CoapBuffer *buf, const SocketAddr *addr)
{
    CHECK_RETURN_LOGW(endpoint != NULL && pkt != NULL && buf != NULL && buf->buffer != NULL && buf->len != 0,
        IOTC_ERR_PARAM_INVALID, "param invalid");
    if (endpoint->retrans.retransCheck == NULL) {
        /* retrans feature not enable */
        return IOTC_OK;
    }

    ENDPOINT_LOCK_RETURN(endpoint, IOTC_ERR_TIMEOUT);
    CoapEndpointRetrans *retrans = &endpoint->retrans;
    CoapRetransNode *newNode = NULL;
    int32_t ret;
    do {
        if (buf->len > retrans->maxBufSize - retrans->curBufSize) {
            IOTC_LOGW("buffer over size %u/%u/%u", buf->len, retrans->maxBufSize, retrans->curBufSize);
            ret = IOTC_CORE_WIFI_TRANS_ERR_COAP_RETRANS_BUFFER_OVER_SIZE;
            break;
        }

        newNode = RetransNodeNew(pkt, buf, addr);
        if (newNode == NULL) {
            IOTC_LOGW("retrans node new error");
            ret = IOTC_CORE_WIFI_TRANS_ERR_COAP_RETRANS_NODE_NEW;
            break;
        }

        newNode->lastTime = IotcGetSysTimeMs();
        if (!retrans->retransCheck(&newNode->param, &newNode->raw, endpoint->userData, &newNode->remainTime)) {
            IOTC_LOGW("retrans first check error");
            ret = IOTC_CORE_WIFI_TRANS_ERR_COAP_RETRANS_CHECK;
            break;
        }

        retrans->curBufSize += newNode->raw.len;
        LIST_INSERT_BEFORE(&newNode->node, &retrans->retransList);
        IOTC_LOGD("add retrans %u/%u/%u/%u", buf->len, retrans->curBufSize,
            retrans->maxBufSize, newNode->remainTime);
        ret = IOTC_OK;
    } while (0);

    ENDPOINT_UNLOCK(endpoint);
    CHECK_RETURN(ret != IOTC_OK, IOTC_OK);

    if (newNode != NULL) {
        if (newNode->raw.data != NULL) {
            IotcFree((void *)newNode->raw.data);
        }
        IotcFree(newNode);
    }
    return ret;
}

void CoapEndpointRetransPrepare(CoapEndpoint *endpoint, uint32_t *timeout)
{
    CHECK_V_RETURN_LOGW(endpoint != NULL && timeout != NULL, "param invalid");
    if (LIST_EMPTY(&endpoint->retrans.retransList)) {
        return;
    }
    ENDPOINT_LOCK_V_RETURN(endpoint);
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &endpoint->retrans.retransList) {
        CoapRetransNode *node = CONTAINER_OF(item, CoapRetransNode, node);
        *timeout = UTILS_MIN(*timeout, node->remainTime);
    }
    ENDPOINT_UNLOCK(endpoint);

    return;
}

bool CoapEndpointRetransCheck(CoapEndpoint *endpoint, uint32_t cur)
{
    CHECK_RETURN_LOGW(endpoint != NULL && cur != 0, IOTC_ERR_PARAM_INVALID, "param invalid");

    bool ready = false;
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &endpoint->retrans.retransList) {
        CoapRetransNode *node = CONTAINER_OF(item, CoapRetransNode, node);
        uint32_t delta = UtilsDeltaTime(cur, node->lastTime);
        if (delta > node->remainTime) {
            node->remainTime = 0;
            ready = true;
        } else {
            node->remainTime -= delta;
        }
        node->lastTime = cur;
    }
    return ready;
}

static inline void RetransNodeFreeBuffer(CoapEndpointRetrans *retrans, CoapRetransNode *node)
{
    retrans->curBufSize = retrans->curBufSize > node->raw.len ? retrans->curBufSize - node->raw.len : 0;
    if (node->raw.data != NULL) {
        IotcFree((void *)node->raw.data);
    }
    IotcFree(node);
}

void CoapEndpointRetransDispatch(CoapEndpoint *endpoint)
{
    CHECK_V_RETURN_LOGW(endpoint != NULL, "param invalid");
    if (LIST_EMPTY(&endpoint->retrans.retransList)) {
        return;
    }

    ENDPOINT_LOCK_V_RETURN(endpoint);
    int32_t ret;
    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &endpoint->retrans.retransList) {
        CoapRetransNode *node = CONTAINER_OF(item, CoapRetransNode, node);
        if (node->remainTime != 0) {
            continue;
        }

        ret = TransSessSendRaw(endpoint->sess, &node->raw, &node->param.addr);
        if (ret != IOTC_OK) {
            IOTC_LOGW("retrans send error %d/%u", ret, node->param.cnt);
        }

        ++node->param.cnt;
        if (endpoint->retrans.retransCheck != NULL &&
            endpoint->retrans.retransCheck(&node->param, &node->raw, endpoint->userData, &node->remainTime)) {
            IOTC_LOGD("coap retrans continue %u/%u/%u/%u",
                node->param.msgId, node->remainTime, node->param.cnt, node->raw.len);
            continue;
        }
        IOTC_LOGD("coap retrans finish %u/%u/%u", node->param.msgId, node->param.cnt, node->raw.len);

        LIST_REMOVE(item);
        RetransNodeFreeBuffer(&endpoint->retrans, node);
    }
    ENDPOINT_UNLOCK(endpoint);

    return;
}

bool CoapEndpointRecvPacketRetransProcess(CoapEndpoint *endpoint, const CoapPacket *pkt, const SocketAddr *addr)
{
    CHECK_RETURN_LOGW(endpoint != NULL && pkt != NULL && addr != NULL, false, "param invaild");

    if (LIST_EMPTY(&endpoint->retrans.retransList)) {
        return false;
    }

    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &endpoint->retrans.retransList) {
        CoapRetransNode *node = CONTAINER_OF(item, CoapRetransNode, node);
        if (node->param.msgId != pkt->header.msgId || node->param.addr.addr != addr->addr ||
            node->param.addr.port != addr->port || node->param.tkl != pkt->header.tkl ||
            memcmp(node->param.token, pkt->token, COAP_TOKEN_MAX_LEN) != 0) {
            continue;
        }

        int32_t ret = TransSessSendRaw(endpoint->sess, &node->raw, &node->param.addr);
        if (ret != IOTC_OK) {
            IOTC_LOGW("retrans resp send error %d", ret);
        } else {
            IOTC_LOGD("recv retrans packet");
        }
        return true;
    }
    return false;
}

bool CoapEndpointRetransDefaultCheckFunc(const CoapRetransParam *param, const CoapData *raw,
    void *userData, uint32_t *next)
{
    CHECK_RETURN_LOGW(param != NULL && next != NULL, false, "param invalid");
    NOT_USED(param);
    NOT_USED(raw);
    NOT_USED(userData);

    if (param->cnt >= COAP_DEFAULT_COAP_RETRANS_CNT) {
        return false;
    }
    *next = COAP_DEFAULT_RETRANS_INTERVAL;
    return true;
}