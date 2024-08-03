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
#include "config_register_info.h"
#include "securec.h"
#include "utils_assert.h"
#include "iotc_errcode.h"

static bool g_regFlag = false;
static DevRegInfo g_devRegInfo;

bool IsRegisterInfoExist(void)
{
    return g_regFlag;
}

int32_t ConfigSaveRegisterInfo(const DevRegInfo *info)
{
    CHECK_RETURN_LOGW(info != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    int32_t ret = memcpy_s(&g_devRegInfo, sizeof(g_devRegInfo), info, sizeof(DevRegInfo));
    if (ret != EOK) {
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    g_regFlag = true;
    return IOTC_OK;
}

int32_t ConfigGetRegisterInfo(DevRegInfo *info)
{
    CHECK_RETURN_LOGW(info != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    if (!g_regFlag) {
        IOTC_LOGW("reg info not exist");
        return IOTC_SDK_AILIFE_DATA_ERR_REG_INFO_NOT_EXISTS;
    }
    int32_t ret = memcpy_s(info, sizeof(DevRegInfo), &g_devRegInfo, sizeof(g_devRegInfo));
    if (ret != EOK) {
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    return IOTC_OK;
}

int32_t ConfigClearRegisterInfo(void)
{
    (void)memset_s(&g_devRegInfo, sizeof(g_devRegInfo), 0, sizeof(g_devRegInfo));
    g_regFlag = false;
    return IOTC_OK;
}