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
#ifndef COAP_ENDPOINT_RETRANS_H
#define COAP_ENDPOINT_RETRANS_H

#include "coap_endpoint.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t msgId;
    uint8_t tkl;
    uint8_t token[COAP_TOKEN_MAX_LEN];
    uint8_t cnt;
    SocketAddr addr;
} CoapRetransParam;

typedef bool (*CoapRetransCheckFunc)(const CoapRetransParam *param, const CoapData *raw,
    void *userData, uint32_t *next);

int32_t CoapEndpointRetransEnable(CoapEndpoint *endpoint, CoapRetransCheckFunc func, uint32_t maxBufSize);

/* 默认重传函数，每秒重发1次，持续4次，盲发 */
bool CoapEndpointRetransDefaultCheckFunc(const CoapRetransParam *param, const CoapData *raw,
    void *userData, uint32_t *next);

#ifdef __cplusplus
}
#endif

#endif /* COAP_ENDPOINT_RETRANS_H */
