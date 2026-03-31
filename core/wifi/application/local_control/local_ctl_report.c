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

static int32_t ReportToTargetClient(IotcJson *json, LocalControlContext *ctx, LocalControlClient *cli)
{
    const CoapOption options[] = {
        { COAP_OPTION_TYPE_URI_PATH, { (const uint8_t *)STR_E2E_DATA_CHANGE, strlen(STR_E2E_DATA_CHANGE) } },
        { COAP_OPTION_TYPE_SESSION_ID, { (const uint8_t *)cli->sessInfo.sessId, LOCAL_CONTROL_SESS_ID_STR_LEN } },
    };
    if (IotcJsonHasObj(json, STR_JSON_SEQ_NUM)) {
        IotcJsonDeleteItem(json, STR_JSON_SEQ_NUM);
    }
    int32_t ret = IotcJsonAddNum2Obj(json, STR_JSON_SEQ_NUM, cli->sessInfo.sendSeq);
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
        .payloadUserData = (void *)json,
        .respHandler = NULL,
        .preSize = 0,
    };
    SocketAddr addr = { LOCAL_CONTROL_REPORT_PORT, cli->appInfo.addr };

    LocalCoapSessMsg sessMsg = {0};
    sessMsg.client = cli;
    ret = CoapClientSendReq(ctx->coapStack.endpoint, &param, &addr, &sessMsg.packet);
    if (ret != IOTC_OK) {
        IOTC_LOGW("send report error %d", ret);
        return ret;
    }

    cli->sessInfo.sendSeq++;
    DFX_ANONYMIZE_IP_ADDR(anonyIp, cli->appInfo.addr);
    DFX_ANONYMIZE_ID_STR(anonyPuuid, cli->appInfo.puuid);
    IOTC_LOGI("send report to %s/%s", anonyIp, anonyPuuid);
    return IOTC_OK;
}

static HashMapTravCode ClientMapTraversalReport(const void *value, va_list argp)
{
    CHECK_RETURN_LOGW(value != NULL, HASH_MAP_TRAVE_BREAK, "param invalid");

    LocalControlClient *client = (LocalControlClient *)value;
    if (!UTILS_IS_BIT_SET(client->bitMap, LOCAL_CTL_CLI_BIT_SESS_ACTIVE)) {
        return HASH_MAP_TRAVE_CONTINUE;
    }

    LocalControlContext *ctx = va_arg(argp, LocalControlContext *);
    IotcJson *reportJson = va_arg(argp, IotcJson *);
    uint32_t *succNum = va_arg(argp, uint32_t *);
    uint32_t *failNum = va_arg(argp, uint32_t *);
    if (reportJson == NULL || succNum == NULL || ctx == NULL || failNum == NULL) {
        IOTC_LOGW("param invalid");
        return HASH_MAP_TRAVE_BREAK;
    }

    int32_t ret = ReportToTargetClient(reportJson, ctx, client);
    if (ret == IOTC_OK) {
        (*succNum)++;
    } else {
        (*failNum)++;
    }
    return HASH_MAP_TRAVE_CONTINUE;
}

static int32_t CreateLocalCtlReportJson(const IotcJson *dataArray, IotcJson **root)
{
    IotcJson *reportJson = IotcJsonCreate();
    if (reportJson == NULL) {
        IOTC_LOGW("json create error");
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }

    IotcJson *dataArrayClone = IotcDuplicateJson(dataArray, true);
    if (dataArrayClone == NULL) {
        IotcJsonDelete(reportJson);
        IOTC_LOGW("json clone error");
        return IOTC_ADAPTER_JSON_ERR_DUPLICATE;
    }

    int32_t ret = IotcJsonAddItem2Obj(reportJson, STR_JSON_SERVICES, dataArrayClone);
    if (ret != IOTC_OK) {
        IotcJsonDelete(reportJson);
        IotcJsonDelete(dataArrayClone);
        IOTC_LOGW("json add item error %d", ret);
        return ret;
    }
    dataArrayClone = NULL;
    *root = reportJson;
    return IOTC_OK;
}

int32_t LocalCtlReportToAllClient(const IotcJson *dataArray, LocalControlContext *ctx)
{
    CHECK_RETURN_LOGW(dataArray != NULL && ctx != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    if (ctx->clientManager.curClientNum == 0) {
        IOTC_LOGD("no client to report");
        return IOTC_OK;
    }
    IotcJson *reportJson = NULL;
    int32_t ret = IOTC_OK;
    if (IotcJsonHasObj(dataArray, STR_JSON_VENDOR) == true) {
        IotcJson* respJson = IotcJsonGetObj(dataArray, STR_JSON_VENDOR);
        ret = CreateLocalCtlReportJson(respJson, &reportJson);
        } else {
        ret = CreateLocalCtlReportJson(dataArray, &reportJson);
    }
    if (ret != IOTC_OK) {
        IOTC_LOGW("create report all json error %d", ret);
        return ret;
    }

    uint32_t succNum = 0;
    uint32_t failNum = 0;
    (void)UtilsHashMapIterate(ctx->clientManager.clientMap, ClientMapTraversalReport, ctx, reportJson,
        &succNum, &failNum);
    IotcJsonDelete(reportJson);
    IOTC_LOGI("send local report %u/%u/%u", succNum, failNum, ctx->clientManager.curClientNum);
    return IOTC_OK;
}

int32_t LocalCtlReportToTargetClient(const IotcJson *json, LocalControlContext *ctx, LocalControlClient *cli)
{
    CHECK_RETURN_LOGW(json != NULL && ctx != NULL && cli != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    if (!UTILS_IS_BIT_SET(cli->bitMap, LOCAL_CTL_CLI_BIT_SESS_ACTIVE)) {
        IOTC_LOGW("client not active");
        return IOTC_CORE_WIFI_LOCAL_CTL_ERR_INVALID_CLIENT;
    }

    IotcJson *reportJson = NULL;
    int32_t ret = CreateLocalCtlReportJson(json, &reportJson);
    if (ret != IOTC_OK) {
        IOTC_LOGW("create report target json error %d", ret);
        return ret;
    }

    ret = ReportToTargetClient(reportJson, ctx, cli);
    IotcJsonDelete(reportJson);
    if (ret != IOTC_OK) {
        IOTC_LOGW("report to target error %d", ret);
        return ret;
    }

    return IOTC_OK;
}
