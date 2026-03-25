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
#include "m2m_cloud_send.h"
#include "coap_codec_utils.h"
#include "securec.h"
#include "iotc_errcode.h"
#include "utils_assert.h"
#include "utils_bit_map.h"
#include "iotc_json.h"
#include "security_random.h"
#include "m2m_cloud_report.h"
static int32_t GetResponeDevId(IotcJson *msg, IotcJson **savedDevid)
{
    if (msg == NULL || savedDevid == NULL) {
        return IOTC_ERR_INVALID_PARAM;
    }

    uint32_t arraySize = 0;
    IotcJsonGetArraySize(msg, &arraySize);

    for (int32_t i = 0; i < arraySize; i++) {
        IotcJson *item = IotcJsonGetArrayItem(msg, i);
        if (item == NULL) {
            continue;
        }

        // 提取并保存 devid
        if (IotcJsonHasObj(item, STR_JSON_DEVID)) {
            if (*savedDevid == NULL && (strcmp(IotcJsonGetObj(item, STR_JSON_DEVID), "0") != 0)) {
                *savedDevid = IotcDuplicateJson(IotcJsonGetObj(item, STR_JSON_DEVID), true);
            }
            IotcJsonDeleteItem(item, STR_JSON_DEVID);
        }

        // 删除msg_id
        if (IotcJsonHasObj(item, STR_JSON_MSG_ID)) {
            IotcJsonDeleteItem(item, STR_JSON_MSG_ID);
        }
    }
    return IOTC_OK;
}

static IotcJson *WrapReportJsonInArray(IotcJson *reportJson)
{
    IotcJson *array = IotcJsonCreateArray();
    if (array == NULL) {
        IOTC_LOGW("json create error");
        return NULL;
    }
    int32_t ret = IotcJsonAddItem2Array(array, reportJson);
    if (ret != IOTC_OK) {
        IOTC_LOGW("add to array error %d", ret);
        IotcJsonDelete(reportJson);
        IotcJsonDelete(array);
        return NULL;
    }
    return array;
}

static IotcJson *GenReportJson(const IotcJson *reqJson, const M2mCloudContext *ctx)
{
    IotcJson *dataArray = IotcJsonGetObj(reqJson, STR_JSON_VENDOR);
    if (dataArray == NULL) {
        IOTC_LOGE("Get Vendor failed");
        return NULL;
    }

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

    IotcJson *savedDevid = NULL;
    GetResponeDevId(dataArrayClone, &savedDevid);

    int32_t ret = IotcJsonAddItem2Obj(reportJson, STR_JSON_SERVICES, dataArrayClone);
    if (ret != IOTC_OK) {
        IotcJsonDelete(reportJson);
        IotcJsonDelete(dataArrayClone);
        IOTC_LOGW("json add item error %d", ret);
        return NULL;
    }
    dataArrayClone = NULL;

    // 判断是否是桥设备的本身上报
    if (savedDevid) {
        IOTC_LOGW("savedDevid = %s", IotcJsonGetStr(savedDevid));
        ret = IotcJsonAddStr2Obj(reportJson, STR_JSON_DEVID, IotcJsonGetStr(savedDevid));
        IotcJsonDelete(savedDevid); // 删除保存的devid
    } else {
        ret = IotcJsonAddStr2Obj(reportJson, STR_JSON_DEVID, ctx->authInfo.loginInfo.devId);
    }

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

    if (ret != IOTC_OK) {
        IotcJsonDelete(reportJson);
        IOTC_LOGW("json add item error %d", ret);
        return NULL;
    }
    return WrapReportJsonInArray(reportJson);
}

int32_t M2mCloudReportMessage(const IotcJson *dataArray, M2mCloudContext *ctx)
{
    CHECK_RETURN_LOGW(dataArray != NULL && ctx != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    IotcJson *reportJson = GenReportJson(dataArray, (const M2mCloudContext *)ctx);
    CHECK_RETURN_LOGW(reportJson != NULL, IOTC_ERR_PARAM_INVALID, "gen report error");

    {
        uint8_t Reqid[REQ_ID_LEN] = {0};
        // 生成随机数
        SecurityRandom((uint8_t *)Reqid, REQ_ID_LEN);
        UtilsHexify(Reqid, sizeof(Reqid), ctx->reqId, sizeof(ctx->reqId));
        ctx->reqId[HEXIFY_LEN(REQ_ID_LEN)] = '\0';
    }

    const CoapOption options[] = {
        {COAP_OPTION_TYPE_URI_PATH, {(const uint8_t *)STR_URI_PATH_SYS, strlen(STR_URI_PATH_SYS)}},
        {COAP_OPTION_TYPE_URI_PATH, {(const uint8_t *)STR_JSON_DATA, strlen(STR_JSON_DATA)}},
        {COAP_OPTION_TYPE_ACCESS_TOKEN_ID, {(const uint8_t *)ctx->tokenInfo.access, strlen(ctx->tokenInfo.access)}},
        {COAP_OPTION_TYPE_REQ_ID, {(const uint8_t *)ctx->reqId, strlen(ctx->reqId)}},
        {COAP_OPTION_TYPE_DEV_ID,
            {(const uint8_t *)ctx->authInfo.loginInfo.devId, strlen(ctx->authInfo.loginInfo.devId)}},
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
