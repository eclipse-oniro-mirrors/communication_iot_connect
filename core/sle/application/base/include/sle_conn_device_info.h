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
#ifndef SLE_CONN_MGT_H
#define SLE_CONN_MGT_H

#include <stdint.h>
#include "iotc_sle_def.h"
#include "utils_list.h"
#include "utils_fsm.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    IotcDeviceInfo *devInfo; // 下挂的生态设备信息（SLE连接后,从生态设备处获取）
    uint8_t type; // 设备地址类型
    // 云端下发的生态设备信息（SLE连接后，从生态设备处获取，详见配网和注册激活流程）
    char devId[DEVICE_ID_MAX_STR_LEN + 1];
    char secret[CLOUD_SECRET_MAX_STR_LEN + 1];
    uint8_t psk[CLOUD_PSK_MAX_LEN];
    char uidHash[BLE_UID_HASH_LEN + 1];
    char authCodeId[BLE_AUTHCODE_ID_LEN + 1];
    char authCode[BLE_AUTHCODE_LEN];
    uint16_t connID;
    uint8_t devAddr[IOTC_ADPT_SLE_ADDR_LEN];
    SpekeState isSecure;
} IotcConDeviceInfo;


typedef struct {
    IotcConDeviceInfo info;
    ListEntry list;
} SleConnDevList;


typedef struct { // 一个端侧生态设备 和云端进行控制交互，屏蔽具体SLE或BLE设备
    char devId[DEVICE_ID_MAX_STR_LEN + 1];
    char secret[CLOUD_SECRET_MAX_STR_LEN + 1];
    UtilsFsm *fsmCtx; // 云端交互状态机

    // 下面可以扩展具体SLE或BLE设备，以及设置一系列回调处理函数
    uint8_t type; // 端侧设备类型是SLE或BLE，后面根据类型做消息发布（发送）
}M2mEndDeviceInfo;


int32_t SleConnDevMgt(const SleConnRetDeviceInfo *retDev);
int32_t SleAddConnDev(const SleConnRetDeviceInfo *retDev);
void SleDeleteConnDev(uint16_t connID);
bool SleFindConnDevByDevId(const char *devID, uint16_t *connID);
void DestroySleConnDevList(void);
void PrintSleConnDevList();
bool IsExitSleConnDev(const uint16_t connID);

// 根据devId获取返回的连接信息
IotcDeviceInfo* SleGetDeviceInfoByDevId(const char *devId);

// 根据devId获取连接信息
IotcConDeviceInfo* SleGetConnectionInfoByDevId(const char *devId);

// 根据connId获取连接信息
IotcConDeviceInfo* SleGetConnectionInfoByConnId(uint16_t connId);

// 根据connId获取连接devId
int32_t SleGetDevIdByConnId(uint16_t connId, char *devId);

#ifdef __cplusplus
}
#endif

#endif /* SLE_CONN_MGT_H */