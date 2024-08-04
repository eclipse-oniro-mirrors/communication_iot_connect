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
#include "securec.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "utils_assert.h"
#include "security_random.h"
#include "iotc_svc_dev.h"
#include "dev_info.h"

#define SOFTAP_NAME_MOULD "Oh-AAAAAAABBBBBBB-XYYYYYRRRR"
#define SOFTAP_NAME_HEAD "Oh"
#define SOFTAP_NAME_VER "1"

#define CUSTOM_NAME_BUF_LEN 64
#define SOFTAP_NAME_SN_LEN 4
#define SOFTAP_NETCFG_CAPCITY 0b01101000
#define SOFTAP_NETCFG_CAPCITY_SIZE 1

int32_t IotcOhGetSoftApName(uint8_t *ssid, uint32_t *len)
{
    CHECK_RETURN_LOGW((ssid != NULL) && (len != NULL), IOTC_ERR_PARAM_INVALID, "invalid param");
    uint32_t ssidSize = *len;
    char customName[CUSTOM_NAME_BUF_LEN + 1] = {0};

    const IotcDeviceInfo *devInfo = ModelGetDevInfo();
    if (devInfo == NULL || devInfo->devTypeName == NULL || devInfo->manuName == NULL ||
        devInfo->prodId == NULL || devInfo->sn == NULL) {
        IOTC_LOGW("get dev info null");
        return IOTC_CORE_PROF_MDL_ERR_DEV_INFO_INVALID;
    }

    if (sprintf_s(customName, sizeof(customName), "%s%s", devInfo->manuName, devInfo->devTypeName) <= 0) {
        IOTC_LOGE("memcpy_s");
        return IOTC_ERROR;
    }
    int32_t snLen = strlen(devInfo->sn);
    if (snLen < SOFTAP_NAME_SN_LEN) {
        IOTC_LOGE("sn len is too short");
        return IOTC_ERROR;
    }
    int ret = sprintf_s((char *)ssid, ssidSize, "%s-%.14s-%s%s%s",
        SOFTAP_NAME_HEAD, customName, SOFTAP_NAME_VER, devInfo->prodId,
        &devInfo->sn[snLen - SOFTAP_NAME_SN_LEN]);
    if (ret <= 0 || (ret + SOFTAP_NETCFG_CAPCITY_SIZE) > ssidSize) {
        IOTC_LOGE("sprintf_s ret=%d,%u,%u", ret, SOFTAP_NETCFG_CAPCITY_SIZE, ssidSize);
        return IOTC_ERROR;
    }
    ssid[ret] = SOFTAP_NETCFG_CAPCITY;
    *len = ret + SOFTAP_NETCFG_CAPCITY_SIZE;
    return IOTC_OK;
}