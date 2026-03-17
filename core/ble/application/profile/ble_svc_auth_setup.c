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
#include "ble_svc_auth_setup.h"
#include "utils_assert.h"
#include "utils_json.h"
#include "iotc_svc_dev.h"
#include "iotc_errcode.h"

static int32_t SaveAuthSetupInfo(char *in)
{
    IotcJson *req = IotcJsonParse((const char *)in);
    CHECK_RETURN_LOGE(req != NULL, IOTC_ADAPTER_JSON_ERR_PARSE, "parse json err");

    int32_t ret = DevSvcProxyRecvAuthInfo(req);
    IotcJsonDelete(req);
    if (ret != IOTC_OK) {
        IOTC_LOGW("auth info process error %d", ret);
        return ret;
    }

    #ifdef HI3863_SDK_CONFIG_JSON_PATH
    const char *path = HI3863_SDK_CONFIG_JSON_PATH;
    IOTC_LOGI("SaveAuthSetupInfo IotcKvSetValue path:%s  strLen:%d", path, strlen(in));
    IotcKvSetValue(path, (const uint8_t *)in, strlen(in));
    #endif
    
    return IOTC_OK;
}

int32_t GetBleSvcAuthSetup(const BtCmdParam *param, uint8_t **out, uint32_t *outLen)
{
    CHECK_RETURN_LOGW((param != NULL) && (param->request != NULL) && (param->requestLen != 0) &&
        (out != NULL) && (outLen != NULL), IOTC_ERR_PARAM_INVALID, "invalid param");

    *out = NULL;
    *outLen = 0;
    int32_t ret = SaveAuthSetupInfo((char*)param->request);
    return UtilsGenErrcodeJsonStr(ret, (char **)out, outLen);
}