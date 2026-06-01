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
#ifndef MODEL_DEV_INFO_H
#define MODEL_DEV_INFO_H

#include <stdint.h>
#include <stddef.h>
#include "iotc_prof_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 设备信息初始化
 *
 * @return 0成功，非0失败
 */
int32_t ModelDevInfoInit(const IotcDeviceInfo *devInfo);

/**
 * @brief 设备信息去初始化
 *
 */
void ModelDevInfoDeinit(void);

/**
 * @brief 获取设备信息
 *
 * @return 设备信息
 */
const IotcDeviceInfo *ModelGetDevInfo(void);

/**
 * @brief 获取设备序列号
 *
 * @return 设备序列号
 */
const char *ModelGetDevSn(void);

/**
 * @brief 获取设备产品ID
 *
 * @return 设备产品ID
 */
const char *ModelGetDevProId(void);

/**
 * @brief 获取设备产品子ID
 *
 * @return 设备产品子ID
 */
const char *ModelGetDevSubProId(void);

/**
 * @brief 获取设备产品型号
 *
 * @return 设备产品型号
 */
const char *ModelGetDevModel(void);

/**
 * @brief 获取设备类型ID
 *
 * @return 设备类型ID
 */
const char *ModelGetDevTypeId(void);

/**
 * @brief 获取设备类型名
 *
 * @return 设备类型名
 */
const char *ModelGetDevTypeName(void);

/**
 * @brief 获取设备厂商ID
 *
 * @return 设备厂商ID
 */
const char *ModelGetDevManuId(void);

/**
 * @brief 获取设备厂商名
 *
 * @return 设备厂商名
 */
const char *ModelGetDevManuName(void);

/**
 * @brief 获取设备固件版本号
 *
 * @return 设备固件版本号
 */
const char *ModelGetDevFwv(void);

/**
 * @brief 获取设备硬件版本号
 *
 * @return 设备硬件版本号
 */
const char *ModelGetDevHwv(void);

/**
 * @brief 获取设备软件版本号
 *
 * @return 设备软件版本号
 */
const char *ModelGetDevSwv(void);

/**
 * @brief 获取设备协议类型
 *
 * @return 设备协议类型
 */
int32_t ModelGetDevProtType(void);

/**
 * @brief 获取设备唯一标识
 *
 * @return 设备唯一标识
*/
const char *ModelGetDevUniqueId(void);
 	  
/**
 * @brief 获取厂家自定义数据
 *
 * @return 厂家自定义数据
*/
const char *ModelGetDevCustomData(void);

int32_t ModelGetUdid(uint8_t *buf, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* MODEL_DEV_INFO_H */
