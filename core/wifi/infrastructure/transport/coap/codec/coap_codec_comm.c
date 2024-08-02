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
#include "coap_codec_comm.h"
#include "adapter_socket.h"
#include "utils_assert.h"
#include "securec.h"
#include "iotc_errcode.h"

static int32_t CoapCommParseExtension(uint32_t *value, const CoapData *raw, uint32_t *pos)
{
    uint32_t extendLen;
    if (*value == COAP_EXTEND_DELTA_VALUE_UINT8) {
        /* 包含1个字节的extend */
        extendLen = 1;
        if (raw->len < *pos + extendLen) {
            IOTC_LOGW("extend uint8 short");
            return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_EXT_SHORT;
        }

        *value = raw->data[*pos] + COAP_DELTA_UINT8_ADD_NUM;
        (*pos) += extendLen;
        return IOTC_OK;
    }
    if (*value == COAP_EXTEND_DELTA_VALUE_UINT16) {
        /* 包含2个字节的extend */
        extendLen = 2;
        if (raw->len < *pos + extendLen) {
            IOTC_LOGW("extend uint16 short");
            return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_EXT_SHORT;
        }
        *value = AdapterNtohs(*((uint16_t *)&raw->data[*pos])) + COAP_DELTA_UINT16_ADD_NUM;
        *pos += extendLen;
        return IOTC_OK;
    }
    if (*value == COAP_EXTEND_DELTA_VALUE_UINT32) {
        /* 包含4个字节的extend */
        extendLen = 4;
        if (raw->len < *pos + extendLen) {
            IOTC_LOGW("extend uint32 short");
            return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_EXT_SHORT;
        }
        uint32_t add = AdapterNtohl(*((uint32_t *)&raw->data[*pos]));
        if (add > UINT32_MAX - COAP_DELTA_UINT32_ADD_NUM) {
            IOTC_LOGW("extend uint32 short");
            return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_EXT_LONG;
        }
        *value = add + COAP_DELTA_UINT32_ADD_NUM;
        *pos += extendLen;
        return IOTC_OK;
    }

    return IOTC_OK;
}

static int32_t CoapCommParseSingleOption(CoapOption *option, uint16_t *delta, const CoapData *raw, uint32_t *pos)
{
    if (raw->len < *pos + COAP_OPTION_MIN_LEN)  {
        IOTC_LOGW("pkt option short");
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_OPTION_SHORT;
    }

    /* 第0字节包含option delta和option length, 高4位为option delta, 低4位为option length */
    uint32_t curDelta = (raw->data[*pos] & 0xF0) >> 4;
    uint32_t curLen = raw->data[*pos] & 0x0F;
    if (curDelta == COAP_EXTEND_DELTA_VALUE_UINT32 || curLen == COAP_EXTEND_DELTA_VALUE_UINT32) {
        /* option中15为保留值 */
        IOTC_LOGW("option use reserved %u/%u", curDelta, curLen);
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_USE_RESERVED;
    }
    (*pos)++;

    int32_t ret = CoapCommParseExtension(&curDelta, raw, pos);
    if (ret != IOTC_OK) {
        IOTC_LOGW("invalid option delta ext");
        return ret;
    }
    if (curDelta > UINT16_MAX - *delta) {
        IOTC_LOGW("invalid option %u/%u", curDelta, *delta);
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_OPTION_BIG;
    }

    ret = CoapCommParseExtension(&curLen, raw, pos);
    if (ret != IOTC_OK) {
        IOTC_LOGW("invalid option len delta ext");
        return ret;
    }

    if (curLen + *pos > UINT32_MAX || raw->len < *pos + curLen) {
        IOTC_LOGW("invalid option len %u/%u", curLen, *pos);
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_OPTION_LONG;
    }

    *delta += curDelta;
    option->option = *delta;
    option->value.data = &raw->data[*pos];
    option->value.len = curLen;
    *pos += curLen;

    return IOTC_OK;
}

