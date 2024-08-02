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
#ifndef TRANS_LINK_H
#define TRANS_LINK_H

#include <stdint.h>
#include <stddef.h>
#include "comm_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TRANS_SOCKET_INVALID_FD (-1)
#define TRANS_SOCKET_NAME_MAX_STR_LEN 20

typedef struct TransSocket TransSocket;

typedef struct {
    uint16_t port;
    uint32_t addr;
} SocketAddr;

typedef struct {
    int32_t (*socketInit)(TransSocket *socket, void *userData);
    /* 返回值：>=0为套接字fd，其他为错误码 */
    int32_t (*socketConnect)(TransSocket *socket);
    int32_t (*socketSend)(TransSocket *socket, uint32_t tmo, const CommData *data, const SocketAddr *addr);
    /* 返回值：0为成功，小于0表示失败，大于0表示buffer不足，接收完整报文还需要的字节数 */
    int32_t (*socketRecv)(TransSocket *socket, uint32_t tmo, CommBuffer *buf, SocketAddr *addr);
    void (*socketClose)(TransSocket *socket);
    void (*socketFree)(TransSocket *socket);
} SocketIf;

struct TransSocket {
    int32_t fd;
    const SocketIf *socketIf;
    const char *name;
};

/**
 * 入参socketIf以及name 应为常量指针
 * 入参userData用于socket初始化时调用的socketIf->socketInit函数
 */
TransSocket *TransSocketNew(const SocketIf *socketIf, uint32_t size, const char *name, void *userData);

int32_t TransSocketGetFd(TransSocket *socket);

int32_t TransSocketConnect(TransSocket *socket);

int32_t TransSocketSend(TransSocket *socket, uint32_t tmo, const CommData *data, const SocketAddr *addr);

int32_t TransSocketRecv(TransSocket *socket, uint32_t tmo, CommBuffer *buf, SocketAddr *addr);

/* close后依然可以通过connect重新建立连接 */
void TransSocketClose(TransSocket *socket);

void TransSocketFree(TransSocket *socket);

#ifdef __cplusplus
}
#endif

#endif /* TRANS_LINK_H */
