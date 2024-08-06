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
#include "trans_socket_tls.h"
#include "comm_def.h"
#include "securec.h"
#include "security_random.h"
#include "adapter_os.h"
#include "utils_common.h"
#include "utils_time.h"
#include "iotc_errcode.h"
#include "utils_assert.h"

typedef struct {
    TransSocket base;
    SocketTlsInitParam initParam;
    AdapterTlsClient *ctx;
} TlsSocket;

static void TlsSocketFree(TransSocket *socket)
{
    TlsSocket *tlsSocket = (TlsSocket *)socket;

    UTILS_FREE_2_NULL(tlsSocket->initParam.host.hostname);
    UTILS_FREE_2_NULL(tlsSocket->initParam.suites.ciphersuites);
    UTILS_FREE_2_NULL(tlsSocket->initParam.psk.pskIdentity);
    if (tlsSocket->initParam.psk.psk != NULL) {
        (void)memset_s((uint8_t *)tlsSocket->initParam.psk.psk, tlsSocket->initParam.psk.pskLen,
            0, tlsSocket->initParam.psk.pskLen);
        AdapterFree((void *)tlsSocket->initParam.psk.psk);
        tlsSocket->initParam.psk.psk = NULL;
    }

    if (tlsSocket->ctx != NULL) {
        AdapterFreeTlsClient(tlsSocket->ctx);
        tlsSocket->ctx = NULL;
    }
}

/* 中途失败资源由外部统一释放 */
static int32_t TlsInitParamCopy(const SocketTlsInitParam *src, SocketTlsInitParam *dst)
{
    CHECK_RETURN_LOGW(src->host.hostname != NULL && src->suites.num < ADAPTER_TLS_CIPHERSUITE_MAX_NUM &&
        src->suites.ciphersuites != NULL && src->suites.num != 0 && src->onUpdateRemainLen != NULL,
        IOTC_ERR_PARAM_INVALID, "param invalid");

    /* 常量字符串 */
    dst->name = src->name;
    dst->onUpdateRemainLen = src->onUpdateRemainLen;
    dst->host.port = src->host.port;
    dst->host.hostname = UtilsStrDup(src->host.hostname);
    if (src->host.hostname == NULL) {
        IOTC_LOGW("copy hostname error");
        return IOTC_CORE_COMM_UTILS_ERR_STR_DUP;
    }

    dst->suites.num = src->suites.num;
    uint32_t size = src->suites.num * sizeof(int32_t);
    dst->suites.ciphersuites = (int32_t *)UtilsMallocCopy((uint8_t *)dst->suites.ciphersuites,
        src->suites.num * sizeof(int32_t));
    if (dst->suites.ciphersuites == NULL) {
        IOTC_LOGW("clone error %u", size);
        return IOTC_CORE_COMM_UTILS_ERR_MALLOC_COPY;
    }

    /* 证书指针为常量字符数组，内部不涉及二级指针资源管理 */
    dst->cert = src->cert;
    if (src->psk.psk != NULL && src->psk.pskLen != 0 &&
        src->psk.pskIdentity != NULL && src->psk.pskIdentityLen != 0) {
        dst->psk.pskIdentityLen = src->psk.pskIdentityLen;
        dst->psk.pskLen = src->psk.pskLen;
        dst->psk.psk = UtilsMallocCopy(src->psk.psk, src->psk.pskLen);
        dst->psk.pskIdentity = UtilsMallocCopy(src->psk.pskIdentity, src->psk.pskIdentityLen);
        if (dst->psk.psk == NULL || dst->psk.pskIdentity == NULL) {
            IOTC_LOGW("copy psk error %u/%u", src->psk.pskLen, src->psk.pskIdentityLen);
            return IOTC_CORE_COMM_UTILS_ERR_MALLOC_COPY;
        }
    }

    return IOTC_OK;
}

