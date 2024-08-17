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
#include <string.h>
#include "svc_softap_coap.h"
#include "svc_softap_ctx.h"
#include "utils_assert.h"
#include "utils_common.h"
#include "sched_timer.h"
#include "adapter_json.h"
#include "coap_codec_utils.h"
#include "utils_bit_map.h"
#include "utils_json.h"
#include "adapter_base64.h"
#include "adapter_os.h"
#include "adapter_os.h"
#include "svc_softap_sess.h"
#include "iotc_conf.h"
#include "svc_softap_report.h"
#include "event_bus.h"
#include "sched_executor.h"
#include "service_proxy.h"
#include "iotc_svc.h"
#include "iotc_svc_dev.h"
#include "iotc_svc_conn.h"
#include "iotc_errcode.h"
#include "e2e_ctl_msg.h"

#define E2E_CTRL_SEQ_WINDOWS 30
#define RETRANS_WAIT_TIME_MS 500

void SoftapCoapSpekeReqHandler(CoapEndpoint *endpoint, const CoapPacket *req, const SocketAddr *addr, void *userData)
{
    CHECK_V_RETURN_LOGW(endpoint != NULL && req != NULL && addr != NULL && userData != NULL, "invalid param");

    SoftapSess *sess = (SoftapSess *)userData;
    SoftapPeerSess *peerSess = SoftapGetPeerSess(addr, sess);
    if (peerSess == NULL) {
        IOTC_LOGW("no peer");
        return;
    }

    int32_t ret =  SoftapPeerSessInitSpeke(peerSess);
    if (ret != IOTC_OK) {
        IOTC_LOGW("speke init error %d", ret);
        return;
    }

    uint8_t *respMsg = NULL;
    uint32_t respLen = 0;
    ret = SpekeProcessPacket(peerSess->speke, (const char *)req->payload.data, &respMsg, &respLen);
    if (ret != IOTC_OK || respMsg == NULL || respLen == 0) {
        IOTC_LOGW("speke process packet error %d/%u", ret, respLen);
        if (respMsg != NULL) {
            AdapterFree(respMsg);
        }
        return;
    }
    /* SPEKE协商及其重协商报文，需要使用明文回复 */
    UTILS_BIT_SET(peerSess->sendBitMap, SOFTAP_PEER_MSG_BIT_MAP_PLAIN);
    UTILS_BIT_SET(peerSess->sendBitMap, SOFTAP_PEER_MSG_BIT_MAP_NO_BASE64);

    CoapPacket packet;
    CoapData payload = {respMsg, respLen};
    CoapServerRespParam respParam = {
        .req = req,
        .type = COAP_MSG_TYPE_NCON,
        .code = COAP_RESPONSE_CODE_CONTENT,
        .opNum = 0,
        .options = NULL,
        .payload = &payload,
        .payloadBuilder = NULL,
        .payloadUserData = NULL,
        .preSize = 0,
    };
    ret = CoapServerSendResp(endpoint, &respParam, addr, &packet);
    UTILS_FREE_2_NULL(respMsg);
    if (ret != IOTC_OK) {
        IOTC_LOGW("send coap resp msg error %d", ret);
    }
}

