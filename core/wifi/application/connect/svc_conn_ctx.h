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
#ifndef SERVICE_CONNECT_CONTEXT_H
#define SERVICE_CONNECT_CONTEXT_H
#include <stdint.h>
#include "service_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CONN_SVC_BIT_MAP_WIFI_CONNECTED = 0,
} ConnSvcBitMap;

typedef struct {
    int32_t instanceId;
    uint32_t bitMap;
    int32_t verifyTimerFd;
    int32_t netCheckTimerFd;
    int32_t reconnectTimerFd;
    ServiceFinishCallback onFinish;
} ConnectServiceContext;

int32_t ConnSvcCtxInit(void);

void ConnSvcCtxDeinit(void);

ConnectServiceContext *GetConnSvcCtx(void);

#ifdef __cplusplus
}
#endif
#endif /* SERVICE_CONNECT_CONTEXT_H */