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
#include <stddef.h>
#include "m2m_cloud_utils.h"
#include "iotc_errcode.h"
#include "comm_def.h"
#include "iotc_log.h"
#include "adapter_network.h"
#include "utils_json.h"
#include "dev_info.h"
#include "svc_info.h"
#include "dev_info_mdl.h"
#include "utils_common.h"
#include "wifi_net_info.h"

int32_t M2mCloudAddDevInfoToJson(AdapterJson *rootObj)
{
    AdapterJson *devInfoObj = MdlBuildDevInfoJson(ModelGetDevInfo());
    if (devInfoObj == NULL) {
        IOTC_LOGW("build dev info error");
        return IOTC_CORE_PROF_MDL_ERR_BUILD_DEV_INFO_JSON;
    }

    char macStr[MAC_ADDR_STR_LEN + 1] = {0};
    int32_t ret = GetWifiMacAddrStr(macStr, sizeof(macStr));
    if (ret != IOTC_OK) {
        IOTC_LOGE("get mac error %d", ret);
        AdapterJsonDelete(devInfoObj);
        return ret;
    }

    UtilsJsonStrItem strItem[] = {
        {STR_JSON_MAC, macStr},
        {STR_JSON_HIV, HILINK_VERSION},
    };

    ret = UtilsJsonAddStrTable(devInfoObj, strItem, ARRAY_SIZE(strItem));
    if (ret != IOTC_OK) {
        IOTC_LOGE("add info err %d", ret);
        AdapterJsonDelete(devInfoObj);
        return ret;
    }

    ret = AdapterJsonAddItem2Obj(rootObj, STR_JSON_DEV_INFO, devInfoObj);
    if (ret != IOTC_OK) {
        IOTC_LOGE("add dev info to root err %d", ret);
        AdapterJsonDelete(devInfoObj);
        return ret;
    }

    return IOTC_OK;
}