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
#include "coap_net_stack.h"
#include "securec.h"
#include "utils_assert.h"
#include "iotc_errcode.h"
#include "trans_buffer_inner.h"
#include "wifi_sched_fd_watch.h"
#include "sched_event_loop.h"

static int32_t CreateCoapNetStackLink(CoapNetStack *stack, const CoapNetStackParam *initParam)
{
    stack->link = TransLinkNew(initParam->socket, stack->recvBuf, initParam->name);
    if (stack->link == NULL) {
        IOTC_LOGW("create link error");
        return IOTC_CORE_WIFI_TRANS_ERR_LINK_CREATE;
    }

    return IOTC_OK;
}

static int32_t CreateCoapNetStackSession(CoapNetStack *stack, const CoapNetStackParam *initParam)
{
    stack->sess = TransSessNew(stack->link, initParam->sessMsgSize, initParam->name, initParam->sessUserData);
    if (stack->sess == NULL) {
        IOTC_LOGW("create session error");
        return IOTC_CORE_WIFI_TRANS_ERR_SESS_CREATE;
    }

    return IOTC_OK;
}

static int32_t CreateCoapUdpSrvEndpoint(CoapNetStack *stack, const CoapNetStackParam *initParam)
{
    stack->endpoint = CoapEndpointNew(stack->sendBuf, stack->sess,
        initParam->encoder, initParam->decoder, initParam->endpointUserData);
    if (stack->endpoint == NULL) {
        IOTC_LOGW("create coap endpoint error");
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_ENDPOINT_CREATE;
    }

    return IOTC_OK;
}

int32_t CoapNetStackCreate(CoapNetStack *stack, const CoapNetStackParam *initParam)
{
    CHECK_RETURN_LOGW(stack != NULL && initParam != NULL && initParam->socket != NULL &&
        initParam->encoder != NULL && initParam->decoder != NULL,
        IOTC_ERR_PARAM_INVALID, "param invalid");
    CoapNetStackDestroy(stack);

    int32_t ret;
    do {
        /* 1. 初始化收发缓冲区 */
        stack->sendBuf = TransCreateSendBuffer();
        stack->recvBuf = TransCreateRecvBuffer();
        if (stack->sendBuf == NULL || stack->recvBuf == NULL) {
            IOTC_LOGW("create buffer error");
            ret = IOTC_CORE_WIFI_TRANS_BUFFER_ERR_CREATE;
            break;
        }

        /* 2. 创建udp链路，不会实际创建套接字 */
        ret = CreateCoapNetStackLink(stack, initParam);
        if (ret != IOTC_OK) {
            break;
        }

        /* 3. 创建会话管理 */
        ret = CreateCoapNetStackSession(stack, initParam);
        if (ret != IOTC_OK) {
            break;
        }

        /* 4. 创建coap端点 */
        ret = CreateCoapUdpSrvEndpoint(stack, initParam);
        if (ret != IOTC_OK) {
            break;
        }

        return IOTC_OK;
    } while (0);
    /* 异常处理 */
    CoapNetStackDestroy(stack);
    return ret;
}

int32_t CoapNetStackStart(CoapNetStack *stack)
{
    CHECK_RETURN_LOGW(stack != NULL && stack->link != NULL && stack->endpoint != NULL,
        IOTC_ERR_PARAM_INVALID, "param invalid");

    int32_t ret;
    do {
        ret = TransLinkConnect(stack->link);
        if (ret != IOTC_OK) {
            IOTC_LOGW("link connect error %d", ret);
            break;
        }

        ret = WifiSchedLinkRecvWatch(stack->link);
        if (ret != IOTC_OK) {
            IOTC_LOGW("link watch error %d", ret);
            break;
        }

        if (stack->source == NULL) {
            stack->source = CoapEndpointEventSourceNew(stack->endpoint);
            if (stack->source == NULL) {
                IOTC_LOGW("create coap source error");
                ret = IOTC_CORE_WIFI_TRANS_ERR_COAP_ENDPOINT_SOURCE_NEW;
                break;
            }
        }

        ret = EventLoopAddSource(GetSchedEventLoop(), stack->source);
        if (ret != IOTC_OK) {
            IOTC_LOGW("add coap source error %d", ret);
            break;
        }

        return IOTC_OK;
    } while (0);
    /* 异常处理 */
    CoapNetStackStop(stack);
    return ret;
}

void CoapNetStackStop(CoapNetStack *stack)
{
    CHECK_V_RETURN_LOGW(stack != NULL, "param invalid");
    if (stack->source != NULL) {
        EventLoopDelSource(GetSchedEventLoop(), stack->source);
        stack->source = NULL;
    }
    if (stack->link != NULL) {
        int32_t fd = TransLinkGetFd(stack->link);
        if (fd >= 0) {
            WifiSchedFdRemove(fd);
            TransLinkClose(stack->link);
        }
    }
}

void CoapNetStackDestroy(CoapNetStack *stack)
{
    CHECK_V_RETURN_LOGW(stack != NULL, "param invalid");

    CoapNetStackStop(stack);
    if (stack->endpoint != NULL) {
        CoapEndpointFree(stack->endpoint);
    }
    if (stack->sess != NULL) {
        TransSessFree(stack->sess);
    }
    if (stack->link != NULL) {
        TransLinkFree(stack->link);
    }
    if (stack->recvBuf != NULL) {
        TransReleaseBuffer(stack->recvBuf);
    }
    if (stack->sendBuf != NULL) {
        TransReleaseBuffer(stack->sendBuf);
    }
    (void)memset_s(stack, sizeof(CoapNetStack), 0, sizeof(CoapNetStack));
}