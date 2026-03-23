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
#ifndef M2M_CLOUD_PSK_H
#define M2M_CLOUD_PSK_H

#include <stdint.h>
#include "m2m_cloud_send.h"

#ifdef __cplusplus
extern "C" {
#endif

const CloudOption *M2mCloudGetPskOption(void);
IotcJson *M2mCloudBuildPskRequest(M2mCloudContext *ctx);
int32_t M2mCloudParsePskResponse(M2mCloudContext *ctx, const CoapPacket *resp, int32_t *errcode);
int32_t PskEncryptData(M2mCloudContext *ctx, const uint8_t *data, uint32_t dataLen,
    uint8_t **encData, uint32_t *encDataLen);
int32_t PskDecryptData(M2mCloudContext *ctx, const uint8_t *data, uint32_t dataLen,
    uint8_t **decData, uint32_t *decDataLen);
int32_t StationSessCalHmac(M2mCloudContext *ctx, const uint8_t *data, uint32_t dataLen,
    uint8_t *calHmac, uint32_t calHmacLen);

#ifdef __cplusplus
}
#endif

#endif