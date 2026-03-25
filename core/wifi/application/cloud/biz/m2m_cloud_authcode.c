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
#include "m2m_cloud_authcode.h"
#include "utils_assert.h"
#include "utils_json.h"
#include "config_register_info.h"
#include "comm_def.h"
#include "securec.h"
#include "utils_common.h"
#include "config_login_info.h"
#include "m2m_cloud_errcode.h"
#include "utils_bit_map.h"
#include "m2m_cloud_utils.h"
#include "iotc_errcode.h"

const CloudOption *M2mCloudGetAuthCodeOption(void)
{
    static const char *sysActivate[] = {STR_URI_PATH_SYS, STR_URI_PATH_AUTHCODE};
    static const CloudOption AUTHCODE_OPTION = {
        .uri = sysActivate,
        .num = ARRAY_SIZE(sysActivate),
        .opBitMap = UTILS_BIT(CLOUD_OPTION_BIT_SEQ_NUM_ID) |
                    UTILS_BIT(CLOUD_OPTION_BIT_REQ_ID) |
                    UTILS_BIT(CLOUD_OPTION_BIT_DEV_ID) |
                    UTILS_BIT(CLOUD_OPTION_BIT_ACCESS_TOKEN_ID),
    };
    return &AUTHCODE_OPTION;
}

IotcJson *M2mCloudBuildAuthCodeRequest(M2mCloudContext *ctx)
{
    CHECK_RETURN_LOGW(ctx != NULL, NULL, "param invalid");
    int32_t ret = IOTC_OK;

    IotcJson *rootJson = IotcJsonCreate();
    if (rootJson == NULL) {
        IOTC_LOGW("create json error");
        return NULL;
    }

    ret = IotcJsonAddStr2Obj(rootJson, STR_NETINFO_DEVICE_ID, ctx->authInfo.loginInfo.devId);

    if (ret == IOTC_OK) {
        return rootJson;
    } else {
        IOTC_LOGW("add devid error %d", ret);
    }

    IotcJsonDelete(rootJson);
    return NULL;
}

int32_t M2mCloudParseAuthCodeResponse(M2mCloudContext *ctx, const CoapPacket *resp, int32_t *errcode)
{
    CHECK_RETURN_LOGW(ctx != NULL && resp != NULL && errcode != NULL && resp->payload.data != NULL &&
        resp->payload.len != 0, IOTC_ERR_PARAM_INVALID, "invalid param");
    char authcodeStr[HEXIFY_LEN(SESSION_AUTHCODE_LEN + 1) + 1] = {0};
    char authcodeIdStr[HEXIFY_LEN(SESSION_AUTHCODE_LEN + 1) + 1] = {0};

    IotcJson *respJson = IotcJsonParseWithLen((const char *)resp->payload.data, resp->payload.len);
    if (respJson == NULL) {
        IOTC_LOGW("create json error");
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }

    int32_t ret = UtilsJsonGetNum(respJson, STR_ERRCODE, errcode);
    if (ret != IOTC_OK) {
        IOTC_LOGE("json get errcode error %d", ret);
        IotcJsonDelete(respJson);
        return ret;
    }
    
    if (*errcode != CLOUD_ERRCODE_OK) {
        IOTC_LOGE("cloud authCode error %d", *errcode);
        IotcJsonDelete(respJson);
        return IOTC_OK;
    }

    ret = UtilsJsonGetString(respJson, STR_JSON_AUTHCODE, (char *)authcodeStr, sizeof(authcodeStr));
    if (ret != IOTC_OK) {
        IOTC_LOGE("json get authcode error %d", ret);
        IotcJsonDelete(respJson);
        return ret;
    }
    UtilsUnhexify(authcodeStr, strlen(authcodeStr), ctx->authCodeInfo.authCode, sizeof(ctx->authCodeInfo.authCode));
    ret = UtilsJsonGetString(respJson, STR_JSON_AUTHCODE_ID, (char *)authcodeIdStr, sizeof(authcodeIdStr));
    if (ret != IOTC_OK) {
        IOTC_LOGE("json get authcodeId error %d", ret);
        IotcJsonDelete(respJson);
        return ret;
    }
    UtilsUnhexify(authcodeIdStr, strlen(authcodeIdStr), ctx->authCodeInfo.authCodeId,
        sizeof(ctx->authCodeInfo.authCodeId));

    ret = UtilsJsonGetNum(respJson, STR_JSON_TIMEOUT, (int32_t *)&(ctx->authCodeInfo.timeout));
    if(ret != IOTC_OK) {
        IotcJsonDelete(respJson);
        return ret;
    }
    if(timeout > UINT32_MAX / UTILS_MS_PER_SECOND) {
        return IOTC_SDK_AILIFE_WIFI_ERR_CLOUD_TOKEN_TIMEOUT_INVALID;
    }
    ctx->authCodeInfo.timeout = timeout;
    (void)memset_s(authcodeStr, sizeof(authcodeStr), 0, sizeof(authcodeStr));
    (void)memset_s(authcodeIdStr, sizeof(authcodeIdStr), 0, sizeof(authcodeIdStr));
    return IOTC_OK;
}

