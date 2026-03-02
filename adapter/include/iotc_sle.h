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
#ifndef IOTC_ADPT_SLE_H
#define IOTC_ADPT_SLE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 设备地址长度 */
#define IOTC_ADPT_SLE_ADDR_LEN 6
/* UUID最大长度 */
#define IOTC_ADPT_SLE_UUID_MAX_LEN 16

/* sle接口执行结果 */
typedef enum {
    IOTC_ADPT_SLE_STATUS_SUCCESS = 0,
    IOTC_ADPT_SLE_STATUS_FAIL = 1
} IotcAdptSleStatus;

/* SSAP断链原因 */
typedef enum {
    IOTC_ADPT_SLE_SSAP_UNKNOWN_REASON
} IotcAdptSleDisconnReason;

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

/* 广播信道 */
typedef enum {
    IOTC_ADPT_SLE_CHNL_37 = 0x01,
    IOTC_ADPT_SLE_CHNL_38 = 0x02,
    IOTC_ADPT_SLE_CHNL_39 = 0x04,
    IOTC_ADPT_SLE_CHNL_ALL = 0x07,
} IotcAdptSleAnnounceChanel;

/* 广播地址*/
typedef struct {
    uint8_t type;
    unsigned char addr[IOTC_ADPT_SLE_ADDR_LEN];
} IotcAdptSleAddr;

/* 广播等级 */
typedef enum {
    SLE_ANNOUNCE_NONE_LEVEL,
    SLE_ANNOUNCE_NORMAL_LEVEL,
    SLE_ANNOUNCE_PRIORITY_LEVEL,
    SLE_ANNOUNCE_PAIRED_LEVEL,
    SLE_ANNOUNCE_SPECIAL_LEVEL,
} IotcAdptSleAnnounceLevel;

/* 设备公开类型 */
typedef enum {
    SLE_ANNOUNCE_NONCONN_NONSCAN_MODE      = 0x00,
    SLE_ANNOUNCE_CONNECTABLE_NONSCAN_MODE  = 0x01,
    SLE_ANNOUNCE_NONCONN_SCANABLE_MODE     = 0x02,
    SLE_ANNOUNCE_CONNECTABLE_SCANABLE_MODE = 0x03,
    SLE_ANNOUNCE_CONNECTABLE_DIRECTED_MODE = 0x07,
} IotcAdptSleAnnounceMode;

/* 角色协商指示 */
typedef enum {
    SLE_ANNOUNCE_T_ROAL_CAN_NEGO = 0,
    SLE_ANNOUNCE_G_ROAL_CAN_NEGO,
    SLE_ANNOUNCE_T_ROAL_NO_NEGO,
    SLE_ANNOUNCE_G_ROAL_NO_NEGO
} IotcAdptSleAnnounceRole;

