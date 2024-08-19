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
#include "m2m_cloud_sync.h"
#include "utils_assert.h"
#include "utils_json.h"
#include "comm_def.h"
#include "securec.h"
#include "utils_common.h"
#include "svc_info.h"
#include "svc_info_mdl.h"
#include "m2m_cloud_errcode.h"
#include "dev_info.h"
#include "m2m_cloud_token.h"
#include "m2m_cloud_heartbeat.h"
#include "utils_bit_map.h"
#include "m2m_cloud_utils.h"
#include "iotc_errcode.h"
#include "iotc_svc_dev.h"
#include "event_bus_pub.h"
#include "iotc_event.h"

static int32_t BuildDevInfoSyncSvcInfo(IotcJson *rootObj, const M2mCloudContext *ctx)
{
    uint32_t num = 0;
    const IotcServiceInfo *svcInfo = ModelGetSvcInfo(&num);
    if (svcInfo == NULL || num == 0) {
        IOTC_LOGW("get svc invalid %u", num);
        return IOTC_CORE_PROF_MDL_ERR_SVC_NUM_INVALID;
    }
    IotcJson *svcInfoArr = MdlBuildSvcJsonArray(svcInfo, num);
    if (svcInfoArr == NULL) {
        IOTC_LOGW("create json error");
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }

    int32_t ret = IotcJsonAddItem2Obj(rootObj, STR_JSON_SERVICES, svcInfoArr);
    if (ret != IOTC_OK) {
        IOTC_LOGE("add svc info to root err %d", ret);
        IotcJsonDelete(svcInfoArr);
        return ret;
    }

    ret = IotcJsonAddStr2Obj(rootObj, STR_JSON_DEVID, ctx->authInfo.loginInfo.devId);
    if (ret != IOTC_OK) {
        IOTC_LOGE("add svc info to root err %d", ret);
        return ret;
    }

    return IOTC_OK;
}

IotcJson *M2mCloudBuildDevInfoSyncRequest(M2mCloudContext *ctx)
{
    CHECK_RETURN_LOGW(ctx != NULL, NULL, "param invalid");
    IotcJson *devInfoArr = IotcJsonCreateArray();
    CHECK_RETURN_LOGW(devInfoArr != NULL, NULL, "create json error");

    IotcJson *devinfoObj = NULL;
    int32_t ret;
    do {
        devinfoObj = IotcJsonCreate();
        if (devinfoObj == NULL) {
            IOTC_LOGW("create json error");
            ret = IOTC_ADAPTER_JSON_ERR_CREATE;
            break;
        }
        ret = M2mCloudAddDevInfoToJson(devinfoObj);
        if (ret != IOTC_OK) {
            IotcJsonDelete(devinfoObj);
            IOTC_LOGW("add dev info error %d", ret);
            break;
        }

        ret = BuildDevInfoSyncSvcInfo(devinfoObj, (const M2mCloudContext *)ctx);
        if (ret != IOTC_OK) {
            IotcJsonDelete(devinfoObj);
            IOTC_LOGW("add svc info error %d", ret);
            break;
        }

        ret = IotcJsonAddItem2Array(devInfoArr, devinfoObj);
        if (ret != IOTC_OK) {
            IotcJsonDelete(devinfoObj);
            IOTC_LOGW("add svc info error %d", ret);
            break;
        }
        return devInfoArr;
    } while (0);

    IotcJsonDelete(devInfoArr);
    return NULL;
}

int32_t M2mCloudParseDevInfoSyncResponse(M2mCloudContext *ctx, const CoapPacket *resp, int32_t *errcode)
{
    CHECK_RETURN_LOGW(ctx != NULL && resp != NULL && errcode != NULL && resp->payload.data != NULL &&
        resp->payload.len != 0, IOTC_ERR_PARAM_INVALID, "invalid param");

    IotcJson *respJson = IotcJsonParseWithLen((const char *)resp->payload.data, resp->payload.len);
    if (respJson == NULL) {
        IOTC_LOGW("create json error");
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }

    int32_t ret = UtilsJsonGetNum(respJson, STR_ERRCODE, errcode);
    if (ret != IOTC_OK) {
        IOTC_LOGE("json get errcode error %d", ret);
        IotcJsonDelete(respJson);
        return ret;
    }

    if (*errcode == CLOUD_ERRCODE_OK) {
        ret = DevSvcProxyCtlReportAll(DEV_REPORT_TYPE_ASYNC);
        if (ret != IOTC_OK) {
            IOTC_LOGW("report all error %d", ret);
        }
        ret = M2mCloudEnableHeartbeat(ctx);
        if (ret != IOTC_OK) {
            IOTC_LOGW("enable heartbeat error %d", ret);
        }
    }
    return IOTC_OK;
}

const CloudOption *M2mCloudGetDevInfoSyncOption(void)
{
    static const char *SYS_SYNC[] = {STR_URI_PATH_SYS, STR_URI_PATH_SYNC};
    static const CloudOption SYNC_OPTION = {
        .uri = SYS_SYNC,
        .num = ARRAY_SIZE(SYS_SYNC),
        .opBitMap = UTILS_BIT(CLOUD_OPTION_BIT_SEQ_NUM_ID) | UTILS_BIT(CLOUD_OPTION_BIT_ACCESS_TOKEN_ID),
    };
    return &SYNC_OPTION;
}