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

int32_t CoapE2eCtrlMsgProcess(const AdapterJson *req)
{
    AdapterJson *dataJsonArray = AdapterJsonGetObj(req, STR_JSON_DATA);
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

    /* 最外层data为空时全量异步上报 */
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
        ret = DevSvcProxyCtlPutCharStates(dataJsonArray, NULL);
        if (ret != IOTC_OK) {
            IOTC_LOGE("ctrl error %d", ret);
            return ret;
        }
    }

    return IOTC_OK;
}
