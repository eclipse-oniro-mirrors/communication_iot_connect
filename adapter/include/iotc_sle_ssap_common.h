
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "iotc_sle_host.h"

#define IOTC_ADPT_SLE_ADDR_COMMON_LEN 6
#define IOTC_ADPT_SLE_LINK_KEY_LEN    16

/* SSAP断链原因 */
typedef enum {
    IOTC_ADPT_SLE_SSAP_UNKNOWN_REASON
} IotcAdptSleDisconnReason;
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

typedef enum {
    IOTC_ADPT_SLE_PAIR_NONE    = 0x01,    /*!< @if Eng Pair state of none
                                     @else   未配对状态 @endif */
    IOTC_ADPT_SLE_PAIR_PAIRING = 0x02,    /*!< @if Eng Pair state of pairing
                                     @else   正在配对 @endif */
    IOTC_ADPT_SLE_PAIR_PAIRED  = 0x03     /*!< @if Eng Pair state of paired
                                     @else   已完成配对 @endif */
} IotcAdptSlePairState;

typedef struct {
    uint16_t interval;              /*!< @if Eng interval
                                         @else   链路调度间隔，单位slot @endif */
    uint16_t latency;               /*!< @if Eng latency
                                         @else   延迟周期，单位slot @endif */
    uint16_t supervision;           /*!< @if Eng timeout
                                         @else   超时时间，单位10ms @endif */
} IotcAdptSleConnectionParamUpdateEvt;

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
    const char *uuid;
    uint32_t permission;
    int32_t(*readFunc)(uint32_t connId, uint8_t *buff, uint32_t *len);
    int32_t(*writeFunc)(uint32_t connId, uint8_t *buff, uint32_t len);
    int32_t descHandle;
} IotcAdptSleSsapCharDesc;

typedef struct {
    const char *uuid;
    uint32_t permission;
    uint32_t property;
    int32_t(*readFunc)(uint32_t connId, uint8_t *buff, uint32_t *len);
    int32_t(*writeFunc)(uint32_t connId, uint8_t *buff, uint32_t len);
    int32_t(*indicateFunc)(uint32_t connId, uint8_t *buff, uint32_t len);
    IotcAdptSleSsapCharDesc *desc;
    uint32_t descNum;
    int32_t charHandle;
} IotcAdptSleSsapsChar;

typedef struct {
    const char *uuid;
    IotcAdptSleSsapsChar *character;
    uint8_t charNum;
    int32_t svcHandle;
    uint8_t serverId;
} IotcAdptSleSsapService;

typedef union {
    struct ConnSvc {
        int32_t connId;
        int32_t serverId;
        uint8_t devAddr[IOTC_ADPT_SLE_ADDR_COMMON_LEN];
    } connSvc;
    /* 断连事件 */
    struct DisconnSvc {
        int32_t connId;
        int32_t serverId;
        uint8_t devAddr[IOTC_ADPT_SLE_ADDR_COMMON_LEN];
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
        IotcAdptSleDeviceAddr addr;
        IotcAdptSleDeviceAddr directAddr;
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

typedef union {
    struct {
        uint16_t conn_id;
        IotcAdptSleDeviceAddr addr;
        IotcAdptSleAcbState conn_state;
        IotcAdptSlePairState pair_state;
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
        IotcAdptSleDeviceAddr addr;
        IotcAdptSleStatus status;
        IotcAdptSleAuthInfoEvt evt;
    } sleAuthComplete;
    struct  {
        uint16_t conn_id;
        IotcAdptSleDeviceAddr addr;
        IotcAdptSleStatus status;
    } slePairComplete;
    struct  {
        uint16_t conn_id;
        int8_t rssi;
        IotcAdptSleStatus status;
    } sleReadRssi;
    struct  {
        uint8_t status;
        IotcAdptSleDeviceAddr addr;
        uint8_t rate;
    } sleLowLatency;
    struct  {
        uint16_t conn_id;
        IotcAdptSleStatus status;
        IotcAdptSleSetPhy param;
    } sleSetPhy;
} IotcAdptSleConnectionEventParam;

typedef struct {
    uint16_t requestId;
    uint32_t status;
    uint16_t valueLen;
    uint8_t *value;
} IotcAdptSleResponseParam;

/* 发送indication或notification参数 */
typedef struct {
    uint16_t handle;
    uint8_t type;
    uint16_t valueLen;
    uint8_t *value;
} IotcAdptSleSendIndicateParam;

typedef int32_t(*IotcAdptSleSsapCallback)(IotcAdptSleSsapEvent event, const IotcAdptSleSsapEventParam *param);
typedef int32_t(*IotcAdptSleAnnounceSeekCallback)(IotcAdptSleAnnounceSeekEvent event, const IotcAdptSleAnnounceSeekEventParam *param);
typedef int32_t(*IotcAdptSleConnectionCallback)(IotcAdptSleConnectionEvent event, const IotcAdptSleConnectionEventParam *param);