typedef struct {
    uint8_t  announceHandle;              /*!< @if Eng announce handle
                                                 @else   设备公开句柄，取值范围[0, 0xFF] @endif */
    uint8_t  announceMode;                /*!< @if Eng announce mode { @ref SleAnnounceModeType }
                                                 @else   设备公开类型， { @ref SleAnnounceModeType } @endif */
    uint8_t  announceGtRole;             /*!< @if Eng G/T role negotiation indication
                                                         { @ref SleAnnounceGtRoleType }
                                                 @else   G/T 角色协商指示，
                                                         { @ref SleAnnounceGtRoleType } @endif */
    uint8_t  announceLevel;               /*!< @if Eng announce level
                                                         { @ref SleAnnounceLevelType }
                                                 @else   发现等级，
                                                         { @ref SleAnnounceLevelType } @endif */
    uint32_t announceIntervalMin;        /*!< @if Eng minimum of announce interval
                                                 @else   最小设备公开周期, 0x000020~0xffffff, 单位125us @endif */
    uint32_t announceIntervalMax;        /*!< @if Eng maximum of announce interval
                                                 @else   最大设备公开周期, 0x000020~0xffffff, 单位125us @endif */
    uint8_t  announceChannelMap;         /*!< @if Eng announce channel map
                                                 @else   设备公开信道, 0:76, 1:77, 2:78 @endif */

    uint8_t announceTxPower;

    IotcAdptSleAddr ownAddr;                    /*!< @if Eng own address
                                                 @else   本端地址 @endif */
    IotcAdptSleAddr peerAddr;                   /*!< @if Eng peer address
                                                 @else   对端地址 @endif */
    uint16_t connIntervalMin;             /*!< @if Eng minimum of connection interval
                                                 @else   连接间隔最小取值，取值范围[0x001E,0x3E80]，
                                                         announce_gt_role 为 OH_SLE_ANNOUNCE_ROLE_T_NO_NEGO
                                                         时无需配置 @endif */
    uint16_t connIntervalMax;             /*!< @if Eng maximum of connection interval
                                                 @else   连接间隔最大取值，取值范围[0x001E,0x3E80]，
                                                         announce_gt_role 为 OH_SLE_ANNOUNCE_ROLE_T_NO_NEGO
                                                         无需配置 @endif */
    uint16_t connMaxLatency;              /*!< @if Eng max connection latency
                                                 @else   最大休眠连接间隔，取值范围[0x0000,0x01F3]，
                                                         announce_gt_role 为 OH_SLE_ANNOUNCE_ROLE_T_NO_NEGO
                                                         无需配置 @endif */
    uint16_t connSupervisionTimeout;      /*!< @if Eng connect supervision timeout
                                                 @else   最大超时时间，取值范围[0x000A,0x0C80]，
                                                         announce_gt_role 为 OH_SLE_ANNOUNCE_ROLE_T_NO_NEGO
                                                         无需配置 @endif */
    void *extParam;                        /*!< @if Eng extend parameter, default value is NULL
                                                 @else   扩展设备公开参数, 缺省时置空 @endif */
} IotcAdptSleAdvParam;

#define IOTC_ADPT_SLE_ADV_VALUE_MAX_LEN 31
/* 广播数据 */
typedef struct {
    uint16_t announceDataLen; /*!< @if Eng announce data length
                                      @else   设备公开数据长度 @endif */
    uint16_t seekRspDataLen;  /*!< @if Eng scan response data length
                                      @else   扫描响应数据长度 @endif */
    uint8_t  *announceData;    /*!< @if Eng announce data
                                      @else   设备公开数据 @endif */
    uint8_t  *seekRspData;     /*!< @if Eng scan response data
                                      @else   扫描响应数据 @endif */
} IotcAdptSleAdvData;

/**
 * @if Eng
 * @brief Struct of read request information.
 * @else
 * @brief 读请求信息。
 * @endif
 */
typedef struct {
    uint16_t requestId;  /*!< @if Eng Request id.
                               @else   请求id。 @endif */
    uint16_t handle;      /*!< @if Eng Properity handle of the read request.
                               @else   请求读的属性句柄。 @endif */
    uint8_t type;         /*!< @if Eng property type { @ref ssap_property_type_t }.
                               @else   属性类型。 @endif  { @ref ssap_property_type_t } */
    bool needRsp;        /*!< @if Eng Whether response is needed.
                               @else   是否需要发送响应。 @endif */
    bool needAuthorize;  /*!< @if Eng Whether authorize is needed.
                               @else   是否授权。 @endif */
} IotcAdptSleReqRead;

/**
 * @if Eng
 * @brief  Struct of ssap info exchange
 * @else
 * @brief  ssap 信息交换结构体。
 * @endif
 */
typedef struct {
    uint32_t mtuSize; /*!< @if Eng mtu size
        ·                   @else   mtu大小 @endif */
    uint16_t version;  /*!< @if Eng version
                            @else   版本 @endif */
} IotcAdptSleExchangeInfo;

/**
 * @if Eng
 * @brief Struct of write request information.
 * @else
 * @brief 写请求信息。
 * @endif
 */
