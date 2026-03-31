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
#include "coap_codec_tcp_v1.h"
#include "utils_assert.h"
#include "coap_codec_comm.h"
#include "iotc_socket.h"
#include "iotc_errcode.h"
#include "m2m_cloud_svc.h"
#include "m2m_cloud_psk.h"
#include "coap_codec_utils.h"
#include "iotc_kdf.h"
#include "securec.h"
#include "iotc_mem.h"
#include <stdlib.h>

#define COAP_TCP_V1_GET_REMAIN_SIZE_FINISH 1
#define COAP_TCP_V1_COAP_HEADER_LEN 2
#define COAP_TCP_V1_VER 1

#define COAP_TCP_DATA_ENCRYPT_PADDING 16
#define COAP_TCP_DATA_POS_EXTEND_DELTA_UINT16 2
#define COAP_TCP_DATA_POS_EXTEND_DELTA_UINT32 4
#define COAP_TCP_DATA_HEADER_1 1
#define COAP_TCP_DATA_HEADER_2 2
#define COAP_TCP_DATA_HEADER_3 3
#define COAP_TCP_DATA_HEADER_4 4
#define COAP_TCP_HEADER_DELTA_UINT8 1
#define COAP_TCP_HEADER_DELTA_UINT16 2
#define COAP_TCP_HEADER_DELTA_UINT32 3
#define COAP_TCP_HEADER_DELTA 5
#define COAP_TCP_HEADER_OFFSET 4
#define COAP_TCP_HEADER_EXLEN_OFFSET_1 8
#define COAP_TCP_HEADER_EXLEN_OFFSET_2 16
#define COAP_TCP_HEADER_EXLEN_OFFSET_3 24
#define COAP_TCP_HEADER_MIN_LEN 32


static int32_t CoapTcpDecrypt(CoapPacket *pkt, const CoapData *raw);
static int32_t CoapTcpEncrypt(const CoapBuildPacket *build, CoapPacket *pkt, CoapBuffer *buf, uint32_t len);

static int32_t CoapTcpV1HmacCompare(uint8_t *src, uint8_t *dest, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        if (src[i] != dest[i]) {
            IOTC_LOGW("%s compare err %02x/%02x", __func__, src[i], dest[i]);
            return IOTC_ERROR;
        }
    }
    return IOTC_OK;
}

static int32_t CoapTcpV1ParseHeader(CoapPacket *pkt, const CoapData *raw, uint32_t *pos)
{
    if (raw->len < *pos + COAP_TCP_V1_COAP_HEADER_LEN + COAP_TCP_V1_RAW_HEADER_LEN) {
        IOTC_LOGW("pkt header short %u", raw->len);
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_TCP_V1_HEADER_SHORT;
    }

    /* 0字节的高4位为len */
    pkt->tcpheader.len = (raw->data[*pos] >> 4) & 0x0f;
    /* 0字节的低4位为token长度 */
    pkt->header.tkl = raw->data[*pos] & 0x0F;
    (*pos)++;

    /* 计算需要偏移长度 */
    if (pkt->tcpheader.len >= 0 && pkt->tcpheader.len <= COAP_EXTEND_DELTA_VALUE_UINT) {
        pkt->tcpheader.exlen = pkt->tcpheader.len;
    } else if (pkt->tcpheader.len == COAP_EXTEND_DELTA_VALUE_UINT8) {
        pkt->tcpheader.exlen = raw->data[(*pos)++] + COAP_DELTA_UINT8_ADD_NUM + COAP_TCP_DATA_HEADER_1;
    } else if (pkt->tcpheader.len == COAP_EXTEND_DELTA_VALUE_UINT16) {
        pkt->tcpheader.exlen = (raw->data[(*pos)++] << COAP_TCP_HEADER_EXLEN_OFFSET_1) |
            (raw->data[(*pos)++]) + COAP_DELTA_UINT16_ADD_NUM + COAP_TCP_DATA_HEADER_2;
    } else if (pkt->tcpheader.len == COAP_EXTEND_DELTA_VALUE_UINT32) {
        pkt->tcpheader.exlen = (raw->data[(*pos)++] << COAP_TCP_HEADER_EXLEN_OFFSET_3) |
            (raw->data[(*pos)++] << COAP_TCP_HEADER_EXLEN_OFFSET_2) |
            (raw->data[(*pos)++] << COAP_TCP_HEADER_EXLEN_OFFSET_1) |
            (raw->data[(*pos)++]) + COAP_DELTA_UINT32_ADD_NUM + COAP_TCP_DATA_HEADER_4;
    }
    //token 长度
    pkt->tcpheader.exlen += pkt->header.tkl;
    //code + 第一个字节
    pkt->tcpheader.exlen += COAP_TCP_V1_COAP_HEADER_LEN;
    /* 1字节为操作码 */
    pkt->header.code = raw->data[*pos];
    IOTC_LOGD("%s pkt header len %u", __func__, pkt->tcpheader.len);
    (*pos)++;
    return IOTC_OK;
}

