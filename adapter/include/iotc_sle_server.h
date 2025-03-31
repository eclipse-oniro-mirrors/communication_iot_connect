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

#ifdef __cplusplus
extern "C" {
#endif

/* 设备地址长度 */
#define IOTC_ADPT_SLE_ADDR_LEN 6
/* UUID最大长度 */
#define IOTC_ADPT_SLE_UUID_MAX_LEN 16

#define IOTC_ADPT_SLE_LINK_KEY_LEN    16

/* sle接口执行结果 */
typedef enum {
    IOTC_ADPT_SLE_STATUS_SUCCESS = 0,
    IOTC_ADPT_SLE_STATUS_FAIL = 1
} IotcAdptSleStatus;


typedef enum {
    IOTC_ADPT_SLE_PAIR_NONE    = 0x01,    /*!< @if Eng Pair state of none
                                     @else   未配对状态 @endif */
    IOTC_ADPT_SLE_PAIR_PAIRING = 0x02,    /*!< @if Eng Pair state of pairing
                                     @else   正在配对 @endif */
    IOTC_ADPT_SLE_PAIR_PAIRED  = 0x03     /*!< @if Eng Pair state of paired
                                     @else   已完成配对 @endif */
} IotcAdptSlePairState;


/**
 * @if Eng
 * @brief Enum of sle pairing state.
 * @else
 * @brief 星闪断链原因。
 * @endif
 */
typedef enum {
    IOTC_ADPT_SLE_DISCONNECT_BY_REMOTE = 0x10,    /*!< @if Eng disconnect by remote
                                             @else   远端断链 @endif */
    IOTC_ADPT_SLE_DISCONNECT_BY_LOCAL  = 0x11,    /*!< @if Eng disconnect by local
                                             @else   本端断链 @endif */
} IotcAdptSleDiscReason;

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
    IOTC_ADPT_SLE_ACB_STATE_NONE          = 0x00,   /*!< @if Eng SLE ACB connect state of none
                                               @else   SLE ACB 未连接状态 @endif */
    IOTC_ADPT_SLE_ACB_STATE_CONNECTED     = 0x01,   /*!< @if Eng SLE ACB connect state of connected
                                               @else   SLE ACB 已连接 @endif */
    IOTC_ADPT_SLE_ACB_STATE_DISCONNECTED  = 0x02,   /*!< @if Eng SLE ACB connect state of disconnected
                                               @else   SLE ACB 已断接 @endif */
} IotcAdptSleAcbState;

typedef struct {
    uint16_t interval;              /*!< @if Eng interval
                                         @else   链路调度间隔，单位slot @endif */
    uint16_t latency;               /*!< @if Eng latency
                                         @else   延迟周期，单位slot @endif */
    uint16_t supervision;           /*!< @if Eng timeout
                                         @else   超时时间，单位10ms @endif */
} IotcAdptSleConnectionParamUpdateEvt;

typedef struct {
    uint16_t intervalMin;        /*!< @if Eng minimum interval
                                       @else   链路调度最小间隔，单位slot @endif */
    uint16_t intervalMax;        /*!< @if Eng maximum interval
                                       @else   链路调度最大间隔，单位slot @endif */
    uint16_t maxLatency;         /*!< @if Eng maximum latency
                                       @else   延迟周期，单位slot @endif */
    uint16_t supervisionTimeout; /*!< @if Eng timeout
                                       @else   超时时间，单位10ms @endif */
} IotcAdptSleConnectionParamUpdateReq;

typedef struct {
    uint8_t linkKey[IOTC_ADPT_SLE_LINK_KEY_LEN];      /*!< @if Eng link key
                                                  @else   链路密钥 @endif */
    uint8_t cryptoAlgo;                     /*!< @if Eng encryption algorithm type { @ref sle_crypto_algo_t }
                                                  @else   加密算法类型 { @ref sle_crypto_algo_t } @endif */
    uint8_t keyDerivAlgo;                  /*!< @if Eng key distribution algorithm type { @ref sle_key_deriv_algo_t }
                                                  @else   秘钥分发算法类型 { @ref sle_key_deriv_algo_t } @endif */
    uint8_t integrChkInd;                  /*!< @if Eng integrity check indication { @ref sle_integr_chk_ind_t }
                                                  @else   完整性校验指示 { @ref sle_integr_chk_ind_t } @endif */
} IotcAdptSleAuthInfoEvt;

