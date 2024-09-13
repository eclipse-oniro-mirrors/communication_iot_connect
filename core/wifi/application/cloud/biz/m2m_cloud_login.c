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
#include "m2m_cloud_login.h"
#include <stdlib.h>
#include "utils_json.h"
#include "utils_assert.h"
#include "config_login_info.h"
#include "securec.h"
#include "comm_def.h"
#include "utils_common.h"
#include "m2m_cloud_token.h"
#include "m2m_cloud_errcode.h"
#include "utils_bit_map.h"
#include "iotc_errcode.h"
#include "iotc_log.h"

IotcJson *M2mCloudBuildLoginRequest(M2mCloudContext *ctx)
{
    CHECK_RETURN_LOGW(ctx != NULL, NULL, "param invalid");
    IotcJson *rootJson = IotcJsonCreate();
    if (rootJson == NULL) {
        IOTC_LOGW("create json error");
        return NULL;
    }

    UtilsJsonStrItem jsonList[] = {
        {STR_JSON_DEVID, ctx->authInfo.loginInfo.devId},
        {STR_JSON_SECRET, ctx->authInfo.loginInfo.secret},
    };
    int32_t ret = UtilsJsonAddStrTable(rootJson, jsonList, ARRAY_SIZE(jsonList));
    if (ret != IOTC_OK) {
        IOTC_LOGE("add json str error %d", ret);
        IotcJsonDelete(rootJson);
        return NULL;
    }

    return rootJson;
}

int32_t M2mCloudLoginResponseParse(M2mCloudContext *ctx, const CoapPacket *resp, int32_t *errcode)
{
    CHECK_RETURN_LOGW(resp != NULL && errcode != NULL && ctx != NULL && resp->payload.data != NULL &&
        resp->payload.len != 0, IOTC_ERR_PARAM_INVALID, "invalid param");

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
    ret = DealErrCodeRsp(*errcode);
    if (ret == IOTC_OK) {
        IotcJsonDelete(respJson);
        return ret;
    }

    if (*errcode == CLOUD_ERRCODE_OK) {
        ret = ParseTokenInfo(ctx, respJson);
    }
    IotcJsonDelete(respJson);

    return ret;
}

const CloudOption *M2mCloudGetLoginOption(void)
{
    static const char *sysLogin[] = {STR_URI_PATH_SYS, STR_URI_PATH_LOGIN};
    static const CloudOption REG_OPTION = {
        .uri = sysLogin,
        .num = ARRAY_SIZE(sysLogin),
        .opBitMap = UTILS_BIT(CLOUD_OPTION_BIT_SEQ_NUM_ID),
    };
    return &REG_OPTION;
}