typedef struct {
    uint16_t requestId;  /*!< @if Eng Request id.
                               @else   请求id。 @endif */
    uint16_t handle;      /*!< @if Eng Properity handle of the write request.
                               @else   请求写的属性句柄。 @endif */
    uint8_t type;         /*!< @if Eng property type { @ref ssap_property_type_t }.
                               @else   属性类型。 @endif  { @ref ssap_property_type_t } */
    bool needRsp;        /*!< @if Eng Whether response is needed.
                               @else   是否需要发送响应。 @endif */
    bool needAuthorize;  /*!< @if Eng Whether authorize is needed.
                               @else   是否授权。 @endif */
    uint16_t length;      /*!< @if Eng Length of write request data.
                               @else   请求写的数据长度。 @endif */
    uint8_t *value;       /*!< @if Eng Write request data.
                               @else   请求写的数据。 @endif */
} IotcAdptSleReqWrite;

/* SSAP事件列表 */
typedef enum {
    IOTC_ADPT_SLE_SSAP_EVENT_CONNECT,
    IOTC_ADPT_SLE_SSAP_EVENT_DISCONNECT,
    IOTC_ADPT_SLE_SSAP_EVENT_START_SVC_RESULT,
    IOTC_ADPT_SLE_SSAP_EVENT_STOP_SVC_RESULT,
    IOTC_ADPT_SLE_SSAP_EVENT_INDICATE_CONF,
    IOTC_ADPT_SLE_SSAP_EVENT_SET_MTU_RESULT,
    IOTC_ADPT_SLE_SSAP_EVENT_START_ADV_RESULT,
    IOTC_ADPT_SLE_SSAP_EVENT_STOP_ADV_RESULT,
    IOTC_ADPT_SLE_SSAP_EVENT_REQ_READ,
    IOTC_ADPT_SLE_SSAP_EVENT_REQ_WRITE
} IotcAdptSleSsapEvent;

/* SSAPS事件参数列表 */
typedef union {
    /* 连接事件 */
    struct  {
        int32_t connId;
        int32_t serverId;
        uint8_t devAddr[IOTC_ADPT_SLE_ADDR_LEN];
    } connSvc;
    /* 断连事件 */
    struct  {
        int32_t connId;
        int32_t serverId;
        uint8_t devAddr[IOTC_ADPT_SLE_ADDR_LEN];
        IotcAdptSleDisconnReason reason;
    } disconnSvc;
    /* 启动服务 */
    struct  {
        IotcAdptSleStatus status;
        int32_t serverId;
        int32_t svcHandle;
    } startSvc;
    /* 停止服务 */
    struct  {
        IotcAdptSleStatus status;
        int32_t serverId;
        int32_t svcHandle;
    } stopSvc;
    /* 发送indication或者notifytion */
    struct  {
        IotcAdptSleStatus status;
        uint32_t handle;
        uint32_t connId;
    } indicateConf;
    /* 设置MTU */
    struct  {
        IotcAdptSleStatus status;
        uint32_t connId;
        uint16_t mtu;
    } setMtu;
    /* 开启广播参数回调参数 */
    struct  {
        IotcAdptSleStatus status;
    } startAdv;
    /* 停止广播参数回调参数 */
    struct  {
        IotcAdptSleStatus status;
    } stopAdv;
    /* 请求读 */
    struct  {
        int32_t connId;
        int32_t attrHandle;
        int32_t transId;
    } reqRead;
    /* 请求写 */
    struct  {
        int32_t connId;
        int32_t attrHandle;
        int32_t transId;
        uint8_t *value;
        int32_t valueLen;
    } reqWrite;
} IotcAdptSleSsapEventParam;

#define IOTC_ADPT_SLE_SSAP_READ_BUF_SIZE 520
typedef int32_t(*IotcAdptSleSsapReadFunc)(uint8_t *buff, uint32_t *len);
typedef int32_t(*IotcAdptSleSsapWriteFunc)(uint8_t *buff, uint32_t len);
typedef int32_t(*IotcAdptSleSsapCallback)(IotcAdptSleSsapEvent event, const IotcAdptSleSsapEventParam *param);

/* 发送indication或notification参数 */
typedef struct {
    uint16_t connId;
    uint8_t  serverId;
    uint16_t handle;
    uint8_t type;
    uint16_t valueLen;
    uint8_t *value;
} IotcAdptSleSendIndicateParam;

