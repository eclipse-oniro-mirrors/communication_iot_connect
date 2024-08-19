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
#ifndef LOCAL_CONTROL_CONTEXT_H
#define LOCAL_CONTROL_CONTEXT_H
#include <stdint.h>
#include "comm_def.h"
#include "utils_hash_map.h"
#include "security_sess_key.h"
#include "service_manager.h"
#include "coap_net_stack.h"
#include "utils_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LOCAL_CONTROL_SESS_ID_LEN 16
#define LOCAL_CONTROL_SESS_ID_STR_LEN HEXIFY_LEN(LOCAL_CONTROL_SESS_ID_LEN)

typedef enum {
    LOCAL_CTL_CTX_COAP_SVR_CREATED = 0,
    LOCAL_CTL_CTX_COAP_SVR_STARTED,
} LocalControlContextBitMap;

typedef enum {
    LOCAL_CTL_CLI_BIT_SESS_CREATED = 0,
    LOCAL_CTL_CLI_BIT_SESS_ACTIVE,
} LocalControlClientBitMap;

typedef enum {
    LOCAL_COAP_PLAIN = 0,
} LocalCoapBitMap;

typedef struct {
    uint32_t bitMap;
    struct {
        uint32_t createTime;
        uint32_t expireTime;
    } timeInfo;
    struct {
        char sessId[LOCAL_CONTROL_SESS_ID_STR_LEN + 1];
        uint8_t transKey[SESS_TRANS_KEY_LEN];
        uint8_t authKey[SESS_AUTH_KEY_LEN];
        uint32_t sendSeq;
        uint32_t recvSeq;
        uint32_t unreachCnt;
        /* seq窗口为 当前seq±30，需记录小于当前seq的报文是否处理过 */
        uint32_t seqMap;
    } sessInfo;
    struct {
        char *puuid;
        uint32_t puuidLen;
        uint32_t addr;
    } appInfo;
} LocalControlClient;

typedef struct {
    CoapPacket packet;
    uint32_t bitMap;
    LocalControlClient *client;
} LocalCoapSessMsg;

typedef struct {
    uint32_t bitMap;
    struct {
        int32_t instanceId;
        ServiceFinishCallback onFinish;
    } svcInfo;
    struct {
        uint32_t maxClientNum;
        uint32_t clientExpireTime;
    } config;
    CoapNetStack coapStack;
    struct {
        uint32_t curClientNum;
        HashMap *clientMap;
        int32_t expireCheckTimer;
    } clientManager;
} LocalControlContext;

LocalControlContext *GetLocalCtlCtx(void);

int32_t LocalCtlCtxInit(void);

#ifdef __cplusplus
}
#endif
#endif /* LOCAL_CONTROL_CONTEXT_H */