static int32_t NetCfgInfoRecvProcess(const char *netInfo, uint32_t len)
{
    AdapterJson *jsonObj = AdapterJsonParseWithLen(netInfo, len);
    if (jsonObj == NULL) {
        IOTC_LOGW("parse json error");
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    AdapterJson *dataObj = AdapterJsonGetObj(jsonObj, STR_JSON_DATA);
    if (dataObj == NULL) {
        AdapterJsonDelete(jsonObj);
        IOTC_LOGW("get json data error");
        return IOTC_ADAPTER_JSON_ERR_GET_OBJ;
    }

#if IOTC_CONF_AILIFE_SUPPORT
    int32_t ret = DevSvcProxyRecvBindInfo(dataObj);
    if (ret != IOTC_OK) {
        AdapterJsonDelete(jsonObj);
        IOTC_LOGW("bind info process error %d", ret);
        return ret;
    }
#else
    /* TODO 端云未就绪，当前仅有绑定信息 */
    int32_t ret = DevSvcProxyRecvAuthInfo(dataObj);
    if (ret != IOTC_OK) {
        AdapterJsonDelete(jsonObj);
        IOTC_LOGW("auth info process error %d", ret);
        return ret;
    }
#endif

    ret = ConnSvcProxySetNetCfgInfo(dataObj);
    AdapterJsonDelete(jsonObj);
    if (ret != IOTC_OK) {
        IOTC_LOGW("net info process error %d", ret);
        return ret;
    }

    return IOTC_OK;
}

static void CloseSoftapService(int32_t id, void *userData)
{
    NOT_USED(id);
    NOT_USED(userData);
    int32_t ret = ServiceProxyStopService(IOTC_SERVICE_ID_SOFTAP, NULL);
    if (ret != IOTC_OK) {
        IOTC_LOGW("stop softap service error %d", ret);
    }
}

void SoftapCoapSetupReqHandler(CoapEndpoint *endpoint, const CoapPacket *req, const SocketAddr *addr, void *userData)
{
    NOT_USED(userData);
    CHECK_V_RETURN_LOGW(endpoint != NULL && req != NULL && addr != NULL, "invalid param");
    SoftapServiceContext *ctx = GetSoftapServiceContext();
    if (ctx == NULL || UTILS_IS_BIT_SET(ctx->bitMap, SOFTAP_CTX_BIT_MAP_NETCFG_RECVED)) {
        IOTC_LOGW("invalid ctx");
        return;
    }

    bool isNetCfgFail = false;
    int32_t ret = NetCfgInfoRecvProcess((const char *)req->payload.data, req->payload.len);
    if (ret != IOTC_OK) {
        IOTC_LOGE("net cfg info process error %d", ret);
    } else {
        IOTC_LOGN("net cfg info process ok");
        UTILS_BIT_SET(ctx->bitMap, SOFTAP_CTX_BIT_MAP_NETCFG_RECVED);
        isNetCfgFail = true;
    }

    AdapterJson *respJson = UtilsJsonCreateErrcode(ret);
    if (respJson == NULL) {
        IOTC_LOGW("create resp json error");
        return;
    }

    CoapServerRespParam respParam;
    ret = CoapServerBuildDefaultRespParam(&respParam, req, respJson);
    if (ret != IOTC_OK) {
        IOTC_LOGW("build resp param error %d", ret);
        AdapterJsonDelete(respJson);
        return;
    }

    /* 配网报文为softap链路的最后一条报文 */
    SoftapPeerSess *peer = SoftapGetPeerSess(addr, &ctx->sess);
    if (peer != NULL) {
        peer->lastMsgId = req->header.msgId;
    }
    CoapPacket packet;
    ret = CoapServerSendResp(endpoint, &respParam, addr, &packet);
    AdapterJsonDelete(respJson);
    respJson = NULL;
    if (ret != IOTC_OK) {
        IOTC_LOGW("send coap resp msg error %d", ret);
    }
    CHECK_V_RETURN(isNetCfgFail);
    if (ctx->stopTimerFd < 0) {
        ctx->stopTimerFd = SchedTimerAdd(EVENT_SOURCE_TIMER_TYPE_ONCE, CloseSoftapService, RETRANS_WAIT_TIME_MS, NULL);
        if (ctx->stopTimerFd < 0) {
            IOTC_LOGE("add timer error %d", ctx->stopTimerFd);
            CloseSoftapService(0, NULL);
        }
    }
}

static bool SessMsgProcessCheck(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info)
{
    CHECK_RETURN(msg != NULL && buf != NULL && buf->buffer != NULL && buf->len != 0 && buf->size >= buf->len &&
        info != NULL && info->addr != NULL && info->userData != NULL, false);

    CoapPacket *pkt = (CoapPacket *)msg;
    if (pkt->payload.len > buf->len) {
        IOTC_LOGW("invalid coap len %u/%u", pkt->payload.len, buf->len);
        return false;
    }
    return true;
}

static bool UriWhiteListMatch(const char **uriList, uint32_t uriListNum, const char *uri)
{
    if (uriList == NULL || uriListNum == 0) {
        return false;
    }
    for (uint32_t i = 0; i < uriListNum; ++i) {
        if (uriList[i] != NULL && strcmp(uriList[i], uri) == 0) {
            return true;
        }
    }
    return false;
}

SessCode SoftapCoapMsgRecvPreProcess(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info)
{
    if (!SessMsgProcessCheck(msg, buf, info)) {
        return SESS_CODE_ERR;
    }
    CoapPacket *pkt = (CoapPacket *)msg;
    SoftapSess *sess = (SoftapSess *)info->userData;
    SoftapPeerSess *peer = SoftapGetPeerSessCreateIfNotExist(info->addr, sess);
    if (peer == NULL) {
        IOTC_LOGW("no invalid peer sess");
        return SESS_CODE_ERR;
    }

    char uriBuf[COAP_URI_MAX_LEN + 1] = {0};
    int32_t ret = CoapUtilsGetUriPath(pkt, uriBuf, COAP_URI_MAX_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get uri error %d", ret);
        return SESS_CODE_ERR;
    }

    UTILS_BIT_CLR(peer->recvBitMap);
    if (UriWhiteListMatch(sess->plainUri, sess->plainUriNum, uriBuf)) {
        UTILS_BIT_SET(peer->recvBitMap, SOFTAP_PEER_MSG_BIT_MAP_PLAIN);
    }
    if (UriWhiteListMatch(sess->noBase64Uri, sess->noBase64Num, uriBuf)) {
        UTILS_BIT_SET(peer->recvBitMap, SOFTAP_PEER_MSG_BIT_MAP_NO_BASE64);
    }

    return SESS_CODE_CONTINUE;
}

SessCode SoftapCoapMsgSendEncryptProcess(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info)
{
    if (!SessMsgProcessCheck(msg, buf, info)) {
        return SESS_CODE_ERR;
    }
    CoapPacket *pkt = (CoapPacket *)msg;
    SoftapSess *sess = (SoftapSess *)info->userData;
    SoftapPeerSess *peer = SoftapGetPeerSess(info->addr, sess);
    if (peer == NULL) {
        IOTC_LOGW("no invalid peer sess");
        return SESS_CODE_ERR;
    }

    if (UTILS_IS_BIT_SET(peer->sendBitMap, SOFTAP_PEER_MSG_BIT_MAP_PLAIN)) {
        return SESS_CODE_CONTINUE;
    }
    /* 未协商出会话，或正在协商 */
    if (peer->speke == NULL ||
        !UTILS_IS_BIT_SET(peer->bitMap, SOFTAP_PEER_SESS_BIT_MAP_SPEKE_SESS_CREATED)) {
        IOTC_LOGW("no invalid speke sess");
        return SESS_CODE_ERR;
    }

    uint8_t *encData = NULL;
    uint32_t encLen = 0;
    /* 加密payload */
    int32_t ret = SpekeEncryptData(peer->speke, pkt->payload.data, pkt->payload.len, &encData, &encLen);
    if (ret != IOTC_OK || encData == NULL) {
        IOTC_LOGW("speke sess dec err %d", ret);
        return SESS_CODE_ERR;
    }

    CoapData payloadEnc = {encData, encLen};
    /* 密文替换掉原始coap报文中的明文 */
    ret = CoapUtilsReplacePayload(pkt, buf, &payloadEnc);
    AdapterFree(encData);
    if (ret != IOTC_OK) {
        IOTC_LOGW("coap replace enc payload error %d", ret);
        return SESS_CODE_ERR;
    }

    return SESS_CODE_CONTINUE;
}

SessCode SoftapCoapMsgSendFinalProcess(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info)
{
    if (!SessMsgProcessCheck(msg, buf, info)) {
        return SESS_CODE_ERR;
    }

    SoftapSess *sess = (SoftapSess *)info->userData;
    SoftapPeerSess *peer = SoftapGetPeerSess(info->addr, sess);
    if (peer == NULL) {
        IOTC_LOGW("no invalid peer sess");
        return SESS_CODE_ERR;
    }
    UTILS_BIT_CLR(peer->sendBitMap);
    return SESS_CODE_CONTINUE;
}

SessCode SoftapCoapMsgRecvDecryptProcess(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info)
{
    if (!SessMsgProcessCheck(msg, buf, info)) {
        return SESS_CODE_ERR;
    }
    CoapPacket *pkt = (CoapPacket *)msg;
    SoftapSess *sess = (SoftapSess *)info->userData;
    SoftapPeerSess *peer = SoftapGetPeerSess(info->addr, sess);
    if (peer == NULL) {
        IOTC_LOGW("no invalid peer sess");
        return SESS_CODE_ERR;
    }
    if (UTILS_IS_BIT_SET(peer->recvBitMap, SOFTAP_PEER_MSG_BIT_MAP_PLAIN)) {
        return SESS_CODE_CONTINUE;
    }
    /* 未协商出会话，或正在协商 */
    if (peer->speke == NULL ||
        !UTILS_IS_BIT_SET(peer->bitMap, SOFTAP_PEER_SESS_BIT_MAP_SPEKE_SESS_CREATED)) {
        IOTC_LOGW("no invalid speke sess");
        return SESS_CODE_ERR;
    }

    uint8_t *decData = NULL;
    uint32_t decLen = 0;
    /* 解密payload */
    int32_t ret = SpekeDecryptData(peer->speke, pkt->payload.data, pkt->payload.len, &decData, &decLen);
    if (ret != IOTC_OK || decData == NULL || decLen == 0) {
        IOTC_LOGW("speke sess dec err %d/%u", ret, decLen);
        return SESS_CODE_ERR;
    }

    CoapData payloadDec = {decData, decLen};
    /* 将解密后的明文替换掉原始coap报文中的密文 */
    ret = CoapUtilsReplacePayload(pkt, buf, &payloadDec);
    AdapterFree(decData);
    if (ret != IOTC_OK) {
        IOTC_LOGW("coap replace dec payload error %d", ret);
        return SESS_CODE_ERR;
    }
    return SESS_CODE_CONTINUE;
}

SessCode SoftapCoapMsgRecvBase64DecodeProcess(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info)
{
    if (!SessMsgProcessCheck(msg, buf, info)) {
        return SESS_CODE_ERR;
    }
    CoapPacket *pkt = (CoapPacket *)msg;
    SoftapSess *sess = (SoftapSess *)info->userData;
    SoftapPeerSess *peer = SoftapGetPeerSess(info->addr, sess);
    if (peer == NULL) {
        IOTC_LOGW("no invalid peer sess");
        return SESS_CODE_ERR;
    }
    if (UTILS_IS_BIT_SET(peer->recvBitMap, SOFTAP_PEER_MSG_BIT_MAP_NO_BASE64)) {
        return SESS_CODE_CONTINUE;
    }

    uint32_t len = 0;
    int32_t ret = IotcBase64Decode(pkt->payload.data, pkt->payload.len, NULL, &len);
    if (ret != IOTC_OK || len > pkt->payload.len) {
        IOTC_LOGW("get len error %d", ret);
        return SESS_CODE_ERR;
    }

    uint8_t *decodeData = (uint8_t *)AdapterCalloc(len, sizeof(uint8_t));
    if (decodeData == NULL) {
        IOTC_LOGW("calloc error %d", len);
        return SESS_CODE_ERR;
    }

    ret = IotcBase64Decode(pkt->payload.data, pkt->payload.len, decodeData, &len);
    if (ret != IOTC_OK) {
        IOTC_LOGW("base64 decode error %d", ret);
        AdapterFree(decodeData);
        return SESS_CODE_ERR;
    }

    CoapData payloadDecode = {decodeData, len};
    ret = CoapUtilsReplacePayload(pkt, buf, &payloadDecode);
    AdapterFree(decodeData);
    if (ret != IOTC_OK) {
        IOTC_LOGW("coap replace decode payload error %d", ret);
        return SESS_CODE_ERR;
    }
    return SESS_CODE_CONTINUE;
}

SessCode SoftapCoapMsgSendBase64EncodeProcess(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info)
{
    if (!SessMsgProcessCheck(msg, buf, info)) {
        return SESS_CODE_ERR;
    }
    CoapPacket *pkt = (CoapPacket *)msg;
    SoftapSess *sess = (SoftapSess *)info->userData;
    SoftapPeerSess *peer = SoftapGetPeerSess(info->addr, sess);
    if (peer == NULL) {
        IOTC_LOGW("no invalid peer sess");
        return SESS_CODE_ERR;
    }
    if (UTILS_IS_BIT_SET(peer->sendBitMap, SOFTAP_PEER_MSG_BIT_MAP_NO_BASE64)) {
        return SESS_CODE_CONTINUE;
    }

    uint32_t len = 0;
    int32_t ret = IotcBase64Encode(pkt->payload.data, pkt->payload.len, NULL, &len);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get len error %d", ret);
        return SESS_CODE_ERR;
    }

    uint8_t *encodeData = (uint8_t *)AdapterCalloc(len, sizeof(uint8_t));
    if (encodeData == NULL) {
        IOTC_LOGW("calloc error %d", len);
        return SESS_CODE_ERR;
    }

    ret = IotcBase64Encode(pkt->payload.data, pkt->payload.len, encodeData, &len);
    if (ret != IOTC_OK) {
        IOTC_LOGW("base64 encode error %d", ret);
        AdapterFree(encodeData);
        return SESS_CODE_ERR;
    }

    CoapData payloadDecode = {encodeData, len};
    ret = CoapUtilsReplacePayload(pkt, buf, &payloadDecode);
    AdapterFree(encodeData);
    if (ret != IOTC_OK) {
        IOTC_LOGW("coap replace encode payload error %d", ret);
        return SESS_CODE_ERR;
    }
    return SESS_CODE_CONTINUE;
}

