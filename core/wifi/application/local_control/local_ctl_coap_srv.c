
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

#include "local_ctl_coap_srv.h"
#include "local_ctl_ctx.h"
#include "utils_bit_map.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "adapter_network.h"
#include "trans_socket_udp.h"
#include "coap_codec_udp.h"
#include "coap_endpoint_server.h"
#include "coap_endpoint_retrans.h"
#include "local_ctl_sess.h"
#include "utils_assert.h"
#include "trans_buffer_inner.h"
#include "wifi_sched_fd_watch.h"
#include "sched_event_loop.h"
#include "securec.h"
#include "local_ctl_coap_api.h"

#define LOCAL_CTL_SESS_NAME "LOCAL_CTL"
#define LOCAL_CTL_COAP_RETRANS_CNT 5
#define LOCAL_CTL_COAP_RETRANS_INTERVAL UTILS_SEC_TO_MS(1)

static int32_t CreateLocalCtlLink(LocalControlContext *ctx)
{
    char local[ADAPTER_IP_STR_MAX_LEN + 1] = {0};
    int32_t ret = AdapterGetLocalIp(local, ADAPTER_IP_STR_MAX_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get local ip error %d", ret);
        return ret;
    }

    SocketUdpInitParam udp = {
        .port = LOCAL_CONTROL_PORT,
        .localAddr = local,
        .multiAddr = NULL,
        .broadAddr = NULL,
    };

    TransSocket *socket = TransSocketUdpNew(&udp);
    if (socket == NULL) {
        IOTC_LOGW("create socket error");
        return IOTC_CORE_WIFI_TRANS_ERR_SOCKET_UDP_CREATE;
    }

    ctx->coapServer.link = TransLinkNew(socket, ctx->coapServer.recvBuf,
        LOCAL_CTL_SESS_NAME);
    if (ctx->coapServer.link == NULL) {
        IOTC_LOGW("create link error");
        TransSocketFree(socket);
        return IOTC_CORE_WIFI_TRANS_ERR_LINK_CREATE;
    }

    return IOTC_OK;
}

static int32_t CreateLocalCtlSession(LocalControlContext *ctx)
{
    ctx->coapServer.sess = TransSessNew(ctx->coapServer.link, sizeof(LocalCoapSessMsg),
        LOCAL_CTL_SESS_NAME, ctx);
    if (ctx->coapServer.sess == NULL) {
        IOTC_LOGW("create session error");
        return IOTC_CORE_WIFI_TRANS_ERR_SESS_CREATE;
    }

    /* 发现、协商不需要会话校验及加密，以NULL为结束符 */
    static const char *WHITE_LIST[] = {
        STR_URI_LOCAL_CONTROL_SEARCH,
        STR_URI_LOCAL_CONTROL_SESS_MNGR,
        NULL,
    };

    /* 本地控收包会话处理：预处理=>base64_decode=>payload解密 */
    TransSessAddTailRecvHandler(ctx->coapServer.sess, LocalCtlSessCoapRecvPreProcess, "pre", WHITE_LIST);
    TransSessAddTailRecvHandler(ctx->coapServer.sess, LocalCtlSessCoapRecvBase64Decode, "base64_decode", NULL);
    TransSessAddTailRecvHandler(ctx->coapServer.sess, LocalCtlSessCoapRecvDecrypt, "decrypt", NULL);

    /* 本地控发包：预处理=>payload加密=>base64_encode */
    TransSessAddTailSendHandler(ctx->coapServer.sess, LocalCtlSessCoapSendEncrypt, "encrypt", NULL);
    TransSessAddTailSendHandler(ctx->coapServer.sess, LocalCtlSessCoapSendBase64Encode, "base64_encode", NULL);

    return IOTC_OK;
}

static bool LocalCoapRetransCheckFunc(const CoapRetransParam *param, const CoapData *raw,
    void *userData, uint32_t *next)
{
    CHECK_RETURN_LOGW(param != NULL && next != NULL, false, "param invalid");
    NOT_USED(param);
    NOT_USED(raw);
    NOT_USED(userData);

    if (param->cnt >= LOCAL_CTL_COAP_RETRANS_CNT) {
        return false;
    }
    *next = LOCAL_CTL_COAP_RETRANS_INTERVAL;
    return true;
}

static int32_t CreateLocalCtlCoapEndpoint(LocalControlContext *ctx)
{
    ctx->coapServer.endpoint = CoapEndpointNew(ctx->coapServer.sendBuf, ctx->coapServer.sess,
        CoapUdpEncode, CoapUdpDecode, ctx);
    if (ctx->coapServer.endpoint == NULL) {
        IOTC_LOGW("create coap endpoint error");
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_ENDPOINT_CREATE;
    }

    static const CoapResource LOCAL_CTL_COAP_RES[] = {
        {UTILS_BIT(COAP_METHOD_TYPE_GET), STR_URI_LOCAL_CONTROL_SEARCH, NULL,
            LocalCtlCoapSearchHandler},
        {UTILS_BIT(COAP_METHOD_TYPE_POST), STR_URI_LOCAL_CONTROL_SESS_MNGR, NULL,
            LocalCtlCoapSessMngrHandler},
        {UTILS_BIT(COAP_METHOD_TYPE_POST), STR_E2E_CONTROL, NULL,
            LocalCtlCoapControlHandler},
    };

    int32_t ret = CoapServerAddResource(ctx->coapServer.endpoint, LOCAL_CTL_COAP_RES, ARRAY_SIZE(LOCAL_CTL_COAP_RES));
    if (ret != IOTC_OK) {
        IOTC_LOGW("add coap res error %d", ret);
        return ret;
    }

    /* 盲发重传 */
    ret = CoapEndpointRetransEnable(ctx->coapServer.endpoint, LocalCoapRetransCheckFunc, TransGetSendBufferResSize());
    if (ret != IOTC_OK) {
        IOTC_LOGW("enable coap retrans error %d", ret);
        return ret;
    }

    return IOTC_OK;
}