typedef struct {
    uint8_t txFormat;          /*!< @if Eng Transmitted radio frame type, @ref sle_radio_frame_t
                                     @else 发送无线帧类型，参考 { @ref sle_radio_frame_t }。 @endif */
    uint8_t rxFormat;          /*!< @if Eng Received radio frame type, @ref sle_radio_frame_t
                                     @else 接收无线帧类型，参考 { @ref sle_radio_frame_t }。 @endif */
    uint8_t txPhy;             /*!< @if Eng Transmitted PHY, @ref sle_phy_tx_rx_t
                                     @else 发送PHY，参考 { @ref sle_phy_tx_rx_t }。 @endif */
    uint8_t rxPhy;             /*!< @if Eng Received PHY, @ref sle_phy_tx_rx_t
                                     @else 接收PHY，参考 { @ref sle_phy_tx_rx_t }。 @endif */
    uint8_t txPilotDensity;   /*!< @if Eng Transmitted pilot density indicator, @ref sle_phy_tx_rx_pilot_density_t
                                     @else 发送导频密度指示，参考 { @ref sle_phy_tx_rx_pilot_density_t }。 @endif */
    uint8_t rxPilotDensity;   /*!< @if Eng Received pilot density indicator, @ref sle_phy_tx_rx_pilot_density_t
                                     @else 接收导频密度指示，参考 { @ref sle_phy_tx_rx_pilot_density_t }。 @endif */
    uint8_t gFeedback;         /*!< @if Eng Indicates the feedback type of the pre-transmitted link.
                                             The value range is 0 to 63.
                                     @else 先发链路反馈类型指示，取值范围0-63。 @endif */
    uint8_t tFeedback;         /*!< @if Eng Indicates the feedback type of the post-transmit link.
                                             The value range is 0-7.
                                     @else 后发链路反馈类型指示，取值范围0-7。 @endif */
} IotcAdptSleSetPhy;

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


typedef enum {
    IOTC_ADPT_SLE_ENABLE_EVENT,
    IOTC_ADPT_SLE_DISABLE_EVENT,
    IOTC_ADPT_SLE_ANNOUNCE_ENABLE_EVENT,
    IOTC_ADPT_SLE_ANNOUNCE_DISABLE_EVENT,
    IOTC_ADPT_SLE_ANNOUNCE_TERMINAL_EVENT,
    IOTC_ADPT_SLE_ANNOUNCE_REMOVE_EVENT,
    IOTC_ADPT_SLE_SEEK_ENABLE_EVENT,
    IOTC_ADPT_SLE_SEEK_DISABLE_EVENT,
    IOTC_ADPT_SLE_SEEK_RESULT_EVENT,
} IotcAdptSleAnnounceSeekEvent;

typedef enum {
    IOTC_ADPT_SLE_CONNECT_STATE_CHANGED_EVENT,
    IOTC_ADPT_SLE_CONNECT_PARAM_UPDATE_REQ_EVENT,
    IOTC_ADPT_SLE_CONNECT_PARAM_UPDATE_EVENT,
    IOTC_ADPT_SLE_AUTH_COMPLETE_EVENT,
    IOTC_ADPT_SLE_PAIR_COMPLETE_EVENT,
    IOTC_ADPT_SLE_READ_RSSI_EVENT,
    IOTC_ADPT_SLE_LOW_LATENCY_EVENT,
    IOTC_ADPT_SLE_SET_PHY_EVENT
} IotcAdptSleConnectionEvent;

typedef struct {
    uint8_t len;                
    uint8_t uuid[16];
    
}IotcSleUuidAddr;

