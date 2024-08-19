

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
#include "lan_search_sess.h"
#include "lan_search_ctx.h"
#include "lan_search_peer_mngr.h"
#include "iotc_errcode.h"
#include "utils_bit_map.h"
#include "utils_assert.h"
#include "adapter_base64.h"
#include "coap_codec_utils.h"

typedef enum {
    LAN_SEARCH_SPEKE_TYPE_DECRYPT = 0,
    LAN_SEARCH_SPEKE_TYPE_ENCRYPT,
} LanSearchSpekeType;

typedef enum {
    LAN_SEARCH_BASE64_TYPE_DECODE = 0,
    LAN_SEARCH_BASE64_TYPE_ENCODE,
} LanSearchBase64Type;

static bool LanSearchSessMsgCheck(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info)
{
    CHECK_RETURN(msg != NULL && buf != NULL && buf->buffer != NULL && buf->len != 0 && buf->size >= buf->len &&
        info != NULL && info->addr != NULL && info->userData != NULL, false);

    return true;
}

static bool LanSearchPlainCheck(const CoapPacket *packet, const char *uriWhiteList[])
{
    if (COAP_CODE_CLASS(packet->header.code) != COAP_CODE_CLASS_REQ) {
        return false;
    }

    char uriBuf[COAP_URI_MAX_LEN + 1] = {0};
    int32_t ret = CoapUtilsGetUri(packet, uriBuf, COAP_URI_MAX_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get uri error %d", ret);
        return false;
    }

    for (const char **whiteUri = uriWhiteList; *whiteUri != NULL; ++whiteUri) {
        if (strcmp(*whiteUri, uriBuf) != 0) {
            continue;
        }
        return true;
    }
    return false;
}

SessCode LanSearchSessCoapRecvPreProcess(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info)
{
    CHECK_RETURN_LOGW(LanSearchSessMsgCheck(msg, buf, info), SESS_CODE_ERR, "param invalid");
    CHECK_RETURN_LOGW(info->corData != NULL, SESS_CODE_ERR, "uri white list invalid");

    LanSearchSessMsg *sessMsg = (LanSearchSessMsg *)msg;
    LanSearchContext *ctx = (LanSearchContext *)info->userData;
    (void)LanSearchGetPeer(ctx, info->addr->addr, &sessMsg->peer);

    if (LanSearchPlainCheck(&sessMsg->packet, info->corData)) {
        UTILS_BIT_SET(sessMsg->bitMap, LAN_SEARCH_SESS_MSG_BIT_PLAIN);
    }
    return SESS_CODE_CONTINUE;
}

static bool LanSearchSessSpekeProcess(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info, LanSearchSpekeType type)
{
    LanSearchSessMsg *sessMsg = (LanSearchSessMsg *)msg;
    LanSearchPeer *peer = sessMsg->peer;
    CoapPacket *pkt = &sessMsg->packet;

    if (UTILS_IS_BIT_SET(sessMsg->bitMap, LAN_SEARCH_SESS_MSG_BIT_PLAIN) || pkt->payload.data == NULL ||
        pkt->payload.len == 0) {
        return true;
    }

    if (peer == NULL || !UTILS_IS_BIT_SET(peer->bitMap, LAN_SEARCH_PEER_BIT_SPEKE_FINISHED)) {
        IOTC_LOGW("peer speke invalid");
        return false;
    }

    uint8_t *data = NULL;
    uint32_t dataLen = 0;
    int32_t ret;
    if (type == LAN_SEARCH_SPEKE_TYPE_DECRYPT) {
        ret = SpekeDecryptData(peer->sessInfo.speke, pkt->payload.data, pkt->payload.len, &data, &dataLen);
    } else {
        ret = SpekeEncryptData(peer->sessInfo.speke, pkt->payload.data, pkt->payload.len, &data, &dataLen);
    }
    if (ret != IOTC_OK || data == NULL || dataLen == 0) {
        IOTC_LOGW("speke err %d/%u", ret, dataLen);
        return false;
    }

    CoapData newPayload = {data, dataLen};
    ret = CoapUtilsReplacePayload(pkt, buf, &newPayload);
    AdapterFree(data);
    if (ret != IOTC_OK) {
        IOTC_LOGW("coap replace payload error %d", ret);
        return false;
    }
    return true;
}

