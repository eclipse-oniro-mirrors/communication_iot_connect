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
#include "securec.h"
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
    IotcJson *dataArray;
    E2eCtrlMsgReportAfterGetCmd func;
} E2eCtlAsyncReportParam;

static int32_t E2eCtrlMsgPutProcess(const IotcJson *dataJsonArray)
{
    int32_t ret = DevSvcProxyCtlPutCharStates(dataJsonArray, NULL);
    if (ret != IOTC_OK) {
        IOTC_LOGW("e2e ctrl put error %d", ret);
    }
    return ret;
}

static void ReportAfterGetCmdExecutorCallback(void *userData)
{
    CHECK_V_RETURN_LOGW(userData != NULL, "param invalid");

    E2eCtlAsyncReportParam *param = (E2eCtlAsyncReportParam *)userData;
    if (param->func != NULL) {
        param->func(param->dataArray, param->userData, param->userDataLen);
    } else {
        IOTC_LOGW("report func null");
    }
    if (param->dataArray != NULL) {
        IotcJsonDelete(param->dataArray);
    }
    if (param->userData != NULL) {
        IotcFree(param->userData);
    }
    IotcFree(param);
}

static int32_t E2eCtrlMsgGetProcess(const IotcJson *dataJsonArray,
    E2eCtrlMsgReportAfterGetCmd reportFunc, const void *userData, uint32_t userDataLen)
{
    IotcJson *reportJsonArray = NULL;
    int32_t ret = DevSvcProxyCtlGetCharStates(dataJsonArray, &reportJsonArray);
    if (ret != IOTC_OK) {
        IOTC_LOGW("e2e get char error %d", ret);
        return ret;
    }
    if (reportJsonArray == NULL) {
        IOTC_LOGW("e2e get char no data");
        return IOTC_CORE_WIFI_E2E_CTL_ERR_GET_CHAR_NO_DATA;
    }

    E2eCtlAsyncReportParam *asyncParam = (E2eCtlAsyncReportParam *)IotcMalloc(sizeof(E2eCtlAsyncReportParam));
    if (asyncParam == NULL) {
        IOTC_LOGW("malloc error");
        IotcJsonDelete(reportJsonArray);
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(asyncParam, sizeof(E2eCtlAsyncReportParam), 0, sizeof(E2eCtlAsyncReportParam));
    asyncParam->dataArray = reportJsonArray;
    asyncParam->func = reportFunc;
    if (userData != NULL && userDataLen != 0) {
        asyncParam->userDataLen = userDataLen;
        asyncParam->userData = (void *)UtilsMallocCopy(userData, userDataLen);
        if (asyncParam->userData == NULL) {
            IOTC_LOGW("clone error %u", userDataLen);
            IotcJsonDelete(reportJsonArray);
            IotcFree(asyncParam);
            return IOTC_CORE_COMM_UTILS_ERR_MALLOC_COPY;
        }
    }

    ret = SchedAsyncExecutor(ReportAfterGetCmdExecutorCallback, asyncParam);
    if (ret != IOTC_OK) {
        IOTC_LOGW("async report exec error %d", ret);
        IotcJsonDelete(reportJsonArray);
        if (asyncParam->userData != NULL) {
            IotcFree(asyncParam->userData);
        }
        IotcFree(asyncParam);
        return ret;
    }
    return IOTC_OK;
}

int32_t E2eCtrlMsgProcess(const IotcJson *req, E2eCtrlMsgReportAfterGetCmd reportFunc,
    const void *userData, uint32_t userDataLen)
{
    CHECK_RETURN_LOGW(req != NULL && reportFunc != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    const IotcJson *dataJsonArray = IotcJsonGetObj(req, STR_JSON_DATA);
    if (dataJsonArray == NULL) {
        IOTC_LOGW("no data array");
        return IOTC_CORE_WIFI_NETCFG_ERR_E2E_CTRL_NO_DATA;
    }
    uint32_t dataJsonArraySize = 0;
    int32_t ret = IotcJsonGetArraySize(dataJsonArray, &dataJsonArraySize);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get data size error %d", ret);
        return ret;
    }

    /* data为空时触发全量异步上报 */
    if (dataJsonArraySize == 0) {
        IOTC_LOGI("report all async");
        ret = DevSvcProxyCtlReportAll(DEV_REPORT_TYPE_ASYNC);
        if (ret != IOTC_OK) {
            IOTC_LOGW("async report all error %d", ret);
        }
        return ret;
    }

    bool isCtrl = IotcJsonHasObj(AdapterJsonGetArrayItem(dataJsonArray, 0), STR_JSON_DATA);
    if (isCtrl) {
        return E2eCtrlMsgPutProcess(dataJsonArray);
    } else {
        return E2eCtrlMsgGetProcess(dataJsonArray, reportFunc, userData, userDataLen);
    }
}
