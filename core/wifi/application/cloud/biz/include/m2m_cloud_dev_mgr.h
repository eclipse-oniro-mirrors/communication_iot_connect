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
#ifndef M2M_CLOUD_MANAGER_DEVICE_H
#define M2M_CLOUD_MANAGER_DEVICE_H
#include <stdint.h>
#include "comm_def.h"
#include <time.h>
#include "m2m_cloud_ctx.h"
#include "utils_json.h"
#include "coap_codec_def.h"
#include "m2m_cloud_send.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct M2mEndDeviceInfo {
    char      deviceId[DEVICE_ID_MAX_STR_LEN + 1];
    char      gatewayId[DEVICE_ID_MAX_STR_LEN + 1];
    uint8_t   online;
    char     *lastChange;
    IotcJson *devInfo;
} M2mEndDeviceInfo;

IotcJson *M2mCloudBuildEcoDevInfoRequest(M2mCloudContext *ctx);

IotcJson *M2mCloudBuildEcoDevStateRequest(M2mCloudContext *ctx);

const CloudOption *M2mCloudEcoDevInfoSyncOption(void);

const CloudOption *M2mCloudEcoDevStateSyncOption(void);

void M2mCloudEcoDevInfoRespHandler(const CoapPacket *resp, const SocketAddr *addr,
                                   M2mCloudContext *userData, bool timeout);

void M2mCloudEcoDevStateRespHandler(const CoapPacket *resp,
    const SocketAddr *addr, M2mCloudContext *userData, bool timeout);

IotcJson *M2mCloudBuildDevIdRequest(M2mCloudContext *ctx);

int32_t M2mCloudParseDevInfoSyncResponse(M2mCloudContext *ctx, const CoapPacket *resp, int32_t *errcode);

int32_t M2mCloudParseDevStatusSyncResponse(M2mCloudContext *ctx, const CoapPacket *resp, int32_t *errcode, char* devId);

int32_t SleDeviceCloudFsmInit(void);

const CloudOption *M2mCloudGetAuthCodeByDevIdSyncOption(void);
void M2mCloudDevIdRespHandler(const CoapPacket *resp,
    const SocketAddr *addr, void *userData, bool timeout);

#ifdef __cplusplus
}
#endif

#endif /* M2M_CLOUD_MANAGER_DEVICE_H */