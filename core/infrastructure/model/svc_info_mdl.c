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
#include "svc_info_mdl.h"
#include "utils_assert.h"
#include "utils_common.h"
#include "iotc_errcode.h"
#include "comm_def.h"

AdapterJson *MdlBuildSvcJsonArray(const IotcServiceInfo *svcInfo, uint32_t num)
{
    CHECK_RETURN_LOGE(svcInfo != NULL && num != 0, NULL, "param invalid");

    AdapterJson *svcInfoArr = AdapterJsonCreateArray();
    if (svcInfoArr == NULL) {
        IOTC_LOGW("create json error");
        return NULL;
    }

    int32_t ret;
    for (uint32_t i = 0; i < num; ++i) {
        AdapterJson *cur = AdapterCreateJson();
        if (cur == NULL) {
            IOTC_LOGE("create svc json error");
            break;
        }
        ret = AdapterJsonAddItem2Array(svcInfoArr, cur);
        if (ret != IOTC_OK) {
            IOTC_LOGE("add svc to array err %d", ret);
            AdapterJsonDelete(cur);
            break;
        }

        ret = AdapterJsonAddStr2Obj(cur, STR_JSON_SVC_TYPE, svcInfo[i].svcType);
        if (ret != IOTC_OK) {
            IOTC_LOGE("add st err %s", NON_NULL_STR(svcInfo[i].svcType));
            break;
        }

        ret = AdapterJsonAddStr2Obj(cur, STR_JSON_SVC_ID, svcInfo[i].svcId);
        if (ret != IOTC_OK) {
            IOTC_LOGE("add st err %s", NON_NULL_STR(svcInfo[i].svcId));
            break;
        }
        if (i == num - 1) {
            return svcInfoArr;
        }
    }
    AdapterJsonDelete(svcInfoArr);
    return NULL;
}