static bool SoftCoapE2eCtrlSeqCheck(AdapterJson *payloadJsonObj, SoftapPeerSess *softapSpeke)
{
    uint32_t recvSeq;
    int32_t ret = UtilsJsonGetUint(payloadJsonObj, STR_JSON_SEQ, &recvSeq);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get seq error %d", ret);
        return false;
    }

    if (!UTILS_IS_BIT_SET(softapSpeke->bitMap, SOFTAP_PEER_SESS_BIT_MAP_E2E_SEQ_SET)) {
        softapSpeke->recvSeq = recvSeq;
        UTILS_BIT_SET(softapSpeke->bitMap, SOFTAP_PEER_SESS_BIT_MAP_E2E_SEQ_SET);
        return true;
    }
    uint32_t curSeq = softapSpeke->recvSeq;
    bool isValid = false;

    do {
        if (curSeq <= UINT32_MAX - E2E_CTRL_SEQ_WINDOWS) {
            if (recvSeq > curSeq && recvSeq - curSeq <= E2E_CTRL_SEQ_WINDOWS) {
                isValid = true;
            } else {
                isValid = false;
            }
        } else {
            if (recvSeq > curSeq) {
                isValid = true;
            } else if (recvSeq <= E2E_CTRL_SEQ_WINDOWS && E2E_CTRL_SEQ_WINDOWS - recvSeq < (UINT32_MAX - curSeq + 1)) {
                isValid = true;
            } else {
                isValid = false;
            }
        }
    } while (0);
    if (!isValid) {
        IOTC_LOGW("recv invalid seq %u cur %u", recvSeq, curSeq);
        return false;
    }
    softapSpeke->recvSeq = recvSeq;
    return true;
}

