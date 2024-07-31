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

#ifndef COAP_CODEC_COMM_H
#define COAP_CODEC_COMM_H

#include "coap_codec_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BIT_PER_BYTE 8

int32_t CoapCommParseToken(CoapPacket *pkt, const CoapData *raw, uint32_t *pos);

int32_t CoapCommParseOptions(CoapPacket *pkt, const CoapData *raw, uint32_t *pos);

int32_t CoapCommParsePayload(CoapPacket *pkt, const CoapData *raw, uint32_t *pos);

int32_t CoapCommBuildOption(const CoapBuildPacket *build, CoapPacket *pkt, CoapBuffer *buf);

int32_t CoapCommBuildToken(const CoapBuildPacket *build, CoapPacket *pkt, CoapBuffer *buf);

int32_t CoapCommBuildPayload(const CoapBuildPacket *build, CoapPacket *pkt, CoapBuffer *buf);

#ifdef __cplusplus
}
#endif

#endif /* COAP_CODEC_COMM_H */