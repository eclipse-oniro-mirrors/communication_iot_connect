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
#include "m2m_cloud_tcp_sess.h"
#include "trans_socket_tcp.h"
#include "securec.h"
#include "utils_common.h"
#include "coap_codec_tcp_v1.h"
#include "wifi_sched_fd_watch.h"
#include "utils_assert.h"
#include "iotc_socket.h"
#include "coap_codec_utils.h"
#include "iotc_errcode.h"
#include "comm_def.h"
#include "product_adapter.h"
#include "security_random.h"

static const char *TCP_SESS_NAME = "CLOUD_TCP";

static int32_t CloudTcpTransSocketInit(M2mCloudContext *ctx, CloudTcpUpdateRemainLen remainUpdate)
{
    CHECK_RETURN_LOGW(ctx->linkInfo.urlIndex < M2M_CLOUD_URL_NUM && ctx->linkInfo.url[ctx->linkInfo.urlIndex] != NULL
        && remainUpdate != NULL, IOTC_CORE_WIFI_M2M_ERR_CLOUD_INVALID_CTX, "param invalid");

    SocketTcpInitParam tcpParam;
    (void)memset_s(&tcpParam, sizeof(tcpParam), 0, sizeof(tcpParam));
    tcpParam.name = TCP_SESS_NAME;
    tcpParam.onUpdateRemainLen = remainUpdate;
    tcpParam.host.port = ctx->linkInfo.port == 0 ? STA_CLOUD_TCP_PORT : ctx->linkInfo.port;
    tcpParam.host.hostname = ctx->linkInfo.url[ctx->linkInfo.urlIndex];
    IOTC_LOGI("%s url:%s port:%d", __func__, ctx->linkInfo.url[ctx->linkInfo.urlIndex], ctx->linkInfo.port);

    ctx->linkInfo.socket = TransSocketTcpNew(&tcpParam);
    if (ctx->linkInfo.socket == NULL) {
        IOTC_LOGW("create tcp socket error");
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

static int32_t CloudTcpTransLinkInit(M2mCloudContext *ctx)
{
    ctx->linkInfo.link = TransLinkNew(ctx->linkInfo.socket, ctx->linkInfo.recvBuf, TCP_SESS_NAME);
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

SessCode CloudTcpSessSendUpdateSeqProcess(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info)
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
    *(uint32_t *)seqOption->value.data = IotcHtonl(*seq);
    (*seq)++;

    return SESS_CODE_CONTINUE;
}

static int32_t CloudTcpSessInit(M2mCloudContext *ctx)
{
    ctx->linkInfo.sess = TransSessNew(ctx->linkInfo.link, sizeof(CoapPacket), TCP_SESS_NAME, ctx);
    if (ctx->linkInfo.sess == NULL) {
        IOTC_LOGW("create session error");
        return IOTC_CORE_WIFI_TRANS_ERR_SESS_CREATE;
    }

    TransSessAddTailSendHandler(ctx->linkInfo.sess, CloudTcpSessSendUpdateSeqProcess, "update_seq", NULL);
    return IOTC_OK;
}

static int32_t CloudTcpSessCustomDataInit(M2mCloudContext *ctx)
{
    /* send seq */
    ctx->linkInfo.sessData = (uint32_t *)IotcMalloc(sizeof(uint32_t));
    if (ctx->linkInfo.sessData == NULL) {
        IOTC_LOGW("malloc error");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }

    (void)memset_s(ctx->linkInfo.sessData, sizeof(uint32_t), 0, sizeof(uint32_t));

    uint32_t seq = SecurityRandomUint32();
    *((uint32_t *)(ctx->linkInfo.sessData)) = IotcHtonl(seq);

    return IOTC_OK;
}

int32_t CloudLinkTcpSessInit(M2mCloudContext *ctx, CloudTcpUpdateRemainLen remainUpdate)
{
    CHECK_RETURN(ctx != NULL && remainUpdate != NULL, IOTC_ERR_PARAM_INVALID);

    int32_t ret;
    do {
        ret = CloudTcpSessCustomDataInit(ctx);
        if (ret != IOTC_OK) {
            break;
        }

        ret = CloudTcpTransSocketInit(ctx, remainUpdate);
        if (ret != IOTC_OK) {
            break;
        }

        ret = CloudTcpTransLinkInit(ctx);
        if (ret != IOTC_OK) {
            break;
        }

        ret = CloudTcpSessInit(ctx);
        if (ret != IOTC_OK) {
            break;
        }

        return IOTC_OK;
    } while (0);
    /* 异常处理 */
    CloudLinkTcpSessDeinit(ctx);
    return ret;
}

void CloudLinkTcpSessDeinit(M2mCloudContext *ctx)
{
    CHECK_V_RETURN(ctx != NULL);

    if (ctx->linkInfo.link != NULL) {
        TransLinkFree(ctx->linkInfo.link);
        ctx->linkInfo.link = NULL;
    }
    if (ctx->linkInfo.socket != NULL) {
        TransSocketFree(ctx->linkInfo.socket);
        ctx->linkInfo.socket = NULL;
    }
    if (ctx->linkInfo.sess != NULL) {
        TransSessFree(ctx->linkInfo.sess);
        ctx->linkInfo.sess = NULL;
    }

    UTILS_FREE_2_NULL(ctx->linkInfo.sessData);
}