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
#include <stdbool.h>
#include "trans_socket.h"
#include "utils_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TransLink TransLink;

typedef void (*OnLinkError)(TransLink *link, void *userData);

typedef int32_t (*OnLinkRecvData)(TransLink *link, UtilsBuffer *data, void *userData, const SocketAddr *addr);

/* name应为常量，创建成功后socket资源由link管理，调用方不应再持有socket指针 */
TransLink *TransLinkNew(TransSocket *socket, UtilsBufferCtx *buffer, const char *name);

int32_t TransLinkConnect(TransLink *link);

int32_t TransLinkGetFd(TransLink *link);

void TransLinkClose(TransLink *link);

void TransLinkFree(TransLink *link);

void TransLinkConfTimeout(TransLink *link, uint32_t recvTmo, uint32_t sendTmo);

int32_t TransLinkRegErrorCallback(TransLink *link, OnLinkError cb, void *userData);

int32_t TransLinkRegDataCallback(TransLink *link, OnLinkRecvData cb, void *userData);

void TransLinkUnregErrorCallback(TransLink *link);

void TransLinkUnregDataCallback(TransLink *link);

void TransLinkRecvReadEvent(TransLink *link);

void TransLinkRecvErrorEvent(TransLink *link);

int32_t TransLinkSendData(TransLink *link, const uint8_t *data, uint32_t len, const SocketAddr *addr);

#ifdef __cplusplus
}
#endif

#endif /* TRANS_LINK_H */