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

#include "local_ctl_coap_stack.h"
#include "local_ctl_ctx.h"
#include "utils_bit_map.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "iotc_network.h"
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
#define LOCAL_CTL__COAP_UDP_MULTI_ADDR "238.101.0.0"
#define LOCAL_CTL_COAP_UDP_BROADCAST_ADDR "0.0.0.0"

static int32_t LocalCtlCoapStackCreate(LocalControlContext *ctx)
{
    char local[IOTC_IP_STR_MAX_LEN + 1] = {0};
    int32_t ret = IotcGetLocalIp(local, IOTC_IP_STR_MAX_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get local ip error %d", ret);
        return ret;
    }

    IOTC_LOGI("get local ip  %s", local);
    SocketUdpInitParam udp = {
        .port = LOCAL_CONTROL_PORT,
        .localAddr = local,
        .multicastAddr = LOCAL_CTL_COAP_UDP_MULTI_ADDR,
        .broadcastAddr = LOCAL_CTL_COAP_UDP_BROADCAST_ADDR,
    };

    TransSocket *socket = TransSocketUdpNew(&udp);
    if (socket == NULL) {
        IOTC_LOGW("create local ctl socket error");
        return IOTC_CORE_WIFI_TRANS_ERR_SOCKET_UDP_CREATE;
    }

    CoapNetStackParam stackParam = {
        .name = LOCAL_CTL_SESS_NAME,
        .socket = socket,
        .sessMsgSize = sizeof(LocalCoapSessMsg),
        .sessUserData = ctx,
        .encoder = CoapUdpEncode,
        .decoder = CoapUdpDecode,
        .endpointUserData = ctx,
    };
    ret = CoapNetStackCreate(&ctx->coapStack, &stackParam);
    if (ret != IOTC_OK) {
        IOTC_LOGW("local ctl coap stack create error %d", ret);
        TransSocketFree(socket);
        return ret;
    }

    return IOTC_OK;
}

static int32_t LocalCtlSessionSetup(LocalControlContext *ctx)
{
    /* 发现、协商不需要会话校验及加密，以NULL为结束符 */
    static const char *whiteList[] = {
        STR_URI_LOCAL_CONTROL_SEARCH,
        STR_URI_LOCAL_CONTROL_SESS_MNGR,
        NULL,
    };

    /* 本地控收包会话处理责任链：预处理=>base64_decode=>payload解密 */
    TransSessAddRecvTailHandler(ctx->coapStack.sess, LocalCtlSessCoapRecvPreProcess, "pre", whiteList);
    TransSessAddRecvTailHandler(ctx->coapStack.sess, LocalCtlSessCoapRecvBase64Decode, "base64_decode", NULL);
    TransSessAddRecvTailHandler(ctx->coapStack.sess, LocalCtlSessCoapRecvDecrypt, "decrypt", NULL);

    /* 本地控发包责任链：预处理=>payload加密=>base64_encode */
    TransSessAddSendTailHandler(ctx->coapStack.sess, LocalCtlSessCoapSendEncrypt, "encrypt", NULL);
    TransSessAddSendTailHandler(ctx->coapStack.sess, LocalCtlSessCoapSendBase64Encode, "base64_encode", NULL);

    CoapEndpointSessSetup(ctx->coapStack.endpoint);
    return IOTC_OK;
}

