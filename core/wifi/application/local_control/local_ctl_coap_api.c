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
#include "local_ctl_coap_api.h"
#include "local_ctl_sess.h"
#include "utils_assert.h"
#include "local_ctl_cli_mngr.h"
#include "iotc_errcode.h"
#include "iotc_svc_dev.h"
#include "utils_json.h"
#include "comm_def.h"
#include "utils_common.h"
#include "coap_codec_utils.h"
#include "dfx_anonymize.h"
#include "security_random.h"
#include "e2e_ctl_msg.h"
#include "securec.h"
#include "seq_num_utils.h"
#include "local_ctl_report.h"
#include "m2m_cloud_ctx.h"
#include "iotc_kdf.h"
#include "security_random.h"
#include "iotc_aes.h"
#include "m2m_cloud_ctx.h"

#define LOCAL_CONTROL_SEQ_WINDOW 30

typedef enum {
    LOCAL_MODE_AES_CBC = 0,
    LOCAL_MODE_DTLS,
    LOCAL_MODE_AES_GCM,
} LocalMode;

uint8_t localCtlSn1[SESS_SN_LEN] = {0};
uint8_t localCtlKey[HEXIFY_LEN(SESS_SN_LEN)];
int32_t localCtlSessKeyGen(M2mCloudContext *ctx, const uint8_t *sn1, uint32_t sn1Len,
    const uint8_t *sn2, uint32_t sn2Len);

static IotcJson *CreateLocalSearchRespJson(const DevAuthInfo *authInfo, const LocalControlClient *client)
{
    IotcJson *jsonObj = UtilsJsonCreateErrcode(0);
    if (jsonObj == NULL) {
        IOTC_LOGW("create json obj error");
        return NULL;
    }

    M2mCloudContext *ctx = GetM2mCloudCtx();
    char authcodeIdStr[HEXIFY_LEN(SESSION_AUTHCODE_LEN + 1) + 1] = {0};

    if (ctx != NULL) {
        UtilsHexify(ctx->authCodeInfo.authCodeId, sizeof(ctx->authCodeInfo.authCodeId),
            authcodeIdStr, sizeof(authcodeIdStr));
    }
    
    UtilsJsonStrItem strTable[] = {
        { STR_JSON_DEVID, authInfo->devId },
        { STR_JSON_AUTHCODE_ID, authcodeIdStr },
        { STR_JSON_SESS_ID, client == NULL ? "" : client->sessInfo.sessId },
    };

    int32_t ret = UtilsJsonAddStrTable(jsonObj, strTable, ARRAY_SIZE(strTable));
    if (ret != IOTC_OK) {
        IotcJsonDelete(jsonObj);
        IOTC_LOGW("add str table error %d", ret);
        return NULL;
    }

    return jsonObj;
}

