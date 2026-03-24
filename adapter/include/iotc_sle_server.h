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

#ifndef IOTC_ADPT_SLE_SERVER_H
#define IOTC_ADPT_SLE_SERVER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "iotc_sle_host.h"
#include "iotc_sle_ssap_common.h"
#ifdef __cplusplus
extern "C" {
#endif


/* UUID最大长度 */
#define IOTC_ADPT_SLE_UUID_MAX_LEN 16

typedef struct {
    uint8_t type;
    unsigned char addr[IOTC_ADPT_SLE_ADDR_LEN];
} IotcAdptSleAddr;
/**
 * @if Eng
 * @brief Enum of sle ACB connection state.
 * @else
 * @brief SLE ACB连接状态。
 * @endif
 */
typedef enum {
    OH_SLE_ACB_STATE_NONE          = 0x00,   /*!< @if Eng SLE ACB connect state of none
                                               @else   SLE ACB 未连接状态 @endif */
    OH_SLE_ACB_STATE_CONNECTED     = 0x01,   /*!< @if Eng SLE ACB connect state of connected
                                               @else   SLE ACB 已连接 @endif */
    OH_SLE_ACB_STATE_DISCONNECTED  = 0x02,   /*!< @if Eng SLE ACB connect state of disconnected
                                               @else   SLE ACB 已断接 @endif */
} IotcAdptSleAcbStateType;

/**
 * @if Eng
 * @brief Enum of sle pairing state.
 * @else
 * @brief 星闪配对状态。
 * @endif
 */
typedef enum {
    OH_SLE_PAIR_NONE    = 0x01,    /*!< @if Eng Pair state of none
                                     @else   未配对状态 @endif */
    OH_SLE_PAIR_PAIRING = 0x02,    /*!< @if Eng Pair state of pairing
                                     @else   正在配对 @endif */
    OH_SLE_PAIR_PAIRED  = 0x03     /*!< @if Eng Pair state of paired
                                     @else   已完成配对 @endif */
} IotcAdptSlePairStateType;

/* sle_announce_mode_t
设备公开类型。 Enumerator
SLE_ANNOUNCE_MODE_NONCONN_NONSCAN  不可连接不可扫描。
SLE_ANNOUNCE_MODE_CONNECTABLE_NONSCAN  可连接不可扫描。
SLE_ANNOUNCE_MODE_NONCONN_SCANABLE  不可连接可扫描。
SLE_ANNOUNCE_MODE_CONNECTABLE_SCANABLE  可连接可扫描。
SLE_ANNOUNCE_MODE_CONNECTABLE_DIRECTED  可连接可扫描定向。 */
/* 广播类型 */
typedef enum {
    IOTC_ADPT_SLE_ANNOUNCE_MODE_NONCONN_NONSCAN = 0x00,
    IOTC_ADPT_SLE_ANNOUNCE_MODE_CONNECTABLE_NONSCAN = 0x01,
    IOTC_ADPT_SLE_ANNOUNCE_MODE_NONCONN_SCANABLE = 0x02,
    IOTC_ADPT_SLE_ANNOUNCE_MODE_CONNECTABLE_SCANABLE = 0x03,
    IOTC_ADPT_SLE_ANNOUNCE_MODE_CONNECTABLE_DIRECTED = 0x04,
} IotcAdptSleAdvType;

/* 广播地址类型 */
typedef enum {
    IOTC_ADPT_SLE_ADV_ADDR_PUBLIC = 0x00,
    IOTC_ADPT_SLE_ADV_ADDR_RANDOM = 0x01,
    IOTC_ADPT_SLE_ADV_ADDR_PUBLIC_ID = 0x02,
    IOTC_ADPT_SLE_ADV_ADDR_RANDOM_ID = 0x03,
    IOTC_ADPT_SLE_ADV_ADDR_UNKNOWN_TYPE = 0xFF,
} IotcAdptSleAdvAddr;

/* SSAPS char 属性取�? */
typedef enum {
    IOTC_ADPT_SLE_CHAR_PROP_BROADCAST = 0x01,
    IOTC_ADPT_SLE_CHAR_PROP_READ = 0x02,
    IOTC_ADPT_SLE_CHAR_PROP_WRITE_WITHOUT_RESP = 0x04,
    IOTC_ADPT_SLE_CHAR_PROP_WRITE = 0x08,
    IOTC_ADPT_SLE_CHAR_PROP_NOTIFY = 0x10,
    IOTC_ADPT_SLE_CHAR_PROP_INDICATE = 0x20,
    IOTC_ADPT_SLE_CHAR_PROP_SIGNED_WRITE = 0x40,
    IOTC_ADPT_SLE_CHAR_PROP_EXTENDED_PROPERTY = 0x80,
} IotcAdptSleCharProperty;

/* SSAPS char 权限取�? */
typedef enum {
    IOTC_ADPT_SLE_CHAR_PERM_READ = 0x01,
    IOTC_ADPT_SLE_CHAR_PERM_READ_ENCRYPTED = 0x02,
    IOTC_ADPT_SLE_CHAR_PERM_READ_ENCRYPTED_MITM = 0x04,
    IOTC_ADPT_SLE_CHAR_PERM_WRITE = 0x10,
    IOTC_ADPT_SLE_CHAR_PERM_WRITE_ENCRYPTED = 0x20,
    IOTC_ADPT_SLE_CHAR_PERM_WRITE_ENCRYPTED_MITM = 0x40,
    IOTC_ADPT_SLE_CHAR_PERM_WRITE_SIGNED = 0x80,
    IOTC_ADPT_SLE_CHAR_PERM_WRITE_SIGNED_MITM = 0x100,
} IotcAdptSleCharPermission;


/**
 * @if Eng
 * @brief Struct of read request information.
 * @else
 * @brief 读请求信息。
 * @endif
 */
typedef struct {
    uint16_t requestId;
    uint16_t handle;
    uint8_t type;
    bool needRsp;
    bool needAuthorize;
} IotcAdptSleReqRead;

/**
 * @if Eng
 * @brief  Struct of ssap info exchange
 * @else
 * @brief  ssap 信息交换结构体。
 * @endif
 */
typedef struct {
    uint32_t mtuSize;
    uint16_t version;
} IotcAdptSleMtuInfo;

/**
 * @if Eng
 * @brief Struct of write request information.
 * @else
 * @brief 写请求信息。
 * @endif
 */
typedef struct {
    uint16_t requestId;
    uint16_t handle;
    uint8_t type;
    bool needRsp;
    bool needAuthorize;
    uint16_t valueLen;
    uint8_t *value;
} IotcAdptSleReqWrite;


typedef struct {
    uint8_t len;
    uint8_t uuid[16];
}IotcSleUuidAddr;


#define IOTC_ADPT_SLE_SSAP_READ_BUF_SIZE 520
typedef int32_t(*IotcAdptSleSsapReadFunc)(uint32_t connId, uint8_t *buff, uint32_t *len);
typedef int32_t(*IotcAdptSleSsapWriteFunc)(uint32_t connId, uint8_t *buff, uint32_t len);


typedef struct {
    IotcSleUuidAddr uuid;
    uint16_t startHandle;
    uint16_t endHandle;
    uint8_t type;
    uint16_t valueLen;
    uint8_t *value;
} IotcAdptSleSendIndicateByUuidParam;

/**
 * @if Eng
 * @brief Struct of add property information.
 * @else
 * @brief 添加特征信息。
 * @endif
 */
typedef struct {
    IotcSleUuidAddr uuid;             /*!< @if Eng UUID of SSAP property.
                                      @else   SSAP 特征 UUID。 @endif */
    uint16_t permissions;        /*!< @if Eng Properity permissions. { @ref SsapPermissionType }
                                      @else   特征权限。{ @ref SsapPermissionType }。 @endif */
    uint32_t operateIndication; /*!< @if Eng Operate Indication. { @ref SsapOperateIndication }
                                      @else   操作指示 { @ref SsapOperateIndication } @endif */
    uint16_t valueLen;          /*!< @if Eng Length of reponse data.
                                      @else   响应的数据长度。 @endif */
    uint8_t *value;              /*!< @if Eng Reponse data.
                                      @else   响应的数据。 @endif */
} IotcAdptSleSsapsPropertyInfo;

typedef struct {
    IotcSleUuidAddr uuid;             /*!< @if Eng UUID of SSAP descriptor.
                                      @else   SSAP 描述符 UUID。 @endif */
    uint16_t permissions;        /*!< @if Eng descriptor permissions. { @ref SsapPermissionType }.
                                      @else   特征权限。 { @ref SsapPermissionType } @endif */
    uint32_t operateInd; /*!< @if Eng operate Indication. { @ref SsapOperateIndication }
                                      @else   操作指示 { @ref SsapOperateIndication } @endif */
    uint8_t type;                /*!< @if Eng descriptor type. { @ref SsapPropertyType }.
                                      @else   描述符类型。 { @ref SsapPropertyType } @endif */
    uint16_t valueLen;          /*!< @if Eng data length.
                                      @else   数据长度。 @endif */
    uint8_t *value;              /*!< @if Eng data.
                                      @else   数据。 @endif */
} IotcAdptSleSsapsDescInfo;

/**
 * @brief Initialize the Ssap server
 *
 * @param None
 * @return SleErrorCode
 */
int32_t IotcInitSleSsapsService(void);


/**
 * @brief Initialize the Ssap server
 *
 * @param None
 * @return SleErrorCode
 */
int32_t IotcDeinitSleSsapsService(void);


/**
 * @brief 开启SSAP服务
 *
 * @param svc [IN] 服务表
 * @param svcNum [IN] 服务数量
 * @return 0成功，非0失败
 */
int32_t IotcSleSsapsStartService(uint8_t serviceId, uint16_t serviceHandle);

int32_t IotcSleSsapsStartServiceExt(IotcAdptSleSsapService *svc, uint8_t svcNum);

/**
 * @brief 发送SSAP数据
 *
 * @param param [IN] 参数
 * @return 0成功，非0失败
 */
int32_t IotcSleSendSsapsIndicate(uint8_t serverId, uint16_t connectId, const IotcAdptSleSendIndicateParam *param);

/**
 * @brief 发送SSAP数据
 *
 * @param param [IN] 参数
 * @return 0成功，非0失败
 */
int32_t IotcSleSendSsapsIndicateByUuid(uint8_t serverId, uint16_t connectId,
    const IotcAdptSleSendIndicateByUuidParam *param);

/**
 * @brief 发送SSAP数据
 *
 * @param param [IN] 参数
 * @return 0成功，非0失败  (uint8_t serverId, uint16_t connectId, const SsapsSendRspParam *rspParam
 */
int32_t IotcSleSendSsapsResponse(uint8_t serverId, uint16_t connectId, const IotcAdptSleResponseParam *param);


int32_t IotcSsapsAddProperty(uint8_t serviceId, uint16_t serviceHandle,
    IotcAdptSleSsapsPropertyInfo *property, uint16_t *handle);

int32_t IotcSsapsAddDescriptor(uint8_t serverId, uint16_t serviceHandle, uint16_t propHandle,
    const IotcAdptSleSsapsDescInfo *descParam, uint16_t *descHandle);

int32_t IotcSsapsAddService(uint8_t serviceId, IotcSleUuidAddr *serviceUuid, bool isPrimary, uint16_t *handle);

int32_t IotcSleSsapsRegisterServer(const IotcAdptSleSsapCallback callback);

int32_t IotcSleSsapsUnregisterServer(const IotcAdptSleSsapCallback callback);

int32_t IotcSsapsDeleteAllServices(uint8_t serviceId);

/**
 * @brief Remove a Ssap server
 *
 * @param [in] serverId The ID of the server
 * @return SleErrorCode
 */
int32_t IotcSsapsRemoveSsapServer(uint8_t serverId);

/**
 * @brief Add a Ssap server
 *
 * @param [in] appUuid The UUID of the server application
 * @param [out] serverId The ID of the server
 * @return SleErrorCode
 */
int32_t IotcAddSsapServer(const IotcSleUuidAddr *appUuid, uint8_t *serverId);

/**
 * @brief Set the MTU of the connection
 *
 * @param   [in] serverId The ID of the server
 * @param   [in] connectId The ID of the connection
 * @param   [in] mtuInfo The MTU info of the connection
 * @return  SleErrorCode
 */
int32_t IotcAddSsapSetServerMtuInfo(uint8_t serverId, const IotcAdptSleMtuInfo *mtuInfo);

int32_t IotcSleRegisterAnnounceSeekCallbacks(const IotcAdptSleAnnounceSeekCallback callback);

int32_t IotcSleRegisterConnectionCallbacks(const IotcAdptSleConnectionCallback callback);

int32_t IotcSleStartSeek(void);

int32_t IotcSleStoptSeek(void);

#ifdef __cplusplus
}
#endif

#endif /* IOTC_ADPT_SLE_H */