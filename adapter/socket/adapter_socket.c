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
#include "adapter_socket.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include "securec.h"
#include "iotc_errcode.h"
#include "adapter_network.h"
#include "adapter_mem.h"
#include "adapter_log.h"

#if !IOTC_CONF_ADAPTER_SOCKET_LWIP_SUPPORT
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif
#define MS_PER_SEC 1000
#define US_PER_MS 1000
#define MAX_IP_LEN 40
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define IPV6_ADDR_LEN 28
#define MAX_ADDR_LE _IPV6_ADDR_LEN
#define MAX_DNS_RES_NUM 10

typedef struct AddrInfoInner {
    AdapterAddrInfo hiAddr;
    struct addrinfo *addr;
} AddrInfoInner;

typedef struct {
    AdapterSocketOption option;
    int32_t (*setOptionFunc)(int32_t fd, const void *value, uint32_t len);
} OptionItem;

typedef struct {
    int32_t hiType;
    int32_t sockType;
} SocketTypePair;

static const SocketTypePair AF_MAP[] = {
    {ADAPTER_SOCKET_DOMAIN_AF_INET, AF_INET},
    {ADAPTER_SOCKET_DOMAIN_AF_INET6, AF_INET6},
    {ADAPTER_SOCKET_DOMAIN_UNSPEC, AF_UNSPEC},
};

static const SocketTypePair AP_MAP[] = {
    {ADAPTER_SOCKET_PROTO_IP, IPPROTO_IP},
    {ADAPTER_SOCKET_PROTO_TCP, IPPROTO_TCP},
    {ADAPTER_SOCKET_PROTO_UDP, IPPROTO_UDP},
};

static const SocketTypePair AS_MAP[] = {
    {ADAPTER_SOCKET_TYPE_STREAM, SOCK_STREAM},
    {ADAPTER_SOCKET_TYPE_DGRAM, SOCK_DGRAM},
    {ADAPTER_SOCKET_TYPE_RAW, SOCK_RAW},
};

static int32_t AiFamily2Socket(int32_t af)
{
    for (uint32_t i = 0; i < ARRAY_SIZE(AF_MAP); ++i) {
        if (AF_MAP[i].hiType == af) {
            return AF_MAP[i].sockType;
        }
    }
    return af;
}

static int32_t Socket2AiFamily(int32_t af)
{
    for (uint32_t i = 0; i < ARRAY_SIZE(AF_MAP); ++i) {
        if (AF_MAP[i].sockType == af) {
            return AF_MAP[i].hiType;
        }
    }
    return af;
}

static int32_t AiProtocal2Socket(int32_t ap)
{
    for (uint32_t i = 0; i < ARRAY_SIZE(AP_MAP); ++i) {
        if (AP_MAP[i].hiType == ap) {
            return AP_MAP[i].sockType;
        }
    }
    return ap;
}

static int32_t Socket2AiProtocal(int32_t ap)
{
    for (uint32_t i = 0; i < ARRAY_SIZE(AP_MAP); ++i) {
        if (AP_MAP[i].sockType == ap) {
            return AP_MAP[i].hiType;
        }
    }
    return ap;
}

static int32_t AiSocket2Socket(int32_t as)
{
    for (uint32_t i = 0; i < ARRAY_SIZE(AS_MAP); ++i) {
        if (AS_MAP[i].hiType == as) {
            return AS_MAP[i].sockType;
        }
    }
    return as;
}

static int32_t Socket2AiSocket(int32_t as)
{
    for (uint32_t i = 0; i < ARRAY_SIZE(AS_MAP); ++i) {
        if (AS_MAP[i].sockType == as) {
            return AS_MAP[i].hiType;
        }
    }
    return as;
}

