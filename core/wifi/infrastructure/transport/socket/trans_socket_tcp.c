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
#include "trans_socket_tcp.h"
#include "comm_def.h"
#include "security_random.h"
#include "iotc_os.h"
#include "utils_common.h"
#include "utils_time.h"
#include "iotc_errcode.h"
#include "iotc_socket.h"
#include "utils_assert.h"
#include "iotc_mem.h"
#include "securec.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define TCP_RECV_TIMEOUT_MS 10

typedef struct {
    TransSocket base;
    SocketTcpInitParam initParam;
    int32_t fd;
} TcpSocket;

static int32_t Domain2Ip(const char *domain, uint32_t *netip);

static const char *TCP_NAME = "TCP";

static void TcpSocketFree(TransSocket *socket)
{
    CHECK_V_RETURN_LOGW(socket != NULL, "invalid param");
    TcpSocket *tcpSocket = (TcpSocket *)socket;

    UTILS_FREE_2_NULL(tcpSocket->initParam.host.hostname);
    if (tcpSocket->fd >= 0) {
        IotcClose(tcpSocket->fd);
        tcpSocket->fd = TRANS_SOCKET_INVALID_FD;
    }
}

static int32_t TcpSocketInit(TransSocket *socket, void *userData)
{
    CHECK_RETURN_LOGW(socket != NULL && userData != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");
    const SocketTcpInitParam *initParam = (const SocketTcpInitParam *)userData;
    TcpSocket *tcpSocket = (TcpSocket *)socket;

    tcpSocket->fd = TRANS_SOCKET_INVALID_FD;
    /* 拷贝初始化参数，用于socket生命周期内连接/重连 */
    SocketTcpInitParam *dst = &tcpSocket->initParam;

    dst->name = initParam->name;
    dst->host.port = initParam->host.port;
    dst->onUpdateRemainLen = initParam->onUpdateRemainLen;
    dst->host.hostname = UtilsStrDup(initParam->host.hostname);

    if (initParam->host.hostname == NULL) {
        IOTC_LOGW("copy hostname error");
        return IOTC_CORE_COMM_UTILS_ERR_STR_DUP;
    }
    
    do {
        if (initParam->host.hostname != NULL) {
            dst->host.hostname = UtilsStrDup(initParam->host.hostname);
            if (dst->host.hostname == NULL) {
                IOTC_LOGW("copy host addr error");
                break;
            }
        }
        return IOTC_OK;
    } while (0);
    TcpSocketFree(socket);
    return IOTC_CORE_WIFI_TRANS_ERR_SOCKET_UDP_INIT;
}


static int32_t TcpSocketConnect(TransSocket *socket)
{
    CHECK_RETURN_LOGW(socket != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");
    TcpSocket *tcpSocket = (TcpSocket *)socket;
    
    if (tcpSocket->fd >= 0) {
        return tcpSocket->fd;
    }
    int32_t ret = 0;
    IotcSockaddrIn saddr;
    (void)memset_s(&saddr, sizeof(IotcSockaddrIn), 0, sizeof(IotcSockaddrIn));
    saddr.sinFamily = IOTC_SOCKET_DOMAIN_AF_INET;
    saddr.sinPort = IotcHtons(tcpSocket->initParam.host.port);

    if (tcpSocket->initParam.host.hostname != NULL) {
        Domain2Ip(tcpSocket->initParam.host.hostname, (uint32_t*)&(saddr.sinAddr));
    }

    tcpSocket->fd = IotcSocket(IOTC_SOCKET_DOMAIN_AF_INET, IOTC_SOCKET_TYPE_STREAM,
        IOTC_SOCKET_PROTO_TCP);
    if (tcpSocket->fd < 0) {
        return IOTC_ERROR;
    }
    ret =  IotcConnect(tcpSocket->fd, (const IotcSockaddr*)&saddr, sizeof(saddr));
    if (ret != IOTC_OK) {
        IotcClose(tcpSocket->fd);
        return ret;
    }
    return tcpSocket->fd;
}

static int32_t TcpSocketSend(TransSocket *socket, uint32_t tmo, const CommData *data, const SocketAddr *addr)
{
    CHECK_RETURN_LOGW(socket != NULL && data != NULL && data->len != 0 && data->data != NULL,
        IOTC_ERR_PARAM_INVALID, "param invalid");
    NOT_USED(addr);

    TcpSocket *tcpSocket = (TcpSocket *)socket;
    uint32_t sendLen = 0;
    do {
        int32_t ret = 0;
        
        ret = IotcSend(tcpSocket->fd, data->data, data->len);
        if (ret > 0) {
            sendLen += ret;
        } else if (ret < 0) {
            IOTC_LOGW("tcp send error %d/%d", ret, IotcGetErrno());
            return ret;
        }
    } while (sendLen < data->len);

    if (sendLen < data->len) {
        IOTC_LOGW("tcp send timeout %u/%u/%u", tmo, sendLen, data->len);
        return IOTC_ERR_TIMEOUT;
    }
    return IOTC_OK;
}

static int32_t TcpSocketRecv(TransSocket *socket, uint32_t tmo, CommBuffer *buf, SocketAddr *addr)
{
    CHECK_RETURN_LOGW(socket != NULL && buf != NULL && buf->buffer != NULL && buf->size != 0 && buf->size > buf->len,
        IOTC_ERR_PARAM_INVALID, "invalid param");
    NOT_USED(addr);
    TcpSocket *tcpSocket = (TcpSocket *)socket;
    if (tcpSocket->initParam.onUpdateRemainLen == NULL) {
        IOTC_LOGW("get remain size func null");
        return IOTC_ERR_CALLBACK_NULL;
    }
    uint32_t count = 0;
    uint32_t recSize = 0;
    buf->len = 0;
    do {
        int32_t ret = 0;
        ret = IotcRecv(tcpSocket->fd, &(buf->buffer[recSize]), (uint32_t)(buf->size - recSize - 1));
        if (ret > 0) {
            recSize += ret;
            continue;
        } else {
            if (recSize > 0) {
                buf->len = recSize;
                IOTC_LOGD("%s tcp IotcRecv: len%d size%d", __func__, buf->len, buf->size);
                return IOTC_OK;
            } else {
                IotcSleepMs(TCP_RECV_TIMEOUT_MS);
            }
        }
        count++;
        if (count >= tmo / TCP_RECV_TIMEOUT_MS) {
            IOTC_LOGW("%s tcp recv timeout", __func__);
            return IOTC_ERROR;
        }
    } while (recSize < buf->size - 1);
    /* unreachable */
    return IOTC_ERROR;
}

static void TcpSocketClose(TransSocket *socket)
{
    CHECK_V_RETURN_LOGW(socket != NULL, "invalid param");
    TcpSocket *tcpSocket = (TcpSocket *)socket;

    if (tcpSocket->fd >= 0) {
        IotcClose(tcpSocket->fd);
        tcpSocket->fd = TRANS_SOCKET_INVALID_FD;
    }
}

TransSocket *TransSocketTcpNew(const SocketTcpInitParam *init)
{
    CHECK_RETURN_LOGW(init != NULL, NULL, "param invalid");
    static const SocketIf TCP_IF = {
        .socketInit = TcpSocketInit,
        .socketConnect = TcpSocketConnect,
        .socketSend = TcpSocketSend,
        .socketRecv = TcpSocketRecv,
        .socketClose = TcpSocketClose,
        .socketFree = TcpSocketFree,
    };

    return TransSocketNew(&TCP_IF, sizeof(TcpSocket), TCP_NAME, (void *)init);
}

static int32_t Domain2Ip(const char *domain, uint32_t *netip)
{
    struct IotcAddrInfo hints;
    struct IotcAddrInfo *res;
    (void)memset_s(&hints, sizeof(IotcAddrInfo), 0, sizeof(IotcAddrInfo));
    hints.aiFamily = IOTC_SOCKET_DOMAIN_AF_INET; // 仅支持 IPv4
    hints.aiSocktype = IOTC_SOCKET_TYPE_STREAM;
    int32_t status = IotcGetAddrInfo(domain, NULL, &hints, &res);
    if (status != IOTC_OK) {
        IotcFreeAddrInfo(res);
        return status;
    }
    if (res->aiFamily == IOTC_SOCKET_DOMAIN_AF_INET) {
        IotcSockaddrIn *ipv4 = (IotcSockaddrIn *)res->aiAddr;
        *netip = ipv4->sinAddr;
    } else {
        IotcFreeAddrInfo(res);
        return IOTC_ERROR;
    }
    IotcFreeAddrInfo(res);
    return IOTC_OK;
}
