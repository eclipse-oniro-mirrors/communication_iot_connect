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
#include "coap_endpoint_priv.h"
#include "utils_assert.h"
#include "iotc_mem.h"
#include "securec.h"
#include "coap_codec_utils.h"
#include "coap_codec_udp.h"
#include "iotc_errcode.h"

#define CSM_MSG_MAX_LEN 7

static int32_t CoapEndpointRecvPacket(CoapEndpoint *endpoint, const CoapPacket *pkt, const SocketAddr *addr);

static SessCode SessMsgProcessCoapDecode(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info)
{
    CHECK_RETURN_LOGW(msg != NULL && buf != NULL && buf->buffer != NULL && buf->len != 0 && info != NULL &&
        info->nodeData != NULL, SESS_CODE_ERR, "param invalid");

    CoapData data = { buf->buffer, buf->len };
    CoapEndpointPacketDecode decodeFunc = (CoapEndpointPacketDecode)info->nodeData;
    int32_t ret = decodeFunc(msg, &data);
    if (ret != IOTC_OK) {
        IOTC_LOGW("decode error %d", ret);
        return SESS_CODE_ERR;
    }

    return SESS_CODE_CONTINUE;
}

static SessCode SessMsgProcessCoapEndpoint(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info)
{
    CHECK_RETURN_LOGW(msg != NULL && buf != NULL && buf->buffer != NULL && buf->len != 0 && buf->size >= buf->len &&
        info != NULL && info->nodeData != NULL, false, "param invalid");
    CoapEndpoint *endpoint = (CoapEndpoint *)info->nodeData;
    CoapPacket *pkt = (CoapPacket *)msg;
    int32_t ret = CoapEndpointRecvPacket(endpoint, pkt, info->addr);
    if (ret != IOTC_OK) {
        IOTC_LOGW("coap endpoint recv error %d", ret);
        return SESS_CODE_ERR;
    }
    return SESS_CODE_CONTINUE;
}

void CoapEndpointSessSetup(CoapEndpoint *endpoint)
{
    CHECK_V_RETURN_LOGW(endpoint != NULL && endpoint->sess != NULL && endpoint->decoder != NULL, "param invalid");

    /* coap sess recv msg process : first. code decode -> xxx -> last. coap endpoint dispatch */
    TransSessAddRecvHeadHandler(endpoint->sess, SessMsgProcessCoapDecode, "coap_decode", endpoint->decoder);
    TransSessAddRecvTailHandler(endpoint->sess, SessMsgProcessCoapEndpoint, "coap_endpoint", endpoint);
}

CoapEndpoint *CoapEndpointNew(UtilsBufferCtx *buf, TransSess *sess,
    CoapEndpointPacketEncode encoder, CoapEndpointPacketDecode decoder, void *userData)
{
    CHECK_RETURN_LOGW(buf != NULL && encoder != NULL && sess != NULL, NULL, "param invalid");

    CoapEndpoint *endpoint = (CoapEndpoint *)IotcMalloc(sizeof(CoapEndpoint));
    if (endpoint == NULL) {
        IOTC_LOGW("malloc error");
        return NULL;
    }
    (void)memset_s(endpoint, sizeof(CoapEndpoint), 0, sizeof(CoapEndpoint));

    do {
        endpoint->sess = sess;
        endpoint->userData = userData;
        endpoint->encoder = encoder;
        endpoint->decoder = decoder;
        endpoint->sendBuf = buf;

        endpoint->mutex = UtilsCreateExMutex();
        if (endpoint->mutex == NULL) {
            IOTC_LOGW("mutex create error");
            break;
        }

        int32_t ret = CoapEndpointClientInit(&endpoint->client);
        if (ret != IOTC_OK) {
            IOTC_LOGW("client init error %d", ret);
            break;
        }

        ret = CoapEndpointServerInit(&endpoint->server);
        if (ret != IOTC_OK) {
            IOTC_LOGW("server init error %d", ret);
            break;
        }

        ret = CoapEndpointRetransInit(&endpoint->retrans);
        if (ret != IOTC_OK) {
            IOTC_LOGW("retrans init error %d", ret);
            break;
        }
        return endpoint;
    } while (0);

    CoapEndpointFree(endpoint);
    return NULL;
}