static void GetAdapterAddrInfo(AdapterAddrInfo *out, struct addrinfo *in,
    int32_t level, int32_t maxLevel)
{
    if ((level > maxLevel) || (in == NULL) || (out == NULL)) {
        return;
    }

    out->aiFlags = in->ai_flags;
    out->aiFamily = Socket2AiFamily(in->ai_family);
    out->aiProtocol = Socket2AiProtocal(in->ai_protocol);
    out->aiSocktype = Socket2AiSocket(in->ai_socktype);
    out->aiAddr = (AdapterSockaddr *)in->ai_addr;
    out->aiCanonname = in->ai_canonname;

    if (in->ai_next != NULL) {
        out->aiNext = (AdapterAddrInfo *)AdapterMalloc(sizeof(AdapterAddrInfo));
        if (out->aiNext == NULL) {
            ADAPTER_LOGE("malloc error");
        } else {
            (void)memset_s(out->aiNext, sizeof(AdapterAddrInfo), 0, sizeof(AdapterAddrInfo));
            GetAdapterAddrInfo(out->aiNext, in->ai_next, level + 1, maxLevel);
        }
    }
    return;
}

static void FreeAdapterAddrInfo(AdapterAddrInfo *addr)
{
    if (addr->aiNext != NULL) {
        FreeAdapterAddrInfo(addr->aiNext);
        addr->aiNext = NULL;
    }
    AdapterFree(addr);
}

static void FreeAddrInfoInner(AddrInfoInner *addrInner)
{
    if (addrInner->addr != NULL) {
        freeaddrinfo(addrInner->addr);
        addrInner->addr = NULL;
    }
    FreeAdapterAddrInfo((AdapterAddrInfo *)addrInner);
}

static AdapterAddrInfo *GetAddrInfoInner(struct addrinfo *addr)
{
    if (addr == NULL) {
        return NULL;
    }
    AddrInfoInner *addrInner = (AddrInfoInner *)AdapterMalloc(sizeof(AddrInfoInner));
    if (addrInner == NULL) {
        ADAPTER_LOGE("malloc error");
        freeaddrinfo(addr);
        return NULL;
    }
    (void)memset_s(addrInner, sizeof(AddrInfoInner), 0, sizeof(AddrInfoInner));
    GetAdapterAddrInfo(&addrInner->hiAddr, addr, 1, MAX_DNS_RES_NUM);
    addrInner->addr = addr;
    return (AdapterAddrInfo *)addrInner;
}

int32_t AdapterGetAddrInfo(const char *nodename, const char *servname,
    const AdapterAddrInfo *hints, AdapterAddrInfo **result)
{
    if ((nodename == NULL) || (result == NULL)) {
        ADAPTER_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    struct addrinfo *resInfo = NULL;
    struct addrinfo *hintsInfoP = NULL;
    struct addrinfo hintsInfo;
    ADAPTER_LOGD("get addinfo %s", nodename);
    if (hints != NULL) {
        if (memcpy_s(&hintsInfo, sizeof(struct addrinfo), hints, sizeof(AdapterAddrInfo)) != EOK) {
            ADAPTER_LOGE("memcpy error");
            return IOTC_ERR_SECUREC_MEMCPY;
        }
        hintsInfo.ai_family = AiFamily2Socket(hintsInfo.ai_family);
        hintsInfo.ai_protocol = AiProtocal2Socket(hintsInfo.ai_protocol);
        hintsInfo.ai_socktype = AiSocket2Socket(hintsInfo.ai_socktype);
        hintsInfoP = &hintsInfo;
    }

    int32_t ret = getaddrinfo(nodename, servname, hintsInfoP, &resInfo);
    if ((ret != 0) || (resInfo == NULL)) {
        ADAPTER_LOGD("getaddrinfo failed, ret %d", ret);
        return IOTC_ADAPTER_SOCKET_ERR_DNS;
    }

    *result = GetAddrInfoInner(resInfo);
    return IOTC_OK;
}

void AdapterFreeAddrInfo(AdapterAddrInfo *addrInfo)
{
    if (addrInfo == NULL) {
        ADAPTER_LOGE("invalid param");
        return;
    }
    FreeAddrInfoInner((AddrInfoInner *)addrInfo);
    return;
}

int32_t AdapterSocket(AdapterSocketDomain domain, AdapterSocketType type, AdapterSocketProto proto)
{
    int32_t af = AiFamily2Socket(domain);
    int32_t st = AiSocket2Socket(type);
    int32_t ap = AiProtocal2Socket(proto);

    return socket(af, st, ap);
}

void AdapterClose(int32_t fd)
{
    (void)close(fd);
    return;
}

static int32_t SetFcntl(int32_t fd, bool isBlock)
{
    int32_t flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        ADAPTER_LOGE("fcntl get failed, ret %d", flags);
        return IOTC_ADAPTER_SOCKET_ERR_FCNTL;
    }
    if (isBlock) {
        flags &= (~O_NONBLOCK);
    } else {
        flags |= O_NONBLOCK;
    }
    if (fcntl(fd, F_SETFL, flags) < 0) {
        ADAPTER_LOGE("fcntl set failed");
        return IOTC_ADAPTER_SOCKET_ERR_FCNTL;
    }
    return IOTC_OK;
}

