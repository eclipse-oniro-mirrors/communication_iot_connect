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
#include "coap_codec_udp.h"
#include "comm_def.h"
#include "utils_assert.h"
#include "iotc_socket.h"
#include "securec.h"
#include "coap_codec_comm.h"
#include "iotc_errcode.h"

#define COAP_UDP_HEADER_LEN 4
#define COAP_UDP_VER 1

static int32_t CoapUdpParseHeader(CoapPacket *pkt, const CoapData *raw, uint32_t *pos)
{
    if (raw->len < *pos + COAP_UDP_HEADER_LEN) {
        IOTC_LOGW("pkt header short %u", raw->len);
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_UDP_HEADER_SHORT;
    }

    /* right shift 6 for high two bit version */
    pkt->header.ver = (raw->data[*pos] >> 6) & 0x03;
    if (pkt->header.ver != COAP_UDP_VER) {
        IOTC_LOGW("invalid ver %u", pkt->header.ver);
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_UDP_INVALID_VER;
    }

    /* 0字节的第4/5位为类型 */
    pkt->header.type = ((raw->data[*pos] & 0x30) >> 4) & 0x03;
    /* 0字节的低4位为token长度 */
    pkt->header.tkl = raw->data[*pos] & 0x0F;
    /* 1字节为操作码 */
    (*pos)++;
    pkt->header.code = raw->data[*pos];
    (*pos)++;
    pkt->header.msgId = IotcNtohs(*(uint16_t *)&raw->data[*pos]);
    /* 2 byte for msgid */
    (*pos) += 2;
    return IOTC_OK;
}

int32_t CoapUdpDecode(CoapPacket *pkt, const CoapData *raw)
{
    CHECK_RETURN(pkt != NULL && raw != NULL && raw->data != NULL && raw->len != 0, IOTC_ERR_PARAM_INVALID);
    uint32_t pos = 0;
    int32_t ret = CoapUdpParseHeader(pkt, raw, &pos);
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

static int32_t CoapUdpBuildHeader(const CoapBuildPacket *build, CoapPacket *pkt, CoapBuffer *buf)
{
    if (COAP_UDP_HEADER_LEN > buf->size - buf->len) {
        IOTC_LOGW("buf short for header %u/%u", buf->len, buf->size);
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_BUFFER_SHORT;
    }
    /* code为0只能为空消息 */
    if (build->header.code == 0 &&
        (build->header.tkl != 0 || build->payload != NULL || build->buildFunc != NULL)) {
        IOTC_LOGW("invalid empty msg");
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_INVALID_BUILD;
    }

    pkt->header = build->header;
    pkt->header.ver = COAP_UDP_VER;

    /* high 2 bit is version(right shift 6 bit), 4/5 bit is type(right shift 4 bit), low 4 bit is tkl */
    buf->buffer[buf->len++] = ((pkt->header.ver << 6) | (pkt->header.type << 4) | pkt->header.tkl);

    /* 第1个字节为操作码 */
    buf->buffer[buf->len++] = pkt->header.code;

    /* 2 and 3 byte is msgid */
    buf->buffer[buf->len++] = (uint8_t)((pkt->header.msgId >> BIT_PER_BYTE) & 0xFF);
    buf->buffer[buf->len++] = (uint8_t)(pkt->header.msgId & 0xFF);

    return IOTC_OK;
}

int32_t CoapUdpEncode(const CoapBuildPacket *build, CoapPacket *pkt, CoapBuffer *buf)
{
    CHECK_RETURN(build != NULL && pkt != NULL && buf != NULL && buf->size != 0, IOTC_ERR_PARAM_INVALID);

    int32_t ret = CoapUdpBuildHeader(build, pkt, buf);
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
    return IOTC_OK;
}