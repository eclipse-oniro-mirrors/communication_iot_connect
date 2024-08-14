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
#include "m2m_cloud_sess.h"
#include "trans_socket_tls.h"
#include "securec.h"
#include "utils_common.h"
#include "coap_codec_tcp_v1.h"
#include "wifi_sched_fd_watch.h"
#include "utils_assert.h"
#include "adapter_socket.h"
#include "coap_codec_utils.h"
#include "iotc_errcode.h"
#include "comm_def.h"
#include "product_adapter.h"

/* TODO 证书延迟校验 */
static const char *TLS_SESS_NAME = "CLOUD_TLS";

static int32_t CloudTlsTransSocketInit(M2mCloudContext *ctx, CloudTcpUpdateRemainLen remainUpdate)
{
    CHECK_RETURN_LOGW(ctx->linkInfo.urlIndex < M2M_CLOUD_URL_NUM && ctx->linkInfo.url[ctx->linkInfo.urlIndex] != NULL
        && remainUpdate != NULL, IOTC_CORE_WIFI_M2M_ERR_CLOUD_INVALID_CTX, "param invalid");

    SocketTlsInitParam tlsParam;
    (void)memset_s(&tlsParam, sizeof(tlsParam), 0, sizeof(tlsParam));
    tlsParam.name = TLS_SESS_NAME;
    tlsParam.onUpdateRemainLen = remainUpdate;
    tlsParam.host.port = ctx->linkInfo.port == 0 ? WIFI_CLOUD_TLS_PORT : ctx->linkInfo.port;
    tlsParam.host.hostname = ctx->linkInfo.url[ctx->linkInfo.urlIndex];

    static int32_t CIPHERSUITE_LIST[] = {
        ADAPTER_TLS_CIPHERSUITE_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256,
        ADAPTER_TLS_CIPHERSUITE_ECDHE_RSA_WITH_AES_256_GCM_SHA384,
        ADAPTER_TLS_CIPHERSUITE_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
    };
    tlsParam.suites.ciphersuites = CIPHERSUITE_LIST;
    tlsParam.suites.num = ARRAY_SIZE(CIPHERSUITE_LIST);

    int32_t ret = ProductGetRootCaCert(&tlsParam.cert.certs, &tlsParam.cert.num);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get ca error %d", ret);
        return ret;
    }

    /* time sync after login */
    tlsParam.cert.delayTimeVerify = true;
    tlsParam.cert.hostVerify = true;

    ctx->linkInfo.socket = TransSocketTlsNew(&tlsParam);
    if (ctx->linkInfo.socket == NULL) {
        IOTC_LOGW("create tls socket error");
        return IOTC_CORE_WIFI_TRANS_ERR_SOCKET_TLS_CREATE;
    }
    return IOTC_OK;
}

static void CloudLinkErrorProcess(TransLink *link, void *userData)
{
    NOT_USED(link);
    CHECK_V_RETURN_LOGW(userData != NULL, "param invalid");
    M2mCloudContext *ctx = userData;
    if (ctx->linkInfo.cloudLinkErrorCallback != NULL) {
        ctx->linkInfo.cloudLinkErrorCallback(ctx);
    }
}

static int32_t CloudTlsTransLinkInit(M2mCloudContext *ctx)
{
    ctx->linkInfo.link = TransLinkNew(ctx->linkInfo.socket, ctx->linkInfo.recvBuf, TLS_SESS_NAME);
    if (ctx->linkInfo.link == NULL) {
        IOTC_LOGW("create link error");
        return IOTC_CORE_WIFI_TRANS_ERR_LINK_CREATE;
    }

    int32_t ret = TransLinkRegErrorCallback(ctx->linkInfo.link, CloudLinkErrorProcess, ctx);
    if (ret != IOTC_OK) {
        IOTC_LOGW("reg err callback error %d", ret);
        return ret;
    }

    return IOTC_OK;
}