static int32_t SetSocketOptionNonblock(int32_t fd, const void *value, uint32_t len)
{
    (void)value;
    (void)len;

    return SetFcntl(fd, false);
}

static int32_t SetSocketOptionBlock(int32_t fd, const void *value, uint32_t len)
{
    (void)value;
    (void)len;

    return SetFcntl(fd, true);
}

static int32_t SetSocketTimeout(int32_t fd, uint32_t timeout, bool isRead)
{
    struct timeval tv;
    int32_t flag = isRead ? SO_RCVTIMEO : SO_SNDTIMEO;

    tv.tv_sec = timeout / MS_PER_SEC;
    tv.tv_usec = timeout % MS_PER_SEC * US_PER_MS;
    if (setsockopt(fd, SOL_SOCKET, flag, &tv, sizeof(struct timeval)) != 0) {
        ADAPTER_LOGE("set [%d][%u] failed", flag, timeout);
        return IOTC_ADAPTER_SOCKET_ERR_SET_OPT;
    }
    return IOTC_OK;
}

static int32_t SetSocketOptionReadTimeout(int32_t fd, const void *value, uint32_t len)
{
    if (len < sizeof(uint32_t)) {
        ADAPTER_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    uint32_t timeout = *(const uint32_t *)value;

    return SetSocketTimeout(fd, timeout, true);
}

static int32_t SetSocketOptionSendTimeout(int32_t fd, const void *value, uint32_t len)
{
    if (len < sizeof(uint32_t)) {
        ADAPTER_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    uint32_t timeout = *(const uint32_t *)value;

    return SetSocketTimeout(fd, timeout, false);
}

static int32_t SetSocketOptionEnableReuseAddr(int32_t fd, const void *value, uint32_t len)
{
    (void)value;
    (void)len;
    int32_t opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) != 0) {
        ADAPTER_LOGE("set reuse addr failed");
        return IOTC_ADAPTER_SOCKET_ERR_SET_OPT;
    }
    return IOTC_OK;
}

static int32_t SetSocketOptionDisableReuseAddr(int32_t fd, const void *value, uint32_t len)
{
    (void)value;
    (void)len;
    int32_t opt = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) != 0) {
        ADAPTER_LOGE("close reuse addr failed");
        return IOTC_ADAPTER_SOCKET_ERR_SET_OPT;
    }
    return IOTC_OK;
}

