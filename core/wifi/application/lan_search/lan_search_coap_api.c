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
#include "lan_search_coap_api.h"
#include "lan_search_ctx.h"
#include "utils_json.h"
#include "coap_codec_utils.h"
#include "iotc_errcode.h"
#include "securec.h"
#include "utils_assert.h"
#include "iotc_svc_dev.h"
#include "dfx_anonymize.h"
#include "svc_info_mdl.h"
#include "lan_search_peer_mngr.h"
#include "svc_info.h"
#include "dev_info.h"
#include "utils_common.h"

static void LanSearchSendJsonResp(CoapEndpoint *endpoint, const CoapPacket *req, const SocketAddr *addr,
    IotcJson *respJson, LanSearchSessMsg *sessMsg)
{
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

    int32_t ret = CoapServerSendResp(endpoint, &respParam, addr, &sessMsg->packet);
    if (ret != IOTC_OK) {
        IOTC_LOGW("send json resp msg error %d", ret);
    }
    return;
}

static void LanSearchSendDataResp(CoapEndpoint *endpoint, const CoapPacket *req, const SocketAddr *addr,
    const CoapData *respData, LanSearchSessMsg *sessMsg)
{
    CoapServerRespParam respParam = {
        .req = req,
        .type = COAP_MSG_TYPE_NCON,
        .code = COAP_RESPONSE_CODE_CONTENT,
        .opNum = 0,
        .options = NULL,
        .payload = respData,
        .payloadBuilder = NULL,
        .payloadUserData = NULL,
        .preSize = 0,
    };

    int32_t ret = CoapServerSendResp(endpoint, &respParam, addr, &sessMsg->packet);
    if (ret != IOTC_OK) {
        IOTC_LOGW("send data resp msg error %d", ret);
    }
    return;
}

static bool LanSearchAddDevid(IotcJson *json)
{
    DevAuthInfo authInfo = {0};
    bool isAuthExits = false;
    int32_t ret = DevSvcProxyGetAuthInfo(&isAuthExits, &authInfo);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get auth info error %d", ret);
        return false;
    }
    if (!isAuthExits) {
        ret = IotcJsonAddStr2Obj(json, STR_NETINFO_DEVICE_ID, "");
    } else {
        ret = IotcJsonAddStr2Obj(json, STR_NETINFO_DEVICE_ID, authInfo.devId);
        (void)memset_s(&authInfo, sizeof(DevAuthInfo), 0, sizeof(DevAuthInfo));
    }
    if (ret != IOTC_OK) {
        IOTC_LOGW("add devid error %d", ret);
        return false;
    }
    return true;
}

static inline bool LanSearchDevInfoCheck(const IotcDeviceInfo *devInfo)
{
    CHECK_RETURN_LOGW(devInfo != NULL && devInfo->sn != NULL && devInfo->devTypeId != NULL &&
        devInfo->manuId != NULL && devInfo->prodId != NULL && devInfo->protType != IOTC_PROT_TYPE_INVALID,
        false, "devinfo invalid");
    return true;
}

static bool LanSearchAddDevInfo(IotcJson *json)
{
    IotcJson *devInfoJson = IotcJsonCreate();
    if (devInfoJson == NULL) {
        IOTC_LOGW("create json error");
        return false;
    }

    int32_t ret = IotcJsonAddItem2Obj(json, STR_JSON_DEV_INFO, devInfoJson);
    if (ret != IOTC_OK) {
        IOTC_LOGW("add dev info json error %d", ret);
        IotcJsonDelete(devInfoJson);
        return false;
    }

    const IotcDeviceInfo *devInfo = ModelGetDevInfo();
    if (!LanSearchDevInfoCheck(devInfo)) {
        return false;
    }

    DFX_ANONYMIZE_SN_STR(anonySn, devInfo->sn);
    UtilsJsonStrItem strTable[] = {
        { STR_JSON_SN, anonySn },
        { STR_JSON_DEV_TYPE, devInfo->devTypeId },
        { STR_JSON_MANU, devInfo->manuId },
        { STR_JSON_PROD_ID, devInfo->prodId },
    };
    ret = UtilsJsonAddStrTable(devInfoJson, strTable, ARRAY_SIZE(strTable));
    if (ret != IOTC_OK) {
        IOTC_LOGW("add str to json error %d", ret);
        return false;
    }

    ret = IotcJsonAddNum2Obj(devInfoJson, STR_JSON_PROT_TYPE, devInfo->protType);
    if (ret != IOTC_OK) {
        IOTC_LOGW("add prot err %d", ret);
        return false;
    }

    /* 高四bit预留0110保证为可见字符，低四bit依次为softap coap sle ble发现能力 */
    const uint8_t discType = 0b01100100;
    ret = IotcJsonAddNum2Obj(devInfoJson, STR_JSON_DISC_TYPE, discType);
    if (ret != IOTC_OK) {
        IOTC_LOGW("add disc err %d", ret);
        return false;
    }

    return true;
}

static bool LanSearchAddSvcInfo(IotcJson *json)
{
    uint32_t svcNum = 0;
    const IotcServiceInfo *svcInfo = ModelGetSvcInfo(&svcNum);
    if (svcInfo == NULL || svcNum == 0) {
        IOTC_LOGW("get svc info error");
        return false;
    }

    IotcJson *svcInfoJson = MdlBuildSvcJsonArray(svcInfo, svcNum);
    if (svcInfoJson == NULL) {
        IOTC_LOGW("create svc json error");
        return false;
    }

    int32_t ret = IotcJsonAddItem2Obj(json, STR_JSON_SERVICES, svcInfoJson);
    if (ret != IOTC_OK) {
        IOTC_LOGW("add svc info json error %d", ret);
        IotcJsonDelete(svcInfoJson);
        return false;
    }

    return true;
}

