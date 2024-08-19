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
#include <stdlib.h>
#include <errno.h>
#include "securec.h"
#include "iotc_tls.h"
#include "iotc_mem.h"
#include "iotc_os.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509.h"
#include "mbedtls/platform_time.h"
#include "mbedtls/net_sockets.h"
#include "iotc_errcode.h"
#include "iotc_log.h"

#ifndef MS_PER_SECOND
#define MS_PER_SECOND 1000
#endif
#define TLS_HANDSHAKE_TIMEOUT 4000
#define READ_TIMEOUT_MS (10 * 1000)

typedef struct {
    char *custom;
    int32_t *supportedCiphersuites;
    const char **certs;
    uint32_t certsNum;
    uint16_t port;
    char *hostName;
    bool delayVerifyCert;
    bool hostVerify;
    IotcTlsCertVerify verifyResult;
    uint32_t handshakeTimeoutMs;
    IotcGetRandom rngCb;
    mbedtls_x509_time validFrom;
    mbedtls_x509_time validTo;
    mbedtls_net_context fd;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_x509_crt caCert;
} TlsClient;

static IotcGetTimeCallback g_getTimeCb = NULL;

#if IOTC_CONF_ADAPTER_MBEDTLS_TLS_DEBUG
static void MbedtlsTlsDebug(void *context, int32_t level, const char *file, int32_t line, const char *str)
{
    TlsClient *cli = (TlsClient *)(context);
    IOTC_LOGD("[%s:%d]%s:%d %s", ctx->custom, level, file, line, str);
}
#endif /* HILINK_TLS_DEBUG */

/* 该函数为注册到mbedtls中获取随机数的回调 */
static int32_t MbedtlsSslRng(void *param, unsigned char *output, size_t len)
{
    TlsClient *cli = (TlsClient *)param;
    if ((cli == NULL) || (cli->rngCb == NULL) || (output == NULL) || (len == 0)) {
        IOTC_LOGW("param invalid");
        return IOTC_ERR_PARAM_INVALID;
    }

    return cli->rngCb(output, len);
}

static mbedtls_time_t MbedtlsPlatformGetTime(mbedtls_time_t *timeNow)
{
    (void)timeNow; /* 此参数可能为空 */

    mbedtls_time_t rawSeconds = 0;
    uint64_t timeMs = 0;
    if (g_getTimeCb == NULL) {
        IOTC_LOGN("no cb");
        return rawSeconds;
    }
    int32_t ret = g_getTimeCb(&timeMs);
    if (ret == IOTC_OK) {
        rawSeconds = (mbedtls_time_t)(timeMs / MS_PER_SECOND);
    } else {
        IOTC_LOGI("get local time failed, set rawSeconds=0");
    }

    return (mbedtls_time_t)rawSeconds;
}

static void InitTlsContextData(TlsClient *cli)
{
    mbedtls_ssl_init(&cli->ssl);
    mbedtls_x509_crt_init(&cli->caCert);
    mbedtls_net_init(&cli->fd);
    mbedtls_ssl_config_init(&cli->conf);
#if IOTC_CONF_ADAPTER_MBEDTLS_TLS_DEBUG
    mbedtls_ssl_conf_dbg(&(cli->conf), MbedtlsTlsDebug, cli);
#endif
}