static int32_t SetSocketMultiGroup(int32_t fd, const void *value, uint32_t len, bool isAdd)
{
    if (value == NULL || len < sizeof(AdapterSocketMultiAddr)) {
        ADAPTER_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    AdapterSocketMultiAddr *addr = (AdapterSocketMultiAddr *)value;
    if (addr->local == NULL || addr->multi == NULL) {
        ADAPTER_LOGE("invalid ip");
        return IOTC_ERR_PARAM_INVALID;
    }

    struct ip_mreq group;
    (void)memset_s(&group, sizeof(struct ip_mreq), 0, sizeof(struct ip_mreq));
    group.imr_multiaddr.s_addr = AdapterInetAddr(addr->multi);
    group.imr_interface.s_addr = AdapterInetAddr(addr->local);

    int32_t flag = isAdd ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP;
    int32_t ret = setsockopt(fd, IPPROTO_IP, flag, (void *)&group, sizeof(group));
    if (ret != 0) {
        ADAPTER_LOGE("set opt %d failed %d %d", flag, ret, AdapterGetSocketErrno(fd));
        return IOTC_ADAPTER_SOCKET_ERR_SET_OPT;
    }
    return IOTC_OK;
}

static int32_t SetSocketOptionAddMultiGroup(int32_t fd, const void *value, uint32_t len)
{
    return SetSocketMultiGroup(fd, value, len, true);
}

static int32_t SetSocketOptionDropMultiGroup(int32_t fd, const void *value, uint32_t len)
{
    return SetSocketMultiGroup(fd, value, len, false);
}

static int32_t SetSocketOptionEnableBroadcast(int32_t fd, const void *value, uint32_t len)
{
    (void)value;
    (void)len;
    int32_t opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (const char *)&opt, sizeof(opt)) != 0) {
        ADAPTER_LOGE("set broadcast failed");
        return IOTC_ADAPTER_SOCKET_ERR_SET_OPT;
    }
    return IOTC_OK;
}

static int32_t SetSocketOptionDisableBroadcast(int32_t fd, const void *value, uint32_t len)
{
    (void)value;
    (void)len;
    int32_t opt = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (const char *)&opt, sizeof(opt)) != 0) {
        ADAPTER_LOGE("close broadcast failed");
        return IOTC_ADAPTER_SOCKET_ERR_SET_OPT;
    }
    return IOTC_OK;
}

static int32_t SetSocketOptionEnableMultiLoop(int32_t fd, const void *value, uint32_t len)
{
    (void)value;
    (void)len;
    int32_t opt = 1;
    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, (const char *)&opt, sizeof(opt)) != 0) {
        ADAPTER_LOGE("set loop failed");
        return IOTC_ADAPTER_SOCKET_ERR_SET_OPT;
    }
    return IOTC_OK;
}

static int32_t SetSocketOptionDisableMultiLoop(int32_t fd, const void *value, uint32_t len)
{
    (void)value;
    (void)len;
    int32_t opt = 0;
    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, (const char *)&opt, sizeof(opt)) != 0) {
        ADAPTER_LOGE("set loop failed");
        return IOTC_ADAPTER_SOCKET_ERR_SET_OPT;
    }
    return IOTC_OK;
}

static int32_t SetSocketOptionSendBuffer(int32_t fd, const void *value, uint32_t len)
{
    if (len < sizeof(uint32_t)) {
        ADAPTER_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    uint32_t bufferLen = *(uint32_t *)value;
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void *)&bufferLen, sizeof(bufferLen)) != 0) {
        ADAPTER_LOGE("set sendbuf %u failed", bufferLen);
        return IOTC_ADAPTER_SOCKET_ERR_SET_OPT;
    }
    return IOTC_OK;
}

static int32_t SetSocketOptionReadBuffer(int32_t fd, const void *value, uint32_t len)
{
    if (len < sizeof(uint32_t)) {
        ADAPTER_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    uint32_t bufferLen = *(uint32_t *)value;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void *)&bufferLen, sizeof(bufferLen)) != 0) {
        ADAPTER_LOGE("set recvbuf %u failed", bufferLen);
        return IOTC_ADAPTER_SOCKET_ERR_SET_OPT;
    }
    return IOTC_OK;
}

