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
#include "m2m_cloud_ctl.h"
#include "securec.h"
#include "iotc_errcode.h"
#include "utils_assert.h"
#include "utils_json.h"
#include "adapter_json.h"
#include "coap_codec_utils.h"
#include "coap_endpoint_server.h"
#include "e2e_ctl_msg.h"

static int32_t GetSvcIdFromOption(const CoapPacket *req, char *svcId, uint32_t svcIdLen)
{
    uint32_t seg = 0;
    const CoapOption *uriOption = CoapUtilsFindOption(req, COAP_OPTION_TYPE_URI_PATH, &seg);
    if (uriOption == NULL || seg != 1 || uriOption->value.data == NULL || uriOption->value.len == 0) {
        IOTC_LOGW("invalid cloud packet");
        return IOTC_CORE_WIFI_M2M_ERR_CLOUD_GET_OPTION;
    }

    if (strncpy_s(svcId, svcIdLen, uriOption->value.data, uriOption->value.len) != EOK) {
        ADAPTER_LOGE("strcpy error %u", uriOption->value.len);
        return IOTC_ERR_SECUREC_STRCPY;
    }

    return IOTC_OK;
}

static AdapterJson *ParseCloudCtlMsg(const CoapPacket *req)
{
    char svcId[IOTC_OH_SVC_ID_STR_MAX_LEN] = {0};
    int32_t ret = GetSvcIdFromOption(req, svcId, sizeof(svcId));
    if (ret != IOTC_OK) {
        IOTC_LOGW("Get svcId error %d", ret);
        return NULL;
    }

    AdapterJson *array = AdapterJsonCreateArray();
    if (array == NULL) {
        IOTC_LOGW("create array error %d", ret);
        return NULL;
    }

    do {
        AdapterJson *payloadObj = AdapterCreateJson();
        if (payloadObj == NULL) {
            IOTC_LOGW("add data error");
            break;
        }
        AdapterJson *dataObj = AdapterJsonParseWithLen((const char *)req->payload.data, req->payload.len);
        if (dataObj == NULL) {
            IOTC_LOGW("invalid ctl json");
            AdapterJsonDelete(payloadObj);
            break;
        }
        ret = AdapterJsonAddItem2Obj(payloadObj, STR_JSON_DATA, dataObj);
        if (ret != IOTC_OK) {
            IOTC_LOGW("add data error %d", ret);
            AdapterJsonDelete(payloadObj);
            AdapterJsonDelete(dataObj);
            break;
        }
        ret = AdapterJsonAddStr2Obj(payloadObj, STR_JSON_SID, svcId);
        if (ret != IOTC_OK) {
            IOTC_LOGW("add sid error %d", ret);
            AdapterJsonDelete(payloadObj);
            break;
        }
        ret = AdapterJsonAddItem2Array(array, payloadObj);
        if (ret != IOTC_OK) {
            IOTC_LOGW("add payload error %d", ret);
            AdapterJsonDelete(payloadObj);
            break;
        }
        return array;
    } while (false);

    AdapterJsonDelete(array);
    return NULL;
}

static void M2mCloudCoapControlHandler(CoapEndpoint *endpoint, const CoapPacket *req,
    const SocketAddr *addr, void *userData)
{
    CHECK_V_RETURN_LOGW(endpoint != NULL && req != NULL && addr != NULL && userData != NULL, "invalid param");

    AdapterJson *dataJsonArray = ParseCloudCtlMsg(req);
    if (dataJsonArray == NULL) {
        IOTC_LOGW("Parse Cloud Ctl Msg error");
        return;
    }

    int32_t ret = DevSvcProxyCtlPutCharStates(dataJsonArray, NULL);
    AdapterJsonDelete(dataJsonArray);
    dataJsonArray = NULL;
    if (ret != IOTC_OK) {
        IOTC_LOGE("ctrl error %d", ret);
    }

    AdapterJson *respJson = UtilsJsonCreateErrcode(ret);
    if (respJson == NULL) {
        IOTC_LOGW("create resp json error %d", ret);
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
    ret = CoapServerSendResp(endpoint, &respParam, addr, &packet);
    AdapterJsonDelete(respJson);
    if (ret != IOTC_OK) {
        IOTC_LOGW("send e2e ctrl resp msg error %d", ret);
    }
}

int32_t M2mCloudCtlInit(M2mCloudContext *ctx)
{
    CHECK_RETURN(ctx != NULL, IOTC_ERR_PARAM_INVALID);
    CHECK_RETURN(ctx->linkInfo.endpoint != NULL, IOTC_CORE_WIFI_TRANS_ERR_COAP_ENDPOINT_CREATE);

    int32_t ret = CoapServerAddDefaultReqHandler(ctx->linkInfo.endpoint, M2mCloudCoapControlHandler);
    if (ret != IOTC_OK) {
        IOTC_LOGW("add coap res error %d", ret);
        return ret;
    }

    return IOTC_OK;
}
