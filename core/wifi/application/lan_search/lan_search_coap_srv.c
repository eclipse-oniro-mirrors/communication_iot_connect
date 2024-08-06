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

#include "lan_search_coap_srv.h"
#include "lan_search_ctx.h"
#include "utils_bit_map.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "adapter_network.h"
#include "trans_socket_udp.h"
#include "coap_codec_udp.h"
#include "coap_endpoint_server.h"
#include "coap_endpoint_retrans.h"
#include "utils_assert.h"
#include "trans_buffer_inner.h"
#include "wifi_sched_fd_watch.h"
#include "sched_event_loop.h"
#include "securec.h"
#include "lan_search_sess.h"
#include "lan_search_coap_api.h"

#define LAN_SEARCH_SESS_NAME "LAN_SEARCH"
#define LAN_SEARCH_COAP_RETRANS_CNT 5
#define LAN_SEARCH_COAP_RETRANS_INTERVAL UTILS_SEC_TO_MS(1)

static int32_t CreateLanSearchLink(LanSearchContext *ctx)
{
    char local[ADAPTER_IP_STR_MAX_LEN + 1] = {0};
    int32_t ret = AdapterGetLocalIp(local, ADAPTER_IP_STR_MAX_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get local ip error %d", ret);
        return ret;
    }

    SocketUdpInitParam udp = {
        .port = LAN_SEARCH_PORT,
        .localAddr = local,
        .multiAddr = NULL,
        .broadAddr = NULL,
    };

    TransSocket *socket = TransSocketUdpNew(&udp);
    if (socket == NULL) {
        IOTC_LOGW("create socket error");
        return IOTC_CORE_WIFI_TRANS_ERR_SOCKET_UDP_CREATE;
    }

    ctx->coapServer.link = TransLinkNew(socket, ctx->coapServer.recvBuf, "LAN_SEARCH");
    if (ctx->coapServer.link == NULL) {
        IOTC_LOGW("create link error");
        TransSocketFree(socket);
        return IOTC_CORE_WIFI_TRANS_ERR_LINK_CREATE;
    }

    return IOTC_OK;
}

static int32_t CreateLanSearchSession(LanSearchContext *ctx)
{
    ctx->coapServer.sess = TransSessNew(ctx->coapServer.link, sizeof(LanSearchSessMsg),
        LAN_SEARCH_SESS_NAME, ctx);
    if (ctx->coapServer.sess == NULL) {
        IOTC_LOGW("create session error");
        return IOTC_CORE_WIFI_TRANS_ERR_SESS_CREATE;
    }

    /* 发现、协商不需要会话校验及加密，以NULL为结束符 */
    static const char *WHITE_LIST[] = { STR_URI_LAN_SEARCH, STR_URI_SPKEK_V2 };
    ctx->coapServer.whiteList = WHITE_LIST;
    ctx->coapServer.whiteListNum = ARRAY_SIZE(WHITE_LIST);

    /* 本地发现收包：预处理=>base64解码=>speke解密 */
    TransSessAddTailRecvHandler(ctx->coapServer.sess, LanSearchSessCoapRecvPreProcess, "pre", NULL);
    TransSessAddTailRecvHandler(ctx->coapServer.sess, LanSearchSessCoapRecvBase64Decode, "base64", NULL);
    TransSessAddTailRecvHandler(ctx->coapServer.sess, LanSearchSessCoapRecvDecrypt, "decrypt", NULL);

    /* 本地发现发包：speke加密=>base64编码 */
    TransSessAddTailSendHandler(ctx->coapServer.sess, LanSearchSessCoapSendEncrypt, "encrypt", NULL);
    TransSessAddTailSendHandler(ctx->coapServer.sess, LanSearchSessCoapRecvBase64Encode, "base64", NULL);

    return IOTC_OK;
}

static bool LocalCoapRetransCheckFunc(const CoapRetransParam *param, const CoapData *raw,
    void *userData, uint32_t *next)
{
    CHECK_RETURN_LOGW(param != NULL && next != NULL, false, "param invalid");
    NOT_USED(param);
    NOT_USED(raw);
    NOT_USED(userData);

    if (param->cnt >= LAN_SEARCH_COAP_RETRANS_CNT) {
        return false;
    }
    *next = LAN_SEARCH_COAP_RETRANS_INTERVAL;
    return true;
}

