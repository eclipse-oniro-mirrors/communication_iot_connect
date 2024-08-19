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
#ifndef IOTC_SOCKET_H
#define IOTC_SOCKET_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IOTC_SOCKET_DOMAIN_AF_INET = 0,
    IOTC_SOCKET_DOMAIN_AF_INET6,
    IOTC_SOCKET_DOMAIN_UNSPEC,
} IotcSocketDomain;

typedef enum {
    IOTC_SOCKET_TYPE_STREAM = 0,
    IOTC_SOCKET_TYPE_DGRAM,
    IOTC_SOCKET_TYPE_RAW,
} IotcSocketType;

typedef enum {
    IOTC_SOCKET_PROTO_IP = 0,
    IOTC_SOCKET_PROTO_TCP,
    IOTC_SOCKET_PROTO_UDP,
} IotcSocketProto;

typedef enum {
    IOTC_SOCKET_ERRNO_NO_ERR = 0,
    IOTC_SOCKET_ERRNO_EINTR = 4,
    IOTC_SOCKET_ERRNO_EAGAIN = 11,
    IOTC_SOCKET_ERRNO_EWOULDBLOCK = IOTC_SOCKET_ERRNO_EAGAIN,
    IOTC_SOCKET_ERRNO_EINPROGRESS = 115,
} IotcSocketErrno;

typedef struct {
    const char *local;
    const char *multi;
} IotcSocketMultiAddr;

typedef enum {
    /* 将套接字设置为阻塞模式 */
    IOTC_SOCKET_OPTION_SETFL_BLOCK = 0,
    /* 将套接字设置为非阻塞模式 */
    IOTC_SOCKET_OPTION_SETFL_NONBLOCK,
    /* 设置套接字读取超时时间，附带参数类型为uint32_t *，单位为ms */
    IOTC_SOCKET_OPTION_READ_TIMEOUT,
    /* 设置套接字发送超时时间，附带参数类型为uint32_t *，单位为ms */
    IOTC_SOCKET_OPTION_SEND_TIMEOUT,
    /* 允许套接字重复绑定地址 */
    IOTC_SOCKET_OPTION_ENABLE_REUSEADDR,
    /* 禁用套接字重复绑定地址 */
    IOTC_SOCKET_OPTION_DISABLE_REUSEADDR,
    /* 设置套接字加入组播组，附带参数类型为IotcSocketMultiAddr *，为组播点分ip地址字符串 */
    IOTC_SOCKET_OPTION_ADD_MULTI_GROUP,
    /* 设置套接字退出组播组，附带参数类型为IotcSocketMultiAddr *，为组播点分ip地址字符串 */
    IOTC_SOCKET_OPTION_DROP_MULTI_GROUP,
    /* 允许套接字发送广播 */
    IOTC_SOCKET_OPTION_ENABLE_BROADCAST,
    /* 禁用套接字发送广播 */
    IOTC_SOCKET_OPTION_DISABLE_BROADCAST,
    /* 允许套接字接收组播数据回环 */
    IOTC_SOCKET_OPTION_ENABLE_MULTI_LOOP,
    /* 禁用套接字接收组播数据回环 */
    IOTC_SOCKET_OPTION_DISABLE_MULTI_LOOP,
    /* 设置套接字发送缓冲区大小，附带参数类型为(uint32_t *) */
    IOTC_SOCKET_OPTION_SEND_BUFFER,
    /* 设置套接字读取缓冲区大小，附带参数类型为(uint32_t *) */
    IOTC_SOCKET_OPTION_READ_BUFFER,
} IotcSocketOption;

typedef struct {
    uint16_t saFamily;
#define ADAPTER_SA_DATA_LEN 14
    uint8_t saData[ADAPTER_SA_DATA_LEN];
} IotcSockaddr;

typedef struct {
    uint16_t sinFamily;
    uint16_t sinPort;
    uint32_t sinAddr;
#define ADAPTER_SIN_ZERO_LEN 8
    uint8_t sinZero[ADAPTER_SIN_ZERO_LEN];
} IotcSockaddrIn;

typedef struct IotcAddrInfo {
    int32_t aiFlags;
    int32_t aiFamily;
    int32_t aiSocktype;
    int32_t aiProtocol;
    int32_t aiAddrlen;
    IotcSockaddr *aiAddr;
    char *aiCanonname;
    struct IotcAddrInfo *aiNext;
} IotcAddrInfo;

typedef struct {
    uint32_t num;
    int32_t *fdSet;
} IotcFdSet;

int32_t IotcGetAddrInfo(const char *nodename, const char *servname,
    const IotcAddrInfo *hints, IotcAddrInfo **result);

void IotcFreeAddrInfo(IotcAddrInfo *addrInfo);

int32_t IotcSocket(IotcSocketDomain domain, IotcSocketType type, IotcSocketProto proto);

void IotcClose(int32_t fd);

int32_t IotcSetSocketOpt(int32_t fd, IotcSocketOption option, const void *value, uint32_t len);

int32_t IotcBind(int32_t fd, const IotcSockaddr *addr, uint32_t addrLen);

int32_t IotcConnect(int32_t fd, const IotcSockaddr *addr, uint32_t addrLen);

int32_t IotcRecv(int32_t fd, uint8_t *buf, uint32_t len);

int32_t IotcSend(int32_t fd, const uint8_t *buf, uint32_t len);

int32_t IotcRecvFrom(int32_t fd, uint8_t *buf, uint32_t len, IotcSockaddr *from, uint32_t *fromLen);

int32_t IotcSendTo(int32_t fd, const uint8_t *buf, uint32_t len, const IotcSockaddr *to, uint32_t toLen);

int32_t IotcSelect(IotcFdSet *readSet, IotcFdSet *writeSet, IotcFdSet *exceptSet, uint32_t ms);

int32_t IotcGetSocketErrno(int32_t fd);

uint32_t IotcHtonl(uint32_t hl);

uint32_t IotcNtohl(uint32_t nl);

uint16_t IotcHtons(uint16_t hs);

uint16_t IotcNtohs(uint16_t ns);

int32_t IotcInetAton(const char *ip, uint32_t *addr);

uint32_t IotcInetAddr(const char *ip);

const char *IotcInetNtoa(uint32_t addr, char *buf, uint32_t buflen);

#ifdef __cplusplus
}
#endif

#endif /* IOTC_SOCKET_H */
