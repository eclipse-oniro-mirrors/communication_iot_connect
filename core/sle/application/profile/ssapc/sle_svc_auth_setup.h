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
#ifndef SLE_SVC_AUTH_SETUP_H
#define SLE_SVC_AUTH_SETUP_H

#include <stdint.h>
#include "sle_linklayer.h"
#include "sle_profile.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CreateSvcAuthSetupIssue
 * 客户端向服务端发送authcode请求
 *
 * @param out Output buffer
 * @param outLen Output buffer length
 *
 * @return 0: success, others: fail
 */
int32_t CreateSvcAuthSetupIssue(uint8_t **out, uint32_t *outLen);

/**
 * @brief CreateSvcAuthSetupGetReq
 * 创建client端请求server的AuthSetup信息
 *
 * @param out Output buffer
 * @param outLen Output buffer length
 *
 * @return 0: success, others: fail
 */
int32_t CreateSvcAuthSetupGet(uint8_t **out, uint32_t *outLen);

/**
 * @brief GetSleSvcAuthSetup
 * 获取服务端返回的AuthSetup信息
 *
 * @param param Command parameter
 * @param out Output buffer
 * @param outLen Output buffer length
 *
 * @return 0: success, others: fail
 */
int32_t GetSleSvcAuthSetup(const SleCmdParam *param, uint8_t **out, uint32_t *outLen);

/**
 * @brief AuthSetupAndDevInfo
 * 获取桥设备信息返回的AuthSetup信息
 *
 * @return 0: success, others: fail
 */
int32_t AuthSetupAndDevInfo();

#ifdef __cplusplus
}
#endif

#endif /* SLE_SVC_AUTH_SETUP_H */