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
#ifndef IOTC_ADPT_BLE_H
#define IOTC_ADPT_BLE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 设备地址长度 */
#define IOTC_ADPT_BLE_ADDR_LEN 6
/* UUID最大长度 */
#define IOTC_ADPT_BLE_UUID_MAX_LEN 16

/* ble接口执行结果 */
typedef enum {
    IOTC_ADPT_BLE_STATUS_SUCCESS = 0,
    IOTC_ADPT_BLE_STATUS_FAIL = 1
} IotcAdptBleStatus;

/* GATT断链原因 */
typedef enum {
    IOTC_ADPT_BLE_GATT_UNKNOWN_REASON
} IotcAdptBleDisconnReason;

/* 广播类型 */
typedef enum {
    IOTC_ADPT_BLE_ADV_TYPE_IND = 0x00,
    IOTC_ADPT_BLE_ADV_TYPE_DIRECT_IND_HIGH = 0x01,
    IOTC_ADPT_BLE_ADV_TYPE_SCAN_IND = 0x02,
    IOTC_ADPT_BLE_ADV_TYPE_NONCONN_IND = 0x03,
    IOTC_ADPT_BLE_ADV_TYPE_DIRECT_IND_LOW = 0x04,
} IotcAdptBleAdvType;

/* 广播地址类型 */
typedef enum {
    IOTC_ADPT_BLE_ADV_ADDR_PUBLIC = 0x00,
    IOTC_ADPT_BLE_ADV_ADDR_RANDOM = 0x01,
    IOTC_ADPT_BLE_ADV_ADDR_PUBLIC_ID = 0x02,
    IOTC_ADPT_BLE_ADV_ADDR_RANDOM_ID = 0x03,
    IOTC_ADPT_BLE_ADV_ADDR_UNKNOWN_TYPE = 0xFF,
} IotcAdptBleAdvAddr;

/* GATTS char 属性取值 */
typedef enum {
    IOTC_ADPT_BLE_CHAR_PROP_BROADCAST = 0x01,
    IOTC_ADPT_BLE_CHAR_PROP_READ = 0x02,
    IOTC_ADPT_BLE_CHAR_PROP_WRITE_WITHOUT_RESP = 0x04,
    IOTC_ADPT_BLE_CHAR_PROP_WRITE = 0x08,
    IOTC_ADPT_BLE_CHAR_PROP_NOTIFY = 0x10,
    IOTC_ADPT_BLE_CHAR_PROP_INDICATE = 0x20,
    IOTC_ADPT_BLE_CHAR_PROP_SIGNED_WRITE = 0x40,
    IOTC_ADPT_BLE_CHAR_PROP_EXTENDED_PROPERTY = 0x80,
} IotcAdptBleCharProperty;

/* GATTS char 权限取值 */
typedef enum {
    IOTC_ADPT_BLE_CHAR_PERM_READ = 0x01,
    IOTC_ADPT_BLE_CHAR_PERM_READ_ENCRYPTED = 0x02,
    IOTC_ADPT_BLE_CHAR_PERM_READ_ENCRYPTED_MITM = 0x04,
    IOTC_ADPT_BLE_CHAR_PERM_WRITE = 0x10,
    IOTC_ADPT_BLE_CHAR_PERM_WRITE_ENCRYPTED = 0x20,
    IOTC_ADPT_BLE_CHAR_PERM_WRITE_ENCRYPTED_MITM = 0x40,
    IOTC_ADPT_BLE_CHAR_PERM_WRITE_SIGNED = 0x80,
    IOTC_ADPT_BLE_CHAR_PERM_WRITE_SIGNED_MITM = 0x100,
} IotcAdptBleCharPermission;

/* 广播信道 */
typedef enum {
    IOTC_ADPT_BLE_CHNL_37 = 0x01,
    IOTC_ADPT_BLE_CHNL_38 = 0x02,
    IOTC_ADPT_BLE_CHNL_39 = 0x04,
    IOTC_ADPT_BLE_CHNL_ALL = 0x07,
} IotcAdptBleAdvChannel;

/* 广播参数 */
typedef struct {
    IotcAdptBleAdvType advType; /* 广播类型 */
    uint32_t advMinInt; /* 0.625ms的倍数，最短间隔设置为20ms */
    uint32_t advMaxInt; /* 0.625ms的倍数，最长间隔不超过100ms */
    IotcAdptBleAdvAddr ownerAddrType; /* 采用公开地址 */
    IotcAdptBleAdvAddr directAddrType; /* 采用定向地址 */
    uint8_t *directAddr; /* 定向地址 */
    uint8_t channelMap; /* 37,38,39 */
    int8_t txPower; /* 广播功率 */
} IotcAdptBleAdvParam;

