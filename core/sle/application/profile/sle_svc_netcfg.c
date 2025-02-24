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
#include "sle_svc_netcfg.h"
#include "securec.h"
#include "utils_assert.h"
#include "utils_json.h"
#include "sle_svc_ctx.h"
#include "iotc_svc_sle.h"
#include "service_proxy.h"
#include "iotc_errcode.h"
#include "product_adapter.h"
#include "iotc_svc_dev.h"
#include "iotc_svc_conn.h"

static int32_t SendSleNetCfgMsg(const char *info, uint32_t len)
{
    IotcJson *json = IotcJsonParseWithLen(info, len);
    if (json == NULL) {
        IOTC_LOGW("net info parse error");
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }

    SleSvcCtx *ctx = GetSleSvcCtx();

    ServiceRequestInfo reqInfo = {
        .instanceId = ctx->instanceId,
        .msgId = SLE_SERVICE_MSG_ID_CLOUD_SETUP,
        .req = json,
    };
    int32_t ret = ServiceProxySendMessage(&reqInfo, NULL, NULL);
    IotcJsonDelete(json);
    if (ret != IOTC_OK) {
        IOTC_LOGW("send msg error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static int32_t NetCfgProcessFromJson(const IotcJson *json)
{
    CHECK_RETURN_LOGW(json != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    int32_t ret = DevSvcProxyRecvBindInfo(json);
    if (ret != IOTC_OK) {
        IOTC_LOGW("recv bind info err %d", ret);
        return ret;
    }
    ret = ConnSvcProxySetNetCfgInfo(json);
    if (ret != IOTC_OK) {
        IOTC_LOGW("recv netcfg info err %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static int32_t NetCfgProcessFromString(const char *info, uint32_t len)
{
    IotcJson *json = IotcJsonParseWithLen(info, len);
    if (json == NULL) {
        IOTC_LOGW("net info parse error");
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    int32_t ret = NetCfgProcessFromJson(json);
    IotcJsonDelete(json);
    if (ret != IOTC_OK) {
        return ret;
    }
    return IOTC_OK;
}

int32_t PutSleSvcNetCfg(const BtCmdParam *param, uint8_t **out, uint32_t *outLen)
{
    CHECK_RETURN_LOGW((param != NULL) && (param->request != NULL) && (param->requestLen != 0) &&
        (out != NULL) && (outLen != NULL), IOTC_ERR_PARAM_INVALID, "invalid param");

    *out = NULL;
    *outLen = 0;
    int32_t ret = ProductRecvNetCfgInfo((const char *)param->request, param->requestLen);
    if (ret == IOTC_ERR_CALLBACK_NULL) {
        ret = NetCfgProcessFromString((const char *)param->request, param->requestLen);
        (void)SendSleNetCfgMsg((const char *)param->request, param->requestLen);
        IOTC_LOGN("net info process inside %d", ret);
    }
    if (ret != IOTC_OK) {
        IOTC_LOGW("recv net info error %d", ret);
    }

    return UtilsGenErrcodeJsonStr(ret, (char **)out, outLen);
}