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
#include "iotc_json.h"
#include "coap_codec_utils.h"
#include "coap_endpoint_server.h"
#include "e2e_ctl_msg.h"
#include "m2m_cloud_response.h"
#include "iotc_socket.h"

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

static int32_t GetStrFromOptionMsgIdAndDevId(const CoapPacket *req, char **msgId, char **devId)
{
    uint32_t seg = 0;

    // 获取 reqIdOpt
    const CoapOption *reqIdOpt = CoapUtilsFindOption(req, COAP_OPTION_TYPE_REQ_ID, &seg);
    if (reqIdOpt == NULL || seg != 1 || reqIdOpt->value.data == NULL || reqIdOpt->value.len == 0) {
        return IOTC_ERROR;
    }

    // 获取 devIdOpt
    const CoapOption *devIdOpt = CoapUtilsFindOption(req, COAP_OPTION_TYPE_DEV_ID, &seg);
    if (devIdOpt == NULL || seg != 1 || devIdOpt->value.data == NULL || devIdOpt->value.len == 0) {
        return IOTC_ERROR;
    }

    // 提取 devId
    *devId = IotcMalloc(devIdOpt->value.len + 1);
    if (*devId == NULL) {
        IOTC_LOGE("%s: devId strdup failed", __func__);
        return IOTC_ERROR;
    }
    memcpy_s(*devId, devIdOpt->value.data, devIdOpt->value.len);
    (*devId)[devIdOpt->value.len] = '\0';

    // 提取 msgId
    *msgId = IotcMalloc(reqIdOpt->value.len + 1);
    if (*msgId == NULL) {
        IOTC_LOGE("%s: msgId strdup failed", __func__);
        IotcFree(*devId);
        return IOTC_ERROR;
    }
    memcpy_s(*msgId, reqIdOpt->value.len, reqIdOpt->value.data, reqIdOpt->value.len);
    (*msgId)[reqIdOpt->value.len] = '\0';

    return IOTC_OK;
}

static IotcJson *ParseCloudCtlMsg(const CoapPacket *req)
{
    if (req == NULL) {
        IOTC_LOGW("Invalid request packet");
        return NULL;
    }

    char *msgId = NULL;
    char *devId = NULL;
    char svcId[IOTC_SVC_ID_STR_MAX_LEN] = {0};

    IotcJson *array = IotcJsonCreateArray();
    if (array == NULL) {
        IOTC_LOGW("create array error");
        goto ERROR_EXIT;
    }

    int32_t ret = GetStrFromOption(req, svcId, sizeof(svcId), COAP_OPTION_TYPE_URI_PATH);
    if (ret != IOTC_OK) {
        IOTC_LOGW("Get svcId error %d", ret);
        return NULL;
    }

    if (GetStrFromOptionMsgIdAndDevId(req, &msgId, &devId) != IOTC_OK) {
        IOTC_LOGW("Get msgId and devId error");
        goto ERROR_EXIT;
    }

    IotcJson *payloadObj = IotcJsonCreate();
    if (payloadObj == NULL) {
        IOTC_LOGW("create payload object error");
        goto ERROR_EXIT;
    }

    if (req->header.code != COAP_METHOD_TYPE_GET) {
        if (req->payload.data == NULL || req->payload.len <= 0) {
            IOTC_LOGW("invalid payload data or length");
            goto ERROR_DELETE_PAYLOAD;
        }

        IotcJson *dataObj = IotcJsonParseWithLen((const char *)req->payload.data, req->payload.len);
        if (dataObj == NULL) {
            IOTC_LOGW("invalid ctl json");
            goto ERROR_DELETE_PAYLOAD;
        }

        ret = IotcJsonAddItem2Obj(payloadObj, STR_JSON_DATA, dataObj);
        if (ret != IOTC_OK) {
            IOTC_LOGW("add data error %d", ret);
            IotcJsonDelete(dataObj);
            goto ERROR_DELETE_PAYLOAD;
        }
    }

    ret = IotcJsonAddStr2Obj(payloadObj, STR_JSON_SID, svcId);
    if (ret != IOTC_OK) {
        IOTC_LOGW("add sid error %d", ret);
        goto ERROR_DELETE_PAYLOAD;
    }

    ret = IotcJsonAddStr2Obj(payloadObj, STR_JSON_MSG_ID, msgId);
    if (ret != IOTC_OK) {
        IOTC_LOGW("add msgId error %d", ret);
        goto ERROR_DELETE_PAYLOAD;
    }

    ret = IotcJsonAddStr2Obj(payloadObj, STR_JSON_DEV_ID, devId);
    if (ret != IOTC_OK) {
        IOTC_LOGW("add devId error %d", ret);
        goto ERROR_DELETE_PAYLOAD;
    }

    ret = IotcJsonAddItem2Array(array, payloadObj);
    if (ret != IOTC_OK) {
        IOTC_LOGW("add payload to array error %d", ret);
        goto ERROR_DELETE_PAYLOAD;
    }

    // Success path
    free(msgId);
    free(devId);
    return array;

ERROR_DELETE_PAYLOAD:
    IotcJsonDelete(payloadObj);
ERROR_EXIT:
    if (array != NULL) {
        IotcJsonDelete(array);
    }
    if (msgId != NULL) {
        free(msgId);
    }
    if (devId != NULL) {
        free(devId);
    }
    return NULL;
}

