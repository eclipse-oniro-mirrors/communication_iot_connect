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

static int32_t GetStrFromOption(const CoapPacket *req, char *data, uint32_t dataLen, CoapOptionType type)
{
    uint32_t seg = 0;
    const CoapOption *uriOption = CoapUtilsFindOption(req, type, &seg);
    if (uriOption == NULL || seg != 1 || uriOption->value.data == NULL || uriOption->value.len == 0) {
        IOTC_LOGW("invalid cloud packet");
        return IOTC_CORE_WIFI_M2M_ERR_CLOUD_GET_OPTION;
    }

    if (strncpy_s(data, dataLen, (const char *)uriOption->value.data, uriOption->value.len) != EOK) {
        IOTC_LOGW("strcpy error %u", uriOption->value.len);
        return IOTC_ERR_SECUREC_STRCPY;
    }

    return IOTC_OK;
}

static IotcJson *ParseCloudCtlMsg(const CoapPacket *req)
{
    char svcId[IOTC_SVC_ID_STR_MAX_LEN] = {0};
    int32_t ret = GetStrFromOption(req, svcId, sizeof(svcId), COAP_OPTION_TYPE_URI_PATH);
    if (ret != IOTC_OK) {
        IOTC_LOGW("Get svcId error %d", ret);
        return NULL;
    }

    IotcJson *array = IotcJsonCreateArray();
    if (array == NULL) {
        IOTC_LOGW("create array error %d", ret);
        return NULL;
    }

    do {
        IotcJson *payloadObj = IotcJsonCreate();
        if (payloadObj == NULL) {
            IOTC_LOGW("add data error");
            break;
        }
        IotcJson *dataObj = IotcJsonParseWithLen((const char *)req->payload.data, req->payload.len);
        if (dataObj == NULL) {
            IOTC_LOGW("invalid ctl json");
            IotcJsonDelete(payloadObj);
            break;
        }
        ret = IotcJsonAddItem2Obj(payloadObj, STR_JSON_DATA, dataObj);
        if (ret != IOTC_OK) {
            IOTC_LOGW("add data error %d", ret);
            IotcJsonDelete(payloadObj);
            IotcJsonDelete(dataObj);
            break;
        }
        ret = IotcJsonAddStr2Obj(payloadObj, STR_JSON_SID, svcId);
        if (ret != IOTC_OK) {
            IOTC_LOGW("add sid error %d", ret);
            IotcJsonDelete(payloadObj);
            break;
        }
        ret = IotcJsonAddItem2Array(array, payloadObj);
        if (ret != IOTC_OK) {
            IOTC_LOGW("add payload error %d", ret);
            IotcJsonDelete(payloadObj);
            break;
        }
        return array;
    } while (false);

    IotcJsonDelete(array);
    return NULL;
}

static int32_t SendCloudCtlMsgResp(int32_t errcode, CoapEndpoint *endpoint, const CoapPacket *req,
    const SocketAddr *addr, const M2mCloudContext *ctx)
{
    IotcJson *respJson = UtilsJsonCreateErrcode(errcode);
    if (respJson == NULL) {
        IOTC_LOGW("create resp json error %d", errcode);
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }

    int32_t ret;
    do {
        uint32_t seg = 0;
        const CoapOption *reqIdOpt = CoapUtilsFindOption(req, COAP_OPTION_TYPE_REQ_ID, &seg);
        if (reqIdOpt == NULL || seg != 1 || reqIdOpt->value.data == NULL || reqIdOpt->value.len == 0) {
            ret = IOTC_CORE_WIFI_M2M_ERR_CLOUD_GET_OPT_REQ_ID;
            break;
        }
        const CoapOption *devIdOpt = CoapUtilsFindOption(req, COAP_OPTION_TYPE_DEV_ID, &seg);
        if (devIdOpt == NULL || seg != 1 || devIdOpt->value.data == NULL || devIdOpt->value.len == 0) {
            ret = IOTC_CORE_WIFI_M2M_ERR_CLOUD_GET_OPT_DEV_ID;
            break;
        }
        const CoapOption *uerIdOpt = CoapUtilsFindOption(req, COAP_OPTION_TYPE_USER_ID, &seg);
        if (uerIdOpt == NULL || seg != 1 || uerIdOpt->value.data == NULL || uerIdOpt->value.len == 0) {
            ret = IOTC_CORE_WIFI_M2M_ERR_CLOUD_GET_OPT_USER_ID;
            break;
        }
        const CoapOption options[] = {
            {COAP_OPTION_TYPE_ACCESS_TOKEN_ID, {(const uint8_t *)ctx->tokenInfo.access, strlen(ctx->tokenInfo.access)}},
            {COAP_OPTION_TYPE_REQ_ID, {(const uint8_t *)reqIdOpt->value.data, reqIdOpt->value.len}},
            {COAP_OPTION_TYPE_DEV_ID, {(const uint8_t *)devIdOpt->value.data, devIdOpt->value.len}},
            {COAP_OPTION_TYPE_USER_ID, {(const uint8_t *)uerIdOpt->value.data, uerIdOpt->value.len}},
            {COAP_OPTION_TYPE_SEQ_NUM_ID, {NULL, sizeof(uint32_t)}},
        };
        CoapServerRespParam respParam = {
            .req = req,
            .type = COAP_MSG_TYPE_NCON,
            .code = COAP_RESPONSE_CODE_CONTENT,
            .opNum = ARRAY_SIZE(options),
            .options = options,
            .payload = NULL,
            .payloadBuilder = CoapUtilsBuildJsonPayloadFunc,
            .payloadUserData = respJson,
            .preSize = 0,
        };
        CoapPacket packet;
        ret = CoapServerSendResp(endpoint, &respParam, addr, &packet);
        if (ret != IOTC_OK) {
            IOTC_LOGW("send e2e ctrl resp msg error %d", ret);
        }
    } while (false);

    IotcJsonDelete(respJson);
    return ret;
}

static void M2mCloudCoapControlHandler(CoapEndpoint *endpoint, const CoapPacket *req,
    const SocketAddr *addr, void *userData)
{
    CHECK_V_RETURN_LOGW(endpoint != NULL && req != NULL && addr != NULL && userData != NULL, "invalid param");

    IotcJson *dataJsonArray = ParseCloudCtlMsg(req);
    if (dataJsonArray == NULL) {
        IOTC_LOGW("Parse Cloud Ctl Msg error");
        return;
    }

    int32_t ret = DevSvcProxyCtlPutCharStates(dataJsonArray, NULL);
    IotcJsonDelete(dataJsonArray);
    dataJsonArray = NULL;
    if (ret != IOTC_OK) {
        IOTC_LOGE("ctrl error %d", ret);
    }

    M2mCloudContext *ctx = GetM2mCloudCtx();
    ret = SendCloudCtlMsgResp(ret, endpoint, req, addr, ctx);
    if (ret != IOTC_OK) {
        IOTC_LOGE("send resp error %d", ret);
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
