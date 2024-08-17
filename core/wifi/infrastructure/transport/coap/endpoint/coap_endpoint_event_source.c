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
#include "coap_endpoint_priv.h"
#include "utils_list.h"
#include "utils_assert.h"
#include "securec.h"
#include "iotc_os.h"

#define COAP_ENDPOINT_EVENT_SOURCE_NAME "ENDPOINT"

typedef struct {
    EventSource base;
    CoapEndpoint *endpoint;
} CoapEndpointEventSource;

static bool CoapEndpointSourcePrepare(EventSource *self, uint32_t *timeout)
{
    CHECK_RETURN(self != NULL && timeout != NULL, false);
    CoapEndpointEventSource *coapSource = (CoapEndpointEventSource *)self;
    CoapEndpoint *endpoint = coapSource->endpoint;
    CHECK_RETURN(endpoint != NULL, false);

    *timeout = UINT32_MAX;

    CoapEndpointRetransPrepare(endpoint, timeout);
    CoapClientReqPrepare(endpoint, timeout);
    return true;
}

static bool CoapEndpointSourceCheck(EventSource *self)
{
    CHECK_RETURN(self != NULL, false);
    CoapEndpointEventSource *coapSource = (CoapEndpointEventSource *)self;
    CoapEndpoint *endpoint = coapSource->endpoint;
    uint32_t now = IotcGetSysTimeMs();
    if (CoapEndpointRetransCheck(endpoint, now)) {
        return true;
    }

    return CoapEndpointReqCheck(endpoint, now);
}

static bool RetransSourceDispatch(EventSource *self)
{
    CHECK_RETURN(self != NULL, false);
    CoapEndpointEventSource *coapSource = (CoapEndpointEventSource *)self;
    CoapEndpoint *endpoint = coapSource->endpoint;

    CoapClientRespTimeoutCheck(endpoint);
    CoapEndpointRetransDispatch(endpoint);
    return true;
}

EventSource *CoapEndpointEventSourceNew(CoapEndpoint *endpoint)
{
    CHECK_RETURN_LOGW(endpoint != NULL, NULL, "param invalid");
    static const EventSourceOps retransSourceOps = {
        .prepare = CoapEndpointSourcePrepare,
        .poll = NULL,
        .check = CoapEndpointSourceCheck,
        .dispatch = RetransSourceDispatch,
        .finalize = NULL,
    };
    
    EventSource *source = EventSourceNew(&retransSourceOps, sizeof(CoapEndpointEventSource),
        COAP_ENDPOINT_EVENT_SOURCE_NAME, NULL);
    if (source == NULL) {
        IOTC_LOGW("source new error");
        return NULL;
    }

    CoapEndpointEventSource *coapSource = (CoapEndpointEventSource *)source;
    coapSource->endpoint = endpoint;

    return source;
}