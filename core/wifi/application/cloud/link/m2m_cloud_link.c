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
#include "m2m_cloud_link.h"
#include "adapter_mem.h"
#include "securec.h"
#include "utils_assert.h"
#include "trans_buffer_inner.h"
#include "m2m_cloud_sess.h"
#include "coap_codec_tcp_v1.h"
#include "utils_common.h"
#include "coap_endpoint_client.h"
#include "sched_timer.h"
#include "sched_event_loop.h"
#include "iotc_errcode.h"
#include "utils_bit_map.h"
#include "wifi_sched_fd_watch.h"

#define CLOUD_RESP_TIMEOUT_MS UTILS_SEC_TO_MS(30)

static int32_t CloudLinkBufferInit(M2mCloudContext *ctx)
{
    ctx->linkInfo.recvBuf = TransCreateRecvBuffer();
    if (ctx->linkInfo.recvBuf == NULL) {
        IOTC_LOGW("get recv buffer error");
        return IOTC_CORE_WIFI_COMM_ERR_GET_RECV_BUFFER;
    }
    ctx->linkInfo.sendBuf = TransCreateSendBuffer();
    if (ctx->linkInfo.sendBuf == NULL) {
        IOTC_LOGW("get send buffer error");
        return IOTC_CORE_WIFI_COMM_ERR_GET_SEND_BUFFER;
    }
    return IOTC_OK;
}

static int32_t CloudLinkUrlInit(M2mCloudContext *ctx)
{
    /* 主备共两个域名 */
    ctx->linkInfo.urlIndex = 0;
    const char *url = NULL;
    const char *backup = NULL;
    if (UTILS_IS_BIT_SET(ctx->bitMap, M2M_CLOUD_CTX_BIT_REGISTER)) {
        url = ctx->authInfo.regInfo.url;
        backup = ctx->authInfo.regInfo.backupUrl;
    } else {
        url = ctx->authInfo.loginInfo.url;
        backup = ctx->authInfo.loginInfo.backupUrl;
    }

    ctx->linkInfo.url[0] = UtilsStrDup(url);
    ctx->linkInfo.url[1] = UtilsStrDup(backup);

    if (ctx->linkInfo.url[0] == NULL || ctx->linkInfo.url[1] == NULL) {
        IOTC_LOGW("url str dup error");
        return IOTC_CORE_COMM_UTILS_ERR_STR_DUP;
    }

    return IOTC_OK;    
}

static int32_t CloudLinkCoapInit(M2mCloudContext *ctx)
{
    CHECK_RETURN(ctx != NULL, IOTC_ERR_PARAM_INVALID);

    ctx->linkInfo.endpoint = CoapEndpointNew(ctx->linkInfo.sendBuf, ctx->linkInfo.sess,
        CoapTcpV1Encode, CoapTcpV1Decode, ctx);
    if (ctx->linkInfo.endpoint == NULL) {
        IOTC_LOGW("create coap endpoint error");
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_ENDPOINT_CREATE;
    }

    int32_t ret = CoapClientSetRespTimeout(ctx->linkInfo.endpoint, CLOUD_RESP_TIMEOUT_MS);
    if (ret != IOTC_OK) {
        IOTC_LOGW("set timeout error %d", ret);
        return ret;
    }

    ret = CoapClientSetTokenLen(ctx->linkInfo.endpoint, CLOUD_TOKEN_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGW("set tkl error %d", ret);
        return ret;
    }

    return IOTC_OK;
}

int32_t M2mCloudLinkCreate(M2mCloudContext *ctx)
{
    CHECK_RETURN_LOGW(ctx != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    M2mCloudLinkDeinit(ctx);

    int32_t ret;
    do {
        ret = CloudLinkBufferInit(ctx);
        if (ret != IOTC_OK) {
            IOTC_LOGW("buffer init error %d", ret);
            break;
        }
        ret = CloudLinkUrlInit(ctx);
        if (ret != IOTC_OK) {
            IOTC_LOGW("url init error %d", ret);
            break;
        }
        ret = CloudLinkSessInit(ctx, CoapTcpV1GetRemainSize);
        if (ret != IOTC_OK) {
            IOTC_LOGW("sess init error %d", ret);
            break;
        }
        ret = CloudLinkCoapInit(ctx);
        if (ret != IOTC_OK) {
            IOTC_LOGW("coap init error %d", ret);
            break;
        }
        return IOTC_OK;
    } while (0);

    M2mCloudLinkDeinit(ctx);
    return ret;
}

void M2mCloudLinkDeinit(M2mCloudContext *ctx)
{
    CHECK_V_RETURN_LOGW(ctx != NULL, "param invalid");
    M2mCloudLinkClose(ctx);
    CloudLinkSessDeinit(ctx);
    if (ctx->linkInfo.recvBuf != NULL) {
        TransReleaseBuffer(ctx->linkInfo.recvBuf);
        ctx->linkInfo.recvBuf = NULL;
    }
    if (ctx->linkInfo.sendBuf != NULL) {
        TransReleaseBuffer(ctx->linkInfo.sendBuf);
        ctx->linkInfo.sendBuf = NULL;
    }

    for (uint32_t i = 0; i < ARRAY_SIZE(ctx->linkInfo.url); ++i) {
        UTILS_FREE_2_NULL(ctx->linkInfo.url[i]);
    }
    ctx->linkInfo.urlIndex = 0;

    if (ctx->linkInfo.endpoint != NULL) {
        CoapEndpointFree(ctx->linkInfo.endpoint);
        ctx->linkInfo.endpoint = NULL;
    }
}

int32_t M2mCloudLinkConnect(M2mCloudContext *ctx)
{
    M2mCloudLinkClose(ctx);
    int32_t ret;
    
    do {
        ret = TransLinkConnect(ctx->linkInfo.link);
        if (ret != IOTC_OK) {
            IOTC_LOGW("link connect error %d", ret);
            break;
        }

        ret = WifiSchedLinkRecvWatch(ctx->linkInfo.link);
        if (ret != IOTC_OK) {
            IOTC_LOGW("link watch error %d", ret);
            break;
        }

        ctx->linkInfo.coapSource = CoapEndpointEventSourceNew(ctx->linkInfo.endpoint);
        if (ctx->linkInfo.coapSource == NULL) {
            IOTC_LOGW("create coap source error");
            ret = IOTC_CORE_WIFI_TRANS_ERR_COAP_ENDPOINT_SOURCE_NEW;
            break;
        }

        ret = EventLoopAddSource(GetSchedEventLoop(), ctx->linkInfo.coapSource);
        if (ret != IOTC_OK) {
            IOTC_LOGW("add coap source error %d", ret);
            break;
        }
        return IOTC_OK;
    } while (0);
    M2mCloudLinkClose(ctx);
    return ret;
}

void M2mCloudLinkClose(M2mCloudContext *ctx)
{
    if (ctx->linkInfo.coapSource != NULL) {
        EventLoopDelSource(GetSchedEventLoop(), ctx->linkInfo.coapSource);
        ctx->linkInfo.coapSource = NULL;
    }

    if (ctx->linkInfo.link != NULL && TransLinkGetFd(ctx->linkInfo.link) >= 0) {
        WifiSchedFdRemove(TransLinkGetFd(ctx->linkInfo.link));
        TransLinkClose(ctx->linkInfo.link);
    }
}