int32_t AdapterSetSocketOpt(int32_t fd, AdapterSocketOption option, const void *value, uint32_t len)
{
    static const OptionItem optionList[] = {
        {ADAPTER_SOCKET_OPTION_SETFL_BLOCK, SetSocketOptionBlock},
        {ADAPTER_SOCKET_OPTION_SETFL_NONBLOCK, SetSocketOptionNonblock},
        {ADAPTER_SOCKET_OPTION_READ_TIMEOUT, SetSocketOptionReadTimeout},
        {ADAPTER_SOCKET_OPTION_SEND_TIMEOUT, SetSocketOptionSendTimeout},
        {ADAPTER_SOCKET_OPTION_ENABLE_REUSEADDR, SetSocketOptionEnableReuseAddr},
        {ADAPTER_SOCKET_OPTION_DISABLE_REUSEADDR, SetSocketOptionDisableReuseAddr},
        {ADAPTER_SOCKET_OPTION_ADD_MULTI_GROUP, SetSocketOptionAddMultiGroup},
        {ADAPTER_SOCKET_OPTION_DROP_MULTI_GROUP, SetSocketOptionDropMultiGroup},
        {ADAPTER_SOCKET_OPTION_ENABLE_BROADCAST, SetSocketOptionEnableBroadcast},
        {ADAPTER_SOCKET_OPTION_DISABLE_BROADCAST, SetSocketOptionDisableBroadcast},
        {ADAPTER_SOCKET_OPTION_ENABLE_MULTI_LOOP, SetSocketOptionEnableMultiLoop},
        {ADAPTER_SOCKET_OPTION_DISABLE_MULTI_LOOP, SetSocketOptionDisableMultiLoop},
        {ADAPTER_SOCKET_OPTION_SEND_BUFFER, SetSocketOptionSendBuffer},
        {ADAPTER_SOCKET_OPTION_READ_BUFFER, SetSocketOptionReadBuffer},
    };
    for (uint32_t i = 0; i < (sizeof(optionList) / sizeof(OptionItem)); ++i) {
        if (option == optionList[i].option) {
            return optionList[i].setOptionFunc(fd, value, len);
        }
    }
    ADAPTER_LOGW("unsupport option %d", option);
    return IOTC_ERR_NOT_SUPPORT;
}

