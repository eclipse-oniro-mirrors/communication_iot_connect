/*
 * Copyright (c) 2024-2024 ShenZhen Kaihong Device Co., Ltd.
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

#include "utils_json.h"
#include "sle_client_auth_setup.h"
#include <string.h>
#include "utils_assert.h"
#include "utils_json.h"
#include "iotc_errcode.h"
#include "sle_svc_auth_setup.h"
#include "comm_def.h"
int32_t CreateSvcAuthSetupIssueReq(uint8_t **out, uint32_t *outLen)
{
    CHECK_RETURN_LOGW((out != NULL) && (outLen != NULL), IOTC_ERR_PARAM_INVALID, "invalid param");
    *out = NULL;
    *outLen = 0;
    IotcJson *root = IotcJsonCreate();
    if (root == NULL) {
        IOTC_LOGE("create err");
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }

    int32_t ret = IOTC_OK;
    do {
        ret = IotcJsonAddNum2Obj(root, AUTH_DATA_MESSAGE_JSON, MSG_AUTH_TYPE_ISSUE);
        if (ret != IOTC_OK) {
            IOTC_LOGE("add msg type err ret=%d", ret);
            break;
        }

        ret = IotcJsonAddStr2Obj(root, STR_JSON_DEVID, "1234567890123456789012345678901234567890");
        if (ret != IOTC_OK) {
            IOTC_LOGE("add devId err ret=%d", ret);
            break;
        }

        const char hexStr[] = "48656C6C6F";
        ret = IotcJsonAddStr2Obj(root, STR_JSON_AUTHCODE, hexStr);
        if (ret != IOTC_OK) {
            IOTC_LOGE("add devType err ret=%d", ret);
            break;
        }
        ret = IotcJsonAddStr2Obj(root, STR_JSON_AUTHCODE_ID, "12345678901234567890123456789");
        if (ret != IOTC_OK) {
            IOTC_LOGE("add sn err ret=%d", ret);
            break;
        }

        ret = IotcJsonAddStr2Obj(root, STR_JSON_UIDHASH, "12345678901234567890123456789012345678900");
        if (ret != IOTC_OK) {
            IOTC_LOGE("add model err ret=%d", ret);
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

int32_t CreateSvcAuthSetupGetReq(uint8_t **out, uint32_t *outLen)
{
    CHECK_RETURN_LOGW((out != NULL) && (outLen != NULL), IOTC_ERR_PARAM_INVALID, "invalid param");
    *out = NULL;
    *outLen = 0;
    IotcJson *root = IotcJsonCreate();
    if (root == NULL) {
        IOTC_LOGE("create err");
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }

    int32_t ret = IOTC_OK;
    do {
        ret = IotcJsonAddNum2Obj(root, AUTH_DATA_MESSAGE_JSON, MSG_AUTH_TYPE_REQ);
        if (ret != IOTC_OK) {
            IOTC_LOGE("add msg type err ret=%d", ret);
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

