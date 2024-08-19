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
#ifndef MODEL_SVC_INFO_H
#define MODEL_SVC_INFO_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "iotc_prof_def.h"
#include "iotc_json.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 服务信息初始化
 *
 * @return 0成功，非0失败
 */
int32_t ModelSvcInfoInit(const IotcServiceInfo svc[], uint32_t num);

/**
 * @brief 服务信息去初始化
 *
 */
void ModelSvcInfoDeinit(void);

/**
 * @brief 获取服务信息
 *
 * @param num [OUT] 服务数量
 * @return 服务信息，异常返回NULL
 */
const IotcServiceInfo *ModelGetSvcInfo(uint32_t *num);

/**
 * @brief 是否有效的sid
 *
 * @param sid [IN] 待检测sid
 * @return true 是，false 不是
 */
bool ModelSvcIsValidSid(const char *sid);

/**
 * @brief 是否有效的服务
 *
 * @param sid [IN] 待检测sid
 * * @param sid [IN] 待检测st
 * @return true 是，false 不是
 */
bool ModelSvcIsValidSvc(const char *sid, const char *st);

#ifdef __cplusplus
}
#endif

#endif /* MODEL_SVC_INFO_H */