int32_t AdapterBind(int32_t fd, const AdapterSockaddr *addr, uint32_t addrLen)
{
    (void)addrLen;
    if (addr == NULL) {
        ADAPTER_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    struct sockaddr addrIn;
    addrIn.sa_family = AiFamily2Socket(addr->saFamily);
    if (memcpy_s(addrIn.sa_data, sizeof(addrIn.sa_data), addr->saData, sizeof(addr->saData)) != EOK) {
        ADAPTER_LOGE("memcpy error");
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    return bind(fd, &addrIn, sizeof(struct sockaddr));
}

int32_t AdapterConnect(int32_t fd, const AdapterSockaddr *addr, uint32_t addrLen)
{
    (void)addrLen;
    if (addr == NULL) {
        ADAPTER_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    struct sockaddr addrIn;
    addrIn.sa_family = AiFamily2Socket(addr->saFamily);
    if (memcpy_s(addrIn.sa_data, sizeof(addrIn.sa_data), addr->saData, sizeof(addr->saData)) != EOK) {
        ADAPTER_LOGE("memcpy error");
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    return connect(fd, &addrIn, sizeof(struct sockaddr));
}

int32_t AdapterRecv(int32_t fd, uint8_t *buf, uint32_t len)
{
    return recv(fd, buf, len, MSG_DONTWAIT);
}

int32_t AdapterSend(int32_t fd, const uint8_t *buf, uint32_t len)
{
    return send(fd, buf, len, MSG_DONTWAIT);
}

int32_t AdapterRecvFrom(int32_t fd, uint8_t *buf, uint32_t len, AdapterSockaddr *from, uint32_t *fromLen)
{
    if ((from == NULL) || (fromLen == NULL) || (buf == NULL)) {
        ADAPTER_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    struct sockaddr addr;
    (void)memset_s(&addr, sizeof(struct sockaddr), 0, sizeof(struct sockaddr));
    int32_t ret = recvfrom(fd, buf, len, 0, &addr, (socklen_t *)fromLen);
    from->saFamily = addr.sa_family;
    if (memcpy_s(from->saData, sizeof(from->saData), addr.sa_data, sizeof(addr.sa_data)) != EOK) {
        ADAPTER_LOGE("memcpy error");
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    return ret;
}

int32_t AdapterSendTo(int32_t fd, const uint8_t *buf, uint32_t len, const AdapterSockaddr *to, uint32_t toLen)
{
    if ((to == NULL) || (buf == NULL)) {
        ADAPTER_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    struct sockaddr addr;
    (void)memset_s(&addr, sizeof(struct sockaddr), 0, sizeof(struct sockaddr));
    addr.sa_family = AiFamily2Socket(to->saFamily);
    if (memcpy_s(addr.sa_data, sizeof(addr.sa_data), to->saData, sizeof(to->saData)) != EOK) {
        ADAPTER_LOGE("memcpy error");
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    return sendto(fd, buf, len, 0, &addr, toLen);
}

static void GetFdSet(AdapterFdSet *set, fd_set *fdSet, int32_t *maxfd)
{
    if ((set == NULL) || (set->fdSet == NULL)) {
        return;
    }

    (void)memset_s(fdSet, sizeof(fd_set), 0, sizeof(fd_set));
    for (uint32_t i = 0; i < set->num; ++i) {
        if (set->fdSet[i] >= 0) {
            FD_SET(set->fdSet[i], fdSet);
            *maxfd = MAX(*maxfd, set->fdSet[i]);
        }
    }
}

static void FdIsSet(AdapterFdSet *set, fd_set *fdSet)
{
    if (set == NULL) {
        return;
    }

    for (uint32_t i = 0; i < set->num; ++i) {
        if (FD_ISSET(set->fdSet[i], fdSet) == 0) {
            set->fdSet[i] = -1;
        }
    }
    return;
}

int32_t AdapterSelect(AdapterFdSet *readSet, AdapterFdSet *writeSet, AdapterFdSet *exceptSet, uint32_t ms)
{
    int32_t maxFd = -1;
    fd_set read, write, except;
    GetFdSet(readSet, &read, &maxFd);
    GetFdSet(writeSet, &write, &maxFd);
    GetFdSet(exceptSet, &except, &maxFd);

    struct timeval timeout;
    timeout.tv_sec = ms / MS_PER_SEC;
    timeout.tv_usec = ms % MS_PER_SEC * US_PER_MS;
    int32_t ret = select(maxFd + 1, (readSet == NULL) ? NULL : &read,
        (writeSet == NULL) ? NULL : &write,
        (exceptSet == NULL) ? NULL : &except, &timeout);
    if (ret <= 0) {
        return ret;
    }
    FdIsSet(readSet, &read);
    FdIsSet(writeSet, &write);
    FdIsSet(exceptSet, &except);
    return ret;
}

int32_t AdapterGetSocketErrno(int32_t fd)
{
#if defined(errno)
    if (fd < 0) {
        return errno;
    }
#endif
    int32_t socketErr;
    uint32_t len = sizeof(socklen_t);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &socketErr, (socklen_t *)&len) != 0) {
        ADAPTER_LOGE("get socket errno error");
        return IOTC_ADAPTER_SOCKET_ERR_GET_OPT;
    }

    switch (socketErr) {
        case EINTR:
            return ADAPTER_SOCKET_ERRNO_EINTR;
        case EAGAIN:
            return ADAPTER_SOCKET_ERRNO_EAGAIN;
        case EINPROGRESS:
            return ADAPTER_SOCKET_ERRNO_EINPROGRESS;
        default:
            break;
    }
    return socketErr;
}

uint32_t AdapterHtonl(uint32_t hl)
{
    return htonl(hl);
}

uint32_t AdapterNtohl(uint32_t nl)
{
    return ntohl(nl);
}

uint16_t AdapterHtons(uint16_t hs)
{
    return htons(hs);
}

uint16_t AdapterNtohs(uint16_t ns)
{
    return ntohs(ns);
}

int32_t AdapterInetAton(const char *ip, uint32_t *addr)
{
    return inet_aton(ip, (struct in_addr *)addr);
}

uint32_t AdapterInetAddr(const char *ip)
{
    return inet_addr(ip);
}

const char *AdapterInetNtoa(uint32_t addr, char *buf, uint32_t buflen)
{
    struct in_addr tempAddr;
    tempAddr.s_addr = addr;
#if IOTC_CONF_ADAPTER_SOCKET_LWIP_SUPPORT
    return inet_ntoa_r(tempAddr, buf, buflen);
#else
    return inet_ntop(AF_INET, &tempAddr, buf, sizeof(tempAddr));
#endif
}