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
#ifndef M2M_CLOUD_CONTEXT_H
#define M2M_CLOUD_CONTEXT_H
#include <stdint.h>
#include "comm_def.h"
#include "utils_fsm.h"
#include "service_manager.h"
#include "coap_endpoint_event_source.h"
#include "iotc_svc_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M2M_CLOUD_URL_NUM 3

typedef enum {
    M2M_CLOUD_CTX_BIT_REGISTER = 0,
    M2M_CLOUD_CTX_BIT_REVOKE,
    M2M_CLOUD_CTX_BIT_ENABLE_BACKOFF,
    M2M_CLOUD_CTX_BIT_LOGIN_INFO_READY,
} M2mCloudContextBitMap;

typedef struct M2mCloudContext M2mCloudContext;

struct M2mCloudContext {
    uint32_t bitMap;
    struct {
        int32_t regTimer;
        int32_t fsmTimer;
        int32_t tokenTimer;
        UtilsFsm *fsmCtx;
    } stateManager;
    struct {
        int32_t instanceId;
        ServiceFinishCallback onFinish;
    } svcInfo;
    union {
        DevLoginInfo loginInfo;
        DevRegInfo regInfo;
    } authInfo;
    struct {
        uint16_t port;
        uint16_t urlIndex;
        const char *url[M2M_CLOUD_URL_NUM];
        void (*CloudLinkErrorCallback)(M2mCloudContext *ctx);
        TransSocket *socket;
        TransLink *link;
        TransSess *sess;
        void *sessData;
        UtilsBufferCtx *sendBuf;
        UtilsBufferCtx *recvBuf;
        CoapEndpoint *endpoint;
        EventSource *coapSource;
    } linkInfo;
    struct {
        char access[CLOUD_ACCESS_TOKEN_STR_LEN + 1];
        char refresh[CLOUD_REFRESH_TOKEN_STR_LEN + 1];
        uint32_t timeout;
        uint32_t updateTime;
        uint32_t cnt;
    } tokenInfo;
    struct {
        uint8_t cnt;
        uint32_t before;
        uint32_t interval;
    } backoffInfo;
};

M2mCloudContext *GetM2mCloudCtx(void);

int32_t M2mCloudCtxInit(void);

#ifdef __cplusplus
}
#endif

#endif /* M2M_CLOUD_CONTEXT_H */