typedef union {
    struct {
        int32_t announceId;
        IotcAdptSleStatus status;
    } announceEnable;
    struct {
        int32_t announceId;
        IotcAdptSleStatus status;
    } announceDisable;
    struct  {
        int32_t announceId;
    } announceTerminal;
    struct  {
        int32_t announceId;
        IotcAdptSleStatus status;
    } announceRemove;
    struct  {
        IotcAdptSleStatus status;
    } startSeek;

    struct  {
        IotcAdptSleStatus status;
    } seekDisable;

    struct  {
        uint8_t eventType;
        IotcAdptSleAddr addr;
        IotcAdptSleAddr directAddr;
        uint8_t rssi;
        uint8_t dataStatus;
        uint8_t dataLength;
        uint8_t *data;
    } seekResult;

    struct  {
        IotcAdptSleStatus status;
    } sleEnable;

    struct  {
        IotcAdptSleStatus status;
    } sleDisable;
} IotcAdptSleAnnounceSeekEventParam;

typedef union {
    struct {
        uint16_t conn_id;
        IotcAdptSleAddr addr;
        IotcAdptSleAcbState conn_state;
        IotcAdptSlePairState pairState;
        IotcAdptSleDiscReason disc_reason;
    } sleConnectStateChanged;
    struct {
        uint16_t conn_id;
        IotcAdptSleStatus status;
        IotcAdptSleConnectionParamUpdateEvt param;
    } sleConnectParamUpdate;
    struct  {
        uint16_t conn_id;
        IotcAdptSleStatus status;
        IotcAdptSleConnectionParamUpdateReq param;
    } sleConnectParamUpdateReq;
    struct  {
        uint16_t conn_id;
        IotcAdptSleAddr addr;
        IotcAdptSleStatus status;
        IotcAdptSleAuthInfoEvt evt;
    } sleAuthComplete;
    struct  {
        uint16_t conn_id;
        IotcAdptSleAddr addr;
        IotcAdptSleStatus status;
    } slePairComplete;
    struct  {
        uint16_t conn_id;
        int8_t rssi;
        IotcAdptSleStatus status;
    } sleReadRssi;
    struct  {
        uint8_t status;
        IotcAdptSleAddr addr;
        uint8_t rate;
    } sleLowLatency;
    struct  {
        uint16_t conn_id;
        IotcAdptSleStatus status;
        IotcAdptSleSetPhy param;
    } sleSetPhy;
} IotcAdptSleConnectionEventParam;

/* SSAPS事件参数列表 */
typedef union {
      struct ConnSvc {
        int32_t connId;
        int32_t serverId;
        uint8_t devAddr[IOTC_ADPT_SLE_ADDR_LEN];
    } connSvc;
    /* 断连事件 */
    struct DisconnSvc {
        int32_t connId;
        int32_t serverId;
        uint8_t devAddr[IOTC_ADPT_SLE_ADDR_LEN];
        IotcAdptSleDisconnReason reason;
    } disconnSvc;
    /* 启动服务 */
    struct StartSvc {
        IotcAdptSleStatus status;
        int32_t serverId;
        int32_t svcHandle;
    } startSvc;
    /* 停止服务 */
    struct StopSvc {
        IotcAdptSleStatus status;
        int32_t serverId;
        int32_t svcHandle;
    } stopSvc;
    /* 发送indication或者notifytion */
    struct IndicateConf {
        IotcAdptSleStatus status;
        uint32_t handle;
        uint32_t connId;
    } indicateConf;
    /* 设置MTU */
    struct  {
     IotcAdptSleStatus status;    
    uint8_t serverId;
    uint16_t connectId;
    uint32_t mtuSize; 
    uint16_t version;  
    } setMtu;
 
    /* 请求读 */
    struct  {
    uint8_t serverId;
    uint16_t connectId;
    int16_t requestId;
    uint16_t handle;
    uint8_t type;
    bool needRsp;
    bool needAuthorize;
    } reqRead;

    /* 请求写 */
    struct  {
    uint8_t serverId;
    uint16_t connectId;
    uint16_t requestId;
    uint16_t handle;
    uint8_t type;
    bool needRsp;
    bool needAuthorize;
    uint16_t valueLen;
    uint8_t *value;
    } reqWrite;
} IotcAdptSleSsapEventParam;

