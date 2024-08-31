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
#ifndef TRANS_SESS_H
#define TRANS_SESS_H
#include <stdint.h>
#include <stddef.h>
#include "trans_link.h"
#include "comm_def.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TransSess TransSess;

typedef void SessMsg;

#define TRANS_SESS_MSG_MAX_SIZE 1024

typedef enum {
    SESS_CODE_OK = 0,
    SESS_CODE_ERR,
    SESS_CODE_CONTINUE,
} SessCode;

typedef struct {
    const SocketAddr *addr;
    void *userData;
    void *nodeData;
} SessAddtlInfo;

typedef SessCode (*SessMsgProcess)(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info);

TransSess *TransSessNew(TransLink *link, uint32_t msgSize, const char *name, void *userData);

void TransSessFree(TransSess *sess);

/* comment为描述当前节点的常量字符串 */
void TransSessAddRecvTailHandler(TransSess *sess, SessMsgProcess next, const char *comment, void *nodeData);

void TransSessAddSendTailHandler(TransSess *sess, SessMsgProcess next, const char *comment, void *nodeData);

void TransSessAddRecvHeadHandler(TransSess *sess, SessMsgProcess before, const char *comment, void *nodeData);

void TransSessAddSendHeadHandler(TransSess *sess, SessMsgProcess before, const char *comment, void *nodeData);

void TransSessRemoveHandler(TransSess *sess, SessMsgProcess handler);

int32_t TransSessMsgSend(TransSess *sess, SessMsg *msg, UtilsBuffer *buf, const SocketAddr *addr);

int32_t TransSessSendRaw(TransSess *sess, const CommData *data, const SocketAddr *addr);

int32_t TransSessMsgRecv(TransSess *sess, UtilsBuffer *buf, const SocketAddr *addr);

#ifdef __cplusplus
}
#endif

#endif /* TRANS_SESS_H */