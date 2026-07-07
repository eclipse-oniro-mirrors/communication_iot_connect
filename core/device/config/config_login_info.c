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
#include "config_login_info.h"
#include "utils_assert.h"
#include "config_info.h"
#include "securec.h"
#include "utils_common.h"
#include "config_revoke_flag.h"
#include "iotc_errcode.h"

int32_t ConfigSaveLoginInfo(const DevLoginInfo *info)
{
    CHECK_RETURN_LOGW(info != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    struct SaveItem {
        ConfigInfoKey key;
        const uint8_t *data;
        uint32_t len;
    } loginSaveInfo[] = {
        {CONFIG_INFO_KEY_DEVICE_ID, (const uint8_t *)info->devId, sizeof(info->devId)},
        {CONFIG_INFO_KEY_LOGIN_SECRET, (const uint8_t *)info->secret, sizeof(info->secret)},
        {CONFIG_INFO_KEY_LOGIN_PSK, (const uint8_t *)info->psk, sizeof(info->psk)},
        {CONFIG_INFO_KEY_LOGIN_URL, (const uint8_t *)info->url, sizeof(info->url)},
        {CONFIG_INFO_KEY_LOGIN_BACKUP_URL, (const uint8_t *)info->backupUrl, sizeof(info->backupUrl)},
    };

    int32_t ret = IOTC_ERROR;
    for (uint32_t i = 0; i < ARRAY_SIZE(loginSaveInfo); ++i) {
        ret = ConfigInfoSet(loginSaveInfo[i].key, loginSaveInfo[i].data, loginSaveInfo[i].len);
        if (ret != IOTC_OK) {
            IOTC_LOGW("login info set error %d/%d/%u", ret, loginSaveInfo[i].key, loginSaveInfo[i].len);
            return ret;
        }
    }

    ret = ConfigInfoSave();
    if (ret != IOTC_OK) {
        IOTC_LOGW("login info save error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

int32_t ConfigGetLoginInfo(DevLoginInfo *info)
{
    CHECK_RETURN_LOGW(info != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    struct GetItem {
        ConfigInfoKey key;
        uint8_t *data;
        uint32_t len;
    } loginGetInfo[] = {
        {CONFIG_INFO_KEY_DEVICE_ID, (uint8_t *)info->devId, sizeof(info->devId)},
        {CONFIG_INFO_KEY_LOGIN_SECRET, (uint8_t *)info->secret, sizeof(info->secret)},
        {CONFIG_INFO_KEY_LOGIN_PSK, (uint8_t *)info->psk, sizeof(info->psk)},
        {CONFIG_INFO_KEY_LOGIN_URL, (uint8_t *)info->url, sizeof(info->url)},
        {CONFIG_INFO_KEY_LOGIN_BACKUP_URL, (uint8_t *)info->backupUrl, sizeof(info->backupUrl)},
    };

    int32_t ret = IOTC_ERROR;
    for (uint32_t i = 0; i < ARRAY_SIZE(loginGetInfo); ++i) {
        ret = ConfigInfoGet(loginGetInfo[i].key, loginGetInfo[i].data, &loginGetInfo[i].len);
        if (ret != IOTC_OK) {
            IOTC_LOGW("login info get error %d/%d/%u", ret, loginGetInfo[i].key, loginGetInfo[i].len);
            return ret;
        }
    }

    return IOTC_OK;
}

int32_t ConfigClearLoginInfo(void)
{
    ConfigInfoKey loginKeys[] = {
        CONFIG_INFO_KEY_DEVICE_ID,
        CONFIG_INFO_KEY_LOGIN_SECRET,
        CONFIG_INFO_KEY_LOGIN_PSK,
        CONFIG_INFO_KEY_LOGIN_URL,
        CONFIG_INFO_KEY_LOGIN_BACKUP_URL,
    };
    int32_t err;
    int32_t ret = IOTC_OK;
    for (uint32_t i = 0; i < ARRAY_SIZE(loginKeys); ++i) {
        err = ConfigInfoClear(loginKeys[i]);
        if (err != IOTC_OK) {
            IOTC_LOGW("clear login info error %d/%d", err, loginKeys[i]);
            ret = err;
        }
    }

    err = ConfigInfoSave();
    if (err != IOTC_OK) {
        IOTC_LOGW("clear login info save error %d", err);
        ret = err;
    }
    return ret;
}