#define IOTC_ADPT_BLE_ADV_VALUE_MAX_LEN 31
/* 广播数据 */
typedef struct {
    uint8_t advData[IOTC_ADPT_BLE_ADV_VALUE_MAX_LEN];
    uint32_t advDataLen;
    uint8_t rspData[IOTC_ADPT_BLE_ADV_VALUE_MAX_LEN];
    uint32_t rspDataLen;
} IotcAdptBleAdvData;

/* GATT事件列表 */
typedef enum {
    IOTC_ADPT_BLE_GATT_EVENT_CONNECT,
    IOTC_ADPT_BLE_GATT_EVENT_DISCONNECT,
    IOTC_ADPT_BLE_GATT_EVENT_START_SVC_RESULT,
    IOTC_ADPT_BLE_GATT_EVENT_STOP_SVC_RESULT,
    IOTC_ADPT_BLE_GATT_EVENT_INDICATE_CONF,
    IOTC_ADPT_BLE_GATT_EVENT_SET_MTU_RESULT,
    IOTC_ADPT_BLE_GATT_EVENT_START_ADV_RESULT,
    IOTC_ADPT_BLE_GATT_EVENT_STOP_ADV_RESULT,
    IOTC_ADPT_BLE_GATT_EVENT_REQ_READ,
    IOTC_ADPT_BLE_GATT_EVENT_REQ_WRITE
} IotcAdptBleGattEvent;

/* GATTS事件参数列表 */
typedef union {
    /* 连接事件 */
    struct ConnSvc {
        int32_t connId;
        int32_t serverId;
        uint8_t devAddr[IOTC_ADPT_BLE_ADDR_LEN];
    } connSvc;
    /* 断连事件 */
    struct DisconnSvc {
        int32_t connId;
        int32_t serverId;
        uint8_t devAddr[IOTC_ADPT_BLE_ADDR_LEN];
        IotcAdptBleDisconnReason reason;
    } disconnSvc;
    /* 启动服务 */
    struct StartSvc {
        IotcAdptBleStatus status;
        int32_t serverId;
        int32_t svcHandle;
    } startSvc;
    /* 停止服务 */
    struct StopSvc {
        IotcAdptBleStatus status;
        int32_t serverId;
        int32_t svcHandle;
    } stopSvc;
    /* 发送indication或者notifytion */
    struct IndicateConf {
        IotcAdptBleStatus status;
        uint32_t handle;
        uint32_t connId;
    } indicateConf;
    /* 设置MTU */
    struct SetMtu {
        IotcAdptBleStatus status;
        uint32_t connId;
        uint16_t mtu;
    } setMtu;
    /* 开启广播参数回调参数 */
    struct StartAdv {
        IotcAdptBleStatus status;
    } startAdv;
    /* 停止广播参数回调参数 */
    struct StopAdv {
        IotcAdptBleStatus status;
    } stopAdv;
    /* 请求读 */
    struct ReqRead {
        int32_t connId;
        int32_t attrHandle;
        int32_t transId;
    } reqRead;
    /* 请求写 */
    struct ReqWrite {
        int32_t connId;
        int32_t attrHandle;
        int32_t transId;
        uint8_t *value;
        int32_t valueLen;
    } reqWrite;
} IotcAdptBleGattEventParam;

#define IOTC_ADPT_BLE_GATT_READ_BUF_SIZE 520
typedef int32_t(*IotcAdptBleGattReadFunc)(uint8_t *buff, uint32_t *len);
typedef int32_t(*IotcAdptBleGattWriteFunc)(uint8_t *buff, uint32_t len);
typedef int32_t(*IotcAdptBleGattCallback)(IotcAdptBleGattEvent event, const IotcAdptBleGattEventParam *param);

/* 发送indication或notification参数 */
typedef struct {
    int32_t connId;
    int32_t serverId;
    int32_t svcHandle;
    int32_t charHandle;
    bool needConfirm;
    uint32_t valueLen;
    uint8_t *value;
} IotcAdptBleSendIndicateParam;

/* 设置发现链接参数 */
typedef struct {
    bool isDiscover;
    bool isConnect;
    bool isBond;
} IotcAdptBleConnectParam;

typedef struct {
    int32_t serverId;
    int32_t connectId;
    int32_t transId;
    int32_t valueLen;
    uint8_t *value;
} IotcAdptBleResponseParam;

