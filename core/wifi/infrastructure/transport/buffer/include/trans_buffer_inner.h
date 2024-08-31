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
#ifndef TRANSPORT_BUFFER_INNER_H
#define TRANSPORT_BUFFER_INNER_H

#include "utils_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TRANS_BUFFER_SEND_BUFFER_RES_SIZE = 0,
    TRANS_BUFFER_SEND_BUFFER_MAX_SIZE,
    TRANS_BUFFER_RECV_BUFFER_RES_SIZE,
    TRANS_BUFFER_RECV_BUFFER_MAX_SIZE,
} TransBufferType;

UtilsBufferCtx *TransCreateSendBuffer(void);
UtilsBufferCtx *TransCreateRecvBuffer(void);
void TransReleaseBuffer(UtilsBufferCtx *ctx);
int32_t TransBufferInit(void);
void TransBufferDeinit(void);
uint32_t TransGetBufferSize(TransBufferType type);

#ifdef __cplusplus
}
#endif
#endif /* TRANSPORT_BUFFER_INNER_H */