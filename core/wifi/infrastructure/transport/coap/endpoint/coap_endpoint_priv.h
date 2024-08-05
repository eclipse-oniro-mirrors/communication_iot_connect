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
#ifndef COAP_ENDPOINT_PRIV_H
#define COAP_ENDPOINT_PRIV_H
#include "coap_endpoint.h"
#include "coap_endpoint_server.h"
#include "coap_endpoint_client.h"
#include "coap_endpoint_event_source.h"
#include "coap_endpoint_retrans.h"
#include "utils_mutex_ex.h"
#include "utils_list.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    ListEntry node;
    uint32_t time;
    uint16_t msgId;
    uint8_t token[COAP_TOKEN_MAX_LEN];
    CoapClientRespHandler respHandler;
} CoapReqNode;

typedef struct {
    uint8_t tkl;
    uint16_t msgId;
    uint32_t timeout;
    uint32_t reqCnt;
    ListEntry reqList;
    CoapClientRespHandler defaultHandler;
} CoapEndpointClient;

typedef struct {
    ListEntry node;
    uint32_t num;
    const CoapResource *resource;
} CoapResourceNode;

typedef struct {
    CoapServerReqChecker globalChecker;
    CoapServerReqHandler defaultHandler;
    ListEntry resList;
} CoapEndpointServer;

typedef struct {
    ListEntry node;
    CoapRetransParam param;
    CoapData raw;
    uint32_t remainTime;
    uint32_t lastTime;
} CoapRetransNode;

typedef struct {
    CoapRetransCheckFunc retransCheck;
    uint32_t maxBufSize;
    uint32_t curBufSize;
    ListEntry retransList;
} CoapEndpointRetrans;

struct CoapEndpoint {
    UtilsExMutex *mutex;
    TransSess *sess;
    UtilsBufferCtx *sendBuf;
    CoapEndpointRetrans retrans;
    CoapEndpointClient client;
    CoapEndpointServer server;
    CoapEndpointPacketEncode encoder;
    void *userData;
};

int32_t CoapEndpointSendPacket(CoapEndpoint *endpoint, const CoapBuildPacket *buildPkt,
    CoapPacket *pkt, uint32_t preSize, const SocketAddr *addr);

int32_t CoapEndpointClientInit(CoapEndpointClient *cli);

void CoapEndpointClientDeinit(CoapEndpointClient *cli);

int32_t CoapEndpointClientRecvRespPacket(CoapEndpoint *endpoint, const CoapPacket *pkt, const SocketAddr *addr);

void CoapClientRespTimeoutCheck(CoapEndpoint *endpoint);

void CoapClientReqPrepare(CoapEndpoint *endpoint, uint32_t *timeout);

bool CoapEndpointReqCheck(CoapEndpoint *endpoint, uint32_t cur);

int32_t CoapEndpointServerInit(CoapEndpointServer *server);

void CoapEndpointServerDeinit(CoapEndpointServer *server);

int32_t CoapEndpointRetransInit(CoapEndpointRetrans *retrans);

void CoapEndpointRetransDeinit(CoapEndpointRetrans *retrans);

void CoapEndpointRetransPrepare(CoapEndpoint *endpoint, uint32_t *timeout);

bool CoapEndpointRetransCheck(CoapEndpoint *endpoint, uint32_t cur);

void CoapEndpointRetransDispatch(CoapEndpoint *endpoint);

int32_t CoapRetransAddPacket(CoapEndpoint *endpoint, const CoapPacket *pkt,
    const CoapBuffer *buf, const SocketAddr *addr);

bool CoapEndpointRecvPacketRetransProcess(CoapEndpoint *endpoint, const CoapPacket *pkt, const SocketAddr *addr);

int32_t CoapEndpointServerRecvReqPacket(CoapEndpoint *endpoint, const CoapPacket *pkt, const SocketAddr *addr);

#define ENDPOINT_LOCK_RETURN(endpoint, ret) \
    do { \
        if (!UtilsExMutexLock((endpoint)->mutex)) { \
            return (ret); \
        } \
    } while (0)

#define ENDPOINT_LOCK_V_RETURN(endpoint) \
    do { \
        if (!UtilsExMutexLock((endpoint)->mutex)) { \
            return; \
        } \
    } while (0)

#define ENDPOINT_UNLOCK(endpoint) UtilsExMutexUnlock((endpoint)->mutex)

#ifdef __cplusplus
}
#endif

#endif /* COAP_ENDPOINT_PRIV_H */