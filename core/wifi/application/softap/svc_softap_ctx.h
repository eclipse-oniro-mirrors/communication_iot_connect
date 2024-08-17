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
#ifndef SERVICE_SOFTAP_CTX_H
#define SERVICE_SOFTAP_CTX_H

#include <stdint.h>
#include "wifi_svc_softap.h"
#include "trans_buffer.h"
#include "security_speke.h"
#include "iotc_conf.h"
#include "iotc_wifi.h"
#include "service_manager.h"
#include "iotc_svc_softap.h"
#include "coap_net_stack.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SOFTAP_CTX_BIT_MAP_SOFTAP_STARTED = 0,
    SOFTAP_CTX_BIT_MAP_SESS_CREATED,
    SOFTAP_CTX_BIT_MAP_NETCFG_RECVED,
} SoftapCtxBitMap;

typedef enum {
    SOFTAP_PEER_SESS_BIT_MAP_LINK_ACTIVE = 0,
    SOFTAP_PEER_SESS_BIT_MAP_SPEKE_SESS_CREATED,
    SOFTAP_PEER_SESS_BIT_MAP_E2E_SEQ_SET,
} SoftapPeerSessBitMap;

typedef enum {
    SOFTAP_PEER_MSG_BIT_MAP_PLAIN = 0,
    SOFTAP_PEER_MSG_BIT_MAP_NO_BASE64,
} SoftapPeerMsgBitMap;

typedef struct {
    SocketAddr addrInfo;
    uint8_t mac[IOTC_MAC_ADDRESS_LEN];
    uint8_t bitMap;
    uint8_t sendBitMap;
    uint8_t recvBitMap;
    int32_t timer;
    uint32_t recvSeq;
    uint32_t sendSeq;
    SpekeSession *speke;
    uint16_t lastMsgId;
} SoftapPeerSess;

typedef struct {
    uint8_t bitMap;
    CoapNetStack coapStack;
    SoftapPeerSess peerSess[IOTC_CONF_SOFTAP_MAX_PEER_SESS_NUM];
    const char **plainUri;
    uint32_t plainUriNum;
    const char **noBase64Uri;
    uint32_t noBase64Num;
    int32_t staTimer;
} SoftapSess;

typedef struct {
    uint32_t bitMap;
    int32_t instanceId;
    int32_t tmoTimerFd;
    int32_t startUpTimerFd;
    int32_t stopTimerFd;
    ServiceFinishCallback onFinish;
    uint32_t timeout;
    SoftapSess sess;
    SoftapSvcInitParam initParam;
} SoftapServiceContext;

SoftapServiceContext *GetSoftapServiceContext();

int32_t SoftapServiceContextInit(void);

void SoftapServiceContextDeinit(void);

#ifdef __cplusplus
}
#endif

#endif /* SERVICE_SOFTAP_CTX_H */