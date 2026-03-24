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
#ifndef IOTC_SLE_DEF_H
#define IOTC_SLE_DEF_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "iotc_def.h"
#include "comm_def.h"
#include "iotc_prof_def.h"
#include "iotc_sle_host.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IOTC_SLE_ADV_ALWAYS UINT32_MAX /* 一直开启广播 */

/** SSAP 读写权限 */
enum IotcSleSsapPermission {
    /** 可读 */
    IOTC_SLE_SSAP_PERMISSION_READ = 0x01,
    /** 加密可读 */
    IOTC_SLE_SSAP_PERMISSION_READ_ENCRYPTED = 0x02,
    /** 中间人保护可读 */
    IOTC_SLE_SSAP_PERMISSION_READ_ENCRYPTED_MITM = 0x04,
    /** 可写 */
    IOTC_SLE_SSAP_PERMISSION_WRITE = 0x10,
    /** 加密可写 */
    IOTC_SLE_SSAP_PERMISSION_WRITE_ENCRYPTED = 0x20,
    /** 中间人保护可写 */
    IOTC_SLE_SSAP_PERMISSION_WRITE_ENCRYPTED_MITM = 0x40,
    /** 签名可写 */
    IOTC_SLE_SSAP_PERMISSION_WRITE_SIGNED = 0x80,
    /** 中间人保护签名可写 */
    IOTC_SLE_SSAP_PERMISSION_WRITE_SIGNED_MITM = 0x100,
};

/** SSAP 特征属性 */
enum IotcSleSsapProperties {
    /** 可广播 */
    IOTC_SLE_SSAP_CHARACTER_PROPERTY_BIT_BROADCAST = 0x01,
    /** 可读 */
    IOTC_SLE_SSAP_CHARACTER_PROPERTY_BIT_READ = 0x02,
    /** 可不响应写入 */
    IOTC_SLE_SSAP_CHARACTER_PROPERTY_BIT_WRITE_NO_RSP = 0x04,
    /** 可写入 */
    IOTC_SLE_SSAP_CHARACTER_PROPERTY_BIT_WRITE = 0x08,
    /** 支持通知 */
    IOTC_SLE_SSAP_CHARACTER_PROPERTY_BIT_NOTIFY = 0x10,
    /** 支持指示 */
    IOTC_SLE_SSAP_CHARACTER_PROPERTY_BIT_INDICATE = 0x20,
    /** 支持带签名写入 */
    IOTC_SLE_SSAP_CHARACTER_PROPERTY_BIT_SIGNED_WRITE = 0x40,
    /** 具有扩展属性 */
    IOTC_SLE_SSAP_CHARACTER_PROPERTY_BIT_EXTENDED_PROPERTY = 0x80,
};

typedef struct {
    const char *uuid;
    uint32_t permission;
    int32_t (*readFunc)(uint32_t connId, uint8_t *buff, uint32_t *len);
    int32_t (*writeFunc)(uint32_t connId, uint8_t *buff, uint32_t len);
} IotcSleSsapProfileDesc;

typedef struct {
    const char *uuid;
    uint32_t permission;
    uint32_t property;
    /** SLE SSAP服务读函数类型 */
    int32_t (*readFunc)(uint32_t connId, uint8_t *buff, uint32_t *len);
    /** SLE SSAP服务写函数类型 */
    int32_t (*writeFunc)(uint32_t connId, uint8_t *buff, uint32_t len);
    /** SLE SSAP服务指示函数类型 */
    int32_t (*indicateFunc)(uint32_t connId, uint8_t *buff, uint32_t len);
    const IotcSleSsapProfileDesc *desc;
    uint8_t descNum;
    uint8_t serviceId;
} IotcSleSsapProfileChar;

typedef struct {
    const char *uuid;
    const IotcSleSsapProfileChar *character;
    uint8_t charNum;
} IotcSleSsapProfileSvc;

typedef struct {
    const IotcSleSsapProfileSvc *svc;
    uint8_t svcNum;
} IotcSleSsapProfileSvcList;

typedef enum {
    /** 可连接可扫描的非定向广播（默认） */
    IOTC_SLE_ADV_TYPE_IND = 0x00,
    /** 可连接的高占空比定向广播 */
    IOTC_SLE_ADV_TYPE_DIRECT_IND_HIGH = 0x01,
    /** 可扫描的非定向广播 */
    IOTC_SLE_ADV_TYPE_SCAN_IND = 0x02,
    /** 不可连接的非定向广播 */
    IOTC_SLE_ADV_TYPE_NONCONN_IND = 0x03,
    /** 可连接的低占空比定向广播 */
    IOTC_SLE_ADV_TYPE_DIRECT_IND_LOW = 0x04
} IotcSleAdvType;

/** SLE的广播数据和扫描应答数据 */
typedef struct {
    /** 广播数据 */
    uint8_t *announceData;
    /** 广播数据长度 */
    uint32_t announceDataLen;
    /** 响应数据 */
    uint8_t *seekRspData;
    /** 相应数据长度 */
    uint32_t seekRspDataLen;
} IotcSleAdvData;

/* SLE的广播参数 */
typedef struct {
    /** 广播类型 */
    IotcSleAdvType announceMode; // advType;
    /** 最小广播间隔，0.625ms的倍数，最短间隔设置为20ms */
    uint32_t announceIntervalMin; // minInterval;
    /** 最大广播间隔，0.625ms的倍数，最长间隔不超过100ms */
    uint32_t announceIntervalMax; // maxInterval;
    /** bit[0:2] => 37,38,39信道 */
    uint8_t announceChannelMap; // channelMap;
} IotcSleAdvParam;

/* uuid with len */
typedef struct {
    unsigned char uuidLen;
    char *uuid;
} IotcSleUuid;

typedef enum {
    IOTC_SPEKE_SLE_STATE_NONE         = 0x00,
    IOTC_SPEKE_SLE_STATE_INIT         = 0x01,
} SpekeState;

typedef enum {
    IOTC_CONN_SLE_STATE_NONE          = 0x00,   /*!< @if Eng SLE ACB connect state of none
                                               @else   SLE ACB 未连接状态 @endif */
    IOTC_CONN_SLE_STATE_CONNECTED     = 0x01,   /*!< @if Eng SLE ACB connect state of connected
                                               @else   SLE ACB 已连接 @endif */
    IOTC_CONN_SLE_STATE_DISCONNECTED  = 0x02,   /*!< @if Eng SLE ACB connect state of disconnected
                                               @else   SLE ACB 已断接 @endif */
} IotcConnSleState;

typedef struct {
    uint8_t devAddr[IOTC_ADPT_SLE_ADDR_LEN];
    uint16_t connID;
    IotcConnSleState status;
} SleConnRetDeviceInfo;

#ifdef __cplusplus
}
#endif

#endif /* IOTC_SLE_H */
