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
#include "sle_svc_auth_setup.h"
#include "utils_assert.h"
#include "utils_json.h"
#include "iotc_svc_dev.h"
#include "iotc_errcode.h"
#include "iotc_sle_def.h"
#include "securec.h"

static int32_t GetSleSvcAuthSetupInfoReq(uint8_t **out, uint32_t *outLen)
{
    int32_t ret = IOTC_OK;
    IotcJson *devSetup = IotcJsonCreate();
    if (devSetup == NULL) {
        IOTC_LOGE("create json err");
        return IOTC_ERR_SECUREC_SPRINTF;
    }

    DevAuthInfo authInfo = {0};
    bool isAuthInfoExist = false;
    if (DevSvcProxyGetAuthInfo(&isAuthInfoExist, &authInfo) != IOTC_OK) {
        IOTC_LOGE("get auth info err");
        return IOTC_ERR_SECUREC_SPRINTF;
    }
    if (!isAuthInfoExist) {
        IOTC_LOGE("get auth info err");
        return IOTC_ERR_SECUREC_SPRINTF;
    }

    if (IotcJsonAddStr2Obj(devSetup, STR_JSON_DEVID, (const char*)authInfo.devId) != IOTC_OK) {
        IOTC_LOGE("add dev id err");
        return IOTC_ERR_SECUREC_SPRINTF;
    }

    if (IotcJsonAddStr2Obj(devSetup, STR_JSON_UIDHASH, (const char*)authInfo.uidHash) != IOTC_OK) {
        IOTC_LOGE("uidHash err");
        return IOTC_ERR_SECUREC_SPRINTF;
    }

    if (IotcJsonAddStr2Obj(devSetup, STR_JSON_AUTHCODE_ID, (const char*)authInfo.authCodeId) != IOTC_OK) {
        IOTC_LOGE("add authCodeId err");
        return IOTC_ERR_SECUREC_SPRINTF;
    }

    if (IotcJsonAddStr2Obj(devSetup, STR_JSON_AUTHCODE, (const char*)authInfo.authCode) != IOTC_OK) {
        IOTC_LOGE("add model err");
        return IOTC_ERR_SECUREC_SPRINTF;
    }

    if (IotcJsonAddNum2Obj(devSetup, PROFILE_DATA_MESSAGE_JSON, MSG_PROFILE_TYPE_RSP) != IOTC_OK) {
        IOTC_LOGE("add msg message err ret=%d", ret);
        return IOTC_ERR_SECUREC_SPRINTF;
    }

    char *outStr = UtilsJsonPrintByMalloc(devSetup);
    if (outStr == NULL) {
        IOTC_LOGE("json print err");
        ret = IOTC_CORE_COMM_UTILS_ERR_JSON_MALLOC_PRINT;
    }
    *out = (uint8_t *)outStr;
    *outLen = strlen(outStr);
    return ret;
}

static int32_t GetSleSvcAuthSetupInfoRsp(uint8_t **out, uint32_t *outLen)
{
    CHECK_RETURN_LOGW((out != NULL) && (outLen != NULL), IOTC_ERR_PARAM_INVALID, "invalid param");
    *out = NULL;
    *outLen = 0;
    IotcJson *root = IotcJsonCreate();
    if (root == NULL) {
        IOTC_LOGE("create err");
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }
    int32_t ret = 0;
    do {
        //给sevser返回收到
        if (IotcJsonAddNum2Obj(root, PROFILE_DATA_MESSAGE_JSON, MSG_PROFILE_TYPE_NONE) != IOTC_OK) {
            IOTC_LOGE("add msg type err ret=%d", ret);
            break;
        }
        if (IotcJsonAddNum2Obj(root, PROFILE_DATA_ERRCODE, 0) != IOTC_OK) {
            IOTC_LOGE("add prod id err");
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

static int32_t SaveAuthSetupConfigInfo(IotcJson *req)
{
    CHECK_RETURN_LOGE(req != NULL, IOTC_ADAPTER_JSON_ERR_PARSE, "parse json err");

    int32_t ret = DevSvcProxyRecvAuthInfo(req);
    IotcJsonDelete(req);
    if (ret != IOTC_OK) {
        IOTC_LOGW("auth info process error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

int32_t GetSleSvcAuthSetup(const SleCmdParam *param, uint8_t **out, uint32_t *outLen)
{
    CHECK_RETURN_LOGW((param != NULL)&&(out != NULL) && (outLen != NULL), IOTC_ERR_PARAM_INVALID, "invalid param");
    IotcJson *root = IotcJsonParse((const char *)param->request);
    if (root == NULL) {
        IOTC_LOGE("GetSleSvcAuthSetup proc reqPayload err");
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }

    IotcJson *msgTypeObj = IotcJsonGetObj(root, PROFILE_DATA_MESSAGE_JSON);
    if (msgTypeObj == NULL) {
        IOTC_LOGE("GetSleSvcAuthSetup msgType JSON err");
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    int64_t msgTypeInt = 0;
    int32_t ret = IotcJsonGetNum(msgTypeObj, &msgTypeInt);
    if (ret != IOTC_OK) {
        IOTC_LOGE("0 parse msgType err");
        return ret;
    }

    IOTC_LOGI("GetSleSvcAuthSetup msgTypeInt=%d", msgTypeInt);
    switch ((MsgProfileType)msgTypeInt) {
        case MSG_PROFILE_TYPE_REQ:
            ret = GetSleSvcAuthSetupInfoReq(out, outLen);
            break;

        case MSG_PROFILE_TYPE_ISSUE:
            ret = SaveAuthSetupConfigInfo(root);
            if (ret != IOTC_OK) {
                IOTC_LOGE("save device info fail");
            }
            ret = GetSleSvcAuthSetupInfoRsp(out, outLen);
            break;
        case MSG_PROFILE_TYPE_NONE:
            *out = 0x00;
            break;
        default:
            break;
    }
    IotcJsonDelete(root);
    return ret;
}
