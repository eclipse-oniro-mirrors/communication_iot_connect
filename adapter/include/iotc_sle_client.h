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

#ifndef IOT_CONNECT_ADAPTER_INCLUDE_IOTC_SLE_CLIENT_H_
#define IOT_CONNECT_ADAPTER_INCLUDE_IOTC_SLE_CLIENT_H_

#include "iotc_sle_host.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SLE_UUID_LEN 16
typedef struct {
    uint8_t len;
    uint8_t id[SLE_UUID_LEN];
} SleUuid;

typedef struct {
    uint16_t startHdl;
    uint16_t endHdl;
    SleUuid uuid;
} IotcAdptSsapcFindServiceResult;

typedef struct {
    uint16_t handle;
    uint32_t operateIndication;
    SleUuid uuid;
    uint8_t descriptorsCount;
    uint8_t* descriptorsType;
} IotcAdptSsapcFindPropertyResult;

typedef struct {
    uint8_t type;
    SleUuid uuid;
} IotcAdptSsapcFindStructureResult;

typedef struct {
    uint16_t handle;
    uint8_t type;
    uint16_t dataLen;
    uint8_t *data;
} IotcAdptSsapcHandleValue, IotcAdptSsapcWriteParam;

typedef struct {
    SleUuid uuid;
    uint8_t type;
} IotcAdptSsapcReadByUuidCmpResult;

typedef struct {
    uint16_t handle;
    uint8_t  type;
} IotcAdptSsapcWriteResult;

typedef struct {
    uint32_t mtuSize;
    uint16_t version;
} IotcAdptSsapExchangeInfo;

typedef union {
    struct {
        uint8_t clientId;
        uint16_t connId;
        IotcAdptSsapcFindServiceResult service;
        IotcAdptSleStatus status;
    } ssapcFindServiceResult;
    struct {
        uint8_t clientId;
        uint16_t connId;
        IotcAdptSsapcFindPropertyResult property;
        IotcAdptSleStatus status;
    } ssapcFindPropertyResult;
    struct  {
        uint8_t clientId;
        uint16_t connId;
        IotcAdptSsapcFindStructureResult structureResult;
        IotcAdptSleStatus status;
    } ssapcFindStructureResult;
    struct  {
        uint8_t clientId;
        uint16_t connId;
        IotcAdptSsapcHandleValue readData;
        IotcAdptSleStatus status;
    } ssapcHandleValue;
    struct  {
        uint8_t clientId;
        uint16_t connId;
        IotcAdptSsapcReadByUuidCmpResult cmpResult;
        IotcAdptSleStatus status;
    } ssapcReadByUuidCmpResult;
    struct  {
        uint8_t clientId;
        uint16_t connId;
        IotcAdptSsapcWriteResult writeResult;
        IotcAdptSleStatus status;
    } ssapcWriteResult;
    struct  {
        uint8_t clientId;
        uint16_t connId;
        IotcAdptSsapExchangeInfo exInfo;
        IotcAdptSleStatus status;
    } ssapExchangeInfo;
    struct  {
        uint8_t clientId;
        uint16_t connId;
        IotcAdptSsapcHandleValue data;
        IotcAdptSleStatus status;
    } ssapcNotification;
    struct  {
        uint8_t clientId;
        uint16_t connId;
        IotcAdptSsapcHandleValue data;
        IotcAdptSleStatus status;
    } ssapcIndication;
} IotcAdptSleSsapClientEventParam;

typedef struct {
    uint8_t type;
    uint16_t startHdl;
    uint16_t endHdl;
    SleUuid uuid;
    uint8_t reserve;
} IotcAdptSsapcFindStructureParam;

typedef struct {
    uint8_t  enableFilterPolicy;
    uint8_t  initiatePhys;
    uint8_t  gtNegotiate;
    uint16_t scanInterval;
    uint16_t scanWindow;
    uint16_t minInterval;
    uint16_t maxInterval;
    uint16_t timeout;
} IotcAdptSleDefaultConnectParam;

