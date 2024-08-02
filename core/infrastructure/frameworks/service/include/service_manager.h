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
#ifndef SERVICE_MANAGER_H
#define SERVICE_MANAGER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ServiceExecutorCallback)(void *userData);

typedef int32_t (*ServiceAsyncExecutor)(ServiceExecutorCallback cb, void *userData);

typedef void (*ServiceFinishCallback)(int32_t instanceId);

typedef struct {
    int32_t (*onStart)(int32_t instanceId, ServiceFinishCallback onFinish, void *param);
    int32_t (*onStop)(int32_t instanceId, void *param);
    int32_t (*onReset)(int32_t instanceId, void *param);
} ServiceHandler;

typedef struct {
    int32_t serviceId;
    const char *name;
    const ServiceHandler *handler;
    uint32_t msgNum;
    const int32_t *msgIds;
    const void *apiHandler;
} ServiceInstance;

int32_t ServiceManagerInit(void);

void ServiceManagerDeinit(void);

void ServiceManagerRegisterExecutor(ServiceAsyncExecutor executor);

int32_t ServiceManagerRegisterInstance(const ServiceInstance *ins);

void ServiceManagerUnregisterInstance(int32_t serviceId);

#ifdef __cplusplus
}
#endif

#endif /* SERVICE_MANAGER_H */