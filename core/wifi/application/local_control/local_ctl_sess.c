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
#include "local_ctl_sess.h"
#include "utils_assert.h"
#include "iotc_errcode.h"
#include "utils_bit_map.h"
#include "local_ctl_cli_mngr.h"
#include "iotc_aes.h"
#include "base64_codec.h"
#include "security_random.h"
#include "dfx_anonymize.h"
#include "coap_codec_utils.h"
#include "securec.h"
#include "coap_sess_utils.h"

/* 窗口使用u32 bitmap承载，该值不能大于32 */
#define LOCAL_CONTROL_SEQ_WINDOW    30
#define LOCAL_CTL_HMAC_LEN          IOTC_MD_SHA256_BYTE_LEN
#define LOCAL_CTL_GCM_IV_LEN        12
#define LOCAL_CTL_GCM_TAG_LEN       16

static bool LocalSessMsgCheck(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info)
{
    CHECK_RETURN_LOGW(msg != NULL && buf != NULL && buf->buffer != NULL && buf->len != 0 && buf->size >= buf->len &&
        info != NULL && info->addr != NULL && info->userData != NULL, false, "param invalid");

    return true;
}

static inline bool LocalCtlSessMsgPlainCheck(LocalCoapSessMsg *sessMsg)
{
    if (UTILS_IS_BIT_SET(sessMsg->bitMap, LOCAL_COAP_PLAIN) || sessMsg->packet.payload.data == NULL ||
        sessMsg->packet.payload.len == 0) {
        return true;
    }
    return false;
}

static bool LocalCtlSessBase64Process(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info, Base64CodecType type)
{
    LocalCoapSessMsg *sessMsg = (LocalCoapSessMsg *)msg;
    CoapPacket *pkt = &sessMsg->packet;

    if (LocalCtlSessMsgPlainCheck(sessMsg)) {
        return true;
    }

    uint32_t dataLen = 0;
    uint8_t *data = GetBase64CodecData(pkt->payload.data, pkt->payload.len, &dataLen, type, buf->size);
    if (data == NULL) {
        IOTC_LOGW("local ctl base64 codec error %u", dataLen);
        return false;
    }

    CoapData newPayload = { data, dataLen };
    int32_t ret = CoapUtilsReplacePayload(pkt, buf, &newPayload);
    IotcFree(data);
    if (ret != IOTC_OK) {
        IOTC_LOGW("local ctl coap replace payload error %d", ret);
        return false;
    }
    return true;
}

SessCode LocalCtlSessCoapRecvPreProcess(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info)
{
    CHECK_RETURN_LOGW(LocalSessMsgCheck(msg, buf, info), SESS_CODE_ERR, "param invalid");
    CHECK_RETURN_LOGW(info->nodeData != NULL, SESS_CODE_ERR, "param invalid");

    DFX_ANONYMIZE_IP_ADDR(anonyIp, info->addr->addr);
    IOTC_LOGD("local recv packet from %s:%u", anonyIp, info->addr->port);

    LocalCoapSessMsg *sessMsg = (LocalCoapSessMsg *)msg;
    /* 发现和会话协商报文明文传输 */
    if (CoapUriWhiteListMatch(&sessMsg->packet, info->nodeData)) {
        UTILS_BIT_SET(sessMsg->bitMap, LOCAL_COAP_PLAIN);
        return SESS_CODE_CONTINUE;
    }

    uint32_t seg = 0;
    const CoapOption *sessOption = CoapUtilsFindOption(&sessMsg->packet, COAP_OPTION_TYPE_SESSION_ID, &seg);
    if (sessOption == NULL || seg != 1 || sessOption->value.data == NULL || sessOption->value.len == 0) {
        IOTC_LOGW("invalid sess id");
        return SESS_CODE_ERR;
    }

    char sessIdStr[HEXIFY_LEN(LOCAL_CONTROL_SESS_ID_LEN) + 1] = { 0 };
    UtilsHexify(sessOption->value.data, sessOption->value.len, sessIdStr, sizeof(sessIdStr));
    int32_t ret = GetLocalControlClient(info->userData, CLIENT_SESS_ID, (const char *)sessIdStr,
        sessOption->value.len * 2, &sessMsg->client);
    if (ret != IOTC_OK || sessMsg->client == NULL) {
        IOTC_LOGW("sess client not exist");
        return SESS_CODE_ERR;
    }

    return SESS_CODE_CONTINUE;
}

SessCode LocalCtlSessCoapRecvBase64Decode(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info)
{
    CHECK_RETURN_LOGW(LocalSessMsgCheck(msg, buf, info), SESS_CODE_ERR, "param invalid");

    if (!LocalCtlSessBase64Process(msg, buf, info, BASE64_CODEC_TYPE_DECODE)) {
        IOTC_LOGW("local ctl base64 dec error");
        return SESS_CODE_ERR;
    }
    return SESS_CODE_CONTINUE;
}

SessCode LocalCtlSessCoapSendBase64Encode(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info)
{
    CHECK_RETURN_LOGW(LocalSessMsgCheck(msg, buf, info), SESS_CODE_ERR, "param invalid");

    if (!LocalCtlSessBase64Process(msg, buf, info, BASE64_CODEC_TYPE_ENCODE)) {
        IOTC_LOGW("local ctl base64 enc error");
        return SESS_CODE_ERR;
    }
    return SESS_CODE_CONTINUE;
}

