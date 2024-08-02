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
#ifndef SERVICE_PROXY_H
#define SERVICE_PROXY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SERVICE_MESSAGE_TYPE_REQUEST = 0,
    SERVICE_MESSAGE_TYPE_RESPONSE,
} ServiceMessageType;

typedef struct {
    int32_t serviceId;
    int32_t msgId;
    ServiceMessageType type;
    const void *msg;
    void *ctx;
} ServiceMessage;

typedef struct {
    int32_t instanceId;
    int32_t msgId;
    void *req;
} ServiceRequestInfo;

typedef void (*ServiceMessageFreeHandler)(void *msg);

typedef struct {
    void *resp;
    ServiceMessageFreeHandler freeHandler;
} ServiceResponseInfo;

typedef int32_t (*ServiceMessageHandler)(const ServiceMessage *req, ServiceResponseInfo *resp);

typedef void (*ServiceResponseHandler)(int32_t msgId, const ServiceMessage *resp, uint32_t respNum);

int32_t ServiceProxyStartService(int32_t serviceId, void *param);

int32_t ServiceProxyStopService(int32_t serviceId, void *param);

int32_t ServiceProxyResetService(int32_t serviceId, void *param);

int32_t ServiceProxyGetApiHandler(int32_t serviceId, const void **apiHandler);

int32_t ServiceProxySubscribeMessage(int32_t instanceId, int32_t serviceId,
    int32_t msgId, ServiceMessageHandler handler);

int32_t ServiceProxyRemoveSubscribe(int32_t instanceId, int32_t serviceId, int32_t msgId);

int32_t ServiceProxySendMessage(const ServiceRequestInfo *req, ServiceMessage **resp, uint32_t *respNum);

int32_t ServiceProxySendAsyncMessage(const ServiceRequestInfo *req, ServiceMessageFreeHandler freeHandler,
    ServiceResponseHandler respHandler);

void ServiceProxyFreeResponseMessage(ServiceMessage *resp, uint32_t respNum);

#ifdef __cplusplus
}
#endif

#endif /* SERVICE_PROXY_H */