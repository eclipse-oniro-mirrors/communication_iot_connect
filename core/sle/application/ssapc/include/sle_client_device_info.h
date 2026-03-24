/*
 * Copyright (c) 2024-2024 ShenZhen Kaihong Device Co., Ltd.
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

#ifndef SLE_CLIENT_DEVICE_INFO_H
#define SLE_CLIENT_DEVICE_INFO_H

#include <stdint.h>
#include "sle_linklayer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建设备信息请求
 *
 * @param out The output buffer
 * @param outLen The output buffer length
 *
 * @return 0 on 成功, -1 on failure
 */
int32_t CreateSvcDeviceInfoReq(uint8_t **out, uint32_t *outLen);

#ifdef __cplusplus
}
#endif

#endif /* SLE_CLIENT_DEVICE_INFO_H */