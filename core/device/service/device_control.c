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
#include "device_control.h"
#include "char_state_mdl.h"
#include "utils_assert.h"
#include "sched_executor.h"
#include "iotc_os.h"
#include "utils_common.h"
#include "iotc_errcode.h"
#include "dev_mngr_ctx.h"
#include "service_proxy.h"
#include "product_adapter.h"

static void ReportAllServiceExecutorCallback(void *userData)
{
    NOT_USED(userData);
    int32_t ret = ProductProfReportAll();
    if (ret != IOTC_OK) {
        IOTC_LOGW("report all error %d", ret);
    }
}

int32_t DeviceControlReportAll(DevReportType type)
{
    int32_t ret;
    if (type == DEV_REPORT_TYPE_ASYNC) {
        ret = SchedAsyncExecutor(ReportAllServiceExecutorCallback, NULL);
    } else {
        ret = ProductProfReportAll();
    }
    if (ret != IOTC_OK) {
        IOTC_LOGW("report all error %d/%d", ret, type);
    }
    return ret;
}

static void ReportByDevIdServiceExecutorCallback(void *userData)
{
    int32_t ret = ProductProfReportByDevId((const char *)userData);
    if (ret != IOTC_OK) {
        IOTC_LOGW("report by devid [%s] error %d",(const char *)userData, ret);
    }
}

int32_t DeviceControlReportByDevId(DevReportType type, const char *devId)
{
    int32_t ret;
    if (type == DEV_REPORT_TYPE_ASYNC) {
        ret = SchedAsyncExecutor(ReportByDevIdServiceExecutorCallback, (void*)devId);
    } else {
        ret = ProductProfReportByDevId(devId);
    }
    if (ret != IOTC_OK) {
        IOTC_LOGW("report by devid [%s] error %d",(const char *)devId, ret);
    }
    return ret;
}

static void ProductCharStateFree(char *data[], uint32_t num)
{
    CHECK_V_RETURN(data != NULL && num != 0);
    for (uint32_t i = 0; i < num; ++i) {
        ProductProfFree(data[i]);
        data[i] = NULL;
    }
}

static int32_t UpdateCharStateByGetChar(IotcCharState *states, uint32_t size)
{
    GetCharStatesData charData = {0};
    int32_t ret = MdlInitGetCharStatesData(size, &charData);
    if (ret != IOTC_OK) {
        IOTC_LOGW("init char data error %u", size);
        return ret;
    }

    do {
        ret = ProductProfGetCharState(states, charData.data, charData.len, size);
        if (ret != IOTC_OK) {
            IOTC_LOGW("get char error %d", ret);
            break;
        }

        ret = MdlUpdateCharStates(states, &charData, size);
        if (ret != IOTC_OK) {
            IOTC_LOGW("update data error %d", ret);
            break;
        }
    } while (0);
    /* get char 内存由产品申请，由产品接口释放 */
    ProductCharStateFree(charData.data, size);
    MdlFreeGetCharStatesData(&charData);
    return ret;
}

int32_t DeviceControlGetCharStates(const IotcJson *inArray, IotcJson **outArray)
{
    CHECK_RETURN_LOGE(inArray != NULL && outArray != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    IotcCharState *states = NULL;
    uint32_t statesNum = 0;
    int32_t ret;
    do {
        ret = MdlGetJsonArrayToCharStates(inArray, &states, &statesNum);
        if (ret != IOTC_OK) {
            IOTC_LOGW("mdl get char states error %d", ret);
            break;
        }

        ret = UpdateCharStateByGetChar(states, statesNum);
        if (ret != IOTC_OK) {
            break;
        }

        ret = MdlCharStatesToJson(states, statesNum, outArray);
        if (ret != IOTC_OK) {
            IOTC_LOGW("build get char json array %d", ret);
            break;
        }
    } while (0);

    MdlCharStatesFree(&states, statesNum);
    if (ret != IOTC_OK && *outArray != NULL) {
        IotcJsonDelete(*outArray);
        *outArray = NULL;
    }
    return ret;
}

static void ReportChangeServiceCallback(void *userData)
{
    CHECK_V_RETURN_LOGW(userData != NULL, "param invalid");

    IotcJson *dataArray = (IotcJson *)userData;
    DeviceContext *ctx = GetDeviceManagerContext();
    ServiceRequestInfo req = {
        .instanceId = ctx->instanceId,
        .msgId = DEVICE_SERVICE_MSG_ID_REPORT,
        .req = dataArray,
    };
    int32_t ret = ServiceProxySendMessage(&req, NULL, NULL);
    IotcJsonDelete(dataArray);
    if (ret != IOTC_OK) {
        IOTC_LOGW("send report msg error %d", ret);
    }
}

int32_t DeviceControlPutCharStates(const IotcJson *inArray, IotcJson **outArray)
{
    CHECK_RETURN_LOGE(inArray != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    IotcCharState *states = NULL;
    uint32_t statesNum = 0;
    int32_t ret;
    do {
        ret = MdlPutJsonArrayToCharStates(inArray, &states, &statesNum);
        if (ret != IOTC_OK) {
            IOTC_LOGW("mdl put char states error %d", ret);
            break;
        }

        ret = ProductProfPutCharState(states, statesNum);
        if (ret != IOTC_OK) {
            IOTC_LOGW("put char error %d", ret);
            break;
        }

        ret = UpdateCharStateByGetChar(states, statesNum);
        if (ret != IOTC_OK) {
            break;
        }

        IotcJson *dataArray = NULL;
        ret = MdlCharStatesToJson(states, statesNum, &dataArray);
        if (ret != IOTC_OK) {
            IOTC_LOGW("build char json error %d", ret);
            break;
        }

        if (outArray != NULL) {
            *outArray = IotcDuplicateJson(dataArray, true);
        }

        ret = SchedAsyncExecutor(ReportChangeServiceCallback, dataArray);
        if (ret != IOTC_OK) {
            IotcJsonDelete(dataArray);
            IOTC_LOGW("add executor error %d", ret);
            break;
        }
        dataArray = NULL;
        if (outArray != NULL && *outArray == NULL) {
            ret = IOTC_ADAPTER_JSON_ERR_DUPLICATE;
            IOTC_LOGW("json dup error");
            break;
        }
    } while (0);

    MdlCharStatesFree(&states, statesNum);
    if (ret != IOTC_OK && outArray != NULL && *outArray != NULL) {
        IotcJsonDelete(*outArray);
        *outArray = NULL;
    }
    return ret;
}