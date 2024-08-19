
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

static int32_t LocalCtlCoapStackCreate(LocalControlContext *ctx)
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
        IOTC_LOGW("stack create error %d", ret);
        TransSocketFree(socket);
        return ret;
    }

    return IOTC_OK;
}

static int32_t LocalCtlSessionSetup(LocalControlContext *ctx)
{
    /* 发现、协商不需要会话校验及加密，以NULL为结束符 */
    static const char *WHITE_LIST[] = {
        STR_URI_LOCAL_CONTROL_SEARCH,
        STR_URI_LOCAL_CONTROL_SESS_MNGR,
        NULL,
    };

    /* 本地控收包会话处理责任链：预处理=>base64_decode=>payload解密 */
    TransSessAddTailRecvHandler(ctx->coapStack.sess, LocalCtlSessCoapRecvPreProcess, "pre", WHITE_LIST);
    TransSessAddTailRecvHandler(ctx->coapStack.sess, LocalCtlSessCoapRecvBase64Decode, "base64_decode", NULL);
    TransSessAddTailRecvHandler(ctx->coapStack.sess, LocalCtlSessCoapRecvDecrypt, "decrypt", NULL);

    /* 本地控发包责任链：预处理=>payload加密=>base64_encode */
    TransSessAddTailSendHandler(ctx->coapStack.sess, LocalCtlSessCoapSendEncrypt, "encrypt", NULL);
    TransSessAddTailSendHandler(ctx->coapStack.sess, LocalCtlSessCoapSendBase64Encode, "base64_encode", NULL);

    CoapEndpointSessSetup(ctx->coapStack.endpoint);
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

static int32_t LocalCtlCoapEndpointSetup(LocalControlContext *ctx)
{
    static const CoapResource LOCAL_CTL_COAP_RES[] = {
        {UTILS_BIT(COAP_METHOD_TYPE_GET), STR_URI_LOCAL_CONTROL_SEARCH, NULL,
            LocalCtlCoapSearchHandler},
        {UTILS_BIT(COAP_METHOD_TYPE_POST), STR_URI_LOCAL_CONTROL_SESS_MNGR, NULL,
            LocalCtlCoapSessMngrHandler},
        {UTILS_BIT(COAP_METHOD_TYPE_POST), STR_E2E_CONTROL, NULL,
            LocalCtlCoapControlHandler},
    };

    int32_t ret = CoapServerAddResource(ctx->coapStack.endpoint, LOCAL_CTL_COAP_RES, ARRAY_SIZE(LOCAL_CTL_COAP_RES));
    if (ret != IOTC_OK) {
        IOTC_LOGW("add coap res error %d", ret);
        return ret;
    }

    /* 盲发重传 */
    ret = CoapEndpointRetransEnable(ctx->coapStack.endpoint, LocalCoapRetransCheckFunc, TransGetSendBufferResSize());
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