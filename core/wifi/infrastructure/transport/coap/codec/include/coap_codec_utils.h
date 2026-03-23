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
#ifndef COAP_CODEC_UTILS_H
#define COAP_CODEC_UTILS_H

#include "coap_codec_def.h"
#include "iotc_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 获取uri path，输出格式为path1/path2/xxx */
int32_t CoapUtilsGetUriPath(const CoapPacket *pkt, char *buf, uint32_t size);

/* 获取全量uri，包括uri path与uri query，输出格式为path1/path2/xxx?query1&query2&xxx */
int32_t CoapUtilsGetUri(const CoapPacket *pkt, char *buf, uint32_t size);

#if IOTC_CONF_COAP_DEBUG_DUMP_PACKET
void CoapUtilsDumpPacket(const CoapPacket *pkt);
#else
#define CoapUtilsDumpPacket(...)
#endif

int32_t CoapUtilsReplacePayload(CoapPacket *pkt, CoapBuffer *buf, const CoapData *payload);
int32_t CoapUtilsAddPayload(CoapPacket *pkt, CoapBuffer *buf, const CoapData *payload);

const CoapOption *CoapUtilsFindOption(const CoapPacket *pkt, CoapOptionType option, uint32_t *seg);

int32_t CoapUtilsBuildJsonPayloadFunc(const CoapBuildPacket *build, CoapBuffer *buf, void *userData);

#ifdef __cplusplus
}
#endif

#endif /* COAP_CODEC_UTILS_H */