SessCode CloudTlsSessSendUpdateSeqProcess(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info)
{
    CHECK_RETURN(msg != NULL && buf != NULL && buf->buffer != NULL && buf->len != 0 && buf->size >= buf->len &&
        info != NULL && info->addr != NULL && info->userData != NULL, false);
    M2mCloudContext *ctx = (M2mCloudContext *)info->userData;
    if (ctx->linkInfo.sessData == NULL) {
        IOTC_LOGW("sess data seq null");
        return SESS_CODE_ERR;
    }
    uint32_t *seq = (uint32_t *)ctx->linkInfo.sessData;

    CoapPacket *pkt = (CoapPacket *)msg;
    uint32_t seg = 0;
    const CoapOption *seqOption = CoapUtilsFindOption(pkt, COAP_OPTION_TYPE_SEQ_NUM_ID, &seg);
    if (seqOption == NULL || seg == 0) {
        return SESS_CODE_CONTINUE;
    }

    if (seqOption->value.data == NULL || seqOption->value.len != sizeof(uint32_t)) {
        IOTC_LOGW("get seq error");
        return SESS_CODE_ERR;
    }
    *(uint32_t *)seqOption->value.data = AdapterHtonl(*seq);
    (*seq)++;
    return SESS_CODE_CONTINUE;
}

static int32_t CloudTlsSessInit(M2mCloudContext *ctx)
{
    ctx->linkInfo.sess = TransSessNew(ctx->linkInfo.link, sizeof(CoapPacket), TLS_SESS_NAME, ctx);
    if (ctx->linkInfo.sess == NULL) {
        IOTC_LOGW("create session error");
        return IOTC_CORE_WIFI_TRANS_ERR_SESS_CREATE;
    }

    TransSessAddTailSendHandler(ctx->linkInfo.sess, CloudTlsSessSendUpdateSeqProcess, "update_seq", NULL);
    return IOTC_OK;
}

static int32_t CloudTlsSessCustomDataInit(M2mCloudContext *ctx)
{
    /* send seq */
    ctx->linkInfo.sessData = (uint32_t *)AdapterMalloc(sizeof(uint32_t));
    if (ctx->linkInfo.sessData == NULL) {
        IOTC_LOGW("malloc error");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }

    (void)memset_s(ctx->linkInfo.sessData, sizeof(uint32_t), 0, sizeof(uint32_t));
    return IOTC_OK;
}

int32_t CloudLinkSessInit(M2mCloudContext *ctx, CloudTcpUpdateRemainLen remainUpdate)
{
    CHECK_RETURN(ctx != NULL && remainUpdate != NULL, IOTC_ERR_PARAM_INVALID);

    int32_t ret;
    do {
        ret = CloudTlsSessCustomDataInit(ctx);
        if (ret != IOTC_OK) {
            break;
        }

        ret = CloudTlsTransSocketInit(ctx, remainUpdate);
        if (ret != IOTC_OK) {
            break;
        }

        ret = CloudTlsTransLinkInit(ctx);
        if (ret != IOTC_OK) {
            break;
        }

        ret = CloudTlsSessInit(ctx);
        if (ret != IOTC_OK) {
            break;
        }

        return IOTC_OK;
    } while (0);
    /* 异常处理 */
    CloudLinkSessDeinit(ctx);
    return ret;
}

void CloudLinkSessDeinit(M2mCloudContext *ctx)
{
    CHECK_V_RETURN(ctx != NULL);

    if (ctx->linkInfo.sess != NULL) {
        TransSessFree(ctx->linkInfo.sess);
        ctx->linkInfo.sess = NULL;
    }
    if (ctx->linkInfo.link != NULL) {
        TransLinkFree(ctx->linkInfo.link);
        ctx->linkInfo.link = NULL;
    }
    if (ctx->linkInfo.socket != NULL) {
        TransSocketFree(ctx->linkInfo.socket);
        ctx->linkInfo.socket = NULL;
    }
    UTILS_FREE_2_NULL(ctx->linkInfo.sessData);
}