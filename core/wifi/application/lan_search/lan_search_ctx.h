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
#ifndef LAN_SEARCH_CONTEXT_H
#define LAN_SEARCH_CONTEXT_H
#include <stdint.h>
#include "comm_def.h"
#include "coap_net_stack.h"
#include "utils_hash_map.h"
#include "security_sess_key.h"
#include "service_manager.h"
#include "utils_common.h"
#include "security_speke.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LAN_SEARCH_SESS_MSG_BIT_PLAIN = 0,
} LanSearchSessMsgBitMap;

typedef enum {
    LAN_SEARCH_PEER_BIT_SPEKE_FINISHED = 0,
} LanSearchPeerBitMap;

typedef enum {
    LAN_SEARCH_CTX_COAP_SVR_CREATED = 0,
    LAN_SEARCH_CTX_COAP_SVR_STARTED,
} LanSearchCtxBitMap;

typedef struct {
    uint32_t bitMap;
    struct {
        uint32_t createTime;
        uint32_t expireTime;
    } timeInfo;
    struct {
        SpekeSession *speke;
    } sessInfo;
    struct {
        uint32_t addr;
    } peerInfo;
} LanSearchPeer;

typedef struct {
    CoapPacket packet;
    uint32_t bitMap;
    LanSearchPeer *peer;
} LanSearchSessMsg;

typedef struct {
    uint32_t bitMap;
    struct {
        int32_t instanceId;
        ServiceFinishCallback onFinish;
    } svcInfo;
    struct {
        uint32_t maxPeerNum;
        uint32_t peerExpireTime;
    } config;
    CoapNetStack coapStack;
    struct {
        uint32_t curPeerNum;
        int32_t expireCheckTimer;
        HashMap *peerMap;
    } peerManager;
} LanSearchContext;

LanSearchContext *GetLanSearchCtx(void);

int32_t LanSearchContextInit(void);

#ifdef __cplusplus
}
#endif
#endif /* LAN_SEARCH_CONTEXT_H */