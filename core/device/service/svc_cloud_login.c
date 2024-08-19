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
#include "svc_cloud_setup.h"
#include "utils_assert.h"
#include "iotc_errcode.h"
#include "iotc_svc.h"
#include "iotc_svc_softap.h"
#include "utils_json.h"
#include "config_login_info.h"
#include "utils_common.h"
#include "securec.h"

int32_t DeviceServiceRecvLoginInfo(const IotcJson *jsonObj)
{
    DevLoginInfo info = {0};
    char pskHex[HEXIFY_LEN(sizeof(info.psk)) + 1] = {0};
    int32_t ret;
    do {
        ret = UtilsJsonGetString(jsonObj, STR_NETINFO_DEVICE_ID, info.devId, sizeof(info.devId));
        if (ret != IOTC_OK) {
            IOTC_LOGE("get devid error %d", ret);
            break;
        }

        ret = UtilsJsonGetString(jsonObj, STR_NETINFO_PSK, pskHex, sizeof(pskHex));
        if (ret != IOTC_OK) {
            IOTC_LOGE("get psk error %d", ret);
            break;
        }

        if (!UtilsUnhexify(pskHex, HEXIFY_LEN(sizeof(info.psk)), info.psk, sizeof(info.psk))) {
            IOTC_LOGE("unhexify psk error");
            ret = IOTC_CORE_COMM_UTILS_ERR_UNHEXIFY;
            break;
        }

        ret = UtilsJsonGetString(jsonObj, STR_JSON_SECRET, info.secret, sizeof(info.secret));
        if (ret != IOTC_OK) {
            IOTC_LOGE("get code error %d", ret);
            break;
        }

        ret = UtilsJsonGetString(jsonObj, STR_NETINFO_URL, info.url, sizeof(info.url));
        if (ret != IOTC_OK) {
            IOTC_LOGE("get url error %d", ret);
            break;
        }

        ret = UtilsJsonGetString(jsonObj, STR_NETINFO_BACKUP_URL, info.backupUrl, sizeof(info.backupUrl));
        if (ret != IOTC_OK) {
            IOTC_LOGW("get backupUrl error %d", ret);
        }

        ret = ConfigSaveLoginInfo(&info);
        if (ret != IOTC_OK) {
            IOTC_LOGE("save reg info error %d", ret);
            break;
        }
    } while (0);
    (void)memset_s(&info, sizeof(DevLoginInfo), 0, sizeof(DevLoginInfo));
    (void)memset_s(pskHex, sizeof(pskHex), 0, sizeof(pskHex));
    return ret;
}