SessCode LocalCtlSessCoapRecvDecrypt(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info)
{
    CHECK_RETURN_LOGW(LocalSessMsgCheck(msg, buf, info), SESS_CODE_ERR, "param invalid");

    LocalCoapSessMsg *sessMsg = (LocalCoapSessMsg *)msg;
    if (LocalCtlSessMsgPlainCheck(sessMsg)) {
        return SESS_CODE_CONTINUE;
    }

    if (sessMsg->client == NULL) {
        IOTC_LOGW("sess client invalid");
        return SESS_CODE_ERR;
    }

    if (sessMsg->packet.payload.len <= LOCAL_CTL_GCM_IV_LEN + LOCAL_CTL_GCM_TAG_LEN) {
        IOTC_LOGW("invalid packet decrypt len %u", sessMsg->packet.payload.len);
        return SESS_CODE_ERR;
    }

    uint32_t dataLen = sessMsg->packet.payload.len - LOCAL_CTL_GCM_IV_LEN - LOCAL_CTL_GCM_TAG_LEN;
    uint8_t *decBuf = (uint8_t *)IotcCalloc(dataLen, sizeof(uint8_t));
    if (decBuf == NULL) {
        IOTC_LOGW("calloc error %u", dataLen);
        return SESS_CODE_ERR;
    }
    IotcAesGcmParam gcmParam = {
        .key = sessMsg->client->sessInfo.transKey,
        .keyLen = SESS_TRANS_KEY_LEN,
        .iv = sessMsg->packet.payload.data,
        .ivLen = LOCAL_CTL_GCM_IV_LEN,
        .add = NULL,
        .addLen = 0,
        .data = sessMsg->packet.payload.data + LOCAL_CTL_GCM_IV_LEN,
        .dataLen = dataLen,
    };
    int32_t ret = IotcAesGcmDecrypt(&gcmParam, sessMsg->packet.payload.data + LOCAL_CTL_GCM_IV_LEN + dataLen,
        LOCAL_CTL_GCM_TAG_LEN, decBuf);
    if (ret != IOTC_OK) {
        IotcFree(decBuf);
        IOTC_LOGW("gcm dec error %d", ret);
        return SESS_CODE_ERR;
    }

    CoapData decPayload = { decBuf, dataLen };
    ret = CoapUtilsReplacePayload(&sessMsg->packet, buf, &decPayload);
    IotcFree(decBuf);
    if (ret != IOTC_OK) {
        IOTC_LOGW("replace dec payload error %d", ret);
        return SESS_CODE_ERR;
    }

    ClientSessActiveUpdateAddr(info->userData, sessMsg->client->sessInfo.sessId,
        LOCAL_CONTROL_SESS_ID_STR_LEN, info->addr->addr);
    return SESS_CODE_CONTINUE;
}

SessCode LocalCtlSessCoapSendEncrypt(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info)
{
    CHECK_RETURN_LOGW(LocalSessMsgCheck(msg, buf, info), SESS_CODE_ERR, "param invalid");

    DFX_ANONYMIZE_IP_ADDR(anonyIp, info->addr->addr);
    IOTC_LOGD("local send packet to %s:%u", anonyIp, info->addr->port);

    LocalCoapSessMsg *sessMsg = (LocalCoapSessMsg *)msg;
    if (LocalCtlSessMsgPlainCheck(sessMsg)) {
        return SESS_CODE_CONTINUE;
    }

    if (sessMsg->client == NULL || sessMsg->packet.payload.len > buf->len ||
        buf->size - buf->len < LOCAL_CTL_GCM_IV_LEN + LOCAL_CTL_GCM_TAG_LEN) {
        IOTC_LOGW("sess msg invalid %u/%u/%u", sessMsg->packet.payload.len, buf->size, buf->len);
        return SESS_CODE_ERR;
    }

    uint32_t dataLen = sessMsg->packet.payload.len + LOCAL_CTL_GCM_IV_LEN + LOCAL_CTL_GCM_TAG_LEN;
    uint8_t *encBuf = (uint8_t *)IotcCalloc(dataLen, sizeof(uint8_t));
    if (encBuf == NULL) {
        IOTC_LOGW("calloc error %u", dataLen);
        return SESS_CODE_ERR;
    }

    /* 前12字节为随机IV */
    (void)SecurityRandom(encBuf, LOCAL_CTL_GCM_IV_LEN);
    IotcAesGcmParam gcmParam = {
        .key = sessMsg->client->sessInfo.transKey,
        .keyLen = SESS_TRANS_KEY_LEN,
        .iv = encBuf,
        .ivLen = LOCAL_CTL_GCM_IV_LEN,
        .add = NULL,
        .addLen = 0,
        .data = sessMsg->packet.payload.data,
        .dataLen = sessMsg->packet.payload.len,
    };

    int32_t ret = IotcAesGcmEncrypt(&gcmParam, encBuf + sessMsg->packet.payload.len + LOCAL_CTL_GCM_IV_LEN,
        LOCAL_CTL_GCM_TAG_LEN, encBuf + LOCAL_CTL_GCM_IV_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGW("enc error %d", ret);
        IotcFree(encBuf);
        return SESS_CODE_ERR;
    }

    CoapData decPayload = { encBuf, dataLen };
    ret = CoapUtilsReplacePayload(&sessMsg->packet, buf, &decPayload);
    IotcFree(encBuf);
    if (ret != IOTC_OK) {
        IOTC_LOGW("replace enc payload error %d", ret);
        return SESS_CODE_ERR;
    }
    return SESS_CODE_CONTINUE;
}
