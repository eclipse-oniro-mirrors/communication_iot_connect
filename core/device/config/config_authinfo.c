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
#include "config_authinfo.h"
#include "utils_assert.h"
#include "config_info.h"
#include "securec.h"
#include "utils_common.h"
#include "iotc_errcode.h"

int32_t ConfigSaveAuthInfo(const DevAuthInfo *info)
{
    CHECK_RETURN_LOGW(info != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    struct SaveItem {
        ConfigInfoKey key;
        const uint8_t *data;
        uint32_t len;
    } authSaveInfo[] = {
        {CONFIG_INFO_KEY_DEVICE_ID, (const uint8_t *)info->devId, sizeof(info->devId)},
        {CONFIG_INFO_KEY_AUTHCODE_ID, (const uint8_t *)info->authCodeId, sizeof(info->authCodeId)},
        {CONFIG_INFO_KEY_AUTHCODE, (const uint8_t *)info->authCode, sizeof(info->authCode)},
        {CONFIG_INFO_KEY_UID_HASH, (const uint8_t *)info->uidHash, sizeof(info->uidHash)},
    };

    int32_t ret = IOTC_ERROR;
    for (uint32_t i = 0; i < ARRAY_SIZE(authSaveInfo); ++i) {
        ret = ConfigInfoSet(authSaveInfo[i].key, authSaveInfo[i].data, authSaveInfo[i].len);
        if (ret != IOTC_OK) {
            IOTC_LOGW("auth info set error %d/%d/%u", ret, authSaveInfo[i].key, authSaveInfo[i].len);
            return ret;
        }
    }

    ret = ConfigInfoSave();
    if (ret != IOTC_OK) {
        IOTC_LOGW("auth info save error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

int32_t ConfigGetAuthInfo(DevAuthInfo *info)
{
    CHECK_RETURN_LOGW(info != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    struct GetItem {
        ConfigInfoKey key;
        uint8_t *data;
        uint32_t len;
    } authGetInfo[] = {
        {CONFIG_INFO_KEY_DEVICE_ID, (uint8_t *)info->devId, sizeof(info->devId)},
        {CONFIG_INFO_KEY_AUTHCODE_ID, (uint8_t *)info->authCodeId, sizeof(info->authCodeId)},
        {CONFIG_INFO_KEY_AUTHCODE, (uint8_t *)info->authCode, sizeof(info->authCode)},
        {CONFIG_INFO_KEY_UID_HASH, (uint8_t *)info->uidHash, sizeof(info->uidHash)},
    };

    int32_t ret = IOTC_ERROR;
    for (uint32_t i = 0; i < ARRAY_SIZE(authGetInfo); ++i) {
        ret = ConfigInfoGet(authGetInfo[i].key, authGetInfo[i].data, &authGetInfo[i].len);
        if (ret != IOTC_OK) {
            IOTC_LOGW("auth info get error %d/%d/%u", ret, authGetInfo[i].key, authGetInfo[i].len);
            return ret;
        }
    }

    return IOTC_OK;
}

int32_t ConfigClearAuthInfo(void)
{
    int32_t ret = ConfigInfoClear(CONFIG_INFO_KEY_DEVICE_ID);
    CHECK_RETURN(ret == IOTC_OK, ret);
    ret = ConfigInfoClear(CONFIG_INFO_KEY_AUTHCODE_ID);
    CHECK_RETURN(ret == IOTC_OK, ret);
    ret = ConfigInfoClear(CONFIG_INFO_KEY_AUTHCODE);
    CHECK_RETURN(ret == IOTC_OK, ret);
    ret = ConfigInfoClear(CONFIG_INFO_KEY_UID_HASH);
    CHECK_RETURN(ret == IOTC_OK, ret);
    ret = ConfigInfoSave();
    CHECK_RETURN(ret == IOTC_OK, ret);
    return IOTC_OK;
}

bool IsDeviceBinded(void)
{
    char devId[DEVICE_ID_MAX_STR_LEN + 1];
    uint32_t len = sizeof(devId);
    int32_t ret = ConfigInfoGet(CONFIG_INFO_KEY_DEVICE_ID, (uint8_t *)devId, &len);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get devid error %d", ret);
        return false;
    }
    return !UtilsIsEmptyStr(devId);
}