static int32_t SendLocalCtlResp(CoapEndpoint *endpoint, const CoapPacket *reqPkt, const SocketAddr *addr,
    LocalCoapSessMsg *respSessMsg, IotcJson *respJson)
{
    CoapServerRespParam respParam;
    int32_t ret = CoapServerBuildDefaultRespParam(&respParam, reqPkt, respJson);
    if (ret != IOTC_OK) {
        IOTC_LOGW("build resp param error %d", ret);
        return ret;
    }

    ret = CoapServerSendResp(endpoint, &respParam, addr, &respSessMsg->packet);
    if (ret != IOTC_OK) {
        IOTC_LOGW("send coap resp msg error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static inline void BuildLocalCoapSessMsg(LocalCoapSessMsg *respSessMsg, uint32_t bitMap, LocalControlClient *client)
{
    (void)memset_s(respSessMsg, sizeof(LocalCoapSessMsg), 0, sizeof(LocalCoapSessMsg));
    respSessMsg->bitMap = bitMap;
    respSessMsg->client = client;
}

static bool LocalCtlPuuidOptionCheck(const CoapOption *puuidOption, uint32_t seg)
{
    CHECK_RETURN_LOGW(puuidOption != NULL, false, "puuid null");
    /* CI NOTE: puuid仅有1个分片 */
    CHECK_RETURN_LOGW(seg == 1, false, "puuid seg invalid %u", seg);
    CHECK_RETURN_LOGW(puuidOption->value.data != NULL, false, "puuid value null");
    CHECK_RETURN_LOGW(puuidOption->value.len != 0, false, "puuid len invalid %u",
        puuidOption->value.len);
    return true;
}

void LocalCtlCoapSearchHandler(CoapEndpoint *endpoint, const CoapPacket *req, const SocketAddr *addr, void *userData)
{
    CHECK_V_RETURN_LOGW(endpoint != NULL && req != NULL && addr != NULL && userData != NULL, "param invalid");

    uint32_t seg = 0;
    const CoapOption *puuidOption = CoapUtilsFindOption(req, COAP_OPTION_TYPE_PUUID, &seg);
    if (!LocalCtlPuuidOptionCheck(puuidOption, seg)) {
        IOTC_LOGW("invalid search packet");
        return;
    }

    LocalControlClient *client = NULL;
    int32_t ret = GetLocalControlClient(userData, CLIENT_PUUID, (const char *)puuidOption->value.data,
        puuidOption->value.len, &client);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get client error %d", ret);
        return;
    }

    bool isAuthInfoExist = false;
    DevAuthInfo authInfo = {0};
    ret = DevSvcProxyGetAuthInfo(&isAuthInfoExist, &authInfo);
    if (ret != IOTC_OK || !isAuthInfoExist) {
        IOTC_LOGW("get client error %d/%d", ret, isAuthInfoExist);
        return;
    }

    IotcJson *respJson = CreateLocalSearchRespJson(&authInfo, client);
    (void)memset_s(&authInfo, sizeof(DevAuthInfo), 0, sizeof(DevAuthInfo));
    if (respJson == NULL) {
        return;
    }

    LocalCoapSessMsg respSessMsg;
    BuildLocalCoapSessMsg(&respSessMsg, UTILS_BIT(LOCAL_COAP_PLAIN), client);

    ret = SendLocalCtlResp(endpoint, req, addr, &respSessMsg, respJson);
    IotcJsonDelete(respJson);
    if (ret != IOTC_OK) {
        IOTC_LOGE("send local ctl search resp error %d", ret);
    }
}

static IotcJson *CreateSessMngrRespJson(LocalControlClient *client, uint8_t sn2[SESS_SN_LEN])
{
    IotcJson *respJson = UtilsJsonCreateErrcode(0);
    if (respJson == NULL) {
        IOTC_LOGW("create json obj error");
        return NULL;
    }

    do {
        int32_t ret = UtilsJsonAddHexify(respJson, STR_JSON_SN2, sn2, SESS_SN_LEN);
        if (ret != IOTC_OK) {
            IOTC_LOGW("add sn2 error %d", ret);
            break;
        }

        ret = IotcJsonAddStr2Obj(respJson, STR_JSON_SESS_ID, client->sessInfo.sessId);
        if (ret != IOTC_OK) {
            IOTC_LOGW("add sessid error %d", ret);
            break;
        }

        ret = IotcJsonAddNum2Obj(respJson, STR_JSON_SEQ_NUM, client->sessInfo.recvSeq);
        if (ret != IOTC_OK) {
            IOTC_LOGW("add seq error %d", ret);
            break;
        }

        ret = IotcJsonAddNum2Obj(respJson, STR_JSON_MODE_RESP, UTILS_BIT(LOCAL_MODE_AES_GCM));
        if (ret != IOTC_OK) {
            IOTC_LOGW("add mode resp error %d", ret);
            break;
        }
        return respJson;
    } while (0);

    IotcJsonDelete(respJson);
    return NULL;
}

static LocalControlClient *CreateLocalCtlClient(LocalControlContext *ctx, const CoapOption *puuidOption,
    uint32_t addr, const IotcJson *reqJson, uint8_t sn2[SESS_SN_LEN])
{
    uint8_t sn1[SESS_SN_LEN] = {0};
    const char *sn1Hex = IotcJsonGetStr(IotcJsonGetObj(reqJson, STR_JSON_SN1));
    if (sn1Hex == NULL || !UtilsUnhexify(sn1Hex, strlen(sn1Hex), sn1, SESS_SN_LEN)) {
        IOTC_LOGW("json sn1 invalid");
        return NULL;
    }
    uint32_t seq;
    int32_t ret = UtilsJsonGetUint(reqJson, STR_JSON_SEQ_NUM, &seq);
    if (ret != IOTC_OK) {
        IOTC_LOGW("json no seq");
        return NULL;
    }

    LocalControlClient *client = NULL;
    LocalClientBuildParam param = {
        .sn1 = sn1,
        .sn1Len = SESS_SN_LEN,
        .sn2 = sn2,
        .sn2Len = SESS_SN_LEN,
        .sendSeq = seq,
        .puuid = (const char *)puuidOption->value.data,
        .puuidLen = puuidOption->value.len,
        .addr = addr,
    };
    ret = CreateLocalControlClient(ctx, &param, &client);
    if (ret != IOTC_OK) {
        IOTC_LOGW("create client error %d", ret);
        return NULL;
    }
    return client;
}

static bool LocalModeSupportCheck(const IotcJson *reqJson)
{
    uint32_t mode;
    int32_t ret = UtilsJsonGetUint(reqJson, STR_JSON_MODE_SUPPORT, &mode);
    if (ret != IOTC_OK) {
        IOTC_LOGW("json no mode");
        return false;
    }

    if (!UTILS_IS_BIT_SET(mode, LOCAL_MODE_AES_GCM)) {
        IOTC_LOGW("mode not support %u", mode);
        return false;
    }
    return true;
}

void LocalCtlCoapSessMngrHandler(CoapEndpoint *endpoint, const CoapPacket *req, const SocketAddr *addr, void *userData)
{
    CHECK_V_RETURN_LOGW(endpoint != NULL && req != NULL && addr != NULL && userData != NULL, "param invalid");

    uint32_t seg = 0;
    M2mCloudContext *ctx = GetM2mCloudCtx();
    const CoapOption *puuidOption = CoapUtilsFindOption(req, COAP_OPTION_TYPE_PUUID, &seg);
    if (!LocalCtlPuuidOptionCheck(puuidOption, seg)) {
        IOTC_LOGW("invalid mngr packet");
        return;
    }

    IotcJson *reqJson = IotcJsonParseWithLen((const char *)req->payload.data, req->payload.len);
    if (reqJson == NULL) {
        IOTC_LOGW("parse json error");
        return;
    }

    if (!LocalModeSupportCheck(reqJson)) {
        IotcJsonDelete(reqJson);
        return;
    }

    // sn1
    const char *sn1Hex = IotcJsonGetStr(IotcJsonGetObj(reqJson, STR_JSON_SN1));
    if (sn1Hex == NULL) {
        IOTC_LOGW("json sn1 invalid");
    }

    UtilsUnhexify(sn1Hex, strlen(sn1Hex), localCtlSn1, sizeof(localCtlSn1));

    uint8_t sn2[SESS_SN_LEN] = {0};
    (void)SecurityRandom(sn2, SESS_SN_LEN);
    LocalControlClient *client = CreateLocalCtlClient(userData, puuidOption, addr->addr, reqJson, sn2);
    IotcJsonDelete(reqJson);
    reqJson = NULL;
    if (client == NULL) {
        IOTC_LOGW("create client error");
        return;
    }

    localCtlSessKeyGen(ctx, localCtlSn1, sizeof(localCtlSn1), sn2, sizeof(sn2));

    IotcJson *respJson = CreateSessMngrRespJson(client, sn2);
    if (respJson == NULL) {
        return;
    }

    LocalCoapSessMsg respSessMsg;
    BuildLocalCoapSessMsg(&respSessMsg, UTILS_BIT(LOCAL_COAP_PLAIN), client);

    int32_t ret = SendLocalCtlResp(endpoint, req, addr, &respSessMsg, respJson);
    IotcJsonDelete(respJson);
    if (ret != IOTC_OK) {
        IOTC_LOGE("send local ctl sess mngr resp error %d", ret);
    }
}

static int32_t SessCoapRecvSeqCheck(const IotcJson *payloadJson, LocalCoapSessMsg *sessMsg)
{
    IotcJson *seqObj = IotcJsonGetObj(payloadJson, STR_JSON_SEQ_NUM);
    CHECK_RETURN_LOGE(seqObj != NULL, IOTC_ADAPTER_JSON_ERR_GET_OBJ, "get seq json err");
    int64_t seq = 0;
    int32_t ret = IotcJsonGetNum(seqObj, &seq);
    CHECK_RETURN_LOGE(ret == IOTC_OK, IOTC_ADAPTER_JSON_ERR_GET_NUM, "get seq num err");
    CHECK_RETURN_LOGE(seq >= 0 && seq <= UINT32_MAX, IOTC_CORE_BLE_INVALID_SEQ, "get seq num:%ld overflow", seq);

    uint32_t recvSeq = (uint32_t)seq;
    bool isSmall = false;
    uint32_t delta = 0;
    if (!SeqNumCheck(sessMsg->client->sessInfo.recvSeq, recvSeq, LOCAL_CONTROL_SEQ_WINDOW, &isSmall, &delta) ||
        delta > LOCAL_CONTROL_SEQ_WINDOW) {
        IOTC_LOGW("invalid seq %u/%u/%u", sessMsg->client->sessInfo.recvSeq, recvSeq, LOCAL_CONTROL_SEQ_WINDOW);
        return SESS_CODE_ERR;
    }

    if (isSmall) {
        if (UTILS_IS_BIT_SET(sessMsg->client->sessInfo.seqMap, delta)) {
            IOTC_LOGW("recv repeat seq %u/%u", sessMsg->client->sessInfo.recvSeq, recvSeq);
            return SESS_CODE_ERR;
        }
        sessMsg->client->sessInfo.seqMap |= UTILS_BIT(delta);
        IOTC_LOGW("recv before seq %u/%u", sessMsg->client->sessInfo.recvSeq, recvSeq);
    } else {
        IOTC_LOGI("recv seq update %u/%u", sessMsg->client->sessInfo.recvSeq, recvSeq);
        sessMsg->client->sessInfo.recvSeq = recvSeq;
        /* seq map 仅保留当前seq前30个seq是否收到 */
        sessMsg->client->sessInfo.seqMap = (sessMsg->client->sessInfo.seqMap << delta) | UTILS_BIT(delta);
    }

    return IOTC_OK;
}

static void LocalCtrlMsgReportAfterGetCmd(const IotcJson *dataArray,
    const void *userData, uint32_t userDataLen)
{
    CHECK_V_RETURN_LOGW(dataArray != NULL && userData != NULL &&
        userDataLen == LOCAL_CONTROL_SESS_ID_STR_LEN, "param invalid");
    LocalControlContext *ctx = GetLocalCtlCtx();
    CHECK_V_RETURN_LOGE(ctx != NULL, "local ctl ctx null");

    LocalControlClient *client = NULL;
    int32_t ret = GetLocalControlClient(ctx, CLIENT_SESS_ID, (const char *)userData, userDataLen, &client);
    if (ret != IOTC_OK || client == NULL) {
        IOTC_LOGW("report get cli error %d", ret);
        return;
    }

    ret = LocalCtlReportToTargetClient(dataArray, ctx, client);
    if (ret != IOTC_OK) {
        IOTC_LOGW("report to cli error %d", ret);
    }
    return;
}

static int32_t GetStrFromOption(const CoapPacket *req, char *data, uint32_t dataLen, CoapOptionType type)
{
    uint32_t seg = 0;
    const CoapOption *uriOption = CoapUtilsFindOption(req, type, &seg);
    if (uriOption == NULL || seg != 1 || uriOption->value.data == NULL || uriOption->value.len == 0) {
        IOTC_LOGW("invalid e2e packet");
        return IOTC_CORE_WIFI_M2M_ERR_CLOUD_GET_OPTION;
    }

    if (strncpy_s(data, dataLen, (const char *)uriOption->value.data, uriOption->value.len) != EOK) {
        IOTC_LOGW("strcpy error %u", uriOption->value.len);
        return IOTC_ERR_SECUREC_STRCPY;
    }

    return IOTC_OK;
}

static IotcJson *ParseE2eCtlMsg(const CoapPacket *req)
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
        if (req->header.code != COAP_METHOD_TYPE_GET) {
            IotcJson *dataObj = IotcJsonParseWithLen((const char *)req->payload.data, req->payload.len);
            if (dataObj == NULL) {
                IOTC_LOGW("invalid ctl json");
            }
            ret = IotcJsonAddItem2Obj(payloadObj, STR_JSON_DATA, dataObj);
            if (ret != IOTC_OK) {
                IOTC_LOGW("add data error %d", ret);
            }
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

void LocalCtlCoapControlHandler(CoapEndpoint *endpoint, const CoapPacket *req, const SocketAddr *addr, void *userData)
{
    CHECK_V_RETURN_LOGW(endpoint != NULL && req != NULL && addr != NULL && userData != NULL, "param invalid");

    LocalCoapSessMsg *sessMsg = (LocalCoapSessMsg *)req;
    CHECK_V_RETURN_LOGW(sessMsg->client != NULL, "invalid client");
    IotcJson *payloadJsonObj = IotcJsonParseWithLen((const char *)req->payload.data, req->payload.len);
    if (payloadJsonObj == NULL) {
        IOTC_LOGW("invalid ctl json");
        return;
    }

    int32_t ret = SessCoapRecvSeqCheck((const IotcJson *)payloadJsonObj, sessMsg);
    if (ret != IOTC_OK) {
        IOTC_LOGW("check seq error %d", ret);
        return;
    }

    ret = E2eCtrlMsgProcess(payloadJsonObj, LocalCtrlMsgReportAfterGetCmd, sessMsg->client->sessInfo.sessId,
        LOCAL_CONTROL_SESS_ID_STR_LEN);
    IotcJsonDelete(payloadJsonObj);
    payloadJsonObj = NULL;
    if (ret != IOTC_OK) {
        IOTC_LOGW("e2e ctl error %d", ret);
    }

    IotcJson *respJson = UtilsJsonCreateErrcode(ret);
    if (respJson == NULL) {
        IOTC_LOGW("create resp json error %d", ret);
        return;
    }
    ret = IotcJsonAddNum2Obj(respJson, STR_JSON_SEQ_NUM, sessMsg->client->sessInfo.sendSeq);
    if (ret != IOTC_OK) {
        IOTC_LOGW("add seqNum error %d", ret);
        IotcJsonDelete(respJson);
        return;
    }

    LocalCoapSessMsg respSessMsg;
    BuildLocalCoapSessMsg(&respSessMsg, 0, sessMsg->client);

    ret = SendLocalCtlResp(endpoint, req, addr, &respSessMsg, respJson);
    IotcJsonDelete(respJson);
    if (ret != IOTC_OK) {
        IOTC_LOGE("send local ctl resp error %d", ret);
    }
}

void LocalCtlSvcCoapHandler(CoapEndpoint *endpoint, const CoapPacket *req, const SocketAddr *addr, void *userData)
{
    CHECK_V_RETURN_LOGW(endpoint != NULL && req != NULL && addr != NULL && userData != NULL, "param invalid");
    LocalCoapSessMsg *sessMsg = (LocalCoapSessMsg *)req;
    CHECK_V_RETURN_LOGW(sessMsg->client != NULL, "invalid client");
    int32_t ret = IOTC_OK;
    IotcJson *payloadJsonObj = ParseE2eCtlMsg(req);
    if (payloadJsonObj == NULL) {
        IOTC_LOGW("Parse E2eCtl Msg error");
        return;
    }
    /* 创建JSON指针 */
    IotcJson *respJson = IotcJsonCreate();
    if (respJson == NULL) {
        IOTC_LOGW("create resp json error ");
        return;
    }
    if (req->header.code == COAP_METHOD_TYPE_GET) {
        ret = DevSvcProxyCtlGetCharStates(payloadJsonObj, &respJson);
        if (ret != IOTC_OK) {
            IOTC_LOGW("get ctrl error %d", ret);
        }
    } else if (req->header.code == COAP_METHOD_TYPE_POST) {
        int32_t errcode;
        errcode = DevSvcProxyCtlPutCharStates(payloadJsonObj, NULL);
        if (errcode != IOTC_OK) {
            IOTC_LOGE("put ctrl error %d", errcode);
        }
        ret = IotcJsonAddNum2Obj(respJson, STR_ERRCODE, errcode);
        if (ret != IOTC_OK) {
            IOTC_LOGW("add errcode to obj err %d", ret);
            IotcJsonDelete(respJson);
        }
    }
    IotcJsonDelete(payloadJsonObj);
    payloadJsonObj = NULL;
    if (ret != IOTC_OK) {
        IOTC_LOGW("local ctl error %d", ret);
    }
    LocalCoapSessMsg respSessMsg;
    BuildLocalCoapSessMsg(&respSessMsg, 0, sessMsg->client);
    ret = SendLocalCtlResp(endpoint, req, addr, &respSessMsg, respJson);
    IotcJsonDelete(respJson);
    if (ret != IOTC_OK) {
        IOTC_LOGE("send local ctl resp error %d", ret);
    }
}

void LocalCtlSvcGetCoapHandler(CoapEndpoint *endpoint, const CoapPacket *req, const SocketAddr *addr, void *userData)
{
    CHECK_V_RETURN_LOGW(endpoint != NULL && req != NULL && addr != NULL && userData != NULL, "param invalid");

    LocalCoapSessMsg *sessMsg = (LocalCoapSessMsg *)req;
    CHECK_V_RETURN_LOGW(sessMsg->client != NULL, "invalid client");

    int32_t ret = IOTC_OK;

    IotcJson *payloadJsonObj = ParseE2eCtlMsg(req);
    if (payloadJsonObj == NULL) {
        IOTC_LOGW("Parse E2eCtl Msg error");
        return;
    }
    
    //创建JSON指针
    IotcJson *respJson = IotcJsonCreate();
    if (respJson == NULL) {
        IOTC_LOGW("create resp json error ");
        return;
    }
    if (req->header.code == COAP_METHOD_TYPE_GET) {
        ret = DevSvcProxyCtlGetCharStates(payloadJsonObj, &respJson);
        if (ret != IOTC_OK) {
            IOTC_LOGE("get ctrl error %d", ret);
        }
    } else if(req->header.code == COAP_METHOD_TYPE_POST) {
        int32_t errcode;
        errcode = DevSvcProxyCtlPutCharStates(payloadJsonObj, NULL);
        if (errcode != IOTC_OK) {
            IOTC_LOGE("put ctrl error %d", ret);
        }
        ret = IotcJsonAddNum2Obj(respJson, STR_ERRCODE, errcode);
        if (errcode != IOTC_OK) {
            IotcJsonDelete(respJson);
            IOTC_LOGE("add errocde error %d", ret);
        }
    }
    IotcJsonDelete(payloadJsonObj);
    payloadJsonObj = NULL;
    if (ret != IOTC_OK) {
        IOTC_LOGW("local ctl error %d", ret);
    }

    LocalCoapSessMsg respSessMsg;
    BuildLocalCoapSessMsg(&respSessMsg, 0, sessMsg->client);

    ret = SendLocalCtlResp(endpoint, req, addr, &respSessMsg, respJson);
    IotcJsonDelete(respJson);
    if (ret != IOTC_OK) {
        IOTC_LOGE("send local ctl resp error %d", ret);
    }
}

int32_t localCtlSessKeyGen(M2mCloudContext *ctx, const uint8_t *sn1, uint32_t sn1Len,
    const uint8_t *sn2, uint32_t sn2Len)
{
    CHECK_RETURN_LOGE((sn1 != NULL) && (sn1Len > 0) && (sn2 != NULL) && (sn2Len > 0),
        IOTC_ERR_PARAM_INVALID, "param invalid, sn1Len:%u, sn2Len:%u", sn1Len, sn2Len);
    int32_t ret = memcpy_s(ctx->pskInfo.salt, RAND_SN_LEN, sn1, sn1Len);
    CHECK_RETURN_LOGE(ret == EOK, IOTC_ERR_SECUREC_MEMCPY, "cpy sn1 err:%d", ret);
    ret = memcpy_s(ctx->pskInfo.salt + RAND_SN_LEN, RAND_SN_LEN, sn2, sn2Len);
    CHECK_RETURN_LOGE(ret == EOK, IOTC_ERR_SECUREC_MEMCPY, "cpy sn2 err:%d", ret);

    IotcPbkdf2HmacParam param = {
        .md = IOTC_MD_SHA256,
        .password = ctx->authCodeInfo.authCode,
        .passwordLen = sizeof(ctx->authCodeInfo.authCode),
        .salt = ctx->pskInfo.salt,
        .saltLen = SALT_LEN,
        .iterCount = ITER_TIMES
    };
    
    ret = IotcPkcs5Pbkdf2Hmac(&param, localCtlKey, HEXIFY_LEN(SESS_SN_LEN));
    char localCtlKeyStr[HEXIFY_LEN(SESSION_AUTHCODE_LEN) + 1] = { 0 };
    UtilsHexify(localCtlKey, sizeof(localCtlKey), localCtlKeyStr, sizeof(localCtlKeyStr));
    IOTC_LOGI("%s localCtlKey:[%s]", __func__, localCtlKeyStr);

    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "ble sess key gen err:%d", ret);
    return IOTC_OK;
}