static bool LanSearchSessBase64Process(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info, LanSearchBase64Type type)
{
    LanSearchSessMsg *sessMsg = (LanSearchSessMsg *)msg;
    CoapPacket *pkt = &sessMsg->packet;

    if (UTILS_IS_BIT_SET(sessMsg->bitMap, LAN_SEARCH_SESS_MSG_BIT_PLAIN) || pkt->payload.data == NULL ||
        pkt->payload.len == 0) {
        return true;
    }

    uint32_t dataLen = 0;
    int32_t ret;
    /* 获取编解码后的大小 */
    if (type == LAN_SEARCH_BASE64_TYPE_DECODE) {
        ret = AdapterBase64Decode(pkt->payload.data, pkt->payload.len, NULL, &dataLen);
    } else {
        ret = AdapterBase64Encode(pkt->payload.data, pkt->payload.len, NULL, &dataLen);
    }
    if (ret != IOTC_OK || dataLen == 0 || dataLen > buf->size) {
        IOTC_LOGW("get len error %d/%d", ret, type);
        return false;
    }

    uint8_t *data = (uint8_t *)AdapterCalloc(dataLen, sizeof(uint8_t));
    if (data == NULL) {
        IOTC_LOGW("calloc error %u", dataLen);
        return false;
    }

    if (type == LAN_SEARCH_BASE64_TYPE_DECODE) {
        ret = AdapterBase64Decode(pkt->payload.data, pkt->payload.len, data, &dataLen);
    } else {
        ret = AdapterBase64Encode(pkt->payload.data, pkt->payload.len, data, &dataLen);
    }
    if (ret != IOTC_OK) {
        IOTC_LOGW("base64 error %d/%d", ret, type);
        AdapterFree(data);
        return false;
    }

    CoapData newPayload = {data, dataLen};
    ret = CoapUtilsReplacePayload(pkt, buf, &newPayload);
    AdapterFree(data);
    if (ret != IOTC_OK) {
        IOTC_LOGW("coap replace payload error %d", ret);
        return false;
    }
    return true;
}

SessCode LanSearchSessCoapRecvBase64Decode(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info)
{
    CHECK_RETURN_LOGW(LanSearchSessMsgCheck(msg, buf, info), SESS_CODE_ERR, "param invalid");

    if (!LanSearchSessBase64Process(msg, buf, info, LAN_SEARCH_BASE64_TYPE_DECODE)) {
        return SESS_CODE_ERR;
    }
    return SESS_CODE_CONTINUE;
}

SessCode LanSearchSessCoapRecvDecrypt(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info)
{
    CHECK_RETURN_LOGW(LanSearchSessMsgCheck(msg, buf, info), SESS_CODE_ERR, "param invalid");

    if (!LanSearchSessSpekeProcess(msg, buf, info, LAN_SEARCH_SPEKE_TYPE_DECRYPT)) {
        return SESS_CODE_ERR;
    }
    return SESS_CODE_CONTINUE;
}

SessCode LanSearchSessCoapSendEncrypt(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info)
{
    CHECK_RETURN_LOGW(LanSearchSessMsgCheck(msg, buf, info), SESS_CODE_ERR, "param invalid");

    if (!LanSearchSessSpekeProcess(msg, buf, info, LAN_SEARCH_SPEKE_TYPE_ENCRYPT)) {
        return SESS_CODE_ERR;
    }
    return SESS_CODE_CONTINUE;
}

SessCode LanSearchSessCoapRecvBase64Encode(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info)
{
    CHECK_RETURN_LOGW(LanSearchSessMsgCheck(msg, buf, info), SESS_CODE_ERR, "param invalid");

    if (!LanSearchSessBase64Process(msg, buf, info, LAN_SEARCH_BASE64_TYPE_ENCODE)) {
        return SESS_CODE_ERR;
    }
    return SESS_CODE_CONTINUE;
}