typedef enum {
    IOTC_ADPT_BLE_ATTRIB_TYPE_SERVICE = 0,
    IOTC_ADPT_BLE_ATTRIB_TYPE_CHAR,
    IOTC_ADPT_BLE_ATTRIB_TYPE_CHAR_CLIENT_CONFIG,
    IOTC_ADPT_BLE_ATTRIB_TYPE_CHAR_USER_DESCR,
} IotcAdptBleAttribType;

typedef struct {
    const char *uuid;
    uint32_t permission;
    int32_t(*readFunc)(uint8_t *buff, uint32_t *len);
    int32_t(*writeFunc)(uint8_t *buff, uint32_t len);
    int32_t descHandle;
} IotcAdptBleGattCharDesc;

typedef struct {
    const char *uuid;
    uint32_t permission;
    uint32_t property;
    int32_t(*readFunc)(uint8_t *buff, uint32_t *len);
    int32_t(*writeFunc)(uint8_t *buff, uint32_t len);
    int32_t(*indicateFunc)(uint8_t *buff, uint32_t len);
    IotcAdptBleGattCharDesc *desc;
    uint32_t descNum;
    int32_t charHandle;
} IotcAdptBleGattsChar;

typedef struct {
    const char *uuid;
    IotcAdptBleGattsChar *character;
    uint32_t charNum;
    int32_t svcHandle;
    int32_t serverId;
} IotcAdptBleGattService;

/**
 * @brief 初始化协议栈
 *
 * @return 0成功，非0失败
 */
int32_t IotcBleInitStack(void);

/**
 * @brief 设置蓝牙连接参数
 *
 * @param param [IN] 参数
 * @return 0成功，非0失败
 */
int32_t IotcBleSetConnectParam(const IotcAdptBleConnectParam *param);

/**
 * @brief 注册GATT回调
 *
 * @param callback [IN] 回调
 * @return 0成功，非0失败
 */
int32_t IotcBleRegisterGattCb(const IotcAdptBleGattCallback callback);

/**
 * @brief 设置蓝牙名字
 *
 * @param name [IN] 名字
 * @return 0成功，非0失败
 */
int32_t IotcBleSetBleName(const char *name);

/**
 * @brief 开启广播
 *
 * @param advParam [IN] 广播参数
 * @param advData [IN] 广播数据
 * @return 0成功，非0失败
 */
int32_t IotcBleStartAdv(const IotcAdptBleAdvParam *advParam, const IotcAdptBleAdvData *advData);

/**
 * @brief 停止广播
 *
 * @return 0成功，非0失败
 */
int32_t IotcBleStopAdv(void);

/**
 * @brief 开启GATT服务
 *
 * @param svc [IN] 服务表
 * @param svcNum [IN] 服务数量
 * @return 0成功，非0失败
 */
int32_t IotcBleStartGattsService(IotcAdptBleGattService *svc, uint32_t svcNum);

/**
 * @brief 停止GATT服务
 *
 * @param serverId [IN] 服务id
 * @param svcHandle [IN] 服务索引
 * @return 0成功，非0失败
 */
int32_t IotcBleStopGattsService(int32_t serverId, uint32_t svcHandle);

/**
 * @brief 发送GATT数据
 *
 * @param param [IN] 参数
 * @return 0成功，非0失败
 */
int32_t IotcBleSendGattsIndicate(const IotcAdptBleSendIndicateParam *param);

/**
 * @brief 发送GATT回应
 *
 * @param param [IN] 参数
 * @return 0成功，非0失败
 */
int32_t IotcBleSendGattsResponse(const IotcAdptBleResponseParam *param);

/**
 * @brief 获取蓝牙mac地址
 *
 * @param mac [OUT] mac输出缓存
 * @param len [IN] mac输出缓存长度
 * @return 0成功，非0失败
 */
int32_t IotcBleGetBleMac(uint8_t *mac, uint32_t len);

/**
 * @brief 端口与客户端的连接
 *
 * @param bdAddr [IN] 客户端地址
 * @param addrLen [IN] 地址长度
 * @return 0成功，非0失败
 */
int32_t IotcBleDisconnectGatt(const uint8_t *bdAddr, uint32_t addrLen);

/**
 * @brief 销毁蓝牙协议栈
 *
 * @return 0成功，非0失败
 */
int32_t IotcBleDeInitStack(void);

#ifdef __cplusplus
}
#endif

#endif /* IOTC_ADPT_BLE_H */