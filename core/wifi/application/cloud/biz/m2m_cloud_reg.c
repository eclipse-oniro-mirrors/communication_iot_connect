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
#include "m2m_cloud_reg.h"
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
#include "iotc_svc_dev.h"

const CloudOption *M2mCloudGetRegisterOption(void)
{
    static const char *sysActive[] = {STR_URI_PATH_SYS, STR_URI_PATH_ACTIVATE};
    static const CloudOption REG_OPTION = {
        .uri = sysActive,
        .num = ARRAY_SIZE(sysActive),
        .opBitMap = UTILS_BIT(CLOUD_OPTION_BIT_SEQ_NUM_ID),
    };
    return &REG_OPTION;
}

static int32_t ParseRegRespSenseInfo(M2mCloudContext *ctx, IotcJson *jsonObj)
{
    char pskHex[HEXIFY_LEN(sizeof(ctx->authInfo.regInfo.psk)) + 1] = {0};
    if (!UtilsHexify(ctx->authInfo.regInfo.psk, sizeof(ctx->authInfo.regInfo.psk), pskHex, sizeof(pskHex))) {
        IOTC_LOGW("psk hexify error");
        return IOTC_CORE_COMM_UTILS_ERR_HEXIFY;
    }

    UtilsJsonStrItem loginInfo[] = {
        { STR_NETINFO_PSK, pskHex },
        { STR_NETINFO_URL, ctx->authInfo.regInfo.url },
        { STR_NETINFO_BACKUP_URL, ctx->authInfo.regInfo.backupUrl },
    };

    int32_t ret = UtilsJsonAddStrTable(jsonObj, loginInfo, ARRAY_SIZE(loginInfo));
    (void)memset_s(pskHex, sizeof(pskHex), 0, sizeof(pskHex));
    if (ret != IOTC_OK) {
        IOTC_LOGW("add str to json error %d", ret);
        return ret;
    }

    ret = DevSvcProxyRecvLoginInfo(jsonObj);
    if (ret != IOTC_OK) {
        IOTC_LOGW("save login info error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

IotcJson *M2mCloudBuildRegisterRequest(M2mCloudContext *ctx)
{
    CHECK_RETURN_LOGW(ctx != NULL, NULL, "param invalid");

    IotcJson *rootJson = IotcJsonCreate();
    if (rootJson == NULL) {
        IOTC_LOGW("create json error");
        return NULL;
    }

    int32_t ret;
    do {
        ret = IotcJsonAddStr2Obj(rootJson, STR_JSON_CODE, ctx->authInfo.regInfo.code);
        if (ret != IOTC_OK) {
            IOTC_LOGW("add code error %d", ret);
            break;
        }

        ret = M2mCloudAddDevInfoToJson(rootJson);
        if (ret != IOTC_OK) {
            IOTC_LOGW("add dev info error %d", ret);
            break;
        }
    } while (0);

    if (ret == IOTC_OK) {
        return rootJson;
    }

    IotcJsonDelete(rootJson);
    return NULL;
}

int32_t M2mCloudParseRegisterResponse(M2mCloudContext *ctx, const CoapPacket *resp, int32_t *errcode)
{
    CHECK_RETURN_LOGW(ctx != NULL && resp != NULL && errcode != NULL && resp->payload.data != NULL &&
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

    if (*errcode != CLOUD_ERRCODE_OK) {
        IOTC_LOGE("reg to cloud error %d", *errcode);
        IotcJsonDelete(respJson);
        return IOTC_OK;
    }

    IOTC_LOGN("reg to cloud success");
    ret = ParseRegRespSenseInfo(ctx, respJson);
    if (ret != IOTC_OK) {
        return ret;
    }

    return IOTC_OK;
}