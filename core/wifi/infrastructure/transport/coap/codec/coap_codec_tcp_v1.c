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

#define COAP_TCP_V1_GET_REMAIN_SIZE_FINISH 1
#define COAP_TCP_V1_COAP_HEADER_LEN 2
#define COAP_TCP_V1_VER 1

static int32_t CoapTcpV1ParseHeader(CoapPacket *pkt, const CoapData *raw, uint32_t *pos)
{
    if (raw->len < *pos + COAP_TCP_V1_COAP_HEADER_LEN + COAP_TCP_V1_RAW_HEADER_LEN) {
        IOTC_LOGW("pkt header short %u", raw->len);
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_TCP_V1_HEADER_SHORT;
    }
    /* 2字节标识报文长度 */
    *pos += COAP_TCP_V1_RAW_HEADER_LEN;

    /* right shift 6 for high two bit version */
    pkt->header.ver = (raw->data[*pos] >> 6) & 0x03;
    if (pkt->header.ver != COAP_TCP_V1_VER) {
        IOTC_LOGW("invalid ver %u", pkt->header.ver);
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_TCP_V1_INVALID_VER;
    }

    /* 0字节的第4/5位为类型 */
    pkt->header.type = ((raw->data[*pos] & 0x30) >> 4) & 0x03;
    /* 0字节的低4位为token长度 */
    pkt->header.tkl = raw->data[*pos] & 0x0F;
    (*pos)++;
    /* 1字节为操作码 */
    pkt->header.code = raw->data[*pos];
    (*pos)++;
    return IOTC_OK;
}

int32_t CoapTcpV1Decode(CoapPacket *pkt, const CoapData *raw)
{
    CHECK_RETURN(pkt != NULL && raw != NULL && raw->data != NULL && raw->len != 0, IOTC_ERR_PARAM_INVALID);

    uint32_t pos = 0;
    int32_t ret = CoapTcpV1ParseHeader(pkt, raw, &pos);
    if (ret != IOTC_OK) {
        IOTC_LOGW("parse header error %d", ret);
        return ret;
    }

    ret = CoapCommParseToken(pkt, raw, &pos);
    if (ret != IOTC_OK) {
        IOTC_LOGW("parse token error %d", ret);
        return ret;
    }

    ret = CoapCommParseOptions(pkt, raw, &pos);
    if (ret != IOTC_OK) {
        IOTC_LOGW("parse option error %d", ret);
        return ret;
    }

    ret = CoapCommParsePayload(pkt, raw, &pos);
    if (ret != IOTC_OK) {
        IOTC_LOGW("parse payload error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static int32_t CoapTcpV1BuildHeader(const CoapBuildPacket *build, CoapPacket *pkt, CoapBuffer *buf)
{
    if (COAP_TCP_V1_COAP_HEADER_LEN + COAP_TCP_V1_RAW_HEADER_LEN > buf->size - buf->len) {
        IOTC_LOGW("buf short for header %u/%u", buf->len, buf->size);
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_BUFFER_SHORT;
    }
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
    pkt->header.ver = COAP_TCP_V1_VER;

    /* 第3个字节的高2位为版本号左移6位, 第4、5位为消息类型, 低4位为token长度 */
    buf->buffer[buf->len++] = ((pkt->header.ver << 6) | (pkt->header.type << 4) | pkt->header.tkl);
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

int32_t CoapTcpV1Encode(const CoapBuildPacket *build, CoapPacket *pkt, CoapBuffer *buf)
{
    CHECK_RETURN(build != NULL && pkt != NULL && buf != NULL && buf->size != 0, IOTC_ERR_PARAM_INVALID);

    int32_t ret = CoapTcpV1BuildHeader(build, pkt, buf);
    if (ret != IOTC_OK) {
        IOTC_LOGW("build header error %d", ret);
        return ret;
    }

    ret = CoapCommBuildToken(build, pkt, buf);
    if (ret != IOTC_OK) {
        IOTC_LOGW("build token error %d", ret);
        return ret;
    }

    ret = CoapCommBuildOption(build, pkt, buf);
    if (ret != IOTC_OK) {
        IOTC_LOGW("build option error %d", ret);
        return ret;
    }

    ret = CoapCommBuildPayload(build, pkt, buf);
    if (ret != IOTC_OK) {
        IOTC_LOGW("build payload error %d", ret);
        return ret;
    }

    ret = CoapTcpV1UpdateRawHeaderLen(buf->buffer, buf->len);
    if (ret != IOTC_OK) {
        IOTC_LOGW("build tcp header error %d", ret);
        return ret;
    }
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