void CoapEndpointFree(CoapEndpoint *endpoint)
{
    if (endpoint == NULL) {
        return;
    }

    TransSessRemoveHandler(endpoint->sess, SessMsgProcessCoapDecode);
    TransSessRemoveHandler(endpoint->sess, SessMsgProcessCoapEndpoint);
    CoapEndpointServerDeinit(&endpoint->server);
    CoapEndpointClientDeinit(&endpoint->client);
    CoapEndpointRetransDeinit(&endpoint->retrans);
    if (endpoint->mutex != NULL) {
        UtilsDestroyExMutex(&endpoint->mutex);
    }
    IotcFree(endpoint);
}

static int32_t CoapEndpointRecvPacket(CoapEndpoint *endpoint, const CoapPacket *pkt, const SocketAddr *addr)
{
    CHECK_RETURN(endpoint != NULL && pkt != NULL, IOTC_ERR_PARAM_INVALID);

    CoapUtilsDumpPacket(pkt);
    if (COAP_CODE_CLASS(pkt->header.code) == COAP_CODE_CLASS_REQ) {
        if (CoapEndpointRecvPacketRetransProcess(endpoint, pkt, addr)) {
            return IOTC_OK;
        }
        return CoapEndpointServerRecvReqPacket(endpoint, pkt, addr);
    } else {
        return CoapEndpointClientRecvRespPacket(endpoint, pkt, addr);
    }
}

int32_t CoapEndpointSendPacket(CoapEndpoint *endpoint, const CoapBuildPacket *buildPkt,
    CoapPacket *pkt, uint32_t preSize, const SocketAddr *addr)
{
    CHECK_RETURN(endpoint != NULL && buildPkt != NULL && pkt != NULL, IOTC_ERR_PARAM_INVALID);
    UtilsBuffer *buf = UtilsGetBuffer(endpoint->sendBuf);
    if (buf == NULL) {
        return IOTC_CORE_COMM_UTILS_ERR_BUFFER_GET;
    }
    int32_t ret = IOTC_OK;

    do {
        if (preSize > buf->size) {
            /* 预估大小超过buffer则扩充buffer */
            ret = UtilsExpendBuffer(endpoint->sendBuf, buf, preSize);
            if (ret != IOTC_OK) {
                IOTC_LOGW("buffer expend error %d/%u", ret, preSize);
                break;
            }
        }

        /* coap编码 */
        ret = endpoint->encoder(buildPkt, pkt, buf);
        if (ret != IOTC_OK) {
            IOTC_LOGW("coap encode error %d", ret);
            break;
        }

        CoapUtilsDumpPacket(pkt);

        /* 报文发送 */
        ret = TransSessMsgSend(endpoint->sess, pkt, buf, addr);
        if (ret != IOTC_OK) {
            IOTC_LOGW("coap send error %d", ret);
            break;
        }

        (void)CoapRetransAddPacket(endpoint, pkt, buf, addr);
    } while (0);

    UtilsReleaseBuffer(endpoint->sendBuf, &buf);
    return ret;
}

int32_t CoapEndpointSendCSM(CoapEndpoint *endpoint,
    CoapPacket *pkt, const SocketAddr *addr)
{
    CHECK_RETURN(endpoint != NULL && pkt != NULL, IOTC_ERR_PARAM_INVALID);
    UtilsBuffer *buf = UtilsGetBuffer(endpoint->sendBuf);
    if (buf == NULL) {
        return IOTC_CORE_COMM_UTILS_ERR_BUFFER_GET;
    }
    int32_t ret = IOTC_OK;
    uint8_t csmMsg[CSM_MSG_MAX_LEN] = {0x50, 0xe1, 0x23, 0x80, 0x01, 0x00, 0x20};

    do {
        buf->len = CSM_MSG_MAX_LEN;
        memcpy_s((void *)buf->buffer, buf->len, csmMsg, buf->len);
        buf->buffer[buf->len] = '\0';
        CoapUtilsDumpPacket(pkt);
        /* 报文发送 */
        ret = TransSessMsgSend(endpoint->sess, pkt, buf, addr);
        if (ret != IOTC_OK) {
            IOTC_LOGW("coap send error %d", ret);
            break;
        }
        (void)CoapRetransAddPacket(endpoint, pkt, buf, addr);
    } while (0);

    UtilsReleaseBuffer(endpoint->sendBuf, &buf);
    return ret;
}