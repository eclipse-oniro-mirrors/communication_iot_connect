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
#include "netcfg_service.h"
#include "securec.h"
#include "comm_def.h"
#include "utils_mutex_global.h"
#include "utils_assert.h"
#include "utils_common.h"
#include "iotc_errcode.h"

#define NETCFG_INVALID_METHOD (-1)
#define DEFAULT_NET_CFG_TIMEOUT (UTILS_MIN_TO_MS(10))

static IotcNetConfigMode g_netCfgMode = IOTC_NET_CONFIG_MODE_NONE;
static uint32_t g_netCfgTimeoutMs = DEFAULT_NET_CFG_TIMEOUT;

void SetNetCfgMode(IotcNetConfigMode mode)
{
    g_netCfgMode = mode;
    IOTC_LOGN("net cfg mode set to %d", mode);
}

void SetNetCfgTimeout(uint32_t timeout)
{
    g_netCfgTimeoutMs = timeout;
    IOTC_LOGN("net cfg timeout set to %u", timeout);
}

IotcNetConfigMode GetNetCfgMode(void)
{
    return g_netCfgMode;
}

uint32_t GetNetCfgTimeout(void)
{
    return g_netCfgTimeoutMs;
}