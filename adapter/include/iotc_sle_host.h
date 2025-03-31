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

#ifndef IOTC_ADPT_SLE_HOST_H
#define IOTC_ADPT_SLE_HOST_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 设备地址长度 */
#define IOTC_ADPT_SLE_ADDR_LEN 6

typedef struct {
    uint8_t type;                         
    unsigned char addr[IOTC_ADPT_SLE_ADDR_LEN];                                                
} IotcAdptSleDeviceAddr;

/**
 *  @brief  sle host state change callback
 *
 *  @param  [in] state - current sle host device state
 *  @return None
 */
typedef void (*IotcSleHostStateChangeCallback)(uint8_t state);

/**
 *  @brief  sle flow monitor event callback
 *
 *  @param  [in] flow - flow
 *  @return None
 */
typedef void (*IotcSleFlowMonitorEventCallback)(float flow);

typedef struct {
    IotcSleHostStateChangeCallback OnSleHostStateChangeCb;
    IotcSleFlowMonitorEventCallback OnSleFlowMonitorEventCb;
} IotcSleHostCallbacks;

/**
 *  @brief  initialize sle service sync
 *
 *  @param  None
 *  @return SleErrorCode - operation result
 */
uint8_t IotcInitSleHostService(void);

/**
 *  @brief  deinitialize sle service sync
 *
 *  @param  None
 *  @return SleErrorCode - operation result
 */
uint8_t IotcDeinitSleHostService(void);

/**
 *  @brief  enable sle sync
 *
 *  @param  None
 *  @return SleErrorCode - operation result
 *
 *  @note   result return in SleHostTimeOut = 3s
 */
uint8_t IotcSleEnable(void);

/**
 *  @brief  disable sle sync
 *
 *  @param  None
 *  @return SleErrorCode - operation result
 *
 *  @note   result return in SleHostTimeOut = 3s
 */
uint8_t IotcSleDisable(void);

/**
 *  @brief  get host address sync
 *
 *  @param  [out] addressType -  hostAddr - struct host device
 *  @return SleErrorCode - operation result
 */
IotcAdptSleDeviceAddr* IotcGetHostAddress(void);

/**
 * @brief 设置SLE名字
 *
 * @param name [IN] 名字
 * @return 0成功，非0失败
 */
uint8_t IotcSleSetSleName(const char *name);


/**
 *  @brief  unregister host callbacks sync
 *
 *  @param  [in] hostCallback - host callbacks
 *  @return SleErrorCode - operation result
 *
 *  @note   hostCallback should be delete by user after SleDisable
 */
uint8_t IotcUnregisterHostCallbacks(IotcSleHostCallbacks *hostCallback);

uint8_t IotcSleRegisterHostCallbacks();

#ifdef __cplusplus
}
#endif
#endif /* IOTC_ADPT_SLE_HOST_H */