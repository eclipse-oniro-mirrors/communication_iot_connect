
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
#include "svc_softap_report.h"
#include <string.h>
#include "utils_json.h"
#include "utils_assert.h"
#include "svc_softap_ctx.h"
#include "coap_endpoint_client.h"
#include "utils_common.h"
#include "coap_codec_utils.h"
#include "utils_bit_map.h"
#include "sched_executor.h"
#include "iotc_errcode.h"

static void SoftapServiceReportRespHandler(const CoapPacket *resp, const SocketAddr *addr,
    void *userData, bool timeout)
{
    NOT_USED(userData);
    if (timeout) {
        IOTC_LOGW("service report timeout");
        return;
    }

    CHECK_V_RETURN_LOGE(resp != NULL && resp->payload.data != NULL && resp->payload.len != 0,
        "resp packet invalid");
    AdapterJson *respJsonObj = AdapterJsonParseWithLen((const char *)resp->payload.data, resp->payload.len);
    if (respJsonObj == NULL) {
        IOTC_LOGW("parse json error %u", resp->payload.len);
        return;
    }

    int32_t errcode;
    int32_t ret = UtilsJsonGetNum(respJsonObj, STR_ERRCODE, &errcode);
    AdapterJsonDelete(respJsonObj);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get json errcode error %d", ret);
        return;
    }
    if (errcode != IOTC_OK) {
        IOTC_LOGE("softap service report errcode %d", errcode);
    } else {
        IOTC_LOGI("softap service report ok");
    }
}

static void ServiceReportToSession(AdapterJson *root, SoftapPeerSess *peer, CoapEndpoint *endpoint)
{
    if (AdapterJsonHasObj(root, STR_JSON_SEQ)) {
        AdapterDeleteItemFromJson(root, STR_JSON_SEQ);
    }

    int32_t ret = AdapterJsonAddNum2Obj(root, STR_JSON_SEQ, peer->sendSeq);
    if (ret != IOTC_OK) {
        IOTC_LOGW("add seq error %d", ret);
        return;
    }

    const CoapOption options[] = {
        {COAP_OPTION_TYPE_URI_PATH, {(const uint8_t *)STR_E2E_DATA_CHANGE, strlen(STR_E2E_DATA_CHANGE)}},
    };

    CoapClientReqParam param = {
        .type = COAP_MSG_TYPE_NCON,
        .code = COAP_METHOD_TYPE_POST,
        .opNum = ARRAY_SIZE(options),
        .options = options,
        .payload = NULL,
        .payloadBuilder = CoapUtilsBuildJsonPayloadFunc,
        .respHandler = SoftapServiceReportRespHandler,
        .payloadUserData = root,
        .preSize = 0,
    };
    SocketAddr addr = { WIFI_SOFTAP_UDP_PORT, peer->addrInfo.addr };

    CoapPacket packet;
    ret = CoapClientSendReq(endpoint, &param, &addr, &packet);
    if (ret != IOTC_OK) {
        IOTC_LOGW("send req error %d", ret);
        return;
    }
    ++peer->sendSeq;
}

static int32_t CreateReportJson(const AdapterJson *dataArray, AdapterJson **root)
{
    AdapterJson *dataArrayClone = AdapterDuplicateJson(dataArray, true);
    if (dataArrayClone == NULL) {
        IOTC_LOGW("copy json error");
        return IOTC_ADAPTER_JSON_ERR_DUPLICATE;
    }

    *root = AdapterCreateJson();
    if (*root == NULL) {
        IOTC_LOGW("create root json error");
        AdapterJsonDelete(dataArrayClone);
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }

    int32_t ret = AdapterJsonAddItem2Obj(*root, STR_JSON_SERVICES, dataArrayClone);
    if (ret != IOTC_OK) {
        IOTC_LOGW("add array to root error %d", ret);
        AdapterJsonDelete(dataArrayClone);
        AdapterJsonDelete(*root);
        *root = NULL;
        return ret;
    }
    return IOTC_OK;
}

int32_t SoftapServiceReportToAllPeer(const AdapterJson *dataArray)
{
    CHECK_RETURN_LOGW(dataArray != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    SoftapServiceContext *ctx = GetSoftapServiceContext();
    if (ctx == NULL) {
        IOTC_LOGW("invalid softap ctx");
        return IOTC_CORE_WIFI_NETCFG_ERR_SOFTAP_INVALID_CTX;
    }

    AdapterJson *root = NULL;
    int32_t ret = CreateReportJson(dataArray, &root);
    if (ret != IOTC_OK) {
        IOTC_LOGW("create report json error %d", ret);
        return ret;
    }

    for (uint32_t i = 0; i < IOTC_CONF_SOFTAP_MAX_PEER_SESS_NUM; ++i) {
        if (!UTILS_IS_BIT_SET(ctx->sess.peerSess[i].bitMap, SOFTAP_PEER_SESS_BIT_MAP_SPEKE_SESS_CREATED)) {
            continue;
        }
        ServiceReportToSession(root, &ctx->sess.peerSess[i], ctx->sess.endpoint);
    }
    AdapterJsonDelete(root);
    return IOTC_OK;
}

int32_t SoftapServiceReportToTargetPeer(const AdapterJson *dataArray, uint32_t peerAddr)
{
    CHECK_RETURN_LOGW(dataArray != NULL && peerAddr != 0, IOTC_ERR_PARAM_INVALID, "param invalid");
    SoftapServiceContext *ctx = GetSoftapServiceContext();
    if (ctx == NULL) {
        IOTC_LOGW("invalid softap ctx");
        return IOTC_CORE_WIFI_NETCFG_ERR_SOFTAP_INVALID_CTX;
    }

    SoftapPeerSess *sess = NULL;
    for (uint32_t i = 0; i < IOTC_CONF_SOFTAP_MAX_PEER_SESS_NUM; ++i) {
        if (!UTILS_IS_BIT_SET(ctx->sess.peerSess[i].bitMap, SOFTAP_PEER_SESS_BIT_MAP_SPEKE_SESS_CREATED) ||
            ctx->sess.peerSess[i].addrInfo.addr != peerAddr) {
            continue;
        }
        sess = &ctx->sess.peerSess[i];
    }
    if (sess == NULL) {
        IOTC_LOGW("invalid peer to report");
        return IOTC_CORE_WIFI_NETCFG_ERR_E2E_CTRL_INVALID_PEER;
    }

    AdapterJson *root = NULL;
    int32_t ret = CreateReportJson(dataArray, &root);
    if (ret != IOTC_OK) {
        IOTC_LOGW("create report json error %d", ret);
        return ret;
    }

    ServiceReportToSession(root, sess, ctx->sess.endpoint);
    AdapterJsonDelete(root);
    return IOTC_OK;
}