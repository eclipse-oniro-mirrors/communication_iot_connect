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
#include "dev_info_mdl.h"
#include "utils_json.h"
#include "utils_assert.h"
#include "utils_common.h"
#include "comm_def.h"
#include "iotc_errcode.h"

IotcJson *MdlBuildDevInfoJson(const IotcDeviceInfo *devInfo)
{
    CHECK_RETURN_LOGE(devInfo != NULL, NULL, "param invalid");
    IotcJson *devInfoObj = IotcJsonCreate();
    UtilsJsonStrItem strItem[] = {
        {STR_JSON_SN, devInfo->sn},
        {STR_JSON_MODEL, devInfo->model},
        {STR_JSON_DEV_TYPE, devInfo->devTypeId},
        {STR_JSON_MANU, devInfo->manuId},
        {STR_JSON_PROD_ID, devInfo->prodId},
        {STR_JSON_FWV, devInfo->fwv},
        {STR_JSON_HWV, devInfo->hwv},
        {STR_JSON_SWV, devInfo->swv},
        {STR_JSON_SUB_PROD_ID, devInfo->subProdId},
    };

    int32_t ret = UtilsJsonAddStrTable(devInfoObj, strItem, ARRAY_SIZE(strItem));
    if (ret != IOTC_OK) {
        IOTC_LOGE("add dev info err %d", ret);
        IotcJsonDelete(devInfoObj);
        return NULL;
    }

    ret = IotcJsonAddNum2Obj(devInfoObj, STR_JSON_PROT_TYPE, devInfo->protType);
    if (ret != IOTC_OK) {
        IOTC_LOGE("add prot err %d", ret);
        IotcJsonDelete(devInfoObj);
        return NULL;
    }

    return devInfoObj;
}