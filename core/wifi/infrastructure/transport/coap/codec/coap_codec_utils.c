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
#include <ctype.h>
#include "coap_codec_utils.h"
#include "comm_def.h"
#include "utils_assert.h"
#include "securec.h"
#include "utils_common.h"
#include "adapter_json.h"
#include "iotc_errcode.h"
#include "dfx_anonymize.h"

int32_t CoapUtilsBuildJsonPayloadFunc(const CoapBuildPacket *build, CoapBuffer *buf, void *userData)
{
    CHECK_RETURN_LOGW(buf != NULL && buf->buffer != NULL && buf->len < buf->size, IOTC_ERR_PARAM_INVALID,
        "param invalid");
    
    AdapterJson *jsonObj = (AdapterJson *)userData;
    uint32_t size = buf->size - buf->len;
    int32_t ret = AdapterJsonPrint2Buffer(jsonObj, (char *)buf->buffer + buf->len, &size);
    if (ret != IOTC_OK) {
        IOTC_LOGW("json print error %u/%u", buf->len, buf->size);
        return ret;
    }
    if (size > buf->size - buf->len) {
        IOTC_LOGW("print invalid %u/%u/%u", size, buf->len, buf->size);
        return IOTC_ADAPTER_JSON_ERR_PRINT;
    }
    buf->len += size;
    return IOTC_OK;
}

static int32_t CoapUtilsGetUriOption(const CoapPacket *pkt, char *buf, uint32_t *bufSize,
    CoapOptionType option, char delimiter)
{
    uint32_t uriSeg = 0;
    const CoapOption *uriOptions = CoapUtilsFindOption(pkt, option, &uriSeg);
    if (uriOptions == NULL || uriSeg == 0) {
        *bufSize = 0;
        return IOTC_OK;
    }

    uint32_t len = 0;
    uint32_t size = *bufSize;
    for (uint32_t i = 0; i < uriSeg; ++i) {
        const CoapOption *curOption = uriOptions + i;
        if (curOption->value.data == NULL || curOption->value.len == 0) {
            IOTC_LOGW("invalid uri option %u", curOption->value.len);
            return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_OPTION_INVALID;
        }
        if (size - len < curOption->value.len) {
            return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_BUFFER_SHORT;
        }
        int32_t ret = memcpy_s(buf + len, size - len, curOption->value.data, curOption->value.len);
        if (ret != EOK) {
            return IOTC_ERR_SECUREC_MEMCPY;
        }
        len += curOption->value.len;
        if (i < uriSeg - 1) {
            if (len >= size) {
                return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_BUFFER_SHORT;
            }
            /* 加入分隔符 */
            buf[len++] = delimiter;
        }
    }

    *bufSize = len;
    return IOTC_OK;
}

int32_t CoapUtilsGetUriPath(const CoapPacket *pkt, char *buf, uint32_t size)
{
    CHECK_RETURN_LOGW(pkt != NULL && buf != NULL && size != 0, IOTC_ERR_PARAM_INVALID, "invalid param");

    uint32_t bufSize = size;
    int32_t ret = CoapUtilsGetUriOption(pkt, buf, &bufSize, COAP_OPTION_TYPE_URI_PATH, '/');
    if (ret != IOTC_OK) {
        return ret;
    }
    if (bufSize >= size) {
        IOTC_LOGW("buffer too short %u/%u", bufSize, size);
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_BUFFER_SHORT;
    }
    buf[bufSize] = '\0';
    return IOTC_OK;
}

int32_t CoapUtilsGetUri(const CoapPacket *pkt, char *buf, uint32_t size)
{
    CHECK_RETURN_LOGW(pkt != NULL && buf != NULL && size != 0, IOTC_ERR_PARAM_INVALID, "invalid param");

    uint32_t len = size;
    int32_t ret = CoapUtilsGetUriOption(pkt, buf, &len, COAP_OPTION_TYPE_URI_PATH, '/');
    if (ret != IOTC_OK) {
        return ret;
    }

    uint32_t uriSeg = 0;
    const CoapOption *uriOptions = CoapUtilsFindOption(pkt, COAP_OPTION_TYPE_URI_PATH, &uriSeg);
    if (uriOptions == NULL || uriSeg == 0) {
        return IOTC_OK;
    }

    if (len >= size - 1) {
        IOTC_LOGW("buffer too short %u/%u", len, size);
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_BUFFER_SHORT;
    }

    /* 预留?位置 */
    ++len;
    uint32_t queryLen = size - len;
    ret = CoapUtilsGetUriOption(pkt, buf + len, &queryLen, COAP_OPTION_TYPE_URI_QUERY, '&');
    if (ret != IOTC_OK) {
        return ret;
    }
    if (queryLen == 0) {
        buf[len - 1] = '\0';
    } else {
        buf[len - 1] = '?';
        len += queryLen;
    }
    return IOTC_OK;
}

