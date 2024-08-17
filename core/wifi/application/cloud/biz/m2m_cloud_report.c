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
#include "m2m_cloud_report.h"
#include "m2m_cloud_send.h"
#include "coap_codec_utils.h"
#include "securec.h"
#include "iotc_errcode.h"
#include "utils_assert.h"
#include "utils_bit_map.h"
#include "adapter_json.h"

static IotcJson *GenReportJson(const IotcJson *dataArray, const M2mCloudContext *ctx)
{
    IotcJson *reportJson = IotcJsonCreate();
    if (reportJson == NULL) {
        IOTC_LOGW("json create error");
        return NULL;
    }

    IotcJson *dataArrayClone = IotcDuplicateJson(dataArray, true);
    if (dataArrayClone == NULL) {
        IotcJsonDelete(reportJson);
        IOTC_LOGW("json clone error");
        return NULL;
    }
    int32_t ret = IotcJsonAddItem2Obj(reportJson, STR_JSON_SERVICES, dataArrayClone);
    if (ret != IOTC_OK) {
        IotcJsonDelete(reportJson);
        IotcJsonDelete(dataArrayClone);
        IOTC_LOGW("json add item error %d", ret);
        return NULL;
    }
    dataArrayClone = NULL;

    ret = IotcJsonAddStr2Obj(reportJson, STR_JSON_DEVID, ctx->authInfo.loginInfo.devId);
    if (ret != IOTC_OK) {
        IotcJsonDelete(reportJson);
        IOTC_LOGW("json add item error %d", ret);
        return NULL;
    }
    IotcJson *array = IotcJsonCreateArray();
    if (array == NULL) {
        IOTC_LOGW("json create error");
        return NULL;
    }

    ret = IotcJsonAddItem2Array(array, reportJson);
    if (ret != IOTC_OK) {
        IOTC_LOGW("add to array error %d", ret);
        IotcJsonDelete(reportJson);
        IotcJsonDelete(array);
        return NULL;
    }

    return array;
}

int32_t M2mCloudReportMessage(const IotcJson *dataArray, M2mCloudContext *ctx)
{
    CHECK_RETURN_LOGW(dataArray != NULL && ctx != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    IotcJson *reportJson = GenReportJson(dataArray, (const M2mCloudContext *)ctx);
    CHECK_RETURN_LOGW(reportJson != NULL, IOTC_ERR_PARAM_INVALID, "gen report error");
    
    const CoapOption options[] = {
        {COAP_OPTION_TYPE_URI_PATH, {(const uint8_t *)STR_URI_PATH_SYS, strlen(STR_URI_PATH_SYS)}},
        {COAP_OPTION_TYPE_URI_PATH, {(const uint8_t *)STR_JSON_DATA, strlen(STR_JSON_DATA)}},
        {COAP_OPTION_TYPE_ACCESS_TOKEN_ID, {(const uint8_t *)ctx->tokenInfo.access, strlen(ctx->tokenInfo.access)}},
        {COAP_OPTION_TYPE_SEQ_NUM_ID, {NULL, sizeof(uint32_t)}},
    };
    CoapClientReqParam param = {
        .type = COAP_MSG_TYPE_CON,
        .code = COAP_METHOD_TYPE_POST,
        .opNum = ARRAY_SIZE(options),
        .options = options,
        .payload = NULL,
        .payloadBuilder = CoapUtilsBuildJsonPayloadFunc,
        .payloadUserData = reportJson,
        .respHandler = NULL,
        .preSize = 0,
    };

    CoapPacket packet;
    int32_t ret = CoapClientSendReq(ctx->linkInfo.endpoint, &param, NULL, &packet);
    IotcJsonDelete(reportJson);
    if (ret != IOTC_OK) {
        IOTC_LOGW("send req error %d", ret);
        return ret;
    }
    IOTC_LOGI("send cloud report succ");
    return IOTC_OK;
}