static void SoftapCtrlMsgReportAfterGetCmd(const AdapterJson *dataArray, const void *userData, uint32_t userDataLen)
{
    CHECK_V_RETURN_LOGW(dataArray != NULL && userData != NULL && userDataLen == sizeof(uint32_t), "param invalid");

    int32_t ret = SoftapServiceReportToTargetPeer(dataArray, *(const uint32_t *)userData);
    if (ret != IOTC_OK) {
        IOTC_LOGW("report to peer error %d", ret);
    }
    return;
}

void SoftapCoapE2eCtrlHandler(CoapEndpoint *endpoint, const CoapPacket *req, const SocketAddr *addr, void *userData)
{
    CHECK_V_RETURN_LOGW(endpoint != NULL && req != NULL && addr != NULL &&
        req->payload.data != NULL && req->payload.len != 0, "invalid param");
    SoftapSess *sess = (SoftapSess *)userData;
    SoftapPeerSess *peer = SoftapGetPeerSess(addr, sess);
    if (peer == NULL) {
        IOTC_LOGW("no valid peer sess");
        return;
    }

    AdapterJson *payloadJsonObj = AdapterJsonParseWithLen((const char *)req->payload.data, req->payload.len);
    if (payloadJsonObj == NULL) {
        IOTC_LOGW("invalid json");
        return;
    }

    if (!SoftCoapE2eCtrlSeqCheck(payloadJsonObj, peer)) {
        AdapterJsonDelete(payloadJsonObj);
        return;
    }

    int32_t ret = E2eCtrlMsgProcess(payloadJsonObj, SoftapCtrlMsgReportAfterGetCmd, &addr->addr, sizeof(addr->addr));
    AdapterJsonDelete(payloadJsonObj);

    AdapterJson *respJson = UtilsJsonCreateErrcode(ret);
    if (respJson == NULL) {
        IOTC_LOGW("create resp json error %d", ret);
        return;
    }

    CoapServerRespParam respParam;
    ret = CoapServerBuildDefaultRespParam(&respParam, req, respJson);
    if (ret != IOTC_OK) {
        IOTC_LOGW("build resp param error %d", ret);
        AdapterJsonDelete(respJson);
        return;
    }

    CoapPacket packet;
    ret = CoapServerSendResp(endpoint, &respParam, addr, &packet);
    AdapterJsonDelete(respJson);
    if (ret != IOTC_OK) {
        IOTC_LOGW("send e2e ctrl resp msg error %d", ret);
    }
}