#define IOTC_ADPT_SLE_SSAP_READ_BUF_SIZE 520
typedef int32_t(*IotcAdptSleSsapReadFunc)(uint8_t *buff, uint32_t *len);
typedef int32_t(*IotcAdptSleSsapWriteFunc)(uint8_t *buff, uint32_t len);
typedef int32_t(*IotcAdptSleSsapCallback)(IotcAdptSleSsapEvent event, const IotcAdptSleSsapEventParam *param);
typedef int32_t(*IotcAdptSleAnnounceSeekCallback)(
    IotcAdptSleAnnounceSeekEvent event, 
    const IotcAdptSleAnnounceSeekEventParam *param
);
typedef int32_t(*IotcAdptSleConnectionCallback)(IotcAdptSleConnectionEvent event, const IotcAdptSleConnectionEventParam *param);

/* 发送indication或notification参数 */
typedef struct {
    uint16_t handle;
    uint8_t type;
    uint16_t valueLen;
    uint8_t *value;
} IotcAdptSleSendIndicateParam;


typedef struct {
    IotcSleUuidAddr uuid;
    uint16_t startHandle;
    uint16_t endHandle;
    uint8_t type;
    uint16_t valueLen;
    uint8_t *value;
} IotcAdptSleSendIndicateByUuidParam;

/* 设置发现链接参数 */
typedef struct {
    bool isDiscover;
    bool isConnect;
    bool isBond;
} IotcAdptSleConnectParam;

