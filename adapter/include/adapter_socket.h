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
#ifndef ADAPTER_SOCKET_H
#define ADAPTER_SOCKET_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ADAPTER_SOCKET_DOMAIN_AF_INET = 0,
    ADAPTER_SOCKET_DOMAIN_AF_INET6,
    ADAPTER_SOCKET_DOMAIN_UNSPEC,
} AdapterSocketDomain;

typedef enum {
    ADAPTER_SOCKET_TYPE_STREAM = 0,
    ADAPTER_SOCKET_TYPE_DGRAM,
    ADAPTER_SOCKET_TYPE_RAW,
} AdapterSocketType;

typedef enum {
    ADAPTER_SOCKET_PROTO_IP = 0,
    ADAPTER_SOCKET_PROTO_TCP,
    ADAPTER_SOCKET_PROTO_UDP,
} AdapterSocketProto;

typedef enum {
    ADAPTER_SOCKET_ERRNO_NO_ERR = 0,
    ADAPTER_SOCKET_ERRNO_EINTR = 4,
    ADAPTER_SOCKET_ERRNO_EAGAIN = 11,
    ADAPTER_SOCKET_ERRNO_EWOULDBLOCK = ADAPTER_SOCKET_ERRNO_EAGAIN,
    ADAPTER_SOCKET_ERRNO_EINPROGRESS = 115,
} AdapterSocketErrno;

typedef struct {
    const char *local;
    const char *multi;
} AdapterSocketMultiAddr;

typedef enum {
    /* 将套接字设置为阻塞模式 */
    ADAPTER_SOCKET_OPTION_SETFL_BLOCK = 0,
    /* 将套接字设置为非阻塞模式 */
    ADAPTER_SOCKET_OPTION_SETFL_NONBLOCK,
    /* 设置套接字读取超时时间，附带参数类型为uint32_t *，单位为ms */
    ADAPTER_SOCKET_OPTION_READ_TIMEOUT,
    /* 设置套接字发送超时时间，附带参数类型为uint32_t *，单位为ms */
    ADAPTER_SOCKET_OPTION_SEND_TIMEOUT,
    /* 允许套接字重复绑定地址 */
    ADAPTER_SOCKET_OPTION_ENABLE_REUSEADDR,
    /* 禁用套接字重复绑定地址 */
    ADAPTER_SOCKET_OPTION_DISABLE_REUSEADDR,
    /* 设置套接字加入组播组，附带参数类型为AdapterSocketMultiAddr *，为组播点分ip地址字符串 */
    ADAPTER_SOCKET_OPTION_ADD_MULTI_GROUP,
    /* 设置套接字退出组播组，附带参数类型为AdapterSocketMultiAddr *，为组播点分ip地址字符串 */
    ADAPTER_SOCKET_OPTION_DROP_MULTI_GROUP,
    /* 允许套接字发送广播 */
    ADAPTER_SOCKET_OPTION_ENABLE_BROADCAST,
    /* 禁用套接字发送广播 */
    ADAPTER_SOCKET_OPTION_DISABLE_BROADCAST,
    /* 允许套接字接收组播数据回环 */
    ADAPTER_SOCKET_OPTION_ENABLE_MULTI_LOOP,
    /* 禁用套接字接收组播数据回环 */
    ADAPTER_SOCKET_OPTION_DISABLE_MULTI_LOOP,
    /* 设置套接字发送缓冲区大小，附带参数类型为(uint32_t *) */
    ADAPTER_SOCKET_OPTION_SEND_BUFFER,
    /* 设置套接字读取缓冲区大小，附带参数类型为(uint32_t *) */
    ADAPTER_SOCKET_OPTION_READ_BUFFER,
} AdapterSocketOption;

typedef struct {
    uint16_t saFamily;
#define ADAPTER_SA_DATA_LEN 14
    uint8_t saData[ADAPTER_SA_DATA_LEN];
} AdapterSockaddr;

typedef struct {
    uint16_t sinFamily;
    uint16_t sinPort;
    uint32_t sinAddr;
#define ADAPTER_SIN_ZERO_LEN 8
    uint8_t sinZero[ADAPTER_SIN_ZERO_LEN];
} AdapterSockaddrIn;

typedef struct AdapterAddrInfo {
    int32_t aiFlags;
    int32_t aiFamily;
    int32_t aiSocktype;
    int32_t aiProtocol;
    int32_t aiAddrlen;
    AdapterSockaddr *aiAddr;
    char *aiCanonname;
    struct AdapterAddrInfo *aiNext;
} AdapterAddrInfo;

typedef struct {
    uint32_t num;
    int32_t *fdSet;
} AdapterFdSet;

int32_t AdapterGetAddrInfo(const char *nodename, const char *servname,
    const AdapterAddrInfo *hints, AdapterAddrInfo **result);

void AdapterFreeAddrInfo(AdapterAddrInfo *addrInfo);

int32_t AdapterSocket(AdapterSocketDomain domain, AdapterSocketType type, AdapterSocketProto proto);

void AdapterClose(int32_t fd);

int32_t AdapterSetSocketOpt(int32_t fd, AdapterSocketOption option, const void *value, uint32_t len);

int32_t AdapterBind(int32_t fd, const AdapterSockaddr *addr, uint32_t addrLen);

int32_t AdapterConnect(int32_t fd, const AdapterSockaddr *addr, uint32_t addrLen);

int32_t AdapterRecv(int32_t fd, uint8_t *buf, uint32_t len);

int32_t AdapterSend(int32_t fd, const uint8_t *buf, uint32_t len);

int32_t AdapterRecvFrom(int32_t fd, uint8_t *buf, uint32_t len, AdapterSockaddr *from, uint32_t *fromLen);

int32_t AdapterSendTo(int32_t fd, const uint8_t *buf, uint32_t len, const AdapterSockaddr *to, uint32_t toLen);

int32_t AdapterSelect(AdapterFdSet *readSet, AdapterFdSet *writeSet, AdapterFdSet *exceptSet, uint32_t ms);

int32_t AdapterGetSocketErrno(int32_t fd);

uint32_t AdapterHtonl(uint32_t hl);

uint32_t AdapterNtohl(uint32_t nl);

uint16_t AdapterHtons(uint16_t hs);

uint16_t AdapterNtohs(uint16_t ns);

int32_t AdapterInetAton(const char *ip, uint32_t *addr);

uint32_t AdapterInetAddr(const char *ip);

const char *AdapterInetNtoa(uint32_t addr, char *buf, uint32_t buflen);

#ifdef __cplusplus
}
#endif

#endif /* ADAPTER_SOCKET_H */
