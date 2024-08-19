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
#ifndef M2M_CLOUD_SEND_H
#define M2M_CLOUD_SEND_H
#include <stdint.h>
#include "coap_endpoint_client.h"
#include "utils_json.h"
#include "m2m_cloud_ctx.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CLOUD_OPTION_BIT_ACCESS_TOKEN_ID = 0,
    CLOUD_OPTION_BIT_SEQ_NUM_ID,
} CloudOptionBitMap;

typedef struct {
    const char **uri;
    uint32_t num;
    uint8_t opBitMap;
} CloudOption;

typedef IotcJson *(*M2mBuildRequest)(M2mCloudContext *ctx);

int32_t M2mCloudSendRequest(M2mCloudContext *ctx, CoapClientRespHandler resp,
    M2mBuildRequest build, const CloudOption *option);

#ifdef __cplusplus
}
#endif

#endif /* M2M_CLOUD_SEND_H */