typedef struct {
    uint16_t requestId;
    uint32_t status;
    uint16_t valueLen;
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

#define OH_SLE_SEEK_PHY_NUM_MAX 3
typedef struct {
    uint8_t ownaddrtype;                        /*!< @if Eng own address type
                                                       @else   本端地址类型 @endif */
    uint8_t filterduplicates;                    /*!< @if Eng duplicates filter
                                                       @else   重复过滤开关，0：关闭，1：开启 @endif */
    uint8_t seekfilterpolicy;                   /*!< @if Eng scan filter policy { @ref SleSeekFilterType }
                                                       @else   扫描设备使用的过滤类型，
                                                               { @ref SleSeekFilterType } @endif */
    uint8_t seekphys;                            /*!< @if Eng scan PHY type { @ref SleSeekPhyType }
                                                       @else   扫描设备所使用的PHY，{ @ref SleSeekPhyType }
                                                       @endif */
    uint8_t seekType[OH_SLE_SEEK_PHY_NUM_MAX];      /*!< @if Eng scan type { @ref sle_seek_scan_t }
                                                       @else   扫描类型，{ @ref SleSeekType }
                                                       @endif */
    uint16_t seekInterval[OH_SLE_SEEK_PHY_NUM_MAX]; /*!< @if Eng scan interval
                                                       @else   扫描间隔，取值范围[0x0004, 0xFFFF]，time = N * 0.125ms
                                                       @endif */
    uint16_t seekWindow[OH_SLE_SEEK_PHY_NUM_MAX];   /*!< @if Eng scan window
                                                       @else   扫描窗口，取值范围[0x0004, 0xFFFF]，time = N * 0.125ms
                                                       @endif */
} IotcAdptSleSeekParam;


typedef struct {
    uint8_t  enableFilterPolicy;      /*!< @if Eng Whether the filtering function is enabled on the link
                                             @else 链路是否打开过滤功能 @endif */
    uint8_t  initiatePhys;             /*!< @if Eng Link scanning communication bandwidth: 1:1M, 2:2M
                                             @else 链路扫描通信带宽： 1:1M, 2:2M @endif */
    uint8_t  gtNegotiate;              /*!< @if Eng Whether G-T interaction is performed during link establishment
                                             @else 链路建立时是否进行G和T交互 @endif */
    uint16_t scanInterval;             /*!< @if Eng Interval for scanning the peer
                                                     device during link establishment
                                             @else 链路建立时扫描对端设备的interval @endif */
    uint16_t scanWindow;               /*!< @if Eng Scans the Windows operating system of the
                                                     peer device during link establishment.
                                             @else 链路建立时扫描对端设备的windows @endif */
    uint16_t minInterval;              /*!< @if Eng Minimum link scheduling interval
                                             @else 链路调度最小interval @endif */
    uint16_t maxInterval;              /*!< @if Eng Maximum link scheduling interval
                                             @else 链路调度最大interval @endif */
    uint16_t timeout;                   /*!< @if Eng Link Timeout Interval
                                             @else 链路超时时间 @endif */
} IotcAdptSleDefaultConnectParam;

/**
 * @brief Initialize the Ssap server
 *
 * @param None
 * @return SleErrorCode
 */
uint8_t IotcInitSleSsapsService(void);


/**
 * @brief Initialize the Ssap server
 *
 * @param None
 * @return SleErrorCode
 */
uint8_t IotcDeinitSleSsapsService(void);


/**
 * @brief 开启SSAP服务
 *
 * @param svc [IN] 服务表
 * @param svcNum [IN] 服务数量
 * @return 0成功，非0失败
 */
uint8_t IotcSleSsapsStartService(uint8_t serviceId, uint16_t serviceHandle);


/**
 * @brief 发送SSAP数据
 *
 * @param param [IN] 参数
 * @return 0成功，非0失败
 */
uint8_t IotcSleSendSsapsIndicate(uint8_t serverId, uint16_t connectId, const IotcAdptSleSendIndicateParam *param);

/**
 * @brief 发送SSAP数据
 *
 * @param param [IN] 参数
 * @return 0成功，非0失败
 */
uint8_t IotcSleSendSsapsIndicateByUuid(uint8_t serverId, uint16_t connectId, const IotcAdptSleSendIndicateByUuidParam *param);

/**
 * @brief 发送SSAP数据
 *
 * @param param [IN] 参数
 * @return 0成功，非0失败  (uint8_t serverId, uint16_t connectId, const SsapsSendRspParam *rspParam
 */
uint8_t IotcSleSendSsapsResponse(uint8_t serverId, uint16_t connectId, const IotcAdptSleResponseParam *param);


uint8_t IotcSsapsAddProperty(uint8_t serviceId, uint16_t serviceHandle, IotcAdptSleSsapsPropertyInfo *property, uint16_t *handle);

uint8_t IotcSsapsAddDescriptor(uint8_t serverId, uint16_t serviceHandle, uint16_t propHandle, const IotcAdptSleSsapsDescInfo *descParam, uint16_t *descHandle);

uint8_t IotcSsapsAddService(uint8_t serviceId, IotcSleUuidAddr *serviceUuid, bool isPrimary, uint16_t *handle);

uint8_t IotcSleSsapsRegisterServer(const IotcAdptSleSsapCallback callback);

uint8_t IotcSleSsapsUnregisterServer(const IotcAdptSleSsapCallback callback);

uint8_t IotcSsapsDeleteAllServices(uint8_t serviceId);

/**
 * @brief Remove a Ssap server
 *
 * @param [in] serverId The ID of the server
 * @return SleErrorCode
 */
uint8_t IotcSsapsRemoveSsapServer(uint8_t serverId);

/**
 * @brief Add a Ssap server
 *
 * @param [in] appUuid The UUID of the server application
 * @param [out] serverId The ID of the server
 * @return SleErrorCode
 */
uint8_t IotcAddSsapServer(const IotcSleUuidAddr *appUuid, uint8_t *serverId);

/**
 * @brief Set the MTU of the connection
 *
 * @param   [in] serverId The ID of the server
 * @param   [in] connectId The ID of the connection
 * @param   [in] mtuInfo The MTU info of the connection
 * @return  SleErrorCode
 */
uint8_t IotcAddSsapSetServerMtuInfo(uint8_t serverId, const IotcAdptSleMtuInfo *mtuInfo);

int32_t IotcSleRegisterAnnounceSeekCallbacks(const IotcAdptSleAnnounceSeekCallback callback);

int32_t IotcSleRegisterConnectionCallbacks(const IotcAdptSleConnectionCallback callback);

#ifdef __cplusplus
}
#endif

#endif /* IOTC_ADPT_SLE_H */