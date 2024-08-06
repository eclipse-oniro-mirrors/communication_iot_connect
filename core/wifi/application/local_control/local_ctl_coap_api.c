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

#define LOCAL_CONTROL_SEQ_WINDOW 30

typedef enum {
    LOCAL_MODE_AES_CBC = 0,
    LOCAL_MODE_DTLS,
    LOCAL_MODE_AES_GCM,
} LocalMode;

static AdapterJson *CreateLocalSearchRespJson(const DevAuthInfo *authInfo, const LocalControlClient *client)
{
    AdapterJson *jsonObj = UtilsJsonCreateErrcode(0);
    if (jsonObj == NULL) {
        IOTC_LOGW("create json obj error");
        return NULL;
    }

    UtilsJsonStrItem strTable[] = {
        { STR_JSON_DEVID, authInfo->devId },
        { STR_JSON_AUTHCODE_ID, authInfo->authCodeId },
        { STR_JSON_SESS_ID, client == NULL ? "" : client->sessInfo.sessId },
    };

    int32_t ret = UtilsJsonAddStrTable(jsonObj, strTable, ARRAY_SIZE(strTable));
    if (ret != IOTC_OK) {
        AdapterJsonDelete(jsonObj);
        IOTC_LOGW("add str table error %d", ret);
        return NULL;
    }

    return jsonObj;
}

static int32_t SendLocalCtlResp(CoapEndpoint *endpoint, const CoapPacket *reqPkt, const SocketAddr *addr,
    LocalCoapSessMsg *respSessMsg, AdapterJson *respJson)
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

void LocalCtlCoapSearchHandler(CoapEndpoint *endpoint, const CoapPacket *req, const SocketAddr *addr, void *userData)
{
    CHECK_V_RETURN_LOGW(endpoint != NULL && req != NULL && addr != NULL && userData != NULL, "invalid param");

    uint32_t seg = 0;
    const CoapOption *puuidOption = CoapUtilsFindOption(req, COAP_OPTION_TYPE_PUUID, &seg);
    if (puuidOption == NULL || seg != 1 || puuidOption->value.data == NULL || puuidOption->value.len == 0) {
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

    AdapterJson *respJson = CreateLocalSearchRespJson(&authInfo, client);
    (void)memset_s(&authInfo, sizeof(DevAuthInfo), 0, sizeof(DevAuthInfo));
    if (respJson == NULL) {
        return;
    }

    LocalCoapSessMsg respSessMsg;
    BuildLocalCoapSessMsg(&respSessMsg, LOCAL_COAP_PLAIN, client);

    ret = SendLocalCtlResp(endpoint, req, addr, &respSessMsg, respJson);
    AdapterJsonDelete(respJson);
    if (ret != IOTC_OK) {
        IOTC_LOGE("send local ctl search resp error %d", ret);
    }
}

static AdapterJson *CreateSessMngrRespJson(LocalControlClient *client, uint8_t sn2[SESS_SN_LEN])
{
    AdapterJson *respJson = UtilsJsonCreateErrcode(0);
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

        ret = AdapterJsonAddStr2Obj(respJson, STR_JSON_SESS_ID, client->sessInfo.sessId);
        if (ret != IOTC_OK) {
            IOTC_LOGW("add sessid error %d", ret);
            break;
        }

        ret = AdapterJsonAddNum2Obj(respJson, STR_JSON_SEQ_NUM, client->sessInfo.recvSeq);
        if (ret != IOTC_OK) {
            IOTC_LOGW("add seq error %d", ret);
            break;
        }

        ret = AdapterJsonAddNum2Obj(respJson, STR_JSON_MODE_RESP, UTILS_BIT(LOCAL_MODE_AES_GCM));
        if (ret != IOTC_OK) {
            IOTC_LOGW("add mode resp error %d", ret);
            break;
        }
        return respJson;
    } while (0);

    AdapterJsonDelete(respJson);
    return NULL;
}