#if IOTC_CONF_COAP_DEBUG_DUMP_PACKET
static void CoapGetOptionChar(const CoapOption *option, char *buf, uint32_t len)
{
    buf[0] = '\0';
    if (option->value.data == NULL || option->value.len == 0) {
        return;
    }
    /* 只打印部分可见的option */
    int32_t ret;
    if (option->option == COAP_OPTION_TYPE_DEV_ID || option->option == COAP_OPTION_TYPE_PUUID) {
        char idBuf[DEVICE_ID_MAX_STR_LEN + 1] = {0};
        ret = strncpy_s(idBuf, sizeof(idBuf), (const char *)option->value.data, option->value.len);
        if (ret != EOK) {
            return;
        }
        ret = DfxAnonymizeStrWithBuffer(idBuf, ANONYMIZE_ID, buf, len);
        if (ret != IOTC_OK) {
            buf[0] = '\0';
            return;
        }
    } else if (option->option == COAP_OPTION_TYPE_URI_PATH || option->option == COAP_OPTION_TYPE_URI_QUERY) {
        ret = strncpy_s(buf, len, (const char *)option->value.data, option->value.len);
        if (ret != EOK) {
            return;
        }
    }
}

#if IOTC_CONF_COAP_DEBUG_DUMP_PAYLOAD
static void CoapGetPayloadChar(const uint8_t *payload, uint32_t len, char *buf, uint32_t bufLen)
{
    buf[0] = '\0';
    if (payload == NULL || len == 0) {
        return;
    }

    uint32_t i = 0;
    for (; i < len && i < bufLen - 1; ++i) {
        if (isprint(payload[i])) {
            buf[i] = payload[i];
        } else {
            buf[i] = ' ';
        }
    }
    buf[i] = '\0';
}
#endif

void CoapUtilsDumpPacket(const CoapPacket *pkt)
{
    char buf[COAP_URI_MAX_LEN + 1] = {0};
    IOTC_LOGD("Header:");
    IOTC_LOGD("  ver       %u", pkt->header.ver);
    IOTC_LOGD("  type      %u", pkt->header.type);
    IOTC_LOGD("  tkl       %u", pkt->header.tkl);
    IOTC_LOGD("  code      %u.%02u", COAP_CODE_CLASS(pkt->header.code), COAP_CODE_DETAIL(pkt->header.code));
    if (pkt->header.msgId != 0) {
        IOTC_LOGD("  msgId     %u", pkt->header.msgId);
    }

    if (pkt->header.tkl != 0) {
        (void)UtilsHexify(pkt->token, pkt->header.tkl, buf, sizeof(buf));
        IOTC_LOGD("Token:      0x%s", buf);
    }

    if (pkt->opNum != 0) {
        IOTC_LOGD("Options:    %u", pkt->opNum);
        for (uint8_t i = 0 ; i < pkt->opNum && i < COAP_OPTION_MAX_NUM; ++i) {
            CoapGetOptionChar(&pkt->options[i], buf, sizeof(buf));
            IOTC_LOGD("  0x%02x      %s", pkt->options[i].option, buf);
        }
    }

    IOTC_LOGD("PayloadLen: %u", pkt->payload.len);
#if IOTC_CONF_COAP_DEBUG_DUMP_PAYLOAD
    if (pkt->payload.len != 0) {
        IOTC_LOGD("Payload:", pkt->payload.len);
#define PRINT_PER_LINE 64
        for (uint32_t i = 0 ; i < pkt->payload.len && pkt->payload.data != NULL; i += PRINT_PER_LINE) {
            uint32_t len = (i + PRINT_PER_LINE) > pkt->payload.len ? pkt->payload.len - i : PRINT_PER_LINE;
            CoapGetPayloadChar(pkt->payload.data + i, len, buf, sizeof(buf));
            IOTC_LOGD("  %s", buf);
        }
    }
#endif
    return;
}
#endif /* IOTC_CONF_COAP_DEBUG_DUMP_PACKET */

int32_t CoapUtilsReplacePayload(CoapPacket *pkt, CoapBuffer *buf, const CoapData *payload)
{
    CHECK_RETURN_LOGW(pkt != NULL && buf != NULL && buf->size != 0 && payload != NULL && payload->data != NULL &&
        payload->len != 0, IOTC_ERR_PARAM_INVALID, "invalid param");
    
    const uint8_t *payloadPoint = pkt->payload.data;
    /* payload指针应该指向buffer内 */
    if (payloadPoint < buf->buffer || payloadPoint > buf->buffer + buf->size) {
        IOTC_LOGW("payload not in buffer");
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_PAYLOAD_INVALID;
    }

    uint32_t payloadBufferSize = buf->buffer + buf->size - payloadPoint;
    /* 预留一个字节用作字符串结束符 */
    if (payloadBufferSize <= payload->len) {
        IOTC_LOGW("buffer short to replace payload %u/%u", payloadBufferSize, payload->len);
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_CODEC_BUFFER_SHORT;
    }
    int32_t ret = memcpy_s(buf->buffer + buf->size - payloadBufferSize, payloadBufferSize, payload->data, payload->len);
    if (ret != EOK) {
        return IOTC_ERR_SECUREC_MEMCPY;
    }

    buf->len = buf->size - payloadBufferSize + payload->len;
    buf->buffer[buf->len] = '\0';
    pkt->payload.len = payload->len;
    return IOTC_OK;
}

const CoapOption *CoapUtilsFindOption(const CoapPacket *pkt, CoapOptionType option, uint32_t *seg)
{
    CHECK_RETURN_LOGW(pkt != NULL && seg != NULL, NULL, "invalid param");

    *seg = 0;
    const CoapOption *first = NULL;
    for (uint8_t i = 0; i < pkt->opNum; i++) {
        if (pkt->options[i].option == option) {
            if (first == NULL) {
                first = &pkt->options[i];
            }
            (*seg)++;
            continue;
        }
        if (first != NULL) {
            break;
        }
    }
    return first;
}