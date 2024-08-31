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
#ifndef LOCAL_CONTROL_CLIENT_MANAGER_H
#define LOCAL_CONTROL_CLIENT_MANAGER_H
#include <stdint.h>
#include "local_ctl_ctx.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CLIENT_SESS_ID = 0,
    CLIENT_PUUID,
} ClientIdType;

typedef struct {
    const uint8_t *sn1;
    uint32_t sn1Len;
    const uint8_t *sn2;
    uint32_t sn2Len;
    uint32_t sendSeq;
    const char *puuid;
    uint32_t puuidLen;
    uint32_t addr;
} LocalClientBuildParam;

int32_t LocalCtlClientManagerInit(LocalControlContext *ctx);

void LocalCtlClientManagerDestroy(LocalControlContext *ctx);

void ClearAllLocalControlClient(LocalControlContext *ctx);

/* 根据puuid或sessid查询客户端，返回非0失败，client为空表示无对应客户端 */
int32_t GetLocalControlClient(LocalControlContext *ctx, ClientIdType type, const char *id, uint32_t len,
    LocalControlClient **client);

/* 创建客户端，客户端数量超限时会移除未协商完成的客户端或者创建时间较早的客户端 */
int32_t CreateLocalControlClient(LocalControlContext *ctx,
    const LocalClientBuildParam *param, LocalControlClient **client);

/* 更新指定sess的地址，并将相同地址的其他sess置为未激活状态 */
void ClientSessActiveUpdateAddr(LocalControlContext *ctx, const char *sessId, uint32_t sessIdLen, uint32_t addr);

#ifdef __cplusplus
}
#endif
#endif /* LOCAL_CONTROL_CLIENT_MANAGER_H */