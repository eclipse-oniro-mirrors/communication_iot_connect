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
#include "m2m_cloud_dev_del.h"
#include "securec.h"
#include "iotc_errcode.h"
#include "utils_assert.h"
#include "utils_json.h"
#include "event_bus.h"
#include "iotc_json.h"
#include "coap_codec_utils.h"
#include "coap_endpoint_server.h"
#include "iotc_event.h"
#include "fwk_main.h"

static IotcJson *BuildRestartMessage(const char *devId)
{
    IotcJson *respJson = IotcJsonCreate();
    if (respJson == NULL) {
        IOTC_LOGE("Failed to create respJson JSON");
        return NULL;
    }
    IotcJson *payload = IotcJsonCreate();
    if (payload == NULL) {
        IOTC_LOGE("Failed to create payload JSON");
        IotcJsonDelete(respJson);
        return NULL;
    }
    IotcJson *dataarray = IotcJsonCreateArray();
    if (dataarray == NULL) {
        IOTC_LOGE("Failed to create dataarray JSON");
        IotcJsonDelete(respJson);
        IotcJsonDelete(payload);
        return NULL;
    }
    IotcJson *data = IotcJsonCreate();
    if (data == NULL) {
        IOTC_LOGE("Failed to create data JSON");
        IotcJsonDelete(respJson);
        IotcJsonDelete(payload);
        IotcJsonDelete(dataarray);
        return NULL;
    }
    IotcJsonAddNum2Obj(data, "restart", 1);
    IotcJsonAddItem2Obj(payload, "data", data);
    IotcJsonAddStr2Obj(payload, "st", "restart");
    IotcJsonAddItem2Array(dataarray, payload);
    IotcJsonAddItem2Obj(respJson, "data", dataarray);
    IotcJsonAddStr2Obj(respJson, "sid", "restart");
    IotcJsonAddStr2Obj(respJson, "msgId", "123456789");
    IotcJsonAddStr2Obj(respJson, "devId", devId);
    return respJson;
}

static int32_t SendData2SubDev(const CoapPacket *req)
{
    IotcJson *dataObj = IotcJsonParseWithLen((const char *)req->payload.data, req->payload.len);
    if (dataObj == NULL) {
        IOTC_LOGW("invalid data json");
        return IOTC_ERROR;
    }
    const char *devId = IotcJsonGetStr(IotcJsonGetObj(dataObj, STR_JSON_DEVID));

    IotcJson *respJson = BuildRestartMessage(devId);
    if (respJson == NULL) {
        IotcJsonDelete(dataObj);
        return IOTC_ERROR;
    }

    IotcJson *array = IotcJsonCreateArray();
    if (array == NULL) {
        IOTC_LOGE("create array error");
        IotcJsonDelete(respJson);
        IotcJsonDelete(dataObj);
        return IOTC_ERROR;
    }
    IotcJsonAddItem2Array(array, respJson);

    int32_t ret = DevSvcProxyCtlPutCharStates(array, NULL);
    IotcJsonDelete(array);
    IotcJsonDelete(dataObj);

    return ret;
}
    const char *devId = IotcJsonGetStr(IotcJsonGetObj(dataObj, STR_JSON_DEVID));

    /* 创建JSON指针 */
    /* {"data":[{"st":"restart","data":{"restart":1}}],"sid":"restart","msgId":"123456789","devId":"xxx"} */
    IotcJson *respJson = IotcJsonCreate();
    if (respJson == NULL) {
        IOTC_LOGE("Failed to create respJson JSON");
        return IOTC_ERROR;
    }
    IotcJson *payload = IotcJsonCreate();
    if (payload == NULL) {
        IOTC_LOGE("Failed to create payload JSON");
        IotcJsonDelete(respJson);
        return IOTC_ERROR;
    }
    IotcJson *dataarray = IotcJsonCreateArray();
    if (dataarray == NULL) {
        IOTC_LOGE("Failed to create dataarray JSON");
        IotcJsonDelete(respJson);
        IotcJsonDelete(payload);
        return IOTC_ERROR;
    }
    IotcJson *data = IotcJsonCreate();
    if (data == NULL) {
        IOTC_LOGE("Failed to create data JSON");
        IotcJsonDelete(respJson);
        IotcJsonDelete(payload);
        IotcJsonDelete(dataarray);
        return IOTC_ERROR;
    }
    IotcJsonAddNum2Obj(data, "restart", 1);
    IotcJsonAddItem2Obj(payload, "data", data);
    IotcJsonAddStr2Obj(payload, "st", "restart");
    IotcJsonAddItem2Array(dataarray, payload);
    IotcJsonAddItem2Obj(respJson, "data", dataarray);
    IotcJsonAddStr2Obj(respJson, "sid", "restart");
    IotcJsonAddStr2Obj(respJson, "msgId", "123456789");
    IotcJsonAddStr2Obj(respJson, "devId", devId);

    IotcJson *array = IotcJsonCreateArray();
    if (array == NULL) {
        IOTC_LOGE("create array error");
        IotcJsonDelete(respJson);
        return IOTC_ERROR;
    }
    IotcJsonAddItem2Array(array, respJson);

    int32_t ret = DevSvcProxyCtlPutCharStates(array, NULL);
    IotcJsonDelete(array);

    return ret;
}

