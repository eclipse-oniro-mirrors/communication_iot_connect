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
#include "config_revoke_flag.h"
#include "config_info.h"
#include "iotc_errcode.h"
#include "iotc_log.h"

bool IsRevokeFlagExist(void)
{
    uint8_t flag = REVOKE_FLAG_NOT_SET;
    uint32_t len = sizeof(flag);
    int32_t ret = ConfigInfoGet(CONFIG_INFO_KEY_REVOKE_FLAG, &flag, &len);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get revoke flag error %d", ret);
        return false;
    }
    IOTC_LOGI("get revoke flag %u", flag);
    return flag == REVOKE_FLAG_SET;
}

int32_t SetRevokeFlag(void)
{
    uint8_t flag = REVOKE_FLAG_SET;
    int32_t ret = ConfigInfoSet(CONFIG_INFO_KEY_REVOKE_FLAG, &flag, sizeof(flag));
    if (ret != IOTC_OK) {
        IOTC_LOGW("set revoke flag error %d", ret);
        return ret;
    }
    ret = ConfigInfoSave();
    if (ret != IOTC_OK) {
        IOTC_LOGW("revoke flag save error %d", ret);
        return ret;
    }
    IOTC_LOGN("set revoke flag");
    return IOTC_OK;
}

int32_t ClearRevokeFlag(void)
{
    uint8_t flag = REVOKE_FLAG_NOT_SET;
    int32_t ret = ConfigInfoSet(CONFIG_INFO_KEY_REVOKE_FLAG, &flag, sizeof(flag));
    if (ret != IOTC_OK) {
        IOTC_LOGW("clear revoke flag error %d", ret);
        return ret;
    }
    IOTC_LOGN("clear revoke flag");
    return IOTC_OK;
}