static int32_t LocalCtlCoapEndpointSetup(LocalControlContext *ctx)
{
    static const CoapResource localCtlCoapRes[] = {
        { UTILS_BIT(COAP_METHOD_TYPE_GET), STR_URI_LOCAL_CONTROL_SEARCH, NULL,
            LocalCtlCoapSearchHandler },
        { UTILS_BIT(COAP_METHOD_TYPE_GET), "switch", NULL,
            LocalCtlSvcGetCoapHandler },
        { UTILS_BIT(COAP_METHOD_TYPE_POST), "switch", NULL,
            LocalCtlSvcCoapHandler },
        { UTILS_BIT(COAP_METHOD_TYPE_GET), "restart", NULL,
            LocalCtlSvcGetCoapHandler },
        { UTILS_BIT(COAP_METHOD_TYPE_POST), "resatrt", NULL,
            LocalCtlSvcCoapHandler },
        { UTILS_BIT(COAP_METHOD_TYPE_GET), "snw", NULL,
            LocalCtlSvcGetCoapHandler },
        { UTILS_BIT(COAP_METHOD_TYPE_POST), "snw", NULL,
            LocalCtlSvcCoapHandler },
        { UTILS_BIT(COAP_METHOD_TYPE_GET), "gps", NULL,
            LocalCtlSvcGetCoapHandler },
        { UTILS_BIT(COAP_METHOD_TYPE_POST), "gps", NULL,
            LocalCtlSvcCoapHandler },
        { UTILS_BIT(COAP_METHOD_TYPE_GET), "devNetInfo", NULL,
            LocalCtlSvcGetCoapHandler },
        { UTILS_BIT(COAP_METHOD_TYPE_POST), "devNetInfo", NULL,
            LocalCtlSvcCoapHandler },
        { UTILS_BIT(COAP_METHOD_TYPE_POST), STR_URI_LOCAL_CONTROL_SESS_MNGR, NULL,
            LocalCtlCoapSessMngrHandler },
        { UTILS_BIT(COAP_METHOD_TYPE_POST), STR_E2E_CONTROL, NULL,
            LocalCtlCoapControlHandler },
    };

    int32_t ret = CoapServerAddResource(ctx->coapStack.endpoint, localCtlCoapRes, ARRAY_SIZE(localCtlCoapRes));
    if (ret != IOTC_OK) {
        IOTC_LOGW("add coap res error %d", ret);
        return ret;
    }

    /* 盲发重传 */
    ret = CoapEndpointRetransEnable(ctx->coapStack.endpoint, CoapEndpointRetransDefaultCheckFunc,
        TransGetBufferSize(TRANS_BUFFER_SEND_BUFFER_RES_SIZE));
    if (ret != IOTC_OK) {
        IOTC_LOGW("enable coap retrans error %d", ret);
        return ret;
    }

    return IOTC_OK;
}

static int32_t LocalCoapStackSetup(LocalControlContext *ctx)
{
    int32_t ret;
    do {
        ret = LocalCtlCoapStackCreate(ctx);
        if (ret != IOTC_OK) {
            break;
        }

        ret = LocalCtlSessionSetup(ctx);
        if (ret != IOTC_OK) {
            break;
        }

        ret = LocalCtlCoapEndpointSetup(ctx);
        if (ret != IOTC_OK) {
            break;
        }

        return IOTC_OK;
    } while (0);
    /* 异常处理 */
    LocalControlCoapStackDestroy(ctx);
    return ret;
}

int32_t LocalControlCoapStackStart(LocalControlContext *ctx)
{
    CHECK_RETURN_LOGW(ctx != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    int32_t ret;

    if (UTILS_IS_BIT_SET(ctx->bitMap, LOCAL_CTL_CTX_COAP_SVR_STARTED)) {
        return IOTC_OK;
    }

    if (!UTILS_IS_BIT_SET(ctx->bitMap, LOCAL_CTL_CTX_COAP_SVR_CREATED)) {
        ret = LocalCoapStackSetup(ctx);
        if (ret != IOTC_OK) {
            IOTC_LOGW("create coap server error %d", ret);
            return ret;
        }
        UTILS_BIT_SET(ctx->bitMap, LOCAL_CTL_CTX_COAP_SVR_CREATED);
    }

    ret = CoapNetStackStart(&ctx->coapStack);
    if (ret != IOTC_OK) {
        IOTC_LOGW("coap net stack start error %d", ret);
        return ret;
    }
    UTILS_BIT_SET(ctx->bitMap, LOCAL_CTL_CTX_COAP_SVR_STARTED);
    return IOTC_OK;
}

void LocalControlCoapStackStop(LocalControlContext *ctx)
{
    CHECK_V_RETURN_LOGW(ctx != NULL, "param invalid");

    CoapNetStackStop(&ctx->coapStack);
    UTILS_BIT_RESET(ctx->bitMap, LOCAL_CTL_CTX_COAP_SVR_STARTED);
}

void LocalControlCoapStackDestroy(LocalControlContext *ctx)
{
    CHECK_V_RETURN_LOGW(ctx != NULL, "param invalid");

    CoapNetStackDestroy(&ctx->coapStack);
    UTILS_BIT_RESET(ctx->bitMap, LOCAL_CTL_CTX_COAP_SVR_STARTED);
    UTILS_BIT_RESET(ctx->bitMap, LOCAL_CTL_CTX_COAP_SVR_CREATED);
}