static bool CheckIsValidDevId(const CoapPacket *req)
{
    IotcJson *dataObj = IotcJsonParseWithLen((const char *)req->payload.data, req->payload.len);
    if (dataObj == NULL) {
        IOTC_LOGW("invalid data json");
        return false;
    }

    const char *devId = IotcJsonGetStr(IotcJsonGetObj(dataObj, STR_JSON_DEVID));
    if (devId == NULL || strcmp(devId, GetM2mCloudCtx()->authInfo.loginInfo.devId) != 0) {
        IotcJsonDelete(dataObj);
        return false;
    }
    IOTC_LOGI("Check devId ok");
    IotcJsonDelete(dataObj);
    return true;
}

static void M2mCloudCoapDeviveDelHandler(CoapEndpoint *endpoint, const CoapPacket *req,
    const SocketAddr *addr, void *userData)
{
    CHECK_V_RETURN_LOGW(endpoint != NULL && req != NULL && addr != NULL && userData != NULL, "invalid param");

    if (!CheckIsValidDevId(req)) {
        if (SendData2SubDev(req) != IOTC_OK) {
            IOTC_LOGE("Send del message to subdev failed!");
        }
        return;
    }

    IotcJson *respJson = UtilsJsonCreateErrcode(0);
    if (respJson == NULL) {
        IOTC_LOGW("create resp json error");
        return;
    }

    CoapServerRespParam respParam = {
        .req = req,
        .type = COAP_MSG_TYPE_NCON,
        .code = COAP_RESPONSE_CODE_CONTENT,
        .opNum = 0,
        .options = NULL,
        .payload = NULL,
        .payloadBuilder = CoapUtilsBuildJsonPayloadFunc,
        .payloadUserData = respJson,
        .preSize = 0,
    };
    CoapPacket packet;
    int32_t ret = CoapServerSendResp(endpoint, &respParam, addr, &packet);
    IotcJsonDelete(respJson);
    respJson = NULL;
    if (ret != IOTC_OK) {
        IOTC_LOGW("send e2e ctrl resp msg error %d", ret);
    }

    ret = DevSvcProxyCleanLoginInfo();
    if (ret != IOTC_OK) {
        IOTC_LOGW("clean loginInfo error %d", ret);
        return;
    }
    ret = IotcFwkRestore();
    if (ret != IOTC_OK) {
        IOTC_LOGF("restore error %d", ret);
    }
}

int32_t M2mCloudDeviveDelInit(M2mCloudContext *ctx)
{
    CHECK_RETURN(ctx != NULL, IOTC_ERR_PARAM_INVALID);
    CHECK_RETURN(ctx->linkInfo.endpoint != NULL, IOTC_CORE_WIFI_TRANS_ERR_COAP_ENDPOINT_CREATE);

    static const CoapResource DEV_DEL_COAP_RES[] = {
        {UTILS_BIT(COAP_METHOD_TYPE_POST), STR_URI_PATH_DEV_DEL, NULL, M2mCloudCoapDeviveDelHandler},
    };

    int32_t ret = CoapServerAddResource(ctx->linkInfo.endpoint, DEV_DEL_COAP_RES, ARRAY_SIZE(DEV_DEL_COAP_RES));
    if (ret != IOTC_OK) {
        IOTC_LOGW("add coap res error %d", ret);
        return ret;
    }

    return IOTC_OK;
}