/* 设置发现链接参数 */
typedef struct {
    bool isDiscover;
    bool isConnect;
    bool isBond;
} IotcAdptSleConnectParam;

#define OH_SLE_SEEK_PHY_NUM_MAX 3
typedef struct {
    uint8_t ownaddrtype;
    uint8_t filterduplicates;
    uint8_t seekfilterpolicy;
    uint8_t seekphys;
    uint8_t seekType[OH_SLE_SEEK_PHY_NUM_MAX];
    uint16_t seekInterval[OH_SLE_SEEK_PHY_NUM_MAX];
    uint16_t seekWindow[OH_SLE_SEEK_PHY_NUM_MAX];
} IotcAdptSleSeekParam;

typedef enum {
    IOTC_ADPT_SLE_SSAPC_FIND_STRUCTURE_EVENT,
    IOTC_ADPT_SLE_SSAPC_FIND_PROPERTY_EVENT,
    IOTC_ADPT_SLE_SSAPC_FIND_STRUCTURE_COMPLETE_EVENT,
    IOTC_ADPT_SLE_SSAPC_READ_CFM_EVENT,
    IOTC_ADPT_SSAPC_READ_BY_UUID_COMPLETE_EVENT,
    IOTC_ADPT_SSAPC_WRITE_CFM_EVENT,
    IOTC_ADPT_SSAPC_EXCHANGE_INFO_EVENT,
    IOTC_ADPT_SSAPC_NOTIFICATION_EVENT,
    IOTC_ADPT_SSAPC_INDICATION_EVENT
} IotcAdptSleSsapClientEvent;

typedef enum {
    IOTC_SLE_SSAP_CONNECT_STATE_NONE          = 0x00,
    IOTC_SLE_SSAP_CONNECT_STATE_CONNECTED     = 0x01,
    IOTC_SLE_SSAP_CONNECT_STATE_DISCONNECTED  = 0x02,
} SleStateType;

typedef int32_t(*IotcAdptSleSsapClientCallback)(IotcAdptSleSsapClientEvent event,
    const IotcAdptSleSsapClientEventParam *param);
int32_t IotcSleConnectRemoteDevice(const IotcAdptSleDeviceAddr *addr);
int32_t IotcSleSetConnectParam(const IotcAdptSleConnectParam *param);
int32_t IotcSleDisconnectSsap(const uint8_t *bdAddr, uint32_t addrLen);
int32_t IotcSleSetSeekParam(const IotcAdptSleSeekParam *param);
int32_t IotcSleStartSeek(void);
int32_t IotcSleStoptSeek(void);
int32_t IotcSleDisconnectRemoteDevice(const IotcAdptSleDeviceAddr *addr);
int32_t IotcSleDefaultConnectionParamSet(const IotcAdptSleDefaultConnectParam *param);
int32_t IotcSleSsapcRegister(SleUuid *appUuid, uint8_t *clientId);
int32_t IotcSleSsapcRegisterUnregister(uint8_t clientId);
int32_t IotcSleSsapcFindStructure(uint8_t clientId, uint16_t connId, IotcAdptSsapcFindStructureParam *param);
int32_t IotcSleSsapcReadReq(uint8_t clientId, uint16_t connId, uint16_t handle, uint8_t type);
// int32_t IotcSleSsapcWriteReq(uint8_t clientId, uint16_t connId, IotcAdptSsapcWriteParam *param);
int32_t IotcSleSsapcExchangeInfoReq(uint8_t clientId, uint16_t connId, IotcAdptSsapExchangeInfo* param);
int32_t IotcSleRegisterSsapClientCallbacks(const IotcAdptSleSsapClientCallback callback);
int32_t SlePairRemoteDevice(const IotcAdptSleDeviceAddr *addr);


#ifdef __cplusplus
}
#endif

#endif