static int32_t SetTlsConfigBase(TlsClient *cli)
{
    int32_t ret = mbedtls_ssl_config_defaults(&cli->conf, MBEDTLS_SSL_IS_CLIENT,
        MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret != 0) {
        IOTC_LOGW("ssl conf error [-0x%04x]", -ret);
        return IOTC_ADAPTER_TLS_ERR_SETUP;
    }
    mbedtls_ssl_conf_rng(&cli->conf, MbedtlsSslRng, cli);
    mbedtls_ssl_conf_min_version(&cli->conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    mbedtls_ssl_conf_max_version(&cli->conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    mbedtls_ssl_conf_authmode(&cli->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    return IOTC_OK;
}

IotcTlsClient *IotcCreateTlsClient(const char *custom)
{
    if (custom == NULL) {
        IOTC_LOGW("param invaild");
        return NULL;
    }
    uint32_t customLen = strlen(custom);
    if (customLen > IOTC_TLS_CUSTOM_MAX_STR_LEN) {
        IOTC_LOGW("param invaild");
        return NULL;
    }
    IOTC_LOGN("custom=[%s] start tls create", custom);
    TlsClient *cli = (TlsClient *)IotcMalloc(sizeof(TlsClient));
    if (cli == NULL) {
        IOTC_LOGW("malloc error");
        return NULL;
    }
    (void)memset_s(cli, sizeof(TlsClient), 0, sizeof(TlsClient));

    int32_t ret;
    do {
        cli->custom = (char *)IotcMalloc(customLen + 1);
        if (cli->custom == NULL) {
            IOTC_LOGW("malloc error");
            break;
        }
        cli->handshakeTimeoutMs = TLS_HANDSHAKE_TIMEOUT;
        ret = strcpy_s(cli->custom, customLen + 1, custom);
        if (ret != EOK) {
            IOTC_LOGW("strcpy error %d", ret);
            break;
        }

        InitTlsContextData(cli);

        ret = SetTlsConfigBase(cli);
        if (ret != IOTC_OK) {
            break;
        }

        return cli;
    } while (0);
    (void)IotcFreeTlsClient(cli);
    return NULL;
}

void IotcFreeTlsClient(IotcTlsClient *cli)
{
    if (cli == NULL) {
        IOTC_LOGN("param invaild");
        return;
    }
    TlsClient *cliCtx = (TlsClient *)cli;

    mbedtls_ssl_free(&cliCtx->ssl);
    mbedtls_net_free(&cliCtx->fd);
    mbedtls_ssl_config_free(&cliCtx->conf);
    mbedtls_x509_crt_free(&cliCtx->caCert);
    IotcFree(cliCtx->supportedCiphersuites);
    cliCtx->supportedCiphersuites = NULL;
    if (cliCtx->custom != NULL) {
        IOTC_LOGN("custom=[%s] free resource", cliCtx->custom);
        IotcFree(cliCtx->custom);
        cliCtx->custom = NULL;
    }
    if (cliCtx->hostName != NULL) {
        IotcFree(cliCtx->hostName);
        cliCtx->hostName = NULL;
    }
    IotcFree(cliCtx);
}

static int32_t InitTlsSocket(TlsClient *cli)
{
    int32_t ret;
    if (cli->fd.fd < 0) {
        if (cli->hostName != NULL) {
#define MAX_PORT_STR_LEN 20
            char portStr[MAX_PORT_STR_LEN];
            if (sprintf_s(portStr, sizeof(portStr), "%u", cli->port) <= 0) {
                IOTC_LOGW("sprintf error");
                return IOTC_ERR_SECUREC_SPRINTF;
            }
            ret = mbedtls_net_connect(&cli->fd, cli->hostName, portStr, MBEDTLS_NET_PROTO_TCP);
            if (ret != 0) {
                IOTC_LOGW("connect error [-0x%04x/%d]", -ret, IotcGetErrno());
                return IOTC_ADAPTER_TLS_ERR_CONNECT;
            }
        } else {
            IOTC_LOGW("no hostname");
            return IOTC_ERR_PARAM_INVALID;
        }
    }

    mbedtls_ssl_conf_read_timeout(&cli->conf, READ_TIMEOUT_MS);
    mbedtls_ssl_set_bio(&cli->ssl, &cli->fd, mbedtls_net_send, NULL, mbedtls_net_recv_timeout);
    return IOTC_OK;
}

static uint64_t DeltaTime(uint64_t timeNew, uint64_t timeOld)
{
    uint64_t deltaTime;

    if (timeNew >= timeOld) {
        deltaTime = timeNew - timeOld;
    } else {
        deltaTime = (UINT32_MAX - timeOld) + timeNew + 1; /* 处理时间翻转 */
    }

    return deltaTime;
}

static int32_t TlsHandshake(TlsClient *cli)
{
    int32_t ret;
    uint64_t curTime;
    uint64_t startTime = IotcGetSysTimeMs();

    do {
        ret = mbedtls_ssl_handshake(&cli->ssl);
        curTime = IotcGetSysTimeMs();
    } while (((ret == MBEDTLS_ERR_SSL_WANT_READ) || (ret == MBEDTLS_ERR_SSL_WANT_WRITE) ||
        (ret == MBEDTLS_ERR_SSL_TIMEOUT)) && (DeltaTime(curTime, startTime) < cli->handshakeTimeoutMs));
    if (ret != 0) {
        IOTC_LOGW("handshake error [-0x%04x/%d]", -ret, IotcGetErrno());
        return IOTC_ADAPTER_TLS_ERR_CONNECT;
    }

    return IOTC_OK;
}

int32_t IotcTlsClientConnect(IotcTlsClient *cli)
{
    if (cli == NULL) {
        IOTC_LOGW("param invaild");
        return IOTC_ERR_PARAM_INVALID;
    }
    TlsClient *cliCtx = (TlsClient *)cli;

    int32_t ret = InitTlsSocket(cliCtx);
    if (ret != IOTC_OK) {
        IOTC_LOGW("init socket error");
        return ret;
    }

    ret = mbedtls_ssl_setup(&cliCtx->ssl, &cliCtx->conf);
    if (ret != 0) {
        IOTC_LOGW("ssl setup error [-0x%04x]", -ret);
        return IOTC_ADAPTER_TLS_ERR_SETUP;
    }

    ret = TlsHandshake(cliCtx);
    if (ret != IOTC_OK) {
        return IOTC_ADAPTER_TLS_ERR_CONNECT;
    }

    return IOTC_OK;
}

int32_t IotcTlsClientGetFd(IotcTlsClient *cli)
{
    if (cli == NULL) {
        IOTC_LOGW("param invaild");
        return -1;
    }

    return ((TlsClient *)cli)->fd.fd;
}

int32_t IotcTlsClientRecv(IotcTlsClient *cli, uint8_t *buf, uint32_t len)
{
    if ((cli == NULL) || (buf == NULL) || (len == 0)) {
        IOTC_LOGW("param invaild");
        return IOTC_ERR_PARAM_INVALID;
    }
    TlsClient *cliCtx = (TlsClient *)cli;

    if (cliCtx->fd.fd < 0) {
        IOTC_LOGW("context invaild");
        return IOTC_ADAPTER_TLS_ERR_CLIENT_INVALID;
    }

    int32_t ret = mbedtls_ssl_read(&cliCtx->ssl, buf, len);
    if (ret > 0) {
        IOTC_LOGD("read exit [%s/%d]", cliCtx->custom, ret);
        return ret;
    } else if ((ret == MBEDTLS_ERR_SSL_TIMEOUT) || (ret == MBEDTLS_ERR_SSL_WANT_READ)) {
        return IOTC_OK;
    }

    IOTC_LOGW("read error [%s/-0x%04x/%d]", cliCtx->custom, -ret, IotcGetErrno());
    return IOTC_ADAPTER_TLS_ERR_RECV;
}

int32_t IotcTlsClientSend(IotcTlsClient *cli, const uint8_t *buf, uint32_t len)
{
    if ((cli == NULL) || (buf == NULL) || (len == 0)) {
        IOTC_LOGW("param invaild");
        return IOTC_ERR_PARAM_INVALID;
    }
    TlsClient *cliCtx = (TlsClient *)cli;

    if (cliCtx->fd.fd < 0) {
        IOTC_LOGW("context invaild");
        return IOTC_ADAPTER_TLS_ERR_CLIENT_INVALID;
    }

    int32_t ret = mbedtls_ssl_write(&cliCtx->ssl, buf, len);
    if (ret > 0) {
        IOTC_LOGD("send exit [%s/%d]", cliCtx->custom, ret);
        return ret;
    } else if ((ret == MBEDTLS_ERR_SSL_TIMEOUT) || (ret == MBEDTLS_ERR_SSL_WANT_WRITE)) {
        return IOTC_OK;
    }
        
    IOTC_LOGW("send error [%s/-0x%04x/%d]", cliCtx->custom, -ret, IotcGetErrno());
    return IOTC_ADAPTER_TLS_ERR_RECV;
}

static int32_t SetTlsClientOptionFd(TlsClient *cli, const void *value, uint32_t len)
{
    if ((len != sizeof(int32_t) || (value == NULL))) {
        IOTC_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    int32_t fd = *(const int32_t *)value;
    if (fd < 0) {
        IOTC_LOGW("invalid fd %d", fd);
        return IOTC_ERR_PARAM_INVALID;
    }
    cli->fd.fd = fd;

    return IOTC_OK;
}

static int32_t SetTlsClientOptionHost(TlsClient *cli, const void *value, uint32_t len)
{
    if ((len != sizeof(IotcTlsHost) || (value == NULL))) {
        IOTC_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    const IotcTlsHost *host = (const IotcTlsHost *)value;
    cli->port = host->port;
    if (host->hostname == NULL) {
        IOTC_LOGI("no host name for [%s]", cli->custom);
        return IOTC_OK;
    }

    uint32_t hostNameLen = strlen(host->hostname);
    if (hostNameLen > IOTC_TLS_HOSTNAME_MAX_STR_LEN) {
        IOTC_LOGW("host name too long [%u]", hostNameLen);
        return IOTC_ERR_PARAM_INVALID;
    }
    int32_t ret = mbedtls_ssl_set_hostname(&cli->ssl, host->hostname);
    if (ret != 0) {
        IOTC_LOGW("set host name error [-0x%04x]", -ret);
        return IOTC_ADAPTER_TLS_ERR_SETUP;
    }
    if (cli->hostName != NULL) {
        IotcFree(cli->hostName);
        cli->hostName = NULL;
    }
    cli->hostName = (char *)IotcMalloc(hostNameLen + 1);
    if (cli->hostName == NULL) {
        IOTC_LOGW("malloc error");
        return IOTC_ADAPTER_TLS_ERR_SETUP;
    }
    ret = strcpy_s(cli->hostName, hostNameLen + 1, host->hostname);
    if (ret != EOK) {
        IOTC_LOGW("strcpy error %d", ret);
        IotcFree(cli->hostName);
        cli->hostName = NULL;
        return IOTC_ADAPTER_TLS_ERR_SETUP;
    }

    return IOTC_OK;
}

static int32_t SetTlsClientOptiontTimeCallback(TlsClient *cli, const void *value, uint32_t len)
{
    (void)cli;
    if ((len != sizeof(IotcGetTimeCallback)) || (value == NULL)) {
        IOTC_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    g_getTimeCb = (IotcGetTimeCallback)value;
    (void)mbedtls_platform_set_time(MbedtlsPlatformGetTime);
    return IOTC_OK;
}

static int32_t SetTlsClientOptiontCiphersuite(TlsClient *cli, const void *value, uint32_t len)
{
    if ((len != sizeof(IotcTlsCiphersuites)) || (value == NULL)) {
        IOTC_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    const IotcTlsCiphersuites *suites = (const IotcTlsCiphersuites *)value;
    if ((suites->ciphersuites == NULL) || (suites->num == 0) || (suites->num > IOTC_TLS_CIPHERSUITE_MAX_NUM)) {
        IOTC_LOGW("invalid suites %u", suites->num);
        return IOTC_ERR_PARAM_INVALID;
    }

    /* 套件枚举与mbedtls套件id的映射 */
    struct CiphersuiteMap {
        IotcTlsCiphersuite iotc;
        int32_t mbedtls;
    } mapList[] = {
        {IOTC_TLS_CIPHERSUITE_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256,
            MBEDTLS_TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256},
        {IOTC_TLS_CIPHERSUITE_ECDHE_RSA_WITH_AES_256_GCM_SHA384,
            MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384},
        {IOTC_TLS_CIPHERSUITE_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
            MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256},
        {IOTC_TLS_CIPHERSUITE_PSK_WITH_AES_128_GCM_SHA256,
            MBEDTLS_TLS_PSK_WITH_AES_128_GCM_SHA256},
        {IOTC_TLS_CIPHERSUITE_PSK_WITH_AES_256_GCM_SHA384,
            MBEDTLS_TLS_PSK_WITH_AES_256_GCM_SHA384},
    };

    /* ciphersuites数组以0为结尾，需预留1 */
    int32_t *suitesInt = (int32_t *)IotcCalloc(suites->num + 1, sizeof(int32_t));
    if (suitesInt == NULL) {
        IOTC_LOGW("malloc error");
        return IOTC_ADAPTER_TLS_ERR_SETUP;
    }
    int32_t *cur = suitesInt;
    for (uint32_t i = 0; i < suites->num; ++i) {
        for (uint32_t j = 0; j < sizeof(mapList) / sizeof(struct CiphersuiteMap); ++j) {
            if (suites->ciphersuites[i] != mapList[j].iotc ||
                mbedtls_ssl_ciphersuite_from_id(mapList[j].mbedtls) == NULL) {
                continue;
            }
            IOTC_LOGD("use ciphersuite [0x%04x]", mapList[j].mbedtls);
            *cur = mapList[j].mbedtls;
            ++cur;
            break;
        }
    }
    cli->supportedCiphersuites = suitesInt;
    mbedtls_ssl_conf_ciphersuites(&cli->conf, cli->supportedCiphersuites);
    IOTC_LOGI("use cipher num %u", cur - suitesInt);
    return IOTC_OK;
}

static bool X509CheckTime(const mbedtls_x509_time *before, const mbedtls_x509_time *after)
{
    if (before->year > after->year) {
        return true;
    } else if (before->year < after->year) {
        return false;
    }
    if (before->mon > after->mon) {
        return true;
    } else if (before->mon < after->mon) {
        return false;
    }
    if (before->day > after->day) {
        return true;
    } else if (before->day < after->day) {
        return false;
    }
    if (before->hour > after->hour) {
        return true;
    } else if (before->hour < after->hour) {
        return false;
    }
    if (before->min > after->min) {
        return true;
    } else if (before->min < after->min) {
        return false;
    }
    if (before->sec > after->sec) {
        return true;
    } else if (before->sec < after->sec) {
        return false;
    }

    return true;
}

static bool RecordValidCertTime(TlsClient *cli, mbedtls_x509_crt *crt)
{
    IOTC_LOGD("vaild cert validFrom=%04d/%02d/%02d %02d:%02d:%02d",
        crt->valid_from.year, crt->valid_from.mon, crt->valid_from.day,
        crt->valid_from.hour, crt->valid_from.min, crt->valid_from.sec);
    IOTC_LOGD("vaild cert validTo=%04d/%02d/%02d %02d:%02d:%02d",
        crt->valid_to.year, crt->valid_to.mon, crt->valid_to.day,
        crt->valid_to.hour, crt->valid_to.min, crt->valid_to.sec);
    /* 取证书验证通过里面最大的有效时间 */
    if (!X509CheckTime(&cli->validTo, &crt->valid_to)) {
        if ((memcpy_s(&cli->validFrom, sizeof(mbedtls_x509_time),
            &crt->valid_from, sizeof(mbedtls_x509_time)) != EOK) ||
            (memcpy_s(&cli->validTo, sizeof(mbedtls_x509_time),
                &crt->valid_to, sizeof(mbedtls_x509_time)) != EOK)) {
            return false;
        }
    }
    IOTC_LOGD("record cert validFrom=%04d/%02d/%02d %02d:%02d:%02d",
        cli->validFrom.year, cli->validFrom.mon, cli->validFrom.day,
        cli->validFrom.hour, cli->validFrom.min, cli->validFrom.sec);
    IOTC_LOGD("record cert validTo=%04d/%02d/%02d %02d:%02d:%02d",
        cli->validTo.year, cli->validTo.mon, cli->validTo.day,
        cli->validTo.hour, cli->validTo.min, cli->validTo.sec);

    return true;
}

static int32_t VerifyOneCert(TlsClient *cli, uint32_t index, const char *rootCa,
    mbedtls_x509_crt *crt, uint32_t *flags)
{
    mbedtls_x509_crt caCert;
    mbedtls_x509_crt_init(&caCert);
    int32_t ret = mbedtls_x509_crt_parse(&caCert, (const unsigned char *)rootCa, strlen(rootCa) + 1);
    if (ret != 0) {
        mbedtls_x509_crt_free(&caCert);
        IOTC_LOGW("cert parse error [%u/-0x%04x]", index, -ret);
        return ret;
    }

    ret = mbedtls_x509_crt_verify(crt, &caCert, NULL, NULL, flags, NULL, NULL);
    mbedtls_x509_crt_free(&caCert);
    IOTC_LOGD("i=%u,flags=%08x,ret=[-0x%04x]", index, *flags, -ret);
    if (ret == 0 && (*flags) == 0) {
        cli->verifyResult = IOTC_TLS_CERT_VERIFY_VALID;
        IOTC_LOGD("verify cert okay");
        return IOTC_OK;
    }

    if (ret != MBEDTLS_ERR_X509_CERT_VERIFY_FAILED) {
        IOTC_LOGW("verify cert error i=%u,flags=%08x,ret=[-0x%04x]", index, *flags, -ret);
        return IOTC_ADAPTER_TLS_ERR_CERT_VERIFY;
    }

    if (cli->delayVerifyCert) {
        /* 延迟证书时间校验 */
        if ((((*flags) & MBEDTLS_X509_BADCERT_EXPIRED) != 0) ||
            (((*flags) & MBEDTLS_X509_BADCERT_FUTURE) != 0)) {
            if (!RecordValidCertTime(cli, crt)) {
                return IOTC_ADAPTER_TLS_ERR_CERT_VERIFY;
            }
            IOTC_LOGN("delay verify cert time");
            *flags &= ~(MBEDTLS_X509_BADCERT_EXPIRED | MBEDTLS_X509_BADCERT_FUTURE);
        }
    }
    if (cli->hostVerify) {
        if (((*flags) & MBEDTLS_X509_BADCERT_CN_MISMATCH) != 0) {
            IOTC_LOGN("not verify cert cn");
            *flags &= ~(MBEDTLS_X509_BADCERT_CN_MISMATCH);
        }
    }
    return (*flags) == 0 ? IOTC_OK : IOTC_ADAPTER_TLS_ERR_CERT_VERIFY;
}

static int32_t VerifyCertProc(void *data, mbedtls_x509_crt *crt, uint32_t *flags)
{
    if (data == NULL) {
        IOTC_LOGW("invaild context");
        return IOTC_ERR_PARAM_INVALID;
    }

    int32_t ret;
    TlsClient *cli = (TlsClient *)data;
    for (uint32_t i = 0; i < cli->certsNum; i++) {
        if (cli->certs[i] == NULL) {
            IOTC_LOGW("invaild cert");
            continue;
        }

        ret = VerifyOneCert(cli, i, cli->certs[i], crt, flags);
        if (ret != IOTC_OK) {
            continue;
        }
        return IOTC_OK;
    }

    return IOTC_ADAPTER_TLS_ERR_CERT_VERIFY;
}

static int32_t VerifyCertCb(void *data, mbedtls_x509_crt *crt, int32_t depth, uint32_t *flags)
{
    (void)depth;
    if ((data == NULL) || (crt == NULL) || (flags == NULL)) {
        IOTC_LOGW("param invaild");
        return IOTC_ERR_PARAM_INVALID;
    }

#if IOTC_CONF_ADAPTER_MBEDTLS_TLS_DEBUG
    char buf[64] = {0}; /* 64为缓冲buf大小 */
    IOTC_LOGD("Verify requested for (Depth %d):", depth);
    mbedtls_x509_crt_info(buf, sizeof(buf) - 1, "", crt);
    IOTC_LOGD("%s", buf);
    if ((*flags) == 0) {
        IOTC_LOGD("This certificate has no flags");
    } else {
        mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", *flags);
        IOTC_LOGD("%s", buf);
    }
#endif /* HILINK_TLS_DEBUG */

    if (VerifyCertProc(data, crt, flags) != IOTC_OK) {
        IOTC_LOGE("verify cert");
    }
    return IOTC_OK;
}


static int32_t SetTlsClientOptiontCert(TlsClient *cli, const void *value, uint32_t len)
{
    if ((len != sizeof(IotcTlsCerts)) || (value == NULL)) {
        IOTC_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    const IotcTlsCerts *certs = (const IotcTlsCerts *)value;
    if ((certs->certs == NULL) || (certs->num == 0)) {
        IOTC_LOGW("invalid certs");
        return IOTC_ERR_PARAM_INVALID;
    }

    /* 对于多个证书，这里只注入一个证书，其他证书在回调函数里验证 */
    int32_t ret = IOTC_ADAPTER_TLS_ERR_CERT_VERIFY;
    for (uint32_t i = 0; i < certs->num; ++i) {
        if (certs->certs[i] == NULL) {
            IOTC_LOGW("invalid certs %u", i);
            return IOTC_ERR_PARAM_INVALID;
        }
        ret = mbedtls_x509_crt_parse(&cli->caCert, (const unsigned char *)certs->certs[i],
            strlen(certs->certs[i]) + 1);
        if (ret == 0) {
            break;
        }
        mbedtls_x509_crt_free(&cli->caCert);
        mbedtls_x509_crt_init(&cli->caCert);
    }
    if (ret != 0) {
        IOTC_LOGW("cert parse error [-0x%04x]", -ret);
        return IOTC_ADAPTER_TLS_ERR_SETUP;
    }
    mbedtls_ssl_conf_ca_chain(&cli->conf, &cli->caCert, NULL);
    mbedtls_ssl_conf_verify(&cli->conf, VerifyCertCb, cli);

    cli->certs = certs->certs;
    cli->certsNum = certs->num;
    cli->delayVerifyCert = certs->delayTimeVerify;
    cli->hostVerify = certs->hostVerify;

    return IOTC_OK;
}

static int32_t SetTlsClientOptiontPsk(TlsClient *cli, const void *value, uint32_t len)
{
    if ((len != sizeof(IotcTlsPsk)) || (value == NULL)) {
        IOTC_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    const IotcTlsPsk *psk = (const IotcTlsPsk *)value;
    if ((psk->psk == NULL) || (psk->pskLen == 0) || (psk->pskIdentity == NULL) ||
        (psk->pskIdentityLen == 0)) {
        IOTC_LOGW("invalid psk");
        return IOTC_ERR_PARAM_INVALID;
    }
#if IOTC_CONF_ADAPTER_MBEDTLS_TLS_PSK_SUPPORT
    int32_t ret = mbedtls_ssl_conf_psk(&cli->conf, psk->psk, psk->pskLen, psk->pskIdentity, psk->pskIdentityLen);
    if (ret < 0) {
        IOTC_LOGW("psk set error [-0x%04x]", -ret);
        return IOTC_ADAPTER_TLS_ERR_SETUP;
    }
    return IOTC_OK;
#else
    return IOTC_ERR_NOT_SUPPORT;
#endif
}

static int32_t SetTlsClientOptiontMaxFragLen(TlsClient *cli, const void *value, uint32_t len)
{
    if ((len != sizeof(uint8_t)) || (value == NULL)) {
        IOTC_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    uint8_t fragLenType = *(const uint8_t *)value;
    if (fragLenType == IOTC_TLS_MAX_FRAG_LEN_DEFAULT) {
        return IOTC_OK;
    }

    int32_t ret = mbedtls_ssl_conf_max_frag_len(&cli->conf, fragLenType);
    if (ret != 0) {
        IOTC_LOGW("set max frag error [%u/-0x%04x]", fragLenType, -ret);
        return IOTC_ADAPTER_TLS_ERR_SETUP;
    }

    return IOTC_OK;
}

static int32_t SetTlsClientOptiontRandomCallback(TlsClient *cli, const void *value, uint32_t len)
{
    if ((len != sizeof(IotcGetRandom)) || (value == NULL)) {
        IOTC_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    cli->rngCb = (IotcGetRandom)value;

    return IOTC_OK;
}

static int32_t SetTlsClientOptiontHandshakeTimeout(TlsClient *cli, const void *value, uint32_t len)
{
    if ((len != sizeof(uint32_t)) || (value == NULL)) {
        IOTC_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    cli->handshakeTimeoutMs = *(const uint32_t *)value;
    return IOTC_OK;
}

int32_t IotcSetTlsClientOption(IotcTlsClient *cli, IotcTlsOption option, const void *value, uint32_t len)
{
    if (cli == NULL) {
        IOTC_LOGW("param invaild");
        return IOTC_ERR_PARAM_INVALID;
    }

    struct OptionItem {
        IotcTlsOption op;
        int32_t (*opFunc)(TlsClient *cli, const void *value, uint32_t len);
    } optionList[] = {
        {IOTC_TLS_OPTION_FD, SetTlsClientOptionFd},
        {IOTC_TLS_OPTION_REG_TIME_CALLBACK, SetTlsClientOptiontTimeCallback},
        {IOTC_TLS_OPTION_HOST, SetTlsClientOptionHost},
        {IOTC_TLS_OPTION_CIPHERSUITE, SetTlsClientOptiontCiphersuite},
        {IOTC_TLS_OPTION_CERT, SetTlsClientOptiontCert},
        {IOTC_TLS_OPTION_PSK, SetTlsClientOptiontPsk},
        {IOTC_TLS_OPTION_MAX_FRAG_LEN, SetTlsClientOptiontMaxFragLen},
        {IOTC_TLS_OPTION_REG_RANDOM_CALLBACK, SetTlsClientOptiontRandomCallback},
        {IOTC_TLS_OPTION_HANDSHAKE_TIMEOUT_MS, SetTlsClientOptiontHandshakeTimeout},
    };
    for (uint32_t i = 0; i < (sizeof(optionList) / sizeof(struct OptionItem)); ++i) {
        if (option == optionList[i].op) {
            return optionList[i].opFunc(cli, value, len);
        }
    }
    IOTC_LOGW("unsupport option %d", option);
    return IOTC_ERR_NOT_SUPPORT;
}

IotcTlsCertVerify IotcTlsClientVerifyCert(IotcTlsClient *cli)
{
    if (cli == NULL) {
        IOTC_LOGW("param invaild");
        return IOTC_TLS_CERT_VERIFY_INVALID;
    }
    TlsClient *cliCtx = (TlsClient *)cli;
    if (cliCtx->verifyResult == IOTC_TLS_CERT_VERIFY_VALID) {
        return IOTC_TLS_CERT_VERIFY_VALID;
    }

    int32_t ret;
    ret = mbedtls_x509_time_is_future(&cliCtx->validFrom);
    if (ret != 0) {
        IOTC_LOGW("ca time is future [-0x%04x %04d/%02d/%02d %02d:%02d:%02d]", -ret,
            cliCtx->validFrom.year, cliCtx->validFrom.mon, cliCtx->validFrom.day,
            cliCtx->validFrom.hour, cliCtx->validFrom.min, cliCtx->validFrom.sec);
        return IOTC_TLS_CERT_VERIFY_INVALID;
    }
    ret = mbedtls_x509_time_is_past(&cliCtx->validTo);
    if (ret != 0) {
        IOTC_LOGW("ca time is past [-0x%04x %04d/%02d/%02d %02d:%02d:%02d]", -ret,
            cliCtx->validFrom.year, cliCtx->validFrom.mon, cliCtx->validFrom.day,
            cliCtx->validFrom.hour, cliCtx->validFrom.min, cliCtx->validFrom.sec);
        return IOTC_TLS_CERT_VERIFY_INVALID;
    }

    IOTC_LOGI("ca time verify ok");
    cliCtx->verifyResult = IOTC_TLS_CERT_VERIFY_VALID;
    return IOTC_TLS_CERT_VERIFY_VALID;
}