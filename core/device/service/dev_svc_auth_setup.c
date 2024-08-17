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
#include <string.h>
#include "dev_svc_auth_setup.h"
#include "utils_common.h"
#include "utils_assert.h"
#include "utils_json.h"
#include "iotc_svc_dev.h"
#include "service_proxy.h"
#include "iotc_svc.h"
#include "config_authinfo.h"
#include "securec.h"
#include "iotc_errcode.h"

static int32_t GetAuthInfoFromJson(const IotcJson *req, DevAuthInfo *info)
{
    int32_t ret = UtilsJsonGetString(req, STR_JSON_DEVID, info->devId, sizeof(info->devId));
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "get devId err %d", ret);

    char authCode[HEXIFY_LEN(BLE_AUTHCODE_LEN) + 1] = { 0 };
    ret = UtilsJsonGetString(req, STR_JSON_AUTHCODE, authCode, sizeof(authCode));
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "get authCode str err %d", ret);
    CHECK_RETURN_LOGE(UtilsUnhexify(authCode, strlen(authCode), info->authCode, sizeof(info->authCode)),
        IOTC_CORE_COMM_UTILS_ERR_UNHEXIFY, "get authCode hex err");

    ret = UtilsJsonGetString(req, STR_JSON_AUTHCODE_ID, info->authCodeId, sizeof(info->authCodeId));
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "get authCode id err %d", ret);

    ret = UtilsJsonGetString(req, STR_JSON_UIDHASH, info->uidHash, sizeof(info->uidHash));
    if (ret != IOTC_OK) {
        /* 非BLE通道绑定无uidHash */
        IOTC_LOGI("no uid hash %d", ret);
    }

    return IOTC_OK;
}

int32_t DeviceServiceRecvAuthInfo(const IotcJson *jsonObj)
{
    CHECK_RETURN_LOGW(jsonObj != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    DevAuthInfo recvAuthinfo = { 0 };
    int32_t ret = GetAuthInfoFromJson(jsonObj, &recvAuthinfo);
    CHECK_RETURN(ret == IOTC_OK, ret);

    DevAuthInfo curAuthInfo = {0};
    ret = ConfigGetAuthInfo(&curAuthInfo);
    if (ret != IOTC_OK) {
        (void)memset_s(&recvAuthinfo, sizeof(DevAuthInfo), 0, sizeof(DevAuthInfo));
        IOTC_LOGW("get auth info error %d", ret);
        return ret;
    }

    if (strlen(curAuthInfo.uidHash) != 0 && strcmp(recvAuthinfo.uidHash, curAuthInfo.uidHash) != 0) {
        IOTC_LOGW("uidhash not match");
        return IOTC_CORE_BLE_UIDHASH_NOT_MATCH;
    }

    ret = ConfigSaveAuthInfo(&recvAuthinfo);
    (void)memset_s(&recvAuthinfo, sizeof(DevAuthInfo), 0, sizeof(DevAuthInfo));
    (void)memset_s(&curAuthInfo, sizeof(DevAuthInfo), 0, sizeof(DevAuthInfo));
    if (ret != IOTC_OK) {
        IOTC_LOGW("save auth info error %d", ret);
        return ret;
    }

    return IOTC_OK;
}