/* 设置发现链接参数 */
typedef struct {
    bool isDiscover;
    bool isConnect;
    bool isBond;
} IotcAdptSleConnectParam;

typedef struct {
    int32_t serverId;
    int32_t connectId;
    int32_t transId;
    int32_t valueLen;
    uint8_t *value;
} IotcAdptSleResponseParam;


typedef struct {
    const char *uuid;
    uint32_t permission;
    int32_t(*readFunc)(uint8_t *buff, uint32_t *len);
    int32_t(*writeFunc)(uint8_t *buff, uint32_t len);
    int32_t descHandle;
} IotcAdptSleSsapCharDesc;

typedef struct {
    const char *uuid;
    uint32_t permission;
    uint32_t property;
    int32_t(*readFunc)(uint8_t *buff, uint32_t *len);
    int32_t(*writeFunc)(uint8_t *buff, uint32_t len);
    int32_t(*indicateFunc)(uint8_t *buff, uint32_t len);
    IotcAdptSleSsapCharDesc *desc;
    uint32_t descNum;
    int32_t charHandle;
} IotcAdptSleSsapsChar;

typedef struct {
    const char *uuid;
    IotcAdptSleSsapsChar *character;
    uint8_t charNum;
    int32_t svcHandle;
    int32_t serverId;
} IotcAdptSleSsapService;

/**
 * @brief 初始化协议栈
 *
 * @return 0成功，非0失败
 */
int32_t IotcSleInitStack(void);

/**
 * @brief 设置蓝牙连接参数
 *
 * @param param [IN] 参数
 * @return 0成功，非0失败
 */
int32_t IotcSleSetConnectParam(const IotcAdptSleConnectParam *param);

/**
 * @brief 注册SSAP回调
 *
 * @param callback [IN] 回调
 * @return 0成功，非0失败
 */
int32_t IotcSleRegisterSsapCb(const IotcAdptSleSsapCallback callback);

/**
 * @brief 设置蓝牙名字
 *
 * @param name [IN] 名字
 * @return 0成功，非0失败
 */
int32_t IotcSleSetBleName(const char *name);

/**
 * @brief 开启广播
 *
 * @param advParam [IN] 广播参数
 * @param advData [IN] 广播数据
 * @return 0成功，非0失败
 */
int32_t IotcSleStartAdv(const IotcAdptSleAdvParam *advParam, const IotcAdptSleAdvData *advData);

/**
 * @brief 停止广播
 *
 * @return 0成功，非0失败
 */
int32_t IotcSleStopAdv();

/**
 * @brief 开启SSAP服务
 *
 * @param svc [IN] 服务表
 * @param svcNum [IN] 服务数量
 * @return 0成功，非0失败
 */
int32_t IotcSleStartSsapsService(IotcAdptSleSsapService *svc, uint8_t svcNum);


/**
 * @brief 发送SSAP数据
 *
 * @param param [IN] 参数
 * @return 0成功，非0失败
 */
int32_t IotcSleSendSsapsIndicate(const IotcAdptSleSendIndicateParam *param);

/**
 * @brief 发送SSAP回应
 *
 * @param param [IN] 参数
 * @return 0成功，非0失败
 */
int32_t IotcSleSendSsapsResponse(const IotcAdptSleResponseParam *param);

/**
 * @brief 获取蓝牙mac地址
 *
 * @param mac [OUT] mac输出缓存
 * @param len [IN] mac输出缓存长度
 * @return 0成功，非0失败
 */
IotcAdptSleAddr* IotSleGetLocalSleAddress(void);

/**
 * @brief 端口与客户端的连接
 *
 * @param bdAddr [IN] 客户端地址
 * @param addrLen [IN] 地址长度
 * @return 0成功，非0失败
 */
int32_t IotcSleDisconnectSsap(const uint8_t *bdAddr, uint32_t addrLen);

/**
 * @brief 销毁蓝牙协议栈
 *
 * @return 0成功，非0失败
 */
int32_t IotcSleDeInitStack(void);

#ifdef __cplusplus
}
#endif

#endif /* IOTC_ADPT_SLE_H */