static IotcJson *CreateLanSearchRespJson(void)
{
    IotcJson *json = UtilsJsonCreateErrcode(0);
    if (json == NULL) {
        IOTC_LOGW("create json error");
        return NULL;
    }

    do {
        if (!LanSearchAddDevid(json)) {
            break;
        }

        if (!LanSearchAddDevInfo(json)) {
            break;
        }

        if (!LanSearchAddSvcInfo(json)) {
            break;
        }
        return json;
    } while (0);
    IotcJsonDelete(json);
    return NULL;
}

void LanSearchCoapSearchHandler(CoapEndpoint *endpoint, const CoapPacket *req, const SocketAddr *addr, void *userData)
{
    CHECK_V_RETURN_LOGW(endpoint != NULL && req != NULL && addr != NULL && userData != NULL, "param invalid");

    IotcJson *respJson = CreateLanSearchRespJson();
    CHECK_V_RETURN_LOGW(respJson != NULL, "create lan search resp error");

    LanSearchSessMsg respMsg;
    (void)memset_s(&respMsg, sizeof(LanSearchSessMsg), 0, sizeof(LanSearchSessMsg));
    UTILS_BIT_SET(respMsg.bitMap, LAN_SEARCH_SESS_MSG_BIT_PLAIN);
    LanSearchSendJsonResp(endpoint, req, addr, respJson, &respMsg);
    IotcJsonDelete(respJson);
}

void LanSearchCoapSpekeHandler(CoapEndpoint *endpoint, const CoapPacket *req, const SocketAddr *addr, void *userData)
{
    CHECK_V_RETURN_LOGW(endpoint != NULL && req != NULL && addr != NULL && userData != NULL, "param invalid");

    LanSearchContext *ctx = (LanSearchContext *)userData;
    LanSearchSessMsg *sessMsg = (LanSearchSessMsg *)req;

    CHECK_V_RETURN_LOGW(req->payload.data != NULL && req->payload.len != 0, "invalid peer sess msg");

    int32_t ret;
    if (sessMsg->peer == NULL) {
        ret = LanSearchCreatePeer(ctx, addr->addr, &sessMsg->peer);
        if (ret != IOTC_OK) {
            IOTC_LOGW("create peer error %d", ret);
            return;
        }
    }
    if (sessMsg->peer == NULL || sessMsg->peer->sessInfo.speke == NULL) {
        IOTC_LOGW("invalid peer");
        return;
    }

    uint8_t *respMsg = NULL;
    uint32_t respLen = 0;
    ret = SpekeProcessPacket(sessMsg->peer->sessInfo.speke,
        (const char *)req->payload.data, &respMsg, &respLen);
    if (ret != IOTC_OK || respMsg == NULL || respLen == 0) {
        IOTC_LOGW("speke process packet error %d/%u", ret, respLen);
        if (respMsg != NULL) {
            IotcFree(respMsg);
        }
        return;
    }

    CoapData respPayload = { respMsg, respLen };
    LanSearchSessMsg sendMsg;
    (void)memset_s(&sendMsg, sizeof(LanSearchSessMsg), 0, sizeof(LanSearchSessMsg));
    UTILS_BIT_SET(sendMsg.bitMap, LAN_SEARCH_SESS_MSG_BIT_PLAIN);
    sendMsg.peer = sessMsg->peer;
    LanSearchSendDataResp(endpoint, req, addr, &respPayload, &sendMsg);
    IotcFree(respMsg);
}

void LanSearchCoapCloudSetupHandler(CoapEndpoint *endpoint, const CoapPacket *req,
    const SocketAddr *addr, void *userData)
{
    CHECK_V_RETURN_LOGW(endpoint != NULL && req != NULL && addr != NULL && userData != NULL &&
        req->payload.data != NULL && req->payload.len != 0, "param invalid");
    LanSearchSessMsg *sessMsg = (LanSearchSessMsg *)req;

    IotcJson *reqJson = IotcJsonParseWithLen((const char *)req->payload.data, req->payload.len);
    if (reqJson == NULL) {
        IOTC_LOGW("cloud setup json parse error");
        return;
    }

    IotcJson *dataJson = IotcJsonGetObj(reqJson, STR_JSON_DATA);
    if (dataJson == NULL) {
        IOTC_LOGW("cloud setup get data error");
        IotcJsonDelete(reqJson);
        return;
    }

    /* 当前端云未就绪，配网信息中只包含认证信息 */
    int32_t ret = DevSvcProxyRecvAuthInfo(dataJson);
    IotcJsonDelete(reqJson);
    reqJson = NULL;
    if (ret != IOTC_OK) {
        IOTC_LOGW("bind info process error %d", ret);
    }

    IotcJson *respJson = UtilsJsonCreateErrcode(ret);
    if (respJson == NULL) {
        IOTC_LOGW("create cloud setup resp error %d", ret);
        return;
    }

    LanSearchSessMsg sendMsg;
    (void)memset_s(&sendMsg, sizeof(LanSearchSessMsg), 0, sizeof(LanSearchSessMsg));
    sendMsg.peer = sessMsg->peer;
    LanSearchSendJsonResp(endpoint, req, addr, respJson, &sendMsg);
    IotcJsonDelete(respJson);
}