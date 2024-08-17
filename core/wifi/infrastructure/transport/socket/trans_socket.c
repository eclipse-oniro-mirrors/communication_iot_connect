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
#include "trans_socket.h"
#include "adapter_mem.h"
#include "securec.h"
#include "comm_def.h"
#include "iotc_errcode.h"
#include "utils_assert.h"
#include "utils_common.h"

TransSocket *TransSocketNew(const SocketIf *socketIf, uint32_t size, const char *name, void *userData)
{
    CHECK_RETURN_LOGW(socketIf != NULL && size >= sizeof(TransSocket) && socketIf->socketInit != NULL,
        NULL, "param invalid");

    TransSocket *socket = (TransSocket *)IotcMalloc(size);
    if (socket == NULL) {
        IOTC_LOGW("malloc error %u", size);
        return NULL;
    }
    (void)memset_s(socket, size, 0, size);

    socket->fd = TRANS_SOCKET_INVALID_FD;
    socket->socketIf = socketIf;
    socket->name = name;
    int32_t ret = socketIf->socketInit(socket, userData);
    if (ret != IOTC_OK) {
        TransSocketFree(socket);
        return NULL;
    }

    return socket;
}

int32_t TransSocketGetFd(TransSocket *socket)
{
    CHECK_RETURN_LOGW(socket != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    return socket->fd;
}

int32_t TransSocketConnect(TransSocket *socket)
{
    CHECK_RETURN_LOGW(socket != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    CHECK_RETURN_LOGW(socket->socketIf != NULL && socket->socketIf->socketConnect != NULL,
        IOTC_ERR_CALLBACK_NULL, "callback null");
    if (socket->fd >= 0) {
        return IOTC_OK;
    }

    socket->fd = socket->socketIf->socketConnect(socket);
    if (socket->fd < 0) {
        IOTC_LOGW("socket[%s] connect error:%d", NON_NULL_STR(socket->name), socket->fd);
        return IOTC_CORE_WIFI_TRANS_ERR_SOCKET_CONNECT;
    }

    return IOTC_OK;
}

int32_t TransSocketSend(TransSocket *socket, uint32_t tmo, const CommData *data, const SocketAddr *addr)
{
    CHECK_RETURN_LOGW(socket != NULL && data != NULL && data->len != 0 && data->data != NULL,
        IOTC_ERR_PARAM_INVALID, "param invalid");
    CHECK_RETURN_LOGW(socket->socketIf != NULL && socket->socketIf->socketSend != NULL,
        IOTC_ERR_CALLBACK_NULL, "callback null");
    CHECK_RETURN_LOGW(socket->fd >= 0, IOTC_CORE_WIFI_TRANS_ERR_SOCKET_INVALID,
        "socket[%s] fd invalid:%d", NON_NULL_STR(socket->name), socket->fd);

    int32_t ret = socket->socketIf->socketSend(socket, tmo, data, addr);
    if (ret != IOTC_OK) {
        IOTC_LOGW("socket[%s] send error:%d/%u", NON_NULL_STR(socket->name), ret, data->len);
    }
    return ret;
}

int32_t TransSocketRecv(TransSocket *socket, uint32_t tmo, CommBuffer *buf, SocketAddr *addr)
{
    CHECK_RETURN_LOGW(socket != NULL && buf != NULL && buf->size != 0 && buf->buffer != NULL && buf->size >= buf->len,
        IOTC_ERR_PARAM_INVALID, "param invalid");
    CHECK_RETURN_LOGW(socket->socketIf != NULL && socket->socketIf->socketRecv != NULL,
        IOTC_ERR_CALLBACK_NULL, "callback null");
    CHECK_RETURN_LOGW(socket->fd >= 0, IOTC_CORE_WIFI_TRANS_ERR_SOCKET_INVALID,
        "socket[%s] fd invalid:%d", NON_NULL_STR(socket->name), socket->fd);

    int32_t ret = socket->socketIf->socketRecv(socket, tmo, buf, addr);
    if (ret < IOTC_OK) {
        IOTC_LOGW("socket[%s] recv error:%d/%u", NON_NULL_STR(socket->name), ret, buf->size);
    }
    return ret;
}

void TransSocketClose(TransSocket *socket)
{
    CHECK_V_RETURN_LOGW(socket != NULL && socket->socketIf != NULL && socket->socketIf->socketClose != NULL,
        "param invalid");
    if (socket->fd < 0) {
        return;
    }
    socket->socketIf->socketClose(socket);
    socket->fd = TRANS_SOCKET_INVALID_FD;
}

void TransSocketFree(TransSocket *socket)
{
    if (socket == NULL) {
        return;
    }
    if (socket->socketIf != NULL && socket->socketIf->socketFree != NULL) {
        socket->socketIf->socketFree(socket);
    }
    IotcFree(socket);
}