static int32_t CreateLanSearchCoapEndpoint(LanSearchContext *ctx)
{
    ctx->coapServer.endpoint = CoapEndpointNew(ctx->coapServer.sendBuf, ctx->coapServer.sess,
        CoapUdpEncode, CoapUdpDecode, ctx);
    if (ctx->coapServer.endpoint == NULL) {
        IOTC_LOGW("create coap endpoint error");
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_ENDPOINT_CREATE;
    }

    static const CoapResource LAN_SEARCH_COAP_RES[] = {
        {UTILS_BIT(COAP_METHOD_TYPE_GET), STR_URI_LAN_SEARCH, NULL, LanSearchCoapSearchHandler},
        {UTILS_BIT(COAP_METHOD_TYPE_POST), STR_URI_SPKEK_V2, NULL, LanSearchCoapSpekeHandler},
        {UTILS_BIT(COAP_METHOD_TYPE_POST), STR_URI_CLOUD_SETUP_V2, NULL,
            LanSearchCoapCloudSetupHandler},
    };

    int32_t ret = CoapServerAddResource(ctx->coapServer.endpoint,
        LAN_SEARCH_COAP_RES, ARRAY_SIZE(LAN_SEARCH_COAP_RES));
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

static int32_t LanSearchServerCreate(LanSearchContext *ctx)
{
    int32_t ret;
    do {
        /* 1. 初始化收发缓冲区 */
        ctx->coapServer.sendBuf = TransCreateSendBuffer();
        ctx->coapServer.recvBuf = TransCreateRecvBuffer();
        if (ctx->coapServer.sendBuf == NULL || ctx->coapServer.recvBuf == NULL) {
            IOTC_LOGW("create buffer error");
            ret = IOTC_CORE_WIFI_LAN_SEARCH_ERR_CREATE_BUFFER;
            break;
        }

        /* 2. 创建udp链路，不会实际创建套接字 */
        ret = CreateLanSearchLink(ctx);
        if (ret != IOTC_OK) {
            break;
        }

        /* 3. 创建会话管理 */
        ret = CreateLanSearchSession(ctx);
        if (ret != IOTC_OK) {
            break;
        }

        /* 4. 创建coap端点 */
        ret = CreateLanSearchCoapEndpoint(ctx);
        if (ret != IOTC_OK) {
            break;
        }

        return IOTC_OK;
    } while (0);
    /* 异常处理 */
    LanSearchCoapServerDestroy(ctx);
    return ret;
}

static int32_t LocalCoapServerStartInner(LanSearchContext *ctx)
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

int32_t LanSearchCoapServerStart(LanSearchContext *ctx)
{
    CHECK_RETURN_LOGW(ctx != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    int32_t ret;

    if (UTILS_IS_BIT_SET(ctx->bitMap, LAN_SEARCH_CTX_COAP_SVR_STARTED)) {
        return IOTC_OK;
    }

    if (!UTILS_IS_BIT_SET(ctx->bitMap, LAN_SEARCH_CTX_COAP_SVR_CREATED)) {
        ret = LanSearchServerCreate(ctx);
        if (ret != IOTC_OK) {
            IOTC_LOGW("create coap server error %d", ret);
            return ret;
        }
        UTILS_BIT_SET(ctx->bitMap, LAN_SEARCH_CTX_COAP_SVR_CREATED);
    }

    ret = LocalCoapServerStartInner(ctx);
    if (ret != IOTC_OK) {
        IOTC_LOGW("start coap error %d", ret);
        return ret;
    }
    UTILS_BIT_SET(ctx->bitMap, LAN_SEARCH_CTX_COAP_SVR_STARTED);
    return IOTC_OK;
}

void LanSearchCoapServerStop(LanSearchContext *ctx)
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
    UTILS_BIT_RESET(ctx->bitMap, LAN_SEARCH_CTX_COAP_SVR_STARTED);
}

void LanSearchCoapServerDestroy(LanSearchContext *ctx)
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