int32_t CoapCommParseOptions(CoapPacket *pkt, const CoapData *raw, uint32_t *pos)
{
    CHECK_RETURN(pkt != NULL && raw != NULL && raw->data != NULL && raw->len != 0 && pos != NULL && *pos <= raw->len,
        IOTC_ERR_PARAM_INVALID);
    uint8_t opNum = 0;
    uint16_t delta = 0;
    int32_t ret;

    /* 解析每一个option, 0xFF为payload标识符 */
    while ((*pos < raw->len) && (raw->data[*pos] != 0xFF) && (opNum < COAP_OPTION_MAX_NUM)) {
        ret = CoapCommParseSingleOption(&((pkt->options)[opNum]), &delta, raw, pos);
        if (ret != IOTC_OK) {
            return ret;
        }
        opNum++;
    }

    /* option达到上限 */
    if ((*pos < raw->len) && (raw->data[*pos] != 0xFF) && (opNum >= COAP_OPTION_MAX_NUM)) {
        IOTC_LOGW("options greater than maximum limit");
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_OPTION_NUM;
    }
    pkt->opNum = opNum;
    return IOTC_OK;
}


int32_t CoapCommBuildToken(const CoapBuildPacket *build, CoapPacket *pkt, CoapBuffer *buf)
{
    CHECK_RETURN(build != NULL && pkt != NULL && buf != NULL && buf->size >= buf->len,
        IOTC_ERR_PARAM_INVALID);
    if (build->header.tkl == 0) {
        return IOTC_OK;
    }

    if (build->header.tkl > buf->size - buf->len) {
        IOTC_LOGW("buf short for token %u/%u/%u", buf->len, buf->size, build->header.tkl);
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_BUFFER_SHORT;
    }

    int32_t ret = memcpy_s(pkt->token, sizeof(pkt->token), build->token, build->header.tkl);
    if (ret != EOK) {
        return IOTC_ERR_SECUREC_MEMCPY;
    }

    ret = memcpy_s(buf->buffer + buf->len, buf->size - buf->len, build->token, build->header.tkl);
    if (ret != EOK) {
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    buf->len += build->header.tkl;

    return IOTC_OK;
}

static void CoapCommGetExtensionBase(uint32_t value, uint8_t *base, uint8_t *len)
{
    /* delta的值为0-12, 直接取值低4字节 */
    if (value < COAP_DELTA_UINT8_ADD_NUM) {
        *base = (uint8_t)(value & 0xFF);
        *len = 0;
        return;
    }

    /* option的值大于12, 小于等于13 + 0xFF, 需要使用扩展字13 */
    if (value <= COAP_DELTA_UINT16_ADD_NUM) {
        *base = COAP_EXTEND_DELTA_VALUE_UINT8;
        /* 扩展1字节长度 */
        *len = 1;
        return;
    }

    /* option的值大于13 + 0xFF, 小于等于269 + 0xFFFF, 需要使用扩展字14 */
    if (value <= COAP_DELTA_UINT32_ADD_NUM) {
        *base = COAP_EXTEND_DELTA_VALUE_UINT16;
        /* 扩展2字节长度 */
        *len = 2;
        return;
    }

    /* option的值大于269 + 0xFFFF, 需要使用扩展字节15 */
    *base = COAP_EXTEND_DELTA_VALUE_UINT32;
    /* 扩展4字节长度 */
    *len = 4;
    return;
}

static int32_t CoapCommBuildExtension(CoapBuffer *buf, uint32_t value)
{
    /* delta的值为0-12, 无需扩展 */
    if (value < COAP_DELTA_UINT8_ADD_NUM) {
        return IOTC_OK;
    }

    uint32_t ext;
    /* option的值大于12, 小于等于13 + 0xFF, 扩展1字节，值为value-13 */
    if (value <= COAP_DELTA_UINT16_ADD_NUM) {
        /* 扩展1字节长度 */
        if (buf->size - buf->len < 1) {
            IOTC_LOGW("buf short for uint8 ext");
            return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_BUFFER_SHORT;
        }
        buf->buffer[buf->len++] = (uint8_t)(value - COAP_DELTA_UINT8_ADD_NUM);
        return IOTC_OK;
    }

    /* option的值大于13 + 0xFF, 小于等于269 + 0xFFFF, 扩展2字节，值为value-269 */
    if (value <= COAP_DELTA_UINT32_ADD_NUM) {
        /* 扩展2字节长度 */
        if (buf->size - buf->len < 2) {
            IOTC_LOGW("buf short for uint16 ext");
            return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_BUFFER_SHORT;
        }
        ext = value - COAP_DELTA_UINT16_ADD_NUM;
        buf->buffer[buf->len++] = (uint8_t)((ext >> BIT_PER_BYTE) & 0xFF);
        buf->buffer[buf->len++] = (uint8_t)(ext & 0xFF);
        return IOTC_OK;
    }

    /* option的值大于269 + 0xFFFF, 扩展4字节，值为value-65805 */
    if (buf->size - buf->len < 4) {
        IOTC_LOGW("buf short for uint32 ext");
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_BUFFER_SHORT;
    }

    ext = value - COAP_DELTA_UINT32_ADD_NUM;
    /* 右移24bit获得高位 */
    buf->buffer[buf->len++] = (uint8_t)((ext >> 24) & 0xFF);
    /* 右移16bit获得次高位 */
    buf->buffer[buf->len++] = (uint8_t)((ext >> 16) & 0xFF);
    /* 右移8bit获得次低位 */
    buf->buffer[buf->len++] = (uint8_t)((ext >> 8) & 0xFF);
    buf->buffer[buf->len++] = (uint8_t)(ext & 0xFF);
    return IOTC_OK;
}

static int32_t CoapCommBuildSingleOption(const CoapBuildPacket *build, CoapPacket *pkt, CoapBuffer *buf, uint8_t index)
{
    const CoapOption *curOption = &build->options[index];

    uint16_t delta = curOption->option;
    if (index != 0) {
        delta -= build->options[index - 1].option;
    }

    uint8_t deltaBase;
    uint8_t lenBase;
    uint8_t deltaExtLen = 0;
    uint8_t lenExtLen = 0;
    CoapCommGetExtensionBase(delta, &deltaBase, &deltaExtLen);
    CoapCommGetExtensionBase(curOption->value.len, &lenBase, &lenExtLen);
    if (deltaBase == COAP_EXTEND_DELTA_VALUE_UINT32 || lenBase == COAP_EXTEND_DELTA_VALUE_UINT32) {
        /* option中15为保留值 */
        IOTC_LOGW("option too long %u/%u", deltaBase, lenBase);
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_USE_RESERVED;
    }

    /* option第1个字节标识delta和长度 */
    if (1 + deltaExtLen + lenExtLen + curOption->value.len > buf->size - buf->len) {
        IOTC_LOGW("buf not enough for option %d/%u/%u/%u/%u/%u", curOption->option, deltaExtLen, lenExtLen,
            curOption->value.len, buf->size, buf->len);
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_BUFFER_SHORT;
    }

    /* option第1个字节, 前4位为option delta, 后4位为option长度 */
    buf->buffer[buf->len++] = (deltaBase << 4) | lenBase;
    /* 填入扩展长度 */
    int32_t ret = CoapCommBuildExtension(buf, delta);
    if (ret != IOTC_OK) {
        return ret;
    }
    ret = CoapCommBuildExtension(buf, curOption->value.len);
    if (ret != IOTC_OK) {
        return ret;
    }

    if (curOption->value.len != 0) {
        if (curOption->value.data != NULL) {
            ret = memcpy_s(buf->buffer + buf->len, buf->size - buf->len, curOption->value.data, curOption->value.len);
            if (ret != EOK) {
                return IOTC_ERR_SECUREC_MEMCPY;
            }
        } else {
            (void)memset_s(buf->buffer + buf->len, buf->size - buf->len, 0, curOption->value.len);
        }
    }

    pkt->options[index].value.data = buf->buffer + buf->len;
    pkt->options[index].value.len = curOption->value.len;
    pkt->options[index].option = curOption->option;
    buf->len += curOption->value.len;

    return IOTC_OK;
}


int32_t CoapCommBuildOption(const CoapBuildPacket *build, CoapPacket *pkt, CoapBuffer *buf)
{
    CHECK_RETURN(build != NULL && pkt != NULL && buf != NULL && buf->size >= buf->len,
        IOTC_ERR_PARAM_INVALID);
    if (build->opNum == 0) {
        return IOTC_OK;
    }

    if (build->opNum > COAP_OPTION_MAX_NUM) {
        IOTC_LOGW("invalid op num %u", build->opNum);
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_INVALID_BUILD;
    }

    uint16_t option = 0;
    /* option应为升序 */
    for (uint8_t i = 0; i < build->opNum; ++i) {
        if (build->options[i].option < option) {
            return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_OPTION_ORDER;
        }
        option = build->options[i].option;
    }

    int32_t ret;
    for (uint8_t i = 0; i < build->opNum; ++i) {
        ret = CoapCommBuildSingleOption(build, pkt, buf, i);
        if (ret != IOTC_OK) {
            IOTC_LOGW("add option error %u", i);
            return ret;
        }
    }
    pkt->opNum = build->opNum;

    return IOTC_OK;
}

int32_t CoapCommBuildPayload(const CoapBuildPacket *build, CoapPacket *pkt, CoapBuffer *buf)
{
    CHECK_RETURN(build != NULL && pkt != NULL && buf != NULL && buf->size >= buf->len,
        IOTC_ERR_PARAM_INVALID);
    
    /* 空报文 */
    if (build->payload == NULL && build->buildFunc == NULL) {
        return IOTC_OK;
    }

    if (buf->size == buf->len) {
        IOTC_LOGW("no space for 0xff %u", buf->size);
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_BUFFER_SHORT;
    }
    /* payload标志位0xFF */
    buf->buffer[buf->len++] = 0xFF;
    pkt->payload.data = buf->buffer + buf->len;

    int32_t ret;
    /* 外部回调填充payload */
    if (build->buildFunc != NULL) {
        pkt->payload.len = buf->len;
        ret = build->buildFunc(build, buf, build->userData);
        if (ret != IOTC_OK || buf->len < pkt->payload.len) {
            IOTC_LOGW("build func error %d", ret);
            return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_BUILD_FUNC;
        }
        pkt->payload.len = buf->len - pkt->payload.len;
        return IOTC_OK;
    }

    pkt->payload.len = build->payload->len;
    if (build->payload->len == 0 || build->payload->data == NULL) {
        IOTC_LOGW("invalid payload");
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_INVALID_BUILD;
    }

    if (buf->size - buf->len < build->payload->len) {
        IOTC_LOGW("buf short for payload %u/%u/%u", buf->size, buf->len, build->payload->len);
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_BUFFER_SHORT;
    }

    ret = memcpy_s(buf->buffer + buf->len, buf->size - buf->len, build->payload->data, build->payload->len);
    if (ret != EOK) {
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    buf->len += build->payload->len;
    return IOTC_OK;
}

int32_t CoapCommParseToken(CoapPacket *pkt, const CoapData *raw, uint32_t *pos)
{
    CHECK_RETURN(pkt != NULL && raw != NULL && raw->data != NULL && raw->len != 0 && pos != NULL && *pos <= raw->len,
        IOTC_ERR_PARAM_INVALID);
    if (pkt->header.tkl == 0) {
        return IOTC_OK;
    }

    if (raw->len < *pos + pkt->header.tkl) {
        IOTC_LOGW("pkr token short %u/%u/%u", raw->len, *pos, pkt->header.tkl);
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_UDP_TOKEN_SHORT;
    }

    int32_t ret = memcpy_s(pkt->token, sizeof(pkt->token), &raw->data[*pos], pkt->header.tkl);
    if (ret != EOK) {
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    *pos += pkt->header.tkl;
    return IOTC_OK;
}

int32_t CoapCommParsePayload(CoapPacket *pkt, const CoapData *raw, uint32_t *pos)
{
    CHECK_RETURN(pkt != NULL && raw != NULL && raw->data != NULL && raw->len != 0 && pos != NULL && *pos <= raw->len,
        IOTC_ERR_PARAM_INVALID);
    /* 当前已读完数据或者没有payload标识则解析结束 */
    if (*pos >= raw->len || raw->data[*pos] != 0xFF) {
        pkt->payload.data = NULL;
        pkt->payload.len = 0;
        return IOTC_OK;
    }

    (*pos)++;
    pkt->payload.data = raw->data + *pos;
    pkt->payload.len = raw->len - *pos;
    return IOTC_OK;
}