// 验证必需的CoAP选项
static int32_t ValidateRequiredCoapOptions(const CoapPacket *req, const CoapOption **uriOpt,
                                           const CoapOption **reqIdOpt, const CoapOption **devIdOpt,
                                           const CoapOption **userIdOpt)
{
    uint32_t seg = 0;

    *uriOpt = CoapUtilsFindOption(req, COAP_OPTION_TYPE_URI_PATH, &seg);
    if (*uriOpt == NULL || seg != 1 || (*uriOpt)->value.data == NULL || (*uriOpt)->value.len == 0) {
        return IOTC_CORE_WIFI_M2M_ERR_CLOUD_NO_AVAILABLE_URL;
    }

    *reqIdOpt = CoapUtilsFindOption(req, COAP_OPTION_TYPE_REQ_ID, &seg);
    if (*reqIdOpt == NULL || seg != 1 || (*reqIdOpt)->value.data == NULL || (*reqIdOpt)->value.len == 0) {
        return IOTC_CORE_WIFI_M2M_ERR_CLOUD_GET_OPT_REQ_ID;
    }

    *devIdOpt = CoapUtilsFindOption(req, COAP_OPTION_TYPE_DEV_ID, &seg);
    if (*devIdOpt == NULL || seg != 1 || (*devIdOpt)->value.data == NULL || (*devIdOpt)->value.len == 0) {
        return IOTC_CORE_WIFI_M2M_ERR_CLOUD_GET_OPT_DEV_ID;
    }

    *userIdOpt = CoapUtilsFindOption(req, COAP_OPTION_TYPE_USER_ID, &seg);
    if (*userIdOpt == NULL || seg != 1 || (*userIdOpt)->value.data == NULL || (*userIdOpt)->value.len == 0) {
        return IOTC_CORE_WIFI_M2M_ERR_CLOUD_GET_OPT_USER_ID;
    }

    return IOTC_OK;
}

