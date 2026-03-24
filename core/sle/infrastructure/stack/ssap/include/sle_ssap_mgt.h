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
#ifndef SLE_SSAP_MGT_H
#define SLE_SSAP_MGT_H

#include <stdint.h>
#include "iotc_sle_server.h"
#include "iotc_sle_client.h"
#include "iotc_sle_def.h"
#include "utils_list.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SLE_DEFAULT_MAX_CONN_NUM    10
#define SLE_CONN_HEAD_NODE          11
#define SLE_MTU_SIZE                500

typedef struct {
    uint32_t    connId;
    uint32_t    serverId;
    SleStateType    type;
    IotcAdptSleAcbState   connState;
    IotcAdptSleDeviceAddr devAddr;
    IotcAdptSsapcFindServiceResult handler;
    ListEntry   node;
} SlePeerDevInfo;

typedef struct {
    SlePeerDevInfo *peerDevInfo;
    uint8_t connNum;
    uint8_t svcNum;
    IotcAdptSleSsapService *svc;
    uint8_t startedSvcNum;
    uint8_t handle;
    uint8_t serverId;
} SleSsapMgtApp;

typedef struct {
    const char *svcUuid;
    const char *charUuid;
    uint32_t connId;
    uint32_t valueLen;
    uint8_t *value;
} SleIndicateParam;

typedef struct {
    uint8_t serverId;
    uint16_t connectId;
    uint16_t requestId;
    uint8_t type;
    uint8_t *value;
    int32_t valueLen;
    uint16_t handle;
} SleSsapWriteParam;

typedef struct {
    uint8_t serverId;
    uint16_t connectId;
    uint8_t type;
    uint8_t *value;
    int32_t valueLen;
    uint16_t handle;
} SleSsapReqWriteNotificationParam;

int32_t SleAddSsapSvc(const IotcSleSsapProfileSvc *svc);
int32_t SleSsapMgtInit(void);
void SleSsapMgtDestroy(void);
SleSsapMgtApp *GetSleSsapMgtApp(void);
int32_t SleGetSeviceHandle(const char *svcUuid, int32_t *handle);
int32_t DelSleSsapMgtPeerDevInfo(uint32_t connId);
SlePeerDevInfo *GetSleSsapMgtPeerDevInfo(uint32_t connId);
bool SleIsPair(void);
void SleSetPair(bool isSlePair);
int32_t SleSendIndicateDataInner(const char *svcUuid, const char *charUuid,
    uint32_t connId, const uint8_t *value, uint32_t valueLen);
void PrintSleSsapServiceList(IotcAdptSleSsapService *svc, uint8_t num);
int32_t IotcSleSendIndicateData(const char *svcUuid, const char *charUuid,
    const uint8_t *value, uint32_t valueLen);
int32_t SleScheduleEventInit(void);
void SleSsapDisconnectAll(void);
int32_t SetSleConnectParam(void);
int32_t SleSsapReqRead(uint8_t serverId, uint16_t connId, uint16_t attrHandle, int16_t requestId);
int32_t SleSsapReqWrite(const SleSsapWriteParam *param);
int32_t SleSetServiceAtt(const uint32_t connId, const uint32_t startHdl, const uint32_t endHdl);
void PrintSleSsapConnidAndAddr(void);
int32_t SleSsapReqWriteNotification(const SleSsapReqWriteNotificationParam *param);
#ifdef __cplusplus
}
#endif

#endif /* SLE_SSAP_MGT_H */
