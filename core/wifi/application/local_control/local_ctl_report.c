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
#include "local_ctl_report.h"
#include <string.h>
#include "utils_assert.h"
#include "iotc_errcode.h"
#include "utils_bit_map.h"
#include "coap_endpoint_client.h"
#include "coap_codec_utils.h"
#include "dfx_anonymize.h"

static HashMapTravCode ClientMapTraversalReport(const void *value, va_list argp)
{
    CHECK_RETURN_LOGW(value != NULL, HASH_MAP_TRAVE_BREAK, "param invalid");

    LocalControlClient *client = (LocalControlClient *)value;
    if (!UTILS_IS_BIT_SET(client->bitMap, LOCAL_CTL_CLI_BIT_SESS_ACTIVE)) {
        return HASH_MAP_TRAVE_CONTINUE;
    }

    LocalControlContext *ctx = va_arg(argp, LocalControlContext *);
    const AdapterJson *reportJson = va_arg(argp, const AdapterJson *);
    uint32_t *reportNum = va_arg(argp, uint32_t *);
    if (reportJson == NULL || reportNum == NULL || ctx == NULL) {
        IOTC_LOGW("invalid param");
        return HASH_MAP_TRAVE_BREAK;
    }

    const CoapOption options[] = {
        {COAP_OPTION_TYPE_URI_PATH, {(const uint8_t *)STR_E2E_DATA_CHANGE, strlen(STR_E2E_DATA_CHANGE)}},
        {COAP_OPTION_TYPE_SESSION_ID, {(const uint8_t *)client->sessInfo.sessId, LOCAL_CONTROL_SESS_ID_STR_LEN}},
    };
    int32_t ret = AdapterJsonAddNum2Obj(reportJson, STR_JSON_SEQ_NUM, client->sessInfo.sendSeq);
    if (ret != IOTC_OK) {
        IOTC_LOGW("add seqNum error %d", ret);
        return ret;
    }

    CoapClientReqParam param = {
        .type = COAP_MSG_TYPE_CON,
        .code = COAP_METHOD_TYPE_POST,
        .opNum = ARRAY_SIZE(options),
        .options = options,
        .payload = NULL,
        .payloadBuilder = CoapUtilsBuildJsonPayloadFunc,
        .payloadUserData = (void *)reportJson,
        .respHandler = NULL,
        .preSize = 0,
    };
    SocketAddr addr = { LOCAL_CONTROL_REPORT_PORT, client->appInfo.addr };

    LocalCoapSessMsg sessMsg = {0};
    sessMsg.client = client;
    ret = CoapClientSendReq(ctx->coapServer.endpoint, &param, &addr, &sessMsg.packet);
    if (ret != IOTC_OK) {
        IOTC_LOGW("send report error %d", ret);
        return HASH_MAP_TRAVE_CONTINUE;
    }

    (*reportNum)++;
    client->sessInfo.sendSeq++;
    DFX_ANONYMIZE_IP_ADDR(anonyIp, client->appInfo.addr);
    DFX_ANONYMIZE_ID_STR(anonyPuuid, client->appInfo.puuid);
    IOTC_LOGI("send report to %s/%s", anonyIp, anonyPuuid);
    return HASH_MAP_TRAVE_CONTINUE;
}

int32_t LocalCtlReportToAllClient(const AdapterJson *dataArray, LocalControlContext *ctx)
{
    CHECK_RETURN_LOGW(dataArray != NULL && ctx != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    if (ctx->clientManager.curClientNum == 0) {
        IOTC_LOGD("no client to report");
        return IOTC_OK;
    }

    AdapterJson *reportJson = AdapterCreateJson();
    if (reportJson == NULL) {
        IOTC_LOGW("json create error");
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }

    AdapterJson *dataArrayClone = AdapterDuplicateJson(dataArray, true);
    if (dataArrayClone == NULL) {
        AdapterJsonDelete(reportJson);
        IOTC_LOGW("json clone error");
        return IOTC_ADAPTER_JSON_ERR_DUPLICATE;
    }

    int32_t ret = AdapterJsonAddItem2Obj(reportJson, STR_JSON_SERVICES, dataArrayClone);
    if (ret != IOTC_OK) {
        AdapterJsonDelete(reportJson);
        AdapterJsonDelete(dataArrayClone);
        IOTC_LOGW("json add item error %d", ret);
        return ret;
    }
    dataArrayClone = NULL;
    
    uint32_t reportNum = 0;
    (void)UtilsHashMapIterate(ctx->clientManager.clientMap, ClientMapTraversalReport, ctx, reportJson, &reportNum);
    AdapterJsonDelete(reportJson);
    IOTC_LOGI("send local report %u/%u", reportNum, ctx->clientManager.curClientNum);
    return IOTC_OK;
}