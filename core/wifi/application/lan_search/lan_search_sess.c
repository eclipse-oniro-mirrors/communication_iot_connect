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
#include "base64_codec.h"
#include "coap_codec_utils.h"
#include "coap_sess_utils.h"
#include "iotc_mem.h"

typedef enum {
    LAN_SEARCH_SPEKE_TYPE_DECRYPT = 0,
    LAN_SEARCH_SPEKE_TYPE_ENCRYPT,
} LanSearchSpekeType;

static bool LanSearchSessMsgCheck(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info)
{
    CHECK_RETURN(msg != NULL && buf != NULL && buf->buffer != NULL && buf->len != 0 && buf->size >= buf->len &&
        info != NULL && info->addr != NULL && info->userData != NULL, false);

    return true;
}

SessCode LanSearchSessCoapRecvPreProcess(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info)
{
    CHECK_RETURN_LOGW(LanSearchSessMsgCheck(msg, buf, info), SESS_CODE_ERR, "param invalid");
    CHECK_RETURN_LOGW(info->nodeData != NULL, SESS_CODE_ERR, "uri whitelist invalid");

    LanSearchSessMsg *sessMsg = (LanSearchSessMsg *)msg;
    LanSearchContext *ctx = (LanSearchContext *)info->userData;
    (void)LanSearchGetPeer(ctx, info->addr->addr, &sessMsg->peer);

    if (CoapUriWhiteListMatch(&sessMsg->packet, info->nodeData)) {
        UTILS_BIT_SET(sessMsg->bitMap, LAN_SEARCH_SESS_MSG_BIT_PLAIN);
    }
    return SESS_CODE_CONTINUE;
}

static inline bool LanSearchPlainMsgCheck(LanSearchSessMsg *sessMsg)
{
    if (UTILS_IS_BIT_SET(sessMsg->bitMap, LAN_SEARCH_SESS_MSG_BIT_PLAIN) || sessMsg->packet.payload.data == NULL ||
        sessMsg->packet.payload.len == 0) {
        return true;
    }
    return false;
}

static bool LanSearchSessSpekeProcess(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info, LanSearchSpekeType type)
{
    LanSearchSessMsg *sessMsg = (LanSearchSessMsg *)msg;
    LanSearchPeer *peer = sessMsg->peer;
    CoapPacket *pkt = &sessMsg->packet;

    if (LanSearchPlainMsgCheck(sessMsg)) {
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
    if (ret != IOTC_OK || data == NULL) {
        IOTC_LOGW("speke err %d/%u", ret, dataLen);
        return false;
    }

    CoapData newPayload = { data, dataLen };
    ret = CoapUtilsReplacePayload(pkt, buf, &newPayload);
    IotcFree(data);
    CHECK_RETURN_LOGW(ret == IOTC_OK, false, "coap replace payload error %d", ret);
    return true;
}

static bool LanSearchSessBase64Process(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info, Base64CodecType type)
{
    LanSearchSessMsg *sessMsg = (LanSearchSessMsg *)msg;
    CoapPacket *pkt = &sessMsg->packet;

    if (LanSearchPlainMsgCheck(sessMsg)) {
        return true;
    }

    uint32_t dataLen = 0;
    uint8_t *data = GetBase64CodecData(pkt->payload.data, pkt->payload.len, &dataLen, type, buf->size);
    if (data == NULL) {
        IOTC_LOGW("lan search base64 codec error %u", dataLen);
        return false;
    }

    CoapData newPayload = { data, dataLen };
    int32_t ret = CoapUtilsReplacePayload(pkt, buf, &newPayload);
    IotcFree(data);
    if (ret != IOTC_OK) {
        IOTC_LOGW("lan search coap replace payload error %d", ret);
        return false;
    }
    return true;
}

SessCode LanSearchSessCoapRecvBase64Decode(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info)
{
    CHECK_RETURN_LOGW(LanSearchSessMsgCheck(msg, buf, info), SESS_CODE_ERR, "param invalid");

    if (!LanSearchSessBase64Process(msg, buf, info, BASE64_CODEC_TYPE_DECODE)) {
        IOTC_LOGW("lan search base64 decode error");
        return SESS_CODE_ERR;
    }
    return SESS_CODE_CONTINUE;
}

SessCode LanSearchSessCoapRecvDecrypt(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info)
{
    CHECK_RETURN_LOGW(LanSearchSessMsgCheck(msg, buf, info), SESS_CODE_ERR, "param invalid");

    if (!LanSearchSessSpekeProcess(msg, buf, info, LAN_SEARCH_SPEKE_TYPE_DECRYPT)) {
        IOTC_LOGW("lan search speke dec error");
        return SESS_CODE_ERR;
    }
    return SESS_CODE_CONTINUE;
}

SessCode LanSearchSessCoapSendEncrypt(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info)
{
    CHECK_RETURN_LOGW(LanSearchSessMsgCheck(msg, buf, info), SESS_CODE_ERR, "param invalid");

    if (!LanSearchSessSpekeProcess(msg, buf, info, LAN_SEARCH_SPEKE_TYPE_ENCRYPT)) {
        IOTC_LOGW("lan search speke enc error");
        return SESS_CODE_ERR;
    }
    return SESS_CODE_CONTINUE;
}

SessCode LanSearchSessCoapRecvBase64Encode(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info)
{
    CHECK_RETURN_LOGW(LanSearchSessMsgCheck(msg, buf, info), SESS_CODE_ERR, "param invalid");

    if (!LanSearchSessBase64Process(msg, buf, info, BASE64_CODEC_TYPE_ENCODE)) {
        IOTC_LOGW("lan search base64 encode error");
        return SESS_CODE_ERR;
    }
    return SESS_CODE_CONTINUE;
}