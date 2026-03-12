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
#include "sle_svc_net_cfg_ver.h"
#include "iotc_json.h"
#include "utils_common.h"
#include "utils_assert.h"
#include "utils_json.h"
#include "iotc_errcode.h"
#include "comm_def.h"

#define SLE_CFG_NET_VER 2

int32_t GetSleSvcNetCfgVer(const BtCmdParam *param, uint8_t **out, uint32_t *outLen)
{
    NOT_USED(param);
    CHECK_RETURN_LOGW((out != NULL) && (outLen != NULL), IOTC_ERR_PARAM_INVALID, "invalid param");

    *out = NULL;
    *outLen = 0;
    IotcJson *root = IotcJsonCreate();
    if (root == NULL) {
        IOTC_LOGE("create err");
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }
    int32_t ret = IOTC_ERROR;
    do {
        if (IotcJsonAddNum2Obj(root, STR_JSON_VER, SLE_CFG_NET_VER) != IOTC_OK) {
            IOTC_LOGE("add ver err");
            ret = IOTC_ADAPTER_JSON_ERR_ADD;
            break;
        }
        char *outStr = UtilsJsonPrintByMalloc(root);
        if (outStr == NULL) {
            IOTC_LOGE("json print err");
            ret = IOTC_CORE_COMM_UTILS_ERR_JSON_MALLOC_PRINT;
            break;
        }
        *out = (uint8_t *)outStr;
        *outLen = strlen(outStr);
        ret = IOTC_OK;
    } while (false);
    IotcJsonDelete(root);

    return ret;
}