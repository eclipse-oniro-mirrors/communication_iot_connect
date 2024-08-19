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
#include <string.h>
#include "ble_svc.h"
#include "event_bus.h"
#include "utils_common.h"
#include "utils_assert.h"
#include "utils_json.h"
#include "ble_linklayer.h"
#include "ble_profile.h"
#include "ble_session_mngr.h"
#include "service_proxy.h"
#include "iotc_svc_dev.h"
#include "ble_svc_ctx.h"
#include "iotc_svc.h"
#include "iotc_errcode.h"
#include "iotc_event.h"

static int32_t BleReportCustomSecDataService(const IotcJson *item)
{
    IotcJson *root = IotcJsonCreate();
    CHECK_RETURN_LOGW(root != NULL, IOTC_ADAPTER_JSON_ERR_CREATE, "create json error");

    IotcJson *dupItem = IotcDuplicateJson(item, true);
    if (dupItem == NULL) {
        IOTC_LOGW("duplicate item err");
        IotcJsonDelete(root);
        return IOTC_ADAPTER_JSON_ERR_DUPLICATE;
    }

    int32_t ret = IotcJsonAddItem2Obj(root, STR_JSON_VENDOR, dupItem);
    if (ret != IOTC_OK) {
        IOTC_LOGW("add item error %d", ret);
        IotcJsonDelete(root);
        IotcJsonDelete(dupItem);
        return ret;
    }

    char *data = NULL;
    do {
        ret = IotcJsonAddNum2Obj(root, STR_JSON_SEQ, (int64_t)BleSessSendSeqGet());
        if (ret != IOTC_OK) {
            IOTC_LOGW("add seq error %d", ret);
            break;
        }

        data = UtilsJsonPrintByMalloc(root);
        if (data == NULL) {
            IOTC_LOGW("json print error");
            ret = IOTC_CORE_COMM_UTILS_ERR_JSON_MALLOC_PRINT;
            break;
        }

        ret = LinkLayerReportSvcDataEnc(BLE_SVC_CUSTOM_SEC_DATA, (uint8_t *)data, strlen(data));
        if (ret != IOTC_OK) {
            IOTC_LOGW("ll send error %d", ret);
            break;
        }
        BleSessSendSeqUpdate();
    } while (0);
    IotcJsonDelete(root);
    UTILS_FREE_2_NULL(data);
    return ret;
}

int32_t BleUplinkReportMethod(const IotcJson *dataArray)
{
    CHECK_RETURN_LOGE(dataArray != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    int32_t ret;
#if IOTC_CONF_AILIFE_SUPPORT
    uint32_t size = 0;
    ret = IotcJsonGetArraySize(dataArray, &size);
    CHECK_RETURN_LOGW(ret == IOTC_OK, ret, "get arr size err:%d", ret);
    for (uint32_t i = 0; i < size; i++) {
        IotcJson *item = IotcJsonGetArrayItem(dataArray, i);
        if (item == NULL) {
            IOTC_LOGW("get svc[%u] err", i);
            continue;
        }
        int32_t err = BleReportCustomSecDataService(item);
        if (err != IOTC_OK) {
            IOTC_LOGW("rpt svc[%u] err:%d", i, err);
            ret = err;
        }
    }
#else
    ret = BleReportCustomSecDataService(dataArray);
#endif
    return ret;
}

static int32_t DeviceServiceReportMessageHandler(const ServiceMessage *req, ServiceResponseInfo *resp)
{
    NOT_USED(resp);
    CHECK_RETURN_LOGW(req != NULL && req->msg != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    int32_t ret = BleUplinkReportMethod(req->msg);
    if (ret != IOTC_OK) {
        IOTC_LOGW("ble report error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static void RegBleUplink(uint32_t event, void *param, uint32_t len)
{
    NOT_USED(event);
    NOT_USED(param);
    NOT_USED(len);

    BleSvcCtx *ctx = GetBleSvcCtx();
    int32_t ret = ServiceProxySubscribeMessage(ctx->instanceId, IOTC_SERVICE_ID_DEVICE,
        DEVICE_SERVICE_MSG_ID_REPORT, DeviceServiceReportMessageHandler);
    if (ret != IOTC_OK) {
        IOTC_LOGE("sub report error %d", ret);
    }
    return;
}

static void UnregBleUplink(uint32_t event, void *param, uint32_t len)
{
    NOT_USED(event);
    NOT_USED(param);
    NOT_USED(len);

    BleSvcCtx *ctx = GetBleSvcCtx();
    int32_t ret = ServiceProxyRemoveSubscribe(ctx->instanceId, IOTC_SERVICE_ID_DEVICE, DEVICE_SERVICE_MSG_ID_REPORT);
    if (ret != IOTC_OK) {
        IOTC_LOGW("remove sub report error %d", ret);
    }
}

int32_t BleSvcReportInit(void)
{
    int32_t ret = EventBusSubscribe(RegBleUplink, IOTC_CORE_BLE_EVENT_GATT_CONNECT);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "sub gatt connect err:%d", ret);

    ret = EventBusSubscribe(UnregBleUplink, IOTC_CORE_BLE_EVENT_GATT_DISCONNECT);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "sub gatt disconnect:%d", ret);
    return IOTC_OK;
}

void BleSvcReportDeinit(void)
{
    EventBusUnsubscribe(RegBleUplink);
    EventBusUnsubscribe(UnregBleUplink);
}