static int32_t TlsSocketInit(TransSocket *socket, void *userData)
{
    CHECK_RETURN_LOGW(socket != NULL && userData != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");
    const SocketTlsInitParam *initParam = (const SocketTlsInitParam *)userData;
    TlsSocket *tlsSocket = (TlsSocket *)socket;

    /* 保存初始化参数，连接时使用，socket销毁时释放 */
    int32_t ret = TlsInitParamCopy(initParam, &tlsSocket->initParam);
    if (ret != IOTC_OK) {
        TlsSocketFree(socket);
        return ret;
    }
    
    return IOTC_OK;
}

static int32_t GetUtcTime(uint64_t *stamp)
{
    CHECK_RETURN(stamp != NULL, IOTC_ERR_PARAM_INVALID);

    int32_t ret = UtilsGetUtcTimeStamp(stamp);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get time stamp error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static int32_t CreateAndConnectTls(TlsSocket *tlsSocket)
{
    tlsSocket->ctx = AdapterCreateTlsClient(tlsSocket->initParam.name);
    if (tlsSocket->ctx == NULL) {
        IOTC_LOGW("create tls client error");
        return IOTC_ADAPTER_TLS_ERR_NEW;
    }

    struct TlsOptions {
        AdapterTlsOption option;
        const void *value;
        uint32_t len;
    } ops[ADAPTER_TLS_OPTION_MAX] = {
        {ADAPTER_TLS_OPTION_REG_TIME_CALLBACK, GetUtcTime, sizeof(AdapterGetTimeCallback)},
        {ADAPTER_TLS_OPTION_REG_RANDOM_CALLBACK, SecurityRandom, sizeof(AdapterGetRandom)},
        {ADAPTER_TLS_OPTION_HOST, &tlsSocket->initParam.host, sizeof(tlsSocket->initParam.host)},
        {ADAPTER_TLS_OPTION_CIPHERSUITE, &tlsSocket->initParam.suites, sizeof(tlsSocket->initParam.suites)},
    };
    uint32_t num = 4; /* 4表示已有四个option */

    if (tlsSocket->initParam.psk.psk != NULL) {
        ops[num].option = ADAPTER_TLS_OPTION_PSK;
        ops[num].value = &tlsSocket->initParam.psk;
        ops[num].len = sizeof(tlsSocket->initParam.psk);
        ++num;
    }

    if (tlsSocket->initParam.cert.certs != NULL) {
        ops[num].option = ADAPTER_TLS_OPTION_CERT;
        ops[num].value = &tlsSocket->initParam.cert;
        ops[num].len = sizeof(tlsSocket->initParam.cert);
        ++num;
    }

    int32_t ret = IOTC_OK;
    for (uint32_t i = 0; i < num; ++i) {
        ret = AdapterSetTlsClientOption(tlsSocket->ctx, ops[i].option, ops[i].value, ops[i].len);
        if (ret != IOTC_OK) {
            IOTC_LOGW("tls conf error %d/%d", ret, ops[i].option);
            break;
        }
    }
    
    if (ret == IOTC_OK) {
        ret = AdapterTlsClientConnect(tlsSocket->ctx);
        if (ret != IOTC_OK) {
            IOTC_LOGW("tls connect error %d", ret);
        }
    }
    
    if (ret != IOTC_OK) {
        AdapterFreeTlsClient(tlsSocket->ctx);
        tlsSocket->ctx = NULL;
        return ret;
    }
    return IOTC_OK;
}

static int32_t TlsSocketConnect(TransSocket *socket)
{
    CHECK_RETURN_LOGW(socket != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");
    TlsSocket *tlsSocket = (TlsSocket *)socket;
    
    if (tlsSocket->ctx == NULL) {
        int32_t ret = CreateAndConnectTls(tlsSocket);
        if (ret != IOTC_OK) {
            return ret;
        }
    }

    return AdapterTlsClientGetFd(tlsSocket->ctx);
}

static int32_t TlsSocketSend(TransSocket *socket, uint32_t tmo, const CommData *data, const SocketAddr *addr)
{
    CHECK_RETURN_LOGW(socket != NULL && data != NULL && data->len != 0 && data->data != NULL,
        IOTC_ERR_PARAM_INVALID, "param invalid");
    NOT_USED(addr);
    TlsSocket *tlsSocket = (TlsSocket *)socket;

    uint32_t start = AdapterGetSysTimeMs();
    uint32_t sendLen = 0;
    do {
        int32_t ret = AdapterTlsClientSend(tlsSocket->ctx, data->data + sendLen, data->len - sendLen);
        if (ret > 0) {
            sendLen += ret;
        } else if (ret < 0) {
            IOTC_LOGW("tls send error %d/%d", ret, AdapterGetErrno());
            return ret;
        }
    } while (sendLen < data->len && UtilsDeltaTime(AdapterGetSysTimeMs(), start) < tmo);

    if (sendLen < data->len) {
        IOTC_LOGW("tls send timeout %u/%u/%u", tmo, sendLen, data->len);
        return IOTC_ERR_TIMEOUT;
    }
    return IOTC_OK;
}

static int32_t TlsSocketRecv(TransSocket *socket, uint32_t tmo, CommBuffer *buf, SocketAddr *addr)
{
    CHECK_RETURN_LOGW(socket != NULL && buf != NULL && buf->buffer != NULL && buf->size != 0 && buf->size > buf->len,
        IOTC_ERR_PARAM_INVALID, "invalid param");
    NOT_USED(addr);
    TlsSocket *tlsSocket = (TlsSocket *)socket;
    if (tlsSocket->initParam.onUpdateRemainLen == NULL) {
        IOTC_LOGW("get remain size func null");
        return IOTC_ERR_CALLBACK_NULL;
    }

    uint32_t startTime = AdapterGetSysTimeMs();
    uint8_t *pBuf = buf->buffer + buf->len;
    uint32_t bufSize = buf->size - buf->len;
    uint32_t remainLen;
    uint32_t recvLen = 0;
    do {
        uint32_t curBufSize = bufSize - recvLen;

        int32_t ret = tlsSocket->initParam.onUpdateRemainLen(pBuf, recvLen, &remainLen);
        if (ret != IOTC_OK) {
            IOTC_LOGW("tls get remain len error %d/%u", ret, recvLen);
            return ret;
        }
        if (remainLen > curBufSize) {
            IOTC_LOGD("buffer not enough need expend %u/%u/%u", remainLen, curBufSize, bufSize);
            buf->len += recvLen;
            return remainLen - curBufSize;
        } else if (remainLen == 0) {
            buf->len += recvLen;
            return IOTC_OK;
        }

        if (UtilsDeltaTime(AdapterGetSysTimeMs(), startTime) > tmo) {
            IOTC_LOGW("tls recv timeout %u/%u/%u", tmo, recvLen, remainLen);
            return IOTC_ERR_TIMEOUT;
        }

        ret = AdapterTlsClientRecv(tlsSocket->ctx, pBuf + recvLen, remainLen);
        if (ret > 0) {
            if (ret > remainLen) {
                IOTC_LOGW("tls recv error len %d/%u", ret, remainLen);
                return IOTC_CORE_WIFI_TRANS_ERR_TLS_SOCKET_RECV;
            }
            recvLen += ret;
        } else if (ret < 0) {
            IOTC_LOGW("tls recv error %d/%d", ret, AdapterGetErrno());
            return ret;
        }
    } while (true);

    /* unreachable */
    return IOTC_ERROR;
}
static void TlsSocketClose(TransSocket *socket)
{
    CHECK_V_RETURN_LOGW(socket != NULL, "invalid param");
    TlsSocket *tlsSocket = (TlsSocket *)socket;

    if (tlsSocket->ctx != NULL) {
        AdapterFreeTlsClient(tlsSocket->ctx);
        tlsSocket->ctx = NULL;
    }
}

TransSocket *TransSocketTlsNew(const SocketTlsInitParam *init)
{
    CHECK_RETURN_LOGW(init != NULL, NULL, "param invalid");
    static const SocketIf TLS_IF = {
        .socketInit = TlsSocketInit,
        .socketConnect = TlsSocketConnect,
        .socketSend = TlsSocketSend,
        .socketRecv = TlsSocketRecv,
        .socketClose = TlsSocketClose,
        .socketFree = TlsSocketFree,
    };
    static const char *TLS_NAME = "TLS";

    return TransSocketNew(&TLS_IF, sizeof(TlsSocket), TLS_NAME, (void *)init);
}

AdapterTlsCertVerify TransSocketTlsVerifyCert(TransSocket *socket)
{
    CHECK_RETURN_LOGW(socket != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");
    TlsSocket *tlsSocket = (TlsSocket *)socket;

    return AdapterTlsClientVerifyCert(tlsSocket->ctx);
}