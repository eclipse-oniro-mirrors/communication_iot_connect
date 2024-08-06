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
#ifndef COAP_ENDPOINT_SERVER_H
#define COAP_ENDPOINT_SERVER_H
#include <stdbool.h>
#include "coap_endpoint.h"
#include "utils_bit_map.h"
#include "adapter_json.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*CoapServerReqHandler)(CoapEndpoint *endpoint, const CoapPacket *req,
    const SocketAddr *addr, void *userData);

typedef bool (*CoapServerReqChecker)(CoapEndpoint *endpoint, const CoapPacket *req,
    const SocketAddr *addr, void *userData);

typedef struct {
    uint8_t method;
    const char *uri;
    CoapServerReqChecker checker;
    CoapServerReqHandler handler;
} CoapResource;

typedef struct {
    const CoapPacket *req;
    uint8_t type;
    uint8_t code;
    uint32_t opNum;
    const CoapOption *options;
    const CoapData *payload;
    CoapPacketBuildPayloadFunc payloadBuilder;
    void *payloadUserData;
    uint32_t preSize;
} CoapServerRespParam;

int32_t CoapServerAddGlobalChecker(CoapEndpoint *endpoint, CoapServerReqChecker checker);

int32_t CoapServerAddDefaultReqHandler(CoapEndpoint *endpoint, CoapServerReqHandler handler);

int32_t CoapServerAddResource(CoapEndpoint *endpoint, const CoapResource *res, uint32_t num);

void CoapServerRemoveResource(CoapEndpoint *endpoint, const CoapResource *res);

/* packetBuf应为CoapPacket的派生类，会传递到sess层作为SessMsg */
int32_t CoapServerSendResp(CoapEndpoint *endpoint, const CoapServerRespParam *param, const SocketAddr *addr,
    CoapPacket *packetBuf);

int32_t CoapServerBuildDefaultRespParam(CoapServerRespParam *param, const CoapPacket *reqPkt, AdapterJson *respJson);

#ifdef __cplusplus
}
#endif

#endif /* COAP_ENDPOINT_SERVER_H */