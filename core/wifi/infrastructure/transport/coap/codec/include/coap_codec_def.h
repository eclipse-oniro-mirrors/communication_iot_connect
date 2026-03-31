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
#ifndef COAP_CODEC_DEF_H
#define COAP_CODEC_DEF_H
#include <stdint.h>
#include <stddef.h>
#include "comm_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 消息码格式 |<- 3bit 类型 ->|<- 5bit 码值 ->| */
#define COAP_RESPONSE_CODE(N) (((N) / 100 << 5) | (N) % 100)
#define COAP_OPTION_MAX_NUM 16
#define COAP_TOKEN_MAX_LEN 8
#define COAP_CODE_CLASS_MASK (0xE0)
#define COAP_CODE_DETAIL_MASK (0x1F)
#define COAP_CODE_CLASS(code) (((code) & COAP_CODE_CLASS_MASK) >> 5)
#define COAP_CODE_DETAIL(code) ((code) & COAP_CODE_DETAIL_MASK)
#define COAP_URI_MAX_LEN 128
#define COAP_OPTION_MIN_LEN 1
#define COAP_EXTEND_DELTA_VALUE_UINT   12
#define COAP_EXTEND_DELTA_VALUE_UINT8   13
#define COAP_EXTEND_DELTA_VALUE_UINT16  14
#define COAP_EXTEND_DELTA_VALUE_UINT32  15
#define COAP_DELTA_UINT8_ADD_NUM        13
#define COAP_DELTA_UINT16_ADD_NUM       269
#define COAP_DELTA_UINT32_ADD_NUM       65805
#define COAP_METHOD_TYPE(pkt) ((pkt)->header.code)

typedef CommData CoapData;
typedef CommBuffer CoapBuffer;

typedef enum {
    COAP_CODE_CLASS_REQ = 0,
    COAP_CODE_CLASS_SUCC_RESP = 2,
    COAP_CODE_CLASS_ERR_RESP = 4,
    COAP_CODE_CLASS_SERVER_ERR = 5,
} CoapCodeClass;

typedef enum {
    COAP_RESPONSE_CODE_CONTENT                    = COAP_RESPONSE_CODE(205),
    COAP_RESPONSE_CODE_BAD_REQUEST                = COAP_RESPONSE_CODE(400),
    COAP_RESPONSE_CODE_BAD_OPTION                 = COAP_RESPONSE_CODE(402),
    COAP_RESPONSE_CODE_UNSUPPORTED_CONTENT_FORMAT = COAP_RESPONSE_CODE(415),
    COAP_RESPONSE_CODE_INTERNAL_ERROR             = COAP_RESPONSE_CODE(500),
    COAP_RESPONSE_CODE_NOT_IMPLEMENTED            = COAP_RESPONSE_CODE(501),
    COAP_RESPONSE_CODE_BAD_GATEWAY                = COAP_RESPONSE_CODE(502),
} CoapResponseCode;

/* 消息方法类型 */
typedef enum {
    COAP_METHOD_TYPE_GET    = 1,
    COAP_METHOD_TYPE_POST   = 2,
    COAP_METHOD_TYPE_PUT    = 3,
    COAP_METHOD_TYPE_DELETE = 4,
} CoapMethodType;

/* 消息类型 */
typedef enum {
    COAP_MSG_TYPE_CON = 0,
    COAP_MSG_TYPE_NCON = 1,
    COAP_MSG_TYPE_ACK = 2,
    COAP_MSG_TYPE_RESET = 3
} CoapMsgType;

typedef enum {
    COAP_OPTION_TYPE_TYPE_IF_MATCH   = 1,
    COAP_OPTION_TYPE_URI_HOST        = 3,
    COAP_OPTION_TYPE_ETAG            = 4,
    COAP_OPTION_TYPE_IF_NONE_MATCH   = 5,
    COAP_OPTION_TYPE_OBSERVE         = 6,
    COAP_OPTION_TYPE_URI_PORT        = 7,
    COAP_OPTION_TYPE_LOCATION_PATH   = 8,
    COAP_OPTION_TYPE_URI_PATH        = 11,
    COAP_OPTION_TYPE_CONTENT_FORMAT  = 12,
    COAP_OPTION_TYPE_MAX_AGE         = 14,
    COAP_OPTION_TYPE_URI_QUERY       = 15,
    COAP_OPTION_TYPE_ACCEPT          = 17,
    COAP_OPTION_TYPE_LOCATION_QUERY  = 20,
    COAP_OPTION_TYPE_PROXY_URI       = 35,
    COAP_OPTION_TYPE_PROXY_SCHEME    = 39,
    COAP_OPTION_TYPE_SESSION_ID      = 2048,
    COAP_OPTION_TYPE_ACCESS_TOKEN_ID = 2049,
    COAP_OPTION_TYPE_REQ_ID          = 2050,
    COAP_OPTION_TYPE_DEV_ID          = 2051,
    COAP_OPTION_TYPE_USER_ID         = 2052,
    COAP_OPTION_TYPE_SEQ_NUM_ID      = 2053,
    COAP_OPTION_TYPE_PUUID           = 2056,
    COAP_OPTION_TYPE_PLUGIN          = 2057,
    COAP_OPTION_TYPE_LOCAL_ERR       = 2058,
} CoapOptionType;

/* CoAP消息头 */
typedef struct {
    /* 版本号，CoAP over TCP无该字段 */
    uint8_t ver : 2;
    /* 消息类型 */
    uint8_t type : 2;
    uint8_t tkl : 4;
    /* 消息码，请求时表示方法，响应时表示响应码 */
    uint8_t code;
    /* 消息id，CoAP over TCP无该字段 */
    uint16_t msgId;
} CoapHeader;

/* CoAP over TCP消息头 */
typedef struct {
    uint8_t len : 4;
    uint8_t tkl : 4;
    uint8_t code;
    uint32_t exlen;
} CoapTcpHeader;

typedef struct {
    uint16_t option;
    CoapData value;
} CoapOption;

/* CoAP报文结构 */
typedef struct {
    CoapHeader header;
    CoapTcpHeader tcpheader;
    uint8_t token[COAP_TOKEN_MAX_LEN];
    uint8_t opNum;
    CoapOption options[COAP_OPTION_MAX_NUM];
    CoapData payload;
} CoapPacket;

typedef struct CoapBuildPacket CoapBuildPacket;

typedef int32_t (*CoapPacketBuildPayloadFunc)(const CoapBuildPacket *build, CoapBuffer *buf, void *userData);

struct CoapBuildPacket {
    CoapHeader header;
    CoapTcpHeader tcpheader;
    uint8_t token[COAP_TOKEN_MAX_LEN];
    uint8_t opNum;
    const CoapOption *options;
    const CoapData *payload;
    CoapPacketBuildPayloadFunc buildFunc;
    void *userData;
};

#ifdef __cplusplus
}
#endif

#endif /* COAP_CODEC_DEF_H */
