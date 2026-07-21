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
#include <stddef.h>
#include "ble_svc.h"
#include "iotc_svc.h"
#include "iotc_svc_ble.h"
#include "service_proxy.h"
#include "iotc_errcode.h"
#include "ble_svc_ctx.h"
#include "ble_adv.h"
#include "securec.h"
#include "ble_gatt_data_svc.h"
#include "ble_profile.h"
#include "ble_session_mngr.h"
#include "utils_common.h"
#include "ble_svc_report.h"
#include "utils_assert.h"
#include "ble_gatt_mgt.h"
#include "iotc_ble.h"
#include "ble_adv_ctrl.h"
#include "ble_linklayer.h"
#include "ble_common.h"
#include "ble_svc_netcfg_status.h"
#include "event_bus.h"
#include "iotc_event.h"

static const char *BLE_SERVICE_NAME = "BLE";

static void BleStackDeinit(void)
{
    BleGattDisconnectAll();
    BleGattMgtDestroy();
    int32_t ret = IotcBleDeInitStack();
    if (ret != IOTC_OK) {
        IOTC_LOGW("stack deinit error %d", ret);
    }
}

static int32_t BleStackInit(void)
{
    int32_t ret = SetBleConnectParam();
    if (ret != IOTC_OK) {
        IOTC_LOGE("set connect param error %d", ret);
        return ret;
    }
    ret = IotcBleInitStack();
    if (ret != IOTC_OK) {
        IOTC_LOGE("init stack error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static int32_t BleGattInit(void)
{
    int32_t ret = BleGattDataSvcInit();
    if (ret != IOTC_OK) {
        IOTC_LOGE("ble gatt svc init err ret=%d", ret);
        return ret;
    }
    ret = BleGattMgtInit();
    if (ret != IOTC_OK) {
        IOTC_LOGE("start gatt service err ret=%d", ret);
        return ret;
    }
    return IOTC_OK;
}

static void BleServiceStopInner(void)
{
#ifdef IOTC_CONNECT_BLE_NET_CONFIG_SUPPORT
    BleSvcNetCfgDeinit();
#endif
    BleSvcReportDeinit();
    BleSessReset();
    BleProfileDeinit();
    BleStackDeinit();
    LinkLayerClearAllCachePkg();
    BleSvcCtx *ctx = GetBleSvcCtx();
    if (ctx->onFinish != NULL) {
        ctx->onFinish(ctx->instanceId);
    }
}

static int32_t BleAdvSvcInit(BleSvcCtx *ctx)
{
    int32_t ret = BleAdvInit();
    if (ret != IOTC_OK) {
        IOTC_LOGW("adv init error %d", ret);
        return ret;
    }

    BleSetAdvType(ctx->initParam.advType);

    if (ctx->initParam.onCustomAdv != NULL) {
        ret = RegCustomAdvDataCb(ctx->initParam.onCustomAdv);
        if (ret != IOTC_OK) {
            IOTC_LOGW("reg custom adv cb error %d", ret);
            return ret;
        }
    }

    ret = BleAdvCtrlStart(GetBleStartUpAdvTimeout());
    if (ret != IOTC_OK) {
        IOTC_LOGW("start ble adv error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static int32_t BleServiceInit(BleSvcCtx *ctx)
{
    int32_t ret = BleStackInit();
    if (ret != IOTC_OK) {
        IOTC_LOGW("ble stack init error %d", ret);
        return ret;
    }

    ret = BleScheduleEventInit();
    if (ret != IOTC_OK) {
        IOTC_LOGW("ble event init error %d", ret);
        return ret;
    }

    ret = BleGattInit();
    if (ret != IOTC_OK) {
        IOTC_LOGW("gatt init error %d", ret);
        return ret;
    }

    ret = BleAdvSvcInit(ctx);
    if (ret != IOTC_OK) {
        IOTC_LOGW("adv init error %d", ret);
        return ret;
    }

    ret = BleProfileInit();
    if (ret != IOTC_OK) {
        IOTC_LOGW("profile init error %d", ret);
        return ret;
    }

    ret = BleSessInit();
    if (ret != IOTC_OK) {
        IOTC_LOGW("profile init error %d", ret);
        return ret;
    }

    ret = BleSvcReportInit();
    if (ret != IOTC_OK) {
        IOTC_LOGW("report init error %d", ret);
        return ret;
    }

#ifdef IOTC_CONNECT_BLE_NET_CONFIG_SUPPORT
    ret = BleSvcNetCfgInit();
    if (ret != IOTC_OK) {
        IOTC_LOGW("report status init error %d", ret);
        return ret;
    }
#endif
    return IOTC_OK;
}

static int32_t BleServiceStart(int32_t instanceId, ServiceFinishCallback onFinish, void *param)
{
    CHECK_RETURN_LOGW(onFinish != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    int32_t ret = BleSvcCtxInit();
    if (ret != IOTC_OK) {
        IOTC_LOGW("ctx init error %d", ret);
        return ret;
    }

    BleSvcCtx *ctx = GetBleSvcCtx();
    ctx->instanceId = instanceId;
    ctx->onFinish = onFinish;

    const BleSvcInitParam *initParam = param;
    ret = memcpy_s(&ctx->initParam, sizeof(BleSvcInitParam), initParam, sizeof(BleSvcInitParam));
    if (ret != EOK) {
        return IOTC_ERR_SECUREC_MEMCPY;
    }

    ret = BleServiceInit(ctx);
    if (ret != IOTC_OK) {
        BleServiceStopInner();
        return ret;
    }

    EventBusPublishAsync(IOTC_CORE_COMM_BLE_SVC_START, NULL, 0, NULL);
    return ret;
}

static int32_t BleServiceStop(int32_t instanceId, void *param)
{
    NOT_USED(param);
    BleSvcCtx *ctx = GetBleSvcCtx();
    if (ctx->instanceId != instanceId) {
        IOTC_LOGW("invalid instance id %d/%d", instanceId, ctx->instanceId);
        return IOTC_ERR_PARAM_INVALID;
    }
    BleServiceStopInner();
    (void)BleSvcCtxInit();
    return IOTC_OK;
}

static int32_t SendCustomSecData(const uint8_t *data, uint32_t len)
{
    CHECK_RETURN_LOGW(data != NULL || len != 0, IOTC_ERR_PARAM_INVALID, "param invalid");
    return LinkLayerReportSvcDataEnc(BLE_SVC_CUSTOM_SEC_DATA, data, len);
}

int32_t BleConnectServiceInit(void)
{
    static const ServiceHandler BLE_SERVICE_HANDLER = {
        .onStart = BleServiceStart,
        .onStop = BleServiceStop,
        .onReset = NULL,
    };

    int32_t msgs[BLE_SERVICE_MSG_ID_MAX] = {0};
    for (uint32_t i = 0 ; i < BLE_SERVICE_MSG_ID_MAX; ++i) {
        msgs[i] = i;
    }

    static const BleSvcApi BLE_SERVICE_API = {
        .onStartAdv = BleAdvCtrlStart,
        .onStopAdv = BleAdvCtrlStop,
        .onSetAdvType = BleSetAdvType,
        .onSendCustomSecData = SendCustomSecData,
        .onSendIndicateData = IotcBleSendIndicateData,
    };

    ServiceInstance instance = {
        .serviceId = IOTC_SERVICE_ID_BLE,
        .name = BLE_SERVICE_NAME,
        .handler = &BLE_SERVICE_HANDLER,
        .msgNum = BLE_SERVICE_MSG_ID_MAX,
        .msgIds = msgs,
        .apiHandler = &BLE_SERVICE_API,
    };

    int32_t ret = ServiceManagerRegisterInstance(&instance);
    if (ret != IOTC_OK) {
        IOTC_LOGE("reg device service instance error %d", ret);
        return ret;
    }

    return IOTC_OK;
}

void BleConnectServiceDeinit(void)
{
    BleServiceStopInner();
    ServiceManagerUnregisterInstance(IOTC_SERVICE_ID_BLE);
}
