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
#include "e2e_ctl_msg.h"
#include <stddef.h>
#include "comm_def.h"
#include "iotc_errcode.h"
#include "iotc_svc_dev.h"
#include "iotc_log.h"
#include "utils_assert.h"
#include "adapter_mem.h"
#include "utils_common.h"
#include "sched_executor.h"

typedef struct {
    void *userData;
    uint32_t userDataLen;
    AdapterJson *dataArray;
    E2eCtrlMsgReportAfterGetCmd func;
} E2eCtlAsyncReportParam;

static int32_t E2eCtrlMsgPutProcess(const AdapterJson *dataJsonArray)
{
    int32_t ret = DevSvcProxyCtlPutCharStates(dataJsonArray, NULL);
    if (ret != IOTC_OK) {
        IOTC_LOGE("e2e ctrl put error %d", ret);
    }
    return ret;
}

static void ReportAfterGetCmdExecutorCallback(void *userData)
{
    CHECK_V_RETURN_LOGW(userData != NULL, "param invalid");

    E2eCtlAsyncReportParam *param = (E2eCtlAsyncReportParam *)userData;
    if (param->func != NULL) {
        param->func(param->dataArray, param->userData, param->userDataLen);
    }
    if (param->dataArray != NULL) {
        AdapterJsonDelete(param->dataArray);
    }
    UTILS_FREE_2_NULL(param->userData);
    AdapterFree(param);
}

static int32_t E2eCtrlMsgGetProcess(const AdapterJson *dataJsonArray,
    E2eCtrlMsgReportAfterGetCmd reportFunc, const void *userData, uint32_t userDataLen)
{
    AdapterJson *outArray = NULL;
    int32_t ret = DevSvcProxyCtlGetCharStates(dataJsonArray, &outArray);
    if (ret != IOTC_OK) {
        IOTC_LOGE("e2e get char error %d", ret);
        return ret;
    }

    if (outArray == NULL) {
        IOTC_LOGE("e2e get char no data");
        return IOTC_CORE_WIFI_NETCFG_ERR_E2E_CTRL_NO_DATA;
    }

    E2eCtlAsyncReportParam *param = (E2eCtlAsyncReportParam *)AdapterMalloc(sizeof(E2eCtlAsyncReportParam));
    if (param == NULL) {
        IOTC_LOGW("malloc error");
        AdapterJsonDelete(outArray);
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    param->dataArray = outArray;
    param->func = reportFunc;
    if (userData != NULL && userDataLen != 0) {
        param->userDataLen = userDataLen;
        param->userData = UtilsMallocCopy(userData, userDataLen);
        if (param->userData == NULL) {
            IOTC_LOGW("clone error %u", userDataLen);
            AdapterJsonDelete(outArray);
            AdapterFree(param);
            return IOTC_CORE_COMM_UTILS_ERR_MALLOC_COPY;
        }
    }

    ret = SchedAsyncExecutor(ReportAfterGetCmdExecutorCallback, param);
    if (ret != IOTC_OK) {
        IOTC_LOGW("add executor error %d", ret);
        AdapterJsonDelete(outArray);
        UTILS_FREE_2_NULL(param->userData);
        AdapterFree(param);
        return ret;
    }
    return IOTC_OK;
}

int32_t E2eCtrlMsgProcess(const AdapterJson *req, E2eCtrlMsgReportAfterGetCmd reportFunc,
    const void *userData, uint32_t userDataLen)
{
    CHECK_RETURN_LOGW(req != NULL && reportFunc != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    const AdapterJson *dataJsonArray = AdapterJsonGetObj(req, STR_JSON_DATA);
    if (dataJsonArray == NULL) {
        IOTC_LOGE("no data array");
        return IOTC_CORE_WIFI_NETCFG_ERR_E2E_CTRL_NO_DATA;
    }
    uint32_t dataJsonArraySize = 0;
    int32_t ret = AdapterJsonGetArraySize(dataJsonArray, &dataJsonArraySize);
    if (ret != IOTC_OK) {
        IOTC_LOGE("get data size error %d", ret);
        return ret;
    }

    /* data为空时触发全量异步上报 */
    if (dataJsonArraySize == 0) {
        IOTC_LOGI("report all async");
        ret = DevSvcProxyCtlReportAll(DEV_REPORT_TYPE_ASYNC);
        if (ret != IOTC_OK) {
            IOTC_LOGE("async report all error %d", ret);
        }
        return ret;
    }

    /* 携带data字段为控制指令，否则为查询指令 */
    bool isCtrl = AdapterJsonHasObj(AdapterJsonGetArrayItem(dataJsonArray, 0), STR_JSON_DATA);
    if (isCtrl) {
        return E2eCtrlMsgPutProcess(dataJsonArray);
    } else {
        return E2eCtrlMsgGetProcess(dataJsonArray, reportFunc, userData, userDataLen);
    }
}
