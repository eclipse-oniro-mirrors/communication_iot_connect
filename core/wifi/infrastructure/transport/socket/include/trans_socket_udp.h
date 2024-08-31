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
#ifndef TRANS_SOCKET_UDP_H
#define TRANS_SOCKET_UDP_H

#include "trans_socket.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t port;
    const char *localAddr;
    const char *multicastAddr;
    const char *broadcastAddr;
} SocketUdpInitParam;

TransSocket *TransSocketUdpNew(const SocketUdpInitParam *init);

int32_t TransSocketUdpLeaveMulticastGroup(TransSocket *socket);

#ifdef __cplusplus
}
#endif

#endif /* TRANS_SOCKET_UDP_H */