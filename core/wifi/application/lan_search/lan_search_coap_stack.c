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

#include "lan_search_coap_stack.h"
#include "lan_search_ctx.h"
#include "utils_bit_map.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "iotc_network.h"
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

static int32_t LanSearchSessionSetup(LanSearchContext *ctx)
{
    /* 发现、协商不需要会话校验及加密, 以NULL结尾 */
    static const char *WHITE_LIST[] = { STR_URI_LAN_SEARCH, STR_URI_SPKEK_V2, NULL };

    /* 本地发现收包责任链：预处理=>base64解码=>speke解密 */
    TransSessAddTailRecvHandler(ctx->coapStack.sess, LanSearchSessCoapRecvPreProcess, "pre", WHITE_LIST);
    TransSessAddTailRecvHandler(ctx->coapStack.sess, LanSearchSessCoapRecvBase64Decode, "base64", NULL);
    TransSessAddTailRecvHandler(ctx->coapStack.sess, LanSearchSessCoapRecvDecrypt, "decrypt", NULL);

    /* 本地发现发包责任链：speke加密=>base64编码 */
    TransSessAddTailSendHandler(ctx->coapStack.sess, LanSearchSessCoapSendEncrypt, "encrypt", NULL);
    TransSessAddTailSendHandler(ctx->coapStack.sess, LanSearchSessCoapRecvBase64Encode, "base64", NULL);

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

    if (param->cnt >= LAN_SEARCH_COAP_RETRANS_CNT) {
        return false;
    }
    *next = LAN_SEARCH_COAP_RETRANS_INTERVAL;
    return true;
}

static int32_t LanSearchCoapEndpointSetup(LanSearchContext *ctx)
{
    static const CoapResource LAN_SEARCH_COAP_RES[] = {
        {UTILS_BIT(COAP_METHOD_TYPE_GET), STR_URI_LAN_SEARCH, NULL, LanSearchCoapSearchHandler},
        {UTILS_BIT(COAP_METHOD_TYPE_POST), STR_URI_SPKEK_V2, NULL, LanSearchCoapSpekeHandler},
        {UTILS_BIT(COAP_METHOD_TYPE_POST), STR_URI_CLOUD_SETUP_V2, NULL,
            LanSearchCoapCloudSetupHandler},
    };

    int32_t ret = CoapServerAddResource(ctx->coapStack.endpoint,
        LAN_SEARCH_COAP_RES, ARRAY_SIZE(LAN_SEARCH_COAP_RES));
    if (ret != IOTC_OK) {
        IOTC_LOGW("add coap res error %d", ret);
        return ret;
    }

    ret = CoapEndpointRetransEnable(ctx->coapStack.endpoint, LocalCoapRetransCheckFunc,
        TransGetSendBufferResSize());
    if (ret != IOTC_OK) {
        IOTC_LOGW("enable coap retrans error %d", ret);
        return ret;
    }

    return IOTC_OK;
}

static int32_t LanSearchCoapStackCreate(LanSearchContext *ctx)
{
    char local[IOTC_IP_STR_MAX_LEN + 1] = {0};
    int32_t ret = IotcGetLocalIp(local, IOTC_IP_STR_MAX_LEN);
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

    CoapNetStackParam stackParam = {
        .name = LAN_SEARCH_SESS_NAME,
        .socket = socket,
        .sessMsgSize = sizeof(LanSearchSessMsg),
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

static int32_t LanSearchCoapStackSetup(LanSearchContext *ctx)
{
    int32_t ret;
    do {
        ret = LanSearchCoapStackCreate(ctx);
        if (ret != IOTC_OK) {
            break;
        }

        ret = LanSearchSessionSetup(ctx);
        if (ret != IOTC_OK) {
            break;
        }

        ret = LanSearchCoapEndpointSetup(ctx);
        if (ret != IOTC_OK) {
            break;
        }

        return IOTC_OK;
    } while (0);
    /* 异常处理 */
    LanSearchCoapStackDestroy(ctx);
    return ret;
}

int32_t LanSearchCoapStackStart(LanSearchContext *ctx)
{
    CHECK_RETURN_LOGW(ctx != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    int32_t ret;

    if (UTILS_IS_BIT_SET(ctx->bitMap, LAN_SEARCH_CTX_COAP_SVR_STARTED)) {
        return IOTC_OK;
    }

    if (!UTILS_IS_BIT_SET(ctx->bitMap, LAN_SEARCH_CTX_COAP_SVR_CREATED)) {
        ret = LanSearchCoapStackSetup(ctx);
        if (ret != IOTC_OK) {
            IOTC_LOGW("create coap server error %d", ret);
            return ret;
        }
        UTILS_BIT_SET(ctx->bitMap, LAN_SEARCH_CTX_COAP_SVR_CREATED);
    }

    ret = CoapNetStackStart(&ctx->coapStack);
    if (ret != IOTC_OK) {
        IOTC_LOGW("coap net stack start error %d", ret);
        return ret;
    }
    UTILS_BIT_SET(ctx->bitMap, LAN_SEARCH_CTX_COAP_SVR_STARTED);
    return IOTC_OK;
}

void LanSearchCoapStackStop(LanSearchContext *ctx)
{
    CHECK_V_RETURN_LOGW(ctx != NULL, "param invalid");

    CoapNetStackStop(&ctx->coapStack);
    UTILS_BIT_RESET(ctx->bitMap, LAN_SEARCH_CTX_COAP_SVR_STARTED);
}

void LanSearchCoapStackDestroy(LanSearchContext *ctx)
{
    CHECK_V_RETURN_LOGW(ctx != NULL, "param invalid");

    CoapNetStackDestroy(&ctx->coapStack);
    UTILS_BIT_RESET(ctx->bitMap, LAN_SEARCH_CTX_COAP_SVR_STARTED);
    UTILS_BIT_RESET(ctx->bitMap, LAN_SEARCH_CTX_COAP_SVR_CREATED);
}