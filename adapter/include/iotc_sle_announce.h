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

#ifndef IOTC_SLE_ANNOUNCE_H
#define IOTC_SLE_ANNOUNCE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "iotc_sle_host.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
//  *   @brief   Announce state change Callbacks
//  *
//  *   @param   [in] announceId
//  *   @param   [in] announceState
//  *   @param   [in] isTerminaled - whether the host is connected while announcing
//  *   @return  None
//  */
// typedef void (*IotcAdpSleAnnounceStateChangeCallback)(uint8_t announceId, uint8_t announceState, bool isTerminaled);

typedef enum {
    IOTC_ADPT_SLE_ANNOUNCE_NONCONN_NONSCAN_MODE      = 0x00,
    IOTC_ADPT_SLE_ANNOUNCE_CONNECTABLE_NONSCAN_MODE  = 0x01,
    IOTC_ADPT_SLE_ANNOUNCE_NONCONN_SCANABLE_MODE     = 0x02,
    IOTC_ADPT_SLE_ANNOUNCE_CONNECTABLE_SCANABLE_MODE = 0x03,
    IOTC_ADPT_SLE_ANNOUNCE_CONNECTABLE_DIRECTED_MODE = 0x07,
} IotcSleAnnounceMode;


typedef enum {
    IOTC_ADPT_SLE_ANNOUNCE_T_ROAL_CAN_NEGO = 0,
    IOTC_ADPT_SLE_ANNOUNCE_G_ROAL_CAN_NEGO,
    IOTC_ADPT_SLE_ANNOUNCE_T_ROAL_NO_NEGO,
    IOTC_ADPT_SLE_ANNOUNCE_G_ROAL_NO_NEGO
} IotcSleAnnounceRole;


typedef enum {
    IOTC_ADPT_SLE_ANNOUNCE_NONE_LEVEL,
    IOTC_ADPT_SLE_ANNOUNCE_NORMAL_LEVEL,
    IOTC_ADPT_SLE_ANNOUNCE_PRIORITY_LEVEL,
    IOTC_ADPT_SLE_ANNOUNCE_PAIRED_LEVEL,
    IOTC_ADPT_SLE_ANNOUNCE_SPECIAL_LEVEL,
} IotcSleAnnounceLevel;


typedef enum {
    IOTC_ADPT_SLE_ANNOUNCE_76_CHANEL = 1,
    IOTC_ADPT_SLE_ANNOUNCE_77_CHANEL = 2,
    IOTC_ADPT_SLE_ANNOUNCE_78_CHANEL = 4,
    IOTC_ADPT_SLE_ANNOUNCE_DEFAULT_ALL_CHANEL = 7,
} IotcSleAnnounceChanel;


typedef struct {
    uint8_t handle;
    uint8_t mode;
    uint8_t role;
    uint8_t level;
    uint32_t annonceIntervalMin;    //  [0x000020,0xffffff], 单位125us
    uint32_t annonceIntervalMax;    //  [0x000020,0xffffff], 单位125us
    uint8_t channelMap;
    int8_t txPower;                 //  [-127, 20], 0x7F：不设置特定发送功率
    IotcAdptSleDeviceAddr ownAddr;       //  框架内部使用,外部配置无效
    IotcAdptSleDeviceAddr peerAddr;      //  reserved for level:SLE_ANNOUNCE_SPECIAL_LEVEL
    uint16_t connectIntervalMin;    //  [0x001E,0x3E80], role == SLE_ANNOUNCE_T_ROAL_NO_NEGO时无需配置
    uint16_t connectIntervalMax;    //  [0x001E,0x3E80], role == SLE_ANNOUNCE_T_ROAL_NO_NEGO时无需配置
    uint16_t connectLatency;        //  [0x0000,0x01F3], role == SLE_ANNOUNCE_T_ROAL_NO_NEGO时无需配置
    uint16_t connectTimeout;        //  [0x000A,0x0C80], role == SLE_ANNOUNCE_T_ROAL_NO_NEGO时无需配置
    void *extParam;                 //  reserved
} IotcAdptSleAnnounceParam;

#define IOTC_ADPT_SLE_ADV_VALUE_MAX_LEN 31
/* 广播数据 */
typedef struct {
    uint16_t announceLength;
    uint8_t announceData[IOTC_ADPT_SLE_ADV_VALUE_MAX_LEN];
    uint16_t responceLength;
    uint8_t responceData[IOTC_ADPT_SLE_ADV_VALUE_MAX_LEN];
} IotcAdptSleAnnounceData;

typedef union {
    struct {
        uint8_t announceId;
        uint8_t announceState;
        bool isTerminaled;
    } announceStateChnage;
} IotcAdptSleAnnounceEventParam;

typedef enum {
    IOTC_ADPT_SLE_ANNOUNCE_STATE_CHANGE,
} IotcAdptSleAnnounceEvent;

typedef int32_t (*IotcAdptSleAnnounceCallback)(
    IotcAdptSleAnnounceEvent event,
    const IotcAdptSleAnnounceEventParam *param);
/**
 *   @brief   Init SleAnnounceService sync
 *
 *   @param   None
 *   @return  SleErrorCode - operation result
 */
int32_t IotcInitSleAnnounceService(void);

/**
 *   @brief   Deinit SleAnnounceService sync
 *
 *   @param   None
 *   @return  SleErrorCode - operation result
 */
int32_t IotcDeinitSleAnnounceService(void);

/**
 *  @brief  add announce service sync
 *
 *  @param  [out] announceId - announceId will less than SLE_MAX_ANNOUNCE_ID_NUM
 *
 *  @return SleErrorCode - operation result
 */
int32_t IotcAddAnnounce(uint8_t *announceId);

/**
 *  @brief  remove announce service sync
 *
 *  @param  [in] announceId
 *  @return SleErrorCode - operation result
 */
int32_t IotcRemoveAnnounce(uint8_t announceId);

/**
 *   @brief   Start announce
 *
 *   @param   [in] announceId
 *   @param   [in] data
 *   @param   [in] announceParam
 *   @return  SleErrorCode - operation result
 *
 *   @note    dataLen mast less then SLE_ANNOUNCE_DATA_ANN_MAX_LENGTH or SLE_ANNOUNCE_DATA_MAX_RSP_LENGTH
 *   @attention if announceParam is not NULL, all params of announceParam should be set by user
 */
int32_t IotcStartAnnounce(uint8_t announceId, const IotcAdptSleAnnounceData *data,
    const IotcAdptSleAnnounceParam *announceParam);

/**
 *   @brief   Stop announce
 *
 *   @param   [in] announceId
 *   @return  SleErrorCode - operation result
 *
 *   @note    this func will try to Unregister callback
 */
int32_t IotcStopAnnounce(uint8_t announceId);

/**
 *   @brief   Register announce callback sync
 *
 *   @param   [in] announceCallback
 *   @return  SleErrorCode - operation result
 *
 *   @attention   announceCallback must be registered before StartAnnounce to get announceId
 */
int32_t IotcRegisterAnnounceCallbacks(IotcAdptSleAnnounceCallback callback);

/**
 *   @brief   Register announce callback sync
 *
 *   @param   [in] announceCallback
 *   @return  SleErrorCode - operation result
 *
 *   @note    announceCallback should be delete by user when not needed
 */
int32_t IotcUnregisterAnnounceCallbacks(IotcAdptSleAnnounceCallback callback);

#ifdef __cplusplus
}
#endif
#endif /* IOTC_ADPT_SLE_ANNOUNCE__H */
