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
#ifndef COAP_NET_STACK_H
#define COAP_NET_STACK_H
#include <stdint.h>
#include "trans_socket_udp.h"
#include "coap_endpoint.h"
#include "coap_endpoint_event_source.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    UtilsBufferCtx *sendBuf;
    UtilsBufferCtx *recvBuf;
    TransLink *link;
    TransSess *sess;
    CoapEndpoint *endpoint;
    EventSource *source;
} CoapNetStack;

typedef struct {
    const char *name;
    TransSocket *socket;
    uint32_t sessMsgSize;
    void *sessUserData;
    CoapEndpointPacketEncode encoder;
    CoapEndpointPacketDecode decoder;
    void *endpointUserData;
} CoapNetStackParam;

int32_t CoapNetStackCreate(CoapNetStack *stack, const CoapNetStackParam *initParam);

int32_t CoapNetStackStart(CoapNetStack *stack);

void CoapNetStackStop(CoapNetStack *stack);

void CoapNetStackDestroy(CoapNetStack *stack);

#ifdef __cplusplus
}
#endif
#endif /* COAP_NET_STACK_H */