static LocalControlClient *CreateLocalCtlClient(LocalControlContext *ctx, const CoapOption *puuidOption,
    uint32_t addr, const AdapterJson *reqJson, uint8_t sn2[SESS_SN_LEN])
{
    uint8_t sn1[SESS_SN_LEN] = {0};
    const char *sn1Hex = AdapterJsonGetStr(AdapterJsonGetObj(reqJson, STR_JSON_SN1));
    if (!UtilsUnhexify(sn1Hex, strlen(sn1Hex), sn1, SESS_SN_LEN)) {
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

static bool LocalModeSupportCheck(const AdapterJson *reqJson)
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
    CHECK_V_RETURN_LOGW(endpoint != NULL && req != NULL && addr != NULL && userData != NULL, "invalid param");

    uint32_t seg = 0;
    const CoapOption *puuidOption = CoapUtilsFindOption(req, COAP_OPTION_TYPE_PUUID, &seg);
    if (puuidOption == NULL || seg != 1 || puuidOption->value.data == NULL || puuidOption->value.len == 0) {
        IOTC_LOGW("invalid search packet");
        return;
    }

    AdapterJson *reqJson = AdapterJsonParseWithLen((const char *)req->payload.data, req->payload.len);
    if (reqJson == NULL) {
        IOTC_LOGW("parse json error");
        return;
    }

    if (!LocalModeSupportCheck(reqJson)) {
        AdapterJsonDelete(reqJson);
        return;
    }

    uint8_t sn2[SESS_SN_LEN] = {0};
    (void)SecurityRandom(sn2, SESS_SN_LEN);
    LocalControlClient *client = CreateLocalCtlClient(userData, puuidOption, addr->addr, reqJson, sn2);
    AdapterJsonDelete(reqJson);
    reqJson = NULL;
    if (client == NULL) {
        IOTC_LOGW("create client error");
        return;
    }

    AdapterJson *respJson = CreateSessMngrRespJson(client, sn2);
    if (respJson == NULL) {
        return;
    }

    LocalCoapSessMsg respSessMsg;
    BuildLocalCoapSessMsg(&respSessMsg, LOCAL_COAP_PLAIN, client);

    int32_t ret = SendLocalCtlResp(endpoint, req, addr, &respSessMsg, respJson);
    AdapterJsonDelete(respJson);
    if (ret != IOTC_OK) {
        IOTC_LOGE("send local ctl sess mngr resp error %d", ret);
    }
}

static int32_t SessCoapRecvSeqCheck(const AdapterJson *payloadJson, LocalCoapSessMsg *sessMsg)
{
    AdapterJson *seqObj = AdapterJsonGetObj(payloadJson, STR_JSON_SEQ_NUM);
    CHECK_RETURN_LOGE(seqObj != NULL, IOTC_ADAPTER_JSON_ERR_GET_OBJ, "get seq json err");
    int64_t seq = 0;
    int32_t ret = AdapterJsonGetNum(seqObj, &seq);
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

static void LocalCtrlMsgReportAfterGetCmd(const AdapterJson *dataArray,
    const void *userData, uint32_t userDataLen)
{
    CHECK_V_RETURN_LOGW(dataArray != NULL && userData != NULL &&
        userDataLen == LOCAL_CONTROL_SESS_ID_STR_LEN, "param invalid");
    LocalControlContext *ctx = GetLocalCtlCtx();
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

void LocalCtlCoapControlHandler(CoapEndpoint *endpoint, const CoapPacket *req, const SocketAddr *addr, void *userData)
{
    CHECK_V_RETURN_LOGW(endpoint != NULL && req != NULL && addr != NULL && userData != NULL, "invalid param");

    LocalCoapSessMsg *sessMsg = (LocalCoapSessMsg *)req;
    CHECK_V_RETURN_LOGW(sessMsg->client != NULL, "invalid client");
    AdapterJson *payloadJsonObj = AdapterJsonParseWithLen((const char *)req->payload.data, req->payload.len);
    if (payloadJsonObj == NULL) {
        IOTC_LOGW("invalid ctl json");
        return;
    }
    
    int32_t ret = SessCoapRecvSeqCheck((const AdapterJson *)payloadJsonObj, sessMsg);
    if (ret != IOTC_OK) {
        IOTC_LOGW("check seq error %d", ret);
        return;
    }

    ret = E2eCtrlMsgProcess(payloadJsonObj, LocalCtrlMsgReportAfterGetCmd, sessMsg->client->sessInfo.sessId,
        LOCAL_CONTROL_SESS_ID_STR_LEN);
    AdapterJsonDelete(payloadJsonObj);
    payloadJsonObj = NULL;
    if (ret != IOTC_OK) {
        IOTC_LOGW("e2e ctl error %d", ret);
    }

    AdapterJson *respJson = UtilsJsonCreateErrcode(ret);
    if (respJson == NULL) {
        IOTC_LOGW("create resp json error %d", ret);
        return;
    }
    ret = AdapterJsonAddNum2Obj(respJson, STR_JSON_SEQ_NUM, sessMsg->client->sessInfo.sendSeq);
    if (ret != IOTC_OK) {
        IOTC_LOGW("add seqNum error %d", ret);
        AdapterJsonDelete(respJson);
        return;
    }

    LocalCoapSessMsg respSessMsg;
    BuildLocalCoapSessMsg(&respSessMsg, 0, sessMsg->client);

    ret = SendLocalCtlResp(endpoint, req, addr, &respSessMsg, respJson);
    AdapterJsonDelete(respJson);
    if (ret != IOTC_OK) {
        IOTC_LOGE("send local ctl resp error %d", ret);
    }
}