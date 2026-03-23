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
#include "trans_socket_udp.h"
#include "utils_assert.h"
#include "comm_def.h"
#include "securec.h"
#include "iotc_os.h"
#include "utils_common.h"
#include "iotc_socket.h"
#include "iotc_errcode.h"
#include "dfx_anonymize.h"

typedef struct {
    TransSocket base;
    SocketUdpInitParam initParam;
    int32_t fd;
} UdpSocket;

static void UdpSocketFree(TransSocket *socket)
{
    CHECK_V_RETURN_LOGW(socket != NULL, "param invalid");
    UdpSocket *udpSocket = (UdpSocket *)socket;

    UTILS_FREE_2_NULL(udpSocket->initParam.broadcastAddr);
    UTILS_FREE_2_NULL(udpSocket->initParam.localAddr);
    UTILS_FREE_2_NULL(udpSocket->initParam.multicastAddr);
    if (udpSocket->fd >= 0) {
        IotcClose(udpSocket->fd);
        udpSocket->fd = TRANS_SOCKET_INVALID_FD;
    }
}

static int32_t UdpSocketInit(TransSocket *socket, void *userData)
{
    CHECK_RETURN_LOGW(socket != NULL && userData != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    const SocketUdpInitParam *initParam = (const SocketUdpInitParam *)userData;
    UdpSocket *udpSocket = (UdpSocket *)socket;

    udpSocket->fd = TRANS_SOCKET_INVALID_FD;
    /* 拷贝初始化参数，用于socket生命周期内连接/重连 */
    SocketUdpInitParam *dst = &udpSocket->initParam;
    dst->port = initParam->port;
    do {
        if (initParam->multicastAddr != NULL) {
            dst->multicastAddr = UtilsStrDup(initParam->multicastAddr);
            if (dst->multicastAddr == NULL) {
                IOTC_LOGW("copy multicast addr error");
                break;
            }
        }
        if (initParam->localAddr != NULL) {
            dst->localAddr = UtilsStrDup(initParam->localAddr);
            if (dst->localAddr == NULL) {
                IOTC_LOGW("copy local addr error");
                break;
            }
        }
        if (initParam->broadcastAddr != NULL) {
            dst->broadcastAddr = UtilsStrDup(initParam->broadcastAddr);
            if (dst->broadcastAddr == NULL) {
                IOTC_LOGW("copy broadcast addr error");
                break;
            }
        }
        return IOTC_OK;
    } while (0);
    UdpSocketFree(socket);
    return IOTC_CORE_WIFI_TRANS_ERR_SOCKET_UDP_INIT;
}

static int32_t UdpSocketBindAddr(int32_t fd, const SocketUdpInitParam *param)
{
    int32_t ret;
    char anonyIpBuf[ANONYMIZE_IP_MIN_BUF_LEN + 1] = {0};
    IotcSockaddrIn addr;
    (void)memset_s(&addr, sizeof(IotcSockaddrIn), 0, sizeof(IotcSockaddrIn));
    addr.sinFamily = IOTC_SOCKET_DOMAIN_AF_INET;
    addr.sinPort = IotcHtons(param->port);
    /* 有广播地址则绑定广播地址，否则绑定本地ip */
    if (param->broadcastAddr != NULL) {
        addr.sinAddr = IotcInetAddr(param->broadcastAddr);
        ret = IotcSetSocketOpt(fd, IOTC_SOCKET_OPTION_ENABLE_BROADCAST, NULL, 0);
        if (ret != IOTC_OK) {
            IOTC_LOGW("enable broadcast error %d", ret);
            return ret;
        }
        (void)DfxAnonymizeStrWithBuffer(param->broadcastAddr, ANONYMIZE_IP, anonyIpBuf, ANONYMIZE_IP_MIN_BUF_LEN);
    } else if (param->localAddr != NULL) {
        addr.sinAddr = IotcInetAddr("0.0.0.0");
        (void)DfxAnonymizeStrWithBuffer(param->localAddr, ANONYMIZE_IP, anonyIpBuf, ANONYMIZE_IP_MIN_BUF_LEN);
    } else {
        IOTC_LOGW("no addr to bind");
        return IOTC_CORE_WIFI_TRANS_ERR_SOCKET_UDP_ADDR;
    }

    ret = IotcBind(fd, (IotcSockaddr *)&addr, sizeof(IotcSockaddrIn));
    if (ret != IOTC_OK) {
        IOTC_LOGW("udp socket[%d] bind to %s:%u error %d", fd, anonyIpBuf, param->port, ret);
    } else {
        IOTC_LOGD("udp socket[%d] bind to %s:%u", fd, anonyIpBuf, param->port);
    }
    return ret;
}

static int32_t JoinMulticastGroup(int32_t fd, const SocketUdpInitParam *param)
{
    if (param->multicastAddr == NULL) {
        return IOTC_OK;
    }

    int32_t ret = IotcSetSocketOpt(fd, IOTC_SOCKET_OPTION_DISABLE_MULTI_LOOP, NULL, 0);
    if (ret != IOTC_OK) {
        IOTC_LOGW("disable multi loop error %d", ret);
        return ret;
    }

    DFX_ANONYMIZE_IP_STR(anonyIp, param->multicastAddr);
    IotcSocketMultiAddr multi = {
        param->broadcastAddr != NULL ? param->broadcastAddr : param->localAddr, param->multicastAddr };
    ret = IotcSetSocketOpt(fd, IOTC_SOCKET_OPTION_ADD_MULTI_GROUP, &multi, sizeof(multi));
    if (ret != IOTC_OK) {
        IOTC_LOGW("add multi group %s error %d", anonyIp, ret);
        return ret;
    }

    IOTC_LOGD("udp socket[%d] join multicast group %s", fd, anonyIp);
    return IOTC_OK;
}

static int32_t SetUdpOpt(int32_t fd, const SocketUdpInitParam *param)
{
    int32_t ret = IotcSetSocketOpt(fd, IOTC_SOCKET_OPTION_SETFL_NONBLOCK, NULL, 0);
    if (ret != IOTC_OK) {
        IOTC_LOGW("set non block error %d", ret);
        return ret;
    }

    ret = IotcSetSocketOpt(fd, IOTC_SOCKET_OPTION_ENABLE_REUSEADDR, NULL, 0);
    if (ret != IOTC_OK) {
        IOTC_LOGW("set reuse addr error %d", ret);
        return ret;
    }

    ret = UdpSocketBindAddr(fd, param);
    if (ret != IOTC_OK) {
        return ret;
    }

    ret = JoinMulticastGroup(fd, param);
    if (ret != IOTC_OK) {
        return ret;
    }

    return IOTC_OK;
}

static int32_t UdpSocketConnect(TransSocket *socket)
{
    CHECK_RETURN_LOGW(socket != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    UdpSocket *udpSocket = (UdpSocket *)socket;

    if (udpSocket->fd >= 0) {
        return udpSocket->fd;
    }
    udpSocket->fd = IotcSocket(IOTC_SOCKET_DOMAIN_AF_INET, IOTC_SOCKET_TYPE_DGRAM,
        IOTC_SOCKET_PROTO_UDP);
    if (udpSocket->fd < 0) {
        IOTC_LOGW("create udp socket error %d/%d", udpSocket->fd, IotcGetErrno());
        return udpSocket->fd;
    }

    int32_t ret = SetUdpOpt(udpSocket->fd, &udpSocket->initParam);
    if (ret != IOTC_OK) {
        IotcClose(udpSocket->fd);
        udpSocket->fd = TRANS_SOCKET_INVALID_FD;
        return ret;
    }
    return udpSocket->fd;
}

static int32_t UdpSocketSend(TransSocket *socket, uint32_t tmo, const CommData *data, const SocketAddr *addr)
{
    NOT_USED(tmo);
    CHECK_RETURN_LOGW(socket != NULL && data != NULL && data->len != 0 && data->data != NULL && addr != NULL,
        IOTC_ERR_PARAM_INVALID, "param invalid");
    UdpSocket *udpSocket = (UdpSocket *)socket;

    if (udpSocket->fd < 0) {
        return IOTC_CORE_WIFI_TRANS_ERR_SOCKET_UDP_NOT_CONNECT;
    }

    IotcSockaddrIn sendAddr;
    (void)memset_s(&sendAddr, sizeof(IotcSockaddrIn), 0, sizeof(IotcSockaddrIn));
    sendAddr.sinFamily = IOTC_SOCKET_DOMAIN_AF_INET;
    sendAddr.sinPort = IotcHtons(addr->port);
    sendAddr.sinAddr = IotcHtonl(addr->addr);

    int32_t ret = IotcSendTo(udpSocket->fd, data->data, data->len, (IotcSockaddr *)&sendAddr, sizeof(sendAddr));
    DFX_ANONYMIZE_IP_ADDR(anonyIp, addr->addr);
    IOTC_LOGD("udp socket[%d] send to %s:%u len:%u ret:%d", udpSocket->fd, anonyIp, addr->port, data->len, ret);
    if (ret > 0) {
        return IOTC_OK;
    }

    int32_t err = IotcGetSocketErrno(udpSocket->fd);
    if (err == IOTC_SOCKET_ERRNO_NO_ERR || err == IOTC_SOCKET_ERRNO_EAGAIN ||
        err == IOTC_SOCKET_ERRNO_EWOULDBLOCK || err == IOTC_SOCKET_ERRNO_EINTR) {
        /* 链路无异常则不抛错 */
        IOTC_LOGD("udp socket[%d] send fail errno ok %d/%d", udpSocket->fd, err, IotcGetErrno());
        return IOTC_OK;
    }

    IOTC_LOGW("udp socket[%d] send error %d/%d/%d", udpSocket->fd, ret, err, IotcGetErrno());
    return err;
}

static int32_t UdpSocketRecv(TransSocket *socket, uint32_t tmo, CommBuffer *buf, SocketAddr *addr)
{
    NOT_USED(tmo);
    CHECK_RETURN_LOGW(socket != NULL && buf != NULL && buf->buffer != NULL && buf->size != 0 &&
        buf->size > buf->len && addr != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    UdpSocket *udpSocket = (UdpSocket *)socket;
    if (udpSocket->fd < 0) {
        return IOTC_CORE_WIFI_TRANS_ERR_SOCKET_UDP_NOT_CONNECT;
    }

    IotcSockaddrIn recvAddr;
    (void)memset_s(&recvAddr, sizeof(IotcSockaddrIn), 0, sizeof(IotcSockaddrIn));
    uint32_t addrLen = sizeof(IotcSockaddrIn);
    int32_t readLen = IotcRecvFrom(udpSocket->fd, buf->buffer + buf->len, buf->size - buf->len,
        (IotcSockaddr *)&recvAddr, &addrLen);
    if (readLen > 0) {
        buf->len += readLen;
        addr->port = IotcNtohs(recvAddr.sinPort);
        addr->addr = IotcNtohl(recvAddr.sinAddr);
        DFX_ANONYMIZE_IP_ADDR(anonyIp, addr->addr);
        IOTC_LOGD("udp socket[%d] recv from %s:%u len:%d", udpSocket->fd, anonyIp, addr->port, readLen);
        return IOTC_OK;
    }

    int32_t err = IotcGetSocketErrno(udpSocket->fd);
    if (err == IOTC_SOCKET_ERRNO_NO_ERR || err == IOTC_SOCKET_ERRNO_EAGAIN ||
        err == IOTC_SOCKET_ERRNO_EWOULDBLOCK || err == IOTC_SOCKET_ERRNO_EINTR) {
        /* 链路无异常则不抛错 */
        IOTC_LOGD("udp socket[%d] recv fail errno ok %d/%d", udpSocket->fd, err, IotcGetErrno());
        return IOTC_OK;
    }

    IOTC_LOGW("udp socket[%d] recv error %d/%d/%d", udpSocket->fd, readLen, err, IotcGetErrno());
    return err;
}

static void UdpSocketClose(TransSocket *socket)
{
    CHECK_V_RETURN_LOGW(socket != NULL, "param invalid");
    UdpSocket *udpSocket = (UdpSocket *)socket;

    if (udpSocket->fd >= 0) {
        IotcClose(udpSocket->fd);
        IOTC_LOGD("udp socket[%d] close", udpSocket->fd);
        udpSocket->fd = TRANS_SOCKET_INVALID_FD;
    }
}

static const SocketIf UDP_IF = {
    .socketInit = UdpSocketInit,
    .socketConnect = UdpSocketConnect,
    .socketSend = UdpSocketSend,
    .socketRecv = UdpSocketRecv,
    .socketClose = UdpSocketClose,
    .socketFree = UdpSocketFree,
};
static const char *UDP_NAME = "UDP";

TransSocket *TransSocketUdpNew(const SocketUdpInitParam *init)
{
    CHECK_RETURN_LOGW(init != NULL, NULL, "param invalid");

    return TransSocketNew(&UDP_IF, sizeof(UdpSocket), UDP_NAME, (void *)init);
}

int32_t TransSocketUdpLeaveMulticastGroup(TransSocket *socket)
{
    CHECK_RETURN_LOGW(socket != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    UdpSocket *udpSocket = (UdpSocket *)socket;
    if (udpSocket->fd < 0 || udpSocket->initParam.multicastAddr == NULL) {
        IOTC_LOGW("udp socket[%d] invalid", udpSocket->fd);
        return IOTC_CORE_WIFI_TRANS_ERR_SOCKET_UDP_INVALID;
    }

    DFX_ANONYMIZE_IP_STR(anonyIp, udpSocket->initParam.multicastAddr);
    int32_t ret = IotcSetSocketOpt(udpSocket->fd, IOTC_SOCKET_OPTION_DROP_MULTI_GROUP,
        udpSocket->initParam.multicastAddr, strlen(udpSocket->initParam.multicastAddr));
    if (ret != IOTC_OK) {
        IOTC_LOGW("udp socket[%d] leave multi group %s error %d", udpSocket->fd, anonyIp, ret);
        return ret;
    }

    IOTC_LOGD("udp socket[%d] leave multicast group %s", udpSocket->fd, anonyIp);
    return IOTC_OK;
}