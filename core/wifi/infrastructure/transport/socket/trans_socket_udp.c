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
#include "adapter_os.h"
#include "utils_common.h"
#include "adapter_socket.h"
#include "iotc_errcode.h"
#include "dfx_anonymize.h"

typedef struct {
    TransSocket base;
    SocketUdpInitParam initParam;
    int32_t fd;
} UdpSocket;

static void UdpSocketFree(TransSocket *socket)
{
    CHECK_V_RETURN_LOGW(socket != NULL, "invalid param");
    UdpSocket *udpSocket = (UdpSocket *)socket;

    UTILS_FREE_2_NULL(udpSocket->initParam.broadAddr);
    UTILS_FREE_2_NULL(udpSocket->initParam.localAddr);
    UTILS_FREE_2_NULL(udpSocket->initParam.multiAddr);
    if (udpSocket->fd >= 0) {
        AdapterClose(udpSocket->fd);
        udpSocket->fd = TRANS_SOCKET_INVALID_FD;
    }
}

static int32_t UdpSocketInit(TransSocket *socket, void *userData)
{
    CHECK_RETURN_LOGW(socket != NULL && userData != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");
    const SocketUdpInitParam *initParam = (const SocketUdpInitParam *)userData;
    UdpSocket *udpSocket = (UdpSocket *)socket;

    udpSocket->fd = TRANS_SOCKET_INVALID_FD;
    /* 拷贝初始化参数，用于socket生命周期内连接/重连 */
    SocketUdpInitParam *dst = &udpSocket->initParam;
    dst->port = initParam->port;
    do {
        if (initParam->multiAddr != NULL) {
            dst->multiAddr = UtilsStrDup(initParam->multiAddr);
            if (dst->multiAddr == NULL) {
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
        if (initParam->broadAddr != NULL) {
            dst->broadAddr = UtilsStrDup(initParam->broadAddr);
            if (dst->broadAddr == NULL) {
                IOTC_LOGW("copy broadcast addr error");
                break;
            }
        }
        return IOTC_OK;
    } while (0);
    UdpSocketFree(socket);
    return IOTC_CORE_WIFI_TRANS_ERR_SOCKET_UDP_INIT;
}

static int32_t BindUdpSocket(int32_t fd, const SocketUdpInitParam *param)
{
    int32_t ret;
    char anonyIpBuf[ANONYMIZE_IP_MIN_BUF_LEN + 1] = {0};
    AdapterSockaddrIn addr;
    (void)memset_s(&addr, sizeof(AdapterSockaddrIn), 0, sizeof(AdapterSockaddrIn));
    addr.sinFamily = ADAPTER_SOCKET_DOMAIN_AF_INET;
    addr.sinPort = AdapterHtons(param->port);
    /* 有广播地址则绑定广播地址，否则绑定本地ip */
    if (param->broadAddr != NULL) {
        addr.sinAddr = AdapterInetAddr(param->broadAddr);
        ret = AdapterSetSocketOpt(fd, ADAPTER_SOCKET_OPTION_ENABLE_BROADCAST, NULL, 0);
        if (ret != IOTC_OK) {
            IOTC_LOGW("enable broadcast error %d", ret);
            return ret;
        }
        (void)DfxAnonymizeStrWithBuffer(param->broadAddr, ANONYMIZE_IP, anonyIpBuf, ANONYMIZE_IP_MIN_BUF_LEN);
    } else if (param->localAddr != NULL) {
        addr.sinAddr = AdapterInetAddr(param->localAddr);
        (void)DfxAnonymizeStrWithBuffer(param->localAddr, ANONYMIZE_IP, anonyIpBuf, ANONYMIZE_IP_MIN_BUF_LEN);
    } else {
        IOTC_LOGW("invalid addr to bind");
        return IOTC_CORE_WIFI_TRANS_ERR_SOCKET_UDP_ADDR;
    }
    IOTC_LOGD("udp socket[%d] bind to %s:%u", fd, anonyIpBuf, param->port);
    ret = AdapterBind(fd, (AdapterSockaddr *)&addr, sizeof(AdapterSockaddrIn));
    if (ret != IOTC_OK) {
        IOTC_LOGW("udp bind to %s:%u error %d", anonyIpBuf, param->port, ret);
    }
    return ret;
}

static int32_t JoinMulticastGroup(int32_t fd, const SocketUdpInitParam *param)
{
    if (param->multiAddr == NULL) {
        return IOTC_OK;
    }

    int32_t ret = AdapterSetSocketOpt(fd, ADAPTER_SOCKET_OPTION_DISABLE_MULTI_LOOP, NULL, 0);
    if (ret != IOTC_OK) {
        IOTC_LOGW("disable multi loop error %d", ret);
        return ret;
    }

    DFX_ANONYMIZE_IP_STR(anonyIp, param->multiAddr);
    AdapterSocketMultiAddr multi = {
            param->broadAddr != NULL ? param->broadAddr : param->localAddr, param->multiAddr };
    ret = AdapterSetSocketOpt(fd, ADAPTER_SOCKET_OPTION_ADD_MULTI_GROUP, &multi, sizeof(multi));
    if (ret != IOTC_OK) {
        IOTC_LOGW("add multi group %s error %d", anonyIp, ret);
        return ret;
    }
    
    IOTC_LOGD("udp socket[%d] join multicast group %s", fd, anonyIp);
    return IOTC_OK;
}

static int32_t SetUdpOpt(int32_t fd, const SocketUdpInitParam *param)
{
    int32_t ret = AdapterSetSocketOpt(fd, ADAPTER_SOCKET_OPTION_SETFL_NONBLOCK, NULL, 0);
    if (ret != IOTC_OK) {
        IOTC_LOGW("set non block error %d", ret);
        return ret;
    }

    ret = AdapterSetSocketOpt(fd, ADAPTER_SOCKET_OPTION_ENABLE_REUSEADDR, NULL, 0);
    if (ret != IOTC_OK) {
        IOTC_LOGW("set reuse addr error %d", ret);
        return ret;
    }

    ret = BindUdpSocket(fd, param);
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
    CHECK_RETURN_LOGW(socket != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");
    UdpSocket *udpSocket = (UdpSocket *)socket;

    if (udpSocket->fd >= 0) {
        return udpSocket->fd;
    }
    udpSocket->fd = AdapterSocket(ADAPTER_SOCKET_DOMAIN_AF_INET, ADAPTER_SOCKET_TYPE_DGRAM,
        ADAPTER_SOCKET_PROTO_UDP);
    if (udpSocket->fd < 0) {
        return udpSocket->fd;
    }

    int32_t ret = SetUdpOpt(udpSocket->fd, &udpSocket->initParam);
    if (ret != IOTC_OK) {
        AdapterClose(udpSocket->fd);
        udpSocket->fd = TRANS_SOCKET_INVALID_FD;
        return ret;
    }
    return udpSocket->fd;
}

static int32_t UdpSocketSend(TransSocket *socket, uint32_t tmo, const CommData *data, const SocketAddr *addr)
{
    NOT_USED(tmo);
    CHECK_RETURN_LOGW(socket != NULL && data != NULL && data->len != 0 && data->data != NULL && addr != NULL,
        IOTC_ERR_PARAM_INVALID, "invalid param");
    UdpSocket *udpSocket = (UdpSocket *)socket;

    if (udpSocket->fd < 0) {
        return IOTC_CORE_WIFI_TRANS_ERR_SOCKET_UDP_NOT_CONNECT;
    }

    AdapterSockaddrIn sendAddr;
    (void)memset_s(&sendAddr, sizeof(AdapterSockaddrIn), 0, sizeof(AdapterSockaddrIn));
    sendAddr.sinFamily = ADAPTER_SOCKET_DOMAIN_AF_INET;
    sendAddr.sinPort = AdapterHtons(addr->port);
    sendAddr.sinAddr = AdapterHtonl(addr->addr);

    int32_t ret = AdapterSendTo(udpSocket->fd, data->data, data->len, (AdapterSockaddr *)&sendAddr, sizeof(sendAddr));
    DFX_ANONYMIZE_IP_ADDR(anonyIp, addr->addr);
    IOTC_LOGD("udp socket[%d] send to %s:%u len:%u ret:%d", udpSocket->fd, anonyIp, addr->port, data->len, ret);
    if (ret > 0) {
        return IOTC_OK;
    }

    int32_t err = AdapterGetSocketErrno(udpSocket->fd);
    if (err == ADAPTER_SOCKET_ERRNO_NO_ERR || err == ADAPTER_SOCKET_ERRNO_EAGAIN ||
        err == ADAPTER_SOCKET_ERRNO_EWOULDBLOCK || err == ADAPTER_SOCKET_ERRNO_EINTR) {
        /* 链路无异常则不抛错 */
        IOTC_LOGD("udp socket[%d] send errno ok %d", udpSocket->fd, err);
        return IOTC_OK;
    }

    IOTC_LOGW("udp socket[%d] send error %d/%d", udpSocket->fd, ret, err);
    return err;
}

static int32_t UdpSocketRecv(TransSocket *socket, uint32_t tmo, CommBuffer *buf, SocketAddr *addr)
{
    NOT_USED(tmo);
    CHECK_RETURN_LOGW(socket != NULL && buf != NULL && buf->buffer != NULL && buf->size != 0 &&
        buf->size > buf->len && addr != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");
    UdpSocket *udpSocket = (UdpSocket *)socket;
    if (udpSocket->fd < 0) {
        return IOTC_CORE_WIFI_TRANS_ERR_SOCKET_UDP_NOT_CONNECT;
    }

    AdapterSockaddrIn recvAddr;
    (void)memset_s(&recvAddr, sizeof(AdapterSockaddrIn), 0, sizeof(AdapterSockaddrIn));
    uint32_t addrLen = sizeof(AdapterSockaddrIn);
    int32_t readLen = AdapterRecvFrom(udpSocket->fd, buf->buffer + buf->len, buf->size - buf->len,
        (AdapterSockaddr *)&recvAddr, &addrLen);
    if (readLen > 0) {
        buf->len += readLen;
        addr->port = AdapterNtohs(recvAddr.sinPort);
        addr->addr = AdapterNtohl(recvAddr.sinAddr);
        DFX_ANONYMIZE_IP_ADDR(anonyIp, addr->addr);
        IOTC_LOGD("udp socket[%d] recv from %s:%u len:%d", udpSocket->fd, anonyIp, addr->port, readLen);
        return IOTC_OK;
    }

    int32_t err = AdapterGetSocketErrno(udpSocket->fd);
    if (err == ADAPTER_SOCKET_ERRNO_NO_ERR || err == ADAPTER_SOCKET_ERRNO_EAGAIN ||
        err == ADAPTER_SOCKET_ERRNO_EWOULDBLOCK || err == ADAPTER_SOCKET_ERRNO_EINTR) {
        /* 链路无异常则不抛错 */
        IOTC_LOGD("udp socket[%d] recv errno ok %d", udpSocket->fd, err);
        return IOTC_OK;
    }

    IOTC_LOGW("udp socket[%d] recv error %d/%d", udpSocket->fd, readLen, err);
    return err;
}

static void UdpSocketClose(TransSocket *socket)
{
    CHECK_V_RETURN_LOGW(socket != NULL, "invalid param");
    UdpSocket *udpSocket = (UdpSocket *)socket;

    if (udpSocket->fd >= 0) {
        AdapterClose(udpSocket->fd);
        IOTC_LOGD("udp socket[%d] close", udpSocket->fd);
        udpSocket->fd = TRANS_SOCKET_INVALID_FD;
    }
}

TransSocket *TransSocketUdpNew(const SocketUdpInitParam *init)
{
    CHECK_RETURN_LOGW(init != NULL, NULL, "param invalid");
    static const SocketIf UDP_IF = {
        .socketInit = UdpSocketInit,
        .socketConnect = UdpSocketConnect,
        .socketSend = UdpSocketSend,
        .socketRecv = UdpSocketRecv,
        .socketClose = UdpSocketClose,
        .socketFree = UdpSocketFree,
    };
    static const char *UDP_NAME = "UDP";

    return TransSocketNew(&UDP_IF, sizeof(UdpSocket), UDP_NAME, (void *)init);
}

int32_t TransSocketUdpLeaveMultiGroup(TransSocket *socket)
{
    CHECK_RETURN_LOGW(socket != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");
    UdpSocket *udpSocket = (UdpSocket *)socket;
    if (udpSocket->fd < 0 || udpSocket->initParam.multiAddr == NULL) {
        IOTC_LOGW("udp socket[%d] invalid", udpSocket->fd);
        return IOTC_CORE_WIFI_TRANS_ERR_SOCKET_UDP_INVALID;
    }

    int32_t ret = AdapterSetSocketOpt(udpSocket->fd, ADAPTER_SOCKET_OPTION_DROP_MULTI_GROUP,
        udpSocket->initParam.multiAddr, strlen(udpSocket->initParam.multiAddr));
    if (ret != IOTC_OK) {
        IOTC_LOGW("leave multi group error %d", ret);
        return ret;
    }
    DFX_ANONYMIZE_IP_STR(anonyIp, udpSocket->initParam.multiAddr);
    IOTC_LOGD("udp socket[%d] leave multicast group %s", udpSocket->fd, anonyIp);
    return IOTC_OK;
}