static int32_t CoapTcpV1GetHeader(CoapPacket *pkt)
{
    int32_t buflen = 0;

    if (pkt->tcpheader.exlen < COAP_DELTA_UINT8_ADD_NUM) {
        buflen = COAP_TCP_HEADER_DELTA_UINT8;
    } else if (pkt->tcpheader.exlen <= COAP_DELTA_UINT16_ADD_NUM) {
        buflen = COAP_TCP_HEADER_DELTA_UINT16;
    } else if (pkt->tcpheader.exlen <= COAP_DELTA_UINT32_ADD_NUM) {
        buflen = COAP_TCP_HEADER_DELTA_UINT32;
    } else {
        buflen = COAP_TCP_HEADER_DELTA;
    }
    buflen++;

    return buflen;
}

static int32_t CoapTcpDecrypt(CoapPacket *pkt, const CoapData *raw)
{
    M2mCloudContext *ctx = GetM2mCloudCtx();
    /* payload加解密操作 */
    if (ctx->pskInfo.pskFinish == true && ctx->pskInfo.encrypt == false) {
        uint8_t *decData = NULL;
        int32_t ret;
        uint32_t decLen = 0;
        uint8_t hmacRecv[SESS_HMAC_LEN] = { 0 };
        CoapTcpV1GetHeader(pkt);
        if (pkt->tcpheader.exlen < COAP_TCP_HEADER_MIN_LEN)
            return IOTC_ERROR;
        memcpy_s(hmacRecv, SESS_HMAC_LEN, &raw->data[pkt->tcpheader.exlen - SESS_HMAC_LEN], SESS_HMAC_LEN);
        /* 报文完整性保护 */
        uint8_t *hmacData = NULL;
        uint32_t hmacLen = 0;
        uint8_t hmacKey[SESS_HMAC_LEN] = { 0 };
        hmacLen = (pkt->tcpheader.exlen - SESS_HMAC_LEN);
        hmacData = (uint8_t *)IotcMalloc(hmacLen);
        if (hmacData == NULL) {
            IOTC_LOGE("hmacData malloc(%u) err", hmacLen);
            return IOTC_ADAPTER_MEM_ERR_MALLOC;
        }
        (void)memset_s(hmacData, hmacLen, 0, hmacLen);
        ret = memcpy_s(hmacData, hmacLen, raw->data, hmacLen);
        /* 计算HMAC */
        ret = StationSessCalHmac(ctx, hmacData, hmacLen, hmacKey, SESS_HMAC_LEN);
        /* 对比两个HMAC */
        ret = CoapTcpV1HmacCompare(hmacKey, hmacRecv, SESS_HMAC_LEN);
        if (ret != IOTC_OK) {
            IOTC_LOGW("%s sess hmac compare err %d", __func__, ret);
            return IOTC_ERROR;
        }
        /* 解密payload */
        ret = PskDecryptData(ctx, pkt->payload.data, pkt->payload.len - SESS_HMAC_LEN, &decData, &decLen);
        if (ret != IOTC_OK || decData == NULL || decLen == 0) {
            IOTC_LOGW("speke sess dec err %d/%u", ret, decLen);
            return SESS_CODE_ERR;
        }
        if (ret == IOTC_OK) {
            memcpy_s((void *)pkt->payload.data, decLen, (void *)decData, (size_t)decLen);
            pkt->payload.len = decLen;
        }
    }
    return IOTC_OK;
}

int32_t CoapTcpV1Decode(CoapPacket *pkt, const CoapData *raw)
{
    CHECK_RETURN(pkt != NULL && raw != NULL && raw->data != NULL && raw->len != 0, IOTC_ERR_PARAM_INVALID);

    uint32_t pos = 0;
    int32_t ret = CoapTcpV1ParseHeader(pkt, raw, &pos);
    if (ret != IOTC_OK) {
        IOTC_LOGW("parse coap tcp v1 header error %d", ret);
        return ret;
    }

    ret = CoapCommParseToken(pkt, raw, &pos);
    if (ret != IOTC_OK) {
        IOTC_LOGW("parse coap tcp v1 token error %d", ret);
        return ret;
    }

    ret = CoapCommParseOptions(pkt, raw, &pos);
    if (ret != IOTC_OK) {
        IOTC_LOGW("parse coap tcp v1 option error %d", ret);
        return ret;
    }

    ret = CoapCommParsePayload(pkt, raw, &pos);
    if (ret != IOTC_OK) {
        IOTC_LOGW("parse coap tcp v1 payload error %d", ret);
        return ret;
    }
    CoapTcpDecrypt(pkt, raw);
    return IOTC_OK;
}

