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
#ifndef COAP_CODEC_TCP_V1_H
#define COAP_CODEC_TCP_V1_H

#include "coap_codec_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define COAP_TCP_V1_RAW_HEADER_LEN 2

int32_t CoapTcpV1Decode(CoapPacket *pkt, const CoapData *raw);

int32_t CoapTcpV1Encode(const CoapBuildPacket *build, CoapPacket *pkt, CoapBuffer *buf);

int32_t CoapTcpV1UpdateRawHeaderLen(uint8_t *start, uint32_t len);

int32_t CoapTcpV1GetRemainSize(const uint8_t *packet, uint32_t curLen, uint32_t *remain);

#ifdef __cplusplus
}
#endif

#endif /* COAP_CODEC_TCP_V1_H */