static int32_t LocalCoapServerCreate(LocalControlContext *ctx)
{
    int32_t ret;
    do {
        /* 1. 初始化收发缓冲区 */
        ctx->coapServer.sendBuf = TransCreateSendBuffer();
        ctx->coapServer.recvBuf = TransCreateRecvBuffer();
        if (ctx->coapServer.sendBuf == NULL || ctx->coapServer.recvBuf == NULL) {
            IOTC_LOGW("create buffer error");
            ret = IOTC_CORE_WIFI_LOCAL_CTL_ERR_CREATE_BUFFER;
            break;
        }

        /* 2. 创建udp链路，不会实际创建套接字 */
        ret = CreateLocalCtlLink(ctx);
        if (ret != IOTC_OK) {
            break;
        }

        /* 3. 创建会话管理 */
        ret = CreateLocalCtlSession(ctx);
        if (ret != IOTC_OK) {
            break;
        }

        /* 4. 创建coap端点 */
        ret = CreateLocalCtlCoapEndpoint(ctx);
        if (ret != IOTC_OK) {
            break;
        }

        return IOTC_OK;
    } while (0);
    /* 异常处理 */
    LocalControlCoapServerDestroy(ctx);
    return ret;
}

static int32_t LocalCoapServerStartInner(LocalControlContext *ctx)
{
    int32_t ret = TransLinkConnect(ctx->coapServer.link);
    if (ret != IOTC_OK) {
        IOTC_LOGW("link connect error %d", ret);
        return ret;
    }

    ret = WifiSchedLinkRecvWatch(ctx->coapServer.link);
    if (ret != IOTC_OK) {
        IOTC_LOGW("link watch error %d", ret);
        return ret;
    }

    ctx->coapServer.coapSource = CoapEndpointEventSourceNew(ctx->coapServer.endpoint);
    if (ctx->coapServer.coapSource == NULL) {
        IOTC_LOGW("create coap source error");
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_ENDPOINT_SOURCE_NEW;
    }

    ret = EventLoopAddSource(GetSchedEventLoop(), ctx->coapServer.coapSource);
    if (ret != IOTC_OK) {
        IOTC_LOGW("add coap source error %d", ret);
        return ret;
    }

    return IOTC_OK;
}

int32_t LocalControlCoapServerStart(LocalControlContext *ctx)
{
    CHECK_RETURN_LOGW(ctx != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    int32_t ret;

    if (UTILS_IS_BIT_SET(ctx->bitMap, LOCAL_CTL_CTX_COAP_SVR_STARTED)) {
        return IOTC_OK;
    }

    if (!UTILS_IS_BIT_SET(ctx->bitMap, LOCAL_CTL_CTX_COAP_SVR_CREATED)) {
        ret = LocalCoapServerCreate(ctx);
        if (ret != IOTC_OK) {
            IOTC_LOGW("create coap server error %d", ret);
            return ret;
        }
        UTILS_BIT_SET(ctx->bitMap, LOCAL_CTL_CTX_COAP_SVR_CREATED);
    }

    ret = LocalCoapServerStartInner(ctx);
    if (ret != IOTC_OK) {
        IOTC_LOGW("start coap error %d", ret);
        return ret;
    }
    UTILS_BIT_SET(ctx->bitMap, LOCAL_CTL_CTX_COAP_SVR_STARTED);
    return IOTC_OK;
}

void LocalControlCoapServerStop(LocalControlContext *ctx)
{
    CHECK_V_RETURN_LOGW(ctx != NULL, "param invalid");

    if (ctx->coapServer.coapSource != NULL) {
        EventLoopDelSource(GetSchedEventLoop(), ctx->coapServer.coapSource);
        ctx->coapServer.coapSource = NULL;
    }
    if (ctx->coapServer.link != NULL && TransLinkGetFd(ctx->coapServer.link) >= 0) {
        WifiSchedFdRemove(TransLinkGetFd(ctx->coapServer.link));
        TransLinkClose(ctx->coapServer.link);
    }
    UTILS_BIT_RESET(ctx->bitMap, LOCAL_CTL_CTX_COAP_SVR_STARTED);
}

void LocalControlCoapServerDestroy(LocalControlContext *ctx)
{
    CHECK_V_RETURN_LOGW(ctx != NULL, "param invalid");

    if (ctx->coapServer.coapSource != NULL) {
        EventLoopDelSource(GetSchedEventLoop(), ctx->coapServer.coapSource);
    }
    if (ctx->coapServer.link != NULL && TransLinkGetFd(ctx->coapServer.link) >= 0) {
        WifiSchedFdRemove(TransLinkGetFd(ctx->coapServer.link));
        TransLinkClose(ctx->coapServer.link);
    }
    if (ctx->coapServer.endpoint != NULL) {
        CoapEndpointFree(ctx->coapServer.endpoint);
    }
    if (ctx->coapServer.sess != NULL) {
        TransSessFree(ctx->coapServer.sess);
    }
    if (ctx->coapServer.link != NULL) {
        TransLinkFree(ctx->coapServer.link);
    }
    if (ctx->coapServer.recvBuf != NULL) {
        TransReleaseBuffer(ctx->coapServer.recvBuf);
    }
    if (ctx->coapServer.sendBuf != NULL) {
        TransReleaseBuffer(ctx->coapServer.sendBuf);
    }
    (void)memset_s(&ctx->coapServer, sizeof(ctx->coapServer), 0, sizeof(ctx->coapServer));
}