static int32_t CoapTcpV1BuildHeader(const CoapBuildPacket *build, CoapPacket *pkt, CoapBuffer *buf)
{
    if (COAP_TCP_V1_COAP_HEADER_LEN + COAP_TCP_V1_RAW_HEADER_LEN > buf->size - buf->len) {
        IOTC_LOGW("buf short for header %u/%u", buf->len, buf->size);
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_BUFFER_SHORT;
    }
    uint32_t ext;
    uint8_t len = 0;
    /* code为0只能为空消息 */
    if (build->header.code == 0 &&
        (build->header.tkl != 0 || build->payload != NULL || build->buildFunc != NULL)) {
        IOTC_LOGW("invalid empty msg");
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_INVALID_BUILD;
    }

    /* 预留两字节填充长度 */
    buf->len += COAP_TCP_V1_RAW_HEADER_LEN;

    pkt->header = build->header;
    pkt->header.msgId = 0;
    pkt->header.ver = 0;

    if (pkt->tcpheader.exlen < COAP_DELTA_UINT8_ADD_NUM) {
        len = pkt->tcpheader.exlen;
        buf->len = COAP_TCP_HEADER_DELTA_UINT8;
    } else if (pkt->tcpheader.exlen <= COAP_DELTA_UINT16_ADD_NUM) {
        len = COAP_EXTEND_DELTA_VALUE_UINT8;
        buf->buffer[COAP_TCP_DATA_HEADER_1] = (uint8_t)(pkt->tcpheader.exlen - COAP_DELTA_UINT8_ADD_NUM);
        buf->len = COAP_TCP_HEADER_DELTA_UINT16;
    } else if (pkt->tcpheader.exlen <= COAP_DELTA_UINT32_ADD_NUM) {
        len = COAP_EXTEND_DELTA_VALUE_UINT16;
        ext = pkt->tcpheader.exlen - COAP_DELTA_UINT16_ADD_NUM;
        buf->buffer[COAP_TCP_DATA_HEADER_1] = (uint8_t)((ext >> BIT_PER_BYTE) & 0xFF);
        buf->buffer[COAP_TCP_DATA_HEADER_2] = (uint8_t)(ext & 0xFF);
        buf->len = COAP_TCP_HEADER_DELTA_UINT32;
    } else {
        len = COAP_EXTEND_DELTA_VALUE_UINT32;
        ext = pkt->tcpheader.exlen - COAP_DELTA_UINT32_ADD_NUM;
        /* 右移24bit获得高位 */
        buf->buffer[COAP_TCP_DATA_HEADER_1] = (uint8_t)((ext >> COAP_TCP_HEADER_EXLEN_OFFSET_3) & 0xFF);
        /* 右移16bit获得次高位 */
        buf->buffer[COAP_TCP_DATA_HEADER_2] = (uint8_t)((ext >> COAP_TCP_HEADER_EXLEN_OFFSET_2) & 0xFF);
        /* 右移8bit获得次低位 */
        buf->buffer[COAP_TCP_DATA_HEADER_3] = (uint8_t)((ext >> COAP_TCP_HEADER_EXLEN_OFFSET_1) & 0xFF);
        buf->buffer[COAP_TCP_DATA_HEADER_4] = (uint8_t)(ext & 0xFF);
        buf->len = COAP_TCP_HEADER_DELTA;
    }

    buf->buffer[0] = (((len) << COAP_TCP_HEADER_OFFSET) | pkt->header.tkl);
    /* 第4个字节为操作码 */
    buf->buffer[buf->len++] = pkt->header.code;

    return IOTC_OK;
}

int32_t CoapTcpV1UpdateRawHeaderLen(uint8_t *start, uint32_t len)
{
    CHECK_RETURN(start != NULL && len > COAP_TCP_V1_RAW_HEADER_LEN &&
        len - COAP_TCP_V1_RAW_HEADER_LEN <= UINT16_MAX, IOTC_ERR_PARAM_INVALID);
    uint16_t headerLen = len - COAP_TCP_V1_RAW_HEADER_LEN;
    start[0] = (uint8_t)((headerLen >> BIT_PER_BYTE) & 0xFF);
    start[1] = (uint8_t)(headerLen & 0xFF);

    return IOTC_OK;
}

