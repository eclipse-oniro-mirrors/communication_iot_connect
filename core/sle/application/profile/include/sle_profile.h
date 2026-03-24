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
#ifndef SLE_PROFILE_H
#define SLE_PROFILE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Sle通信内置服务 */
#define SLE_SVC_NET_CFG_VER "netCfgVer"
#define SLE_SVC_DEVICE_INFO "deviceInfo"
#define SLE_SVC_AUTH_SETUP "authSetup"
#define SLE_SVC_CLEAR_REGINFO "clearDevRegInfo"
#define SLE_SVC_SPEKE "speke"
#define SLE_SVC_NETCFG "netCfg"
#define SLE_SVC_CREATE_SESSION "createSession"
#define SLE_SVC_CUSTOM_SEC_DATA "customSecData"

int32_t SleProfileInit(void);
void SleProfileDeinit(void);

#ifdef __cplusplus
}
#endif

#endif /* SLE_PROFILE_H */