static int32_t BuildCloudCtlRespMsg(CoapEndpoint *endpoint, const CoapPacket *req, const SocketAddr *addr,
                                    const M2mCloudContext *ctx, IotcJson *respJson)
{
    int32_t ret = IOTC_OK;
    do {
        const CoapOption *uriOpt = NULL;
        const CoapOption *reqIdOpt = NULL;
        const CoapOption *devIdOpt = NULL;
        const CoapOption *userIdOpt = NULL;

        ret = ValidateRequiredCoapOptions(req, &uriOpt, &reqIdOpt, &devIdOpt, &userIdOpt);
        if (ret != IOTC_OK) {
            break;
        }

#define STR_URI_PATH_DEVCONTROL "devControl"
        uint32_t *seq = (uint32_t *)ctx->linkInfo.sessData;
        const CoapOption options[] = {
            {COAP_OPTION_TYPE_URI_PATH,
             {req->header.code == COAP_METHOD_TYPE_POST ? (const uint8_t *)STR_URI_PATH_DEVCONTROL :
                                                          (const uint8_t *)uriOpt->value.data,
              ((req->header.code == COAP_METHOD_TYPE_POST ? strlen(STR_URI_PATH_DEVCONTROL) : uriOpt->value.len))}},
            {COAP_OPTION_TYPE_ACCESS_TOKEN_ID, {(const uint8_t *)ctx->tokenInfo.access, strlen(ctx->tokenInfo.access)}},
            {COAP_OPTION_TYPE_REQ_ID, {(const uint8_t *)reqIdOpt->value.data, reqIdOpt->value.len}},
            {COAP_OPTION_TYPE_DEV_ID, {(const uint8_t *)devIdOpt->value.data, devIdOpt->value.len}},
            {COAP_OPTION_TYPE_USER_ID, {(const uint8_t *)userIdOpt->value.data, userIdOpt->value.len}},
            {COAP_OPTION_TYPE_SEQ_NUM_ID, {(const uint8_t *)seq, sizeof(uint32_t)}},
        };

        (*seq)++;
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
    return ret;
}

static int32_t SendCloudCtlMsgResp(
    CoapEndpoint *endpoint, const CoapPacket *req, const SocketAddr *addr, const M2mCloudContext *ctx)
{
    IotcJson *dataJsonArray = ParseCloudCtlMsg(req);
    if (dataJsonArray == NULL) {
        IOTC_LOGW("Parse Cloud Ctl Msg error");
        return IOTC_ERROR;
    }
    // 创建响应节点， 返回节点
    CoapResponeNode *respInfo = M2mCloudCreateCoapNode(endpoint, req, addr, ctx);
    if (respInfo == NULL) {
        IOTC_LOGW("create cloud resp node error");
        return IOTC_ERROR;
    }
    /* 创建JSON指针 */
    IotcJson *respJson = IotcJsonCreate();
    if (respJson == NULL) {
        IOTC_LOGW("create resp json error ");
        return IOTC_ERROR;
    }
    int32_t ret = IOTC_OK;
    if (req->header.code == COAP_METHOD_TYPE_GET) {
        ret = DevSvcProxyCtlGetCharStates(dataJsonArray, &respJson);
        if (ret != IOTC_OK) {
            IOTC_LOGW("cloud get char error %d", ret);
        }
    } else if ((req->header.code == COAP_METHOD_TYPE_POST) || (req->header.code == COAP_METHOD_TYPE_PUT)) {
        ret = DevSvcProxyCtlPutCharStates(dataJsonArray, NULL);
        if (ret != IOTC_OK) {
            IOTC_LOGE("ctrl error %d", ret);
        }
        ret = IotcJsonAddNum2Obj(respJson, STR_ERRCODE, ret);
        if (ret != IOTC_OK) {
            IOTC_LOGW("add num to obj err %d", ret);
        }
    }
    BuildCloudCtlRespMsg(endpoint, req, addr, ctx, respJson);
    IotcJsonDelete(dataJsonArray);
    dataJsonArray = NULL;
    IotcJsonDelete(respJson);
    return ret;
}

static void M2mCloudCoapControlHandler(CoapEndpoint *endpoint, const CoapPacket *req,
    const SocketAddr *addr, void *userData)
{
    CHECK_V_RETURN_LOGW(endpoint != NULL && req != NULL && addr != NULL && userData != NULL, "invalid param");

    M2mCloudContext *ctx = GetM2mCloudCtx();
    int32_t ret = SendCloudCtlMsgResp(endpoint, req, addr, ctx);
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