static int32_t CoapTcpEncrypt(const CoapBuildPacket *build, CoapPacket *pkt, CoapBuffer *buf, uint32_t len)
{
    M2mCloudContext *ctx = GetM2mCloudCtx();
    if (ctx->pskInfo.pskFinish == true && ctx->pskInfo.encrypt == false) {
        int32_t ret = IOTC_OK;
        uint8_t *encData = NULL;
        uint32_t encLen = 0;
        ret = PskEncryptData(ctx, pkt->payload.data, pkt->payload.len, &encData, &encLen);
        if (ret != IOTC_OK || encData == NULL) {
            return SESS_CODE_ERR;
        }
        ret = CoapTcpV1BuildHeader(build, pkt, buf);
        if (ret != IOTC_OK) {
            return ret;
        }
        ret = CoapCommBuildToken(build, pkt, buf);
        if (ret != IOTC_OK) {
            return ret;
        }
        ret = CoapCommBuildOption(build, pkt, buf);
        if (ret != IOTC_OK) {
            return ret;
        }
        uint8_t *hmacData = NULL;
        uint32_t hmacLen = encLen + buf->len;
        uint8_t hmacKey[SESS_HMAC_LEN] = { 0 };
        hmacData = (uint8_t *)IotcMalloc(hmacLen + 1);
        if (hmacData == NULL) {
            return IOTC_ADAPTER_MEM_ERR_MALLOC;
        }
        ret = memcpy_s(hmacData, buf->len, buf->buffer, buf->len);
        memcpy_s(&hmacData[buf->len], encLen, encData, encLen);
        ret = StationSessCalHmac(ctx, hmacData, hmacLen, hmacKey, SESS_HMAC_LEN);
        CoapData payloadEnc = {encData, encLen};
        ret = CoapUtilsReplacePayload(pkt, buf, &payloadEnc);
        memcpy_s(&buf->buffer[buf->len], SESS_HMAC_LEN, hmacKey, SESS_HMAC_LEN);
        buf->len += SESS_HMAC_LEN;
        IotcFree(encData);
        IotcFree(hmacData);
    }
    return IOTC_OK;
}

int32_t CoapTcpV1Encode(const CoapBuildPacket *build, CoapPacket *pkt, CoapBuffer *buf)
{
    CHECK_RETURN(build != NULL && pkt != NULL && buf != NULL && buf->size != 0, IOTC_ERR_PARAM_INVALID);
    uint32_t tempLen = 0;
    uint8_t headerLen = 0;
    tempLen = buf->len;
    CoapCommBuildOption(build, pkt, buf);
    CoapCommBuildPayload(build, pkt, buf);
    pkt->tcpheader.exlen = buf->len;
    M2mCloudContext *ctx = GetM2mCloudCtx();
    if (ctx->pskInfo.pskFinish == true && ctx->pskInfo.encrypt == false) {
        pkt->tcpheader.exlen = pkt->tcpheader.exlen + (COAP_TCP_DATA_ENCRYPT_PADDING -
        (pkt->payload.len % COAP_TCP_DATA_ENCRYPT_PADDING)) + SESS_HMAC_LEN;
    }
    buf->len = tempLen;
    int32_t ret = CoapTcpV1BuildHeader(build, pkt, buf);
    if (ret != IOTC_OK) {
        IOTC_LOGW("build coap tcp v1 header error %d", ret);
        return ret;
    }
    headerLen = buf->len;
    ret = CoapCommBuildToken(build, pkt, buf);
    if (ret != IOTC_OK) {
        IOTC_LOGW("build coap tcp v1 token error %d", ret);
        return ret;
    }

    ret = CoapCommBuildOption(build, pkt, buf);
    if (ret != IOTC_OK) {
        IOTC_LOGW("build coap tcp v1 option error %d", ret);
        return ret;
    }

    ret = CoapCommBuildPayload(build, pkt, buf);
    if (ret != IOTC_OK) {
        IOTC_LOGW("build coap tcp v1 payload error %d", ret);
        return ret;
    }
    CoapTcpEncrypt(build, pkt, buf, headerLen);
    return IOTC_OK;
}

int32_t CoapTcpV1GetRemainSize(const uint8_t *packet, uint32_t curLen, uint32_t *remain)
{
    CHECK_RETURN_LOGW(packet != NULL && remain != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    if (curLen < COAP_TCP_V1_RAW_HEADER_LEN) {
        *remain = COAP_TCP_V1_RAW_HEADER_LEN - curLen;
        return IOTC_OK;
    }
    uint32_t bodyLen = IotcNtohs(*(const uint16_t *)packet);
    uint32_t curBodyLen = curLen - COAP_TCP_V1_RAW_HEADER_LEN;
    if (bodyLen == curBodyLen) {
        *remain = 0;
        return IOTC_OK;
    } else if (bodyLen > curBodyLen) {
        *remain = bodyLen - curBodyLen;
        return IOTC_OK;
    } else {
        IOTC_LOGW("invalid packet %u/%u", curBodyLen, bodyLen);
        return IOTC_ERR_PARAM_INVALID;
    }
}
