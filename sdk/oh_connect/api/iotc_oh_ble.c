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
#include "iotc_oh_ble.h"
#include "iotc_errcode.h"
#include "utils_common.h"
#include "utils_combo.h"
#include "ble_profile.h"
#include "ble_linklayer.h"
#include "utils_assert.h"
#include "iotc_conf.h"
#include "ble_adv.h"
#include "ble_adv_ctrl.h"
#include "ble_gatt_mgt.h"
#include "iotc_oh_ble_adv_data.h"
#include "sched_executor.h"
#include "fwk_main.h"
#include "fwk_init.h"
#include "iotc_svc_ble.h"
#include "iotc_svc.h"
#include "netcfg_service.h"
#include "service_proxy.h"
#include "event_bus.h"
#include "core_ble_init.h"
#include "iotc_oh_sdk.h"
#include "iotc_oh_option.h"
#include "product_adapter.h"
#include "ble_common.h"
#include "iotc_event_inner.h"
#include "iotc_svc_dev.h"
#include "ohos_bt_gap.h"
#include <unistd.h>

#define BLE_START_WAIT_SLEEP_S 2

static void EventBusStartBleSvcCallback(uint32_t event, void *param, uint32_t len)
{
    NOT_USED(event);
    NOT_USED(param);
    NOT_USED(len);
    if (UtilsGetComboType() == COMBO_TYPE_COMBO &&
        DevSvcProxyGetBindStatus() == DEV_BIND_STATUS_BIND) {
        IOTC_LOGN("binded");
        return;
    }

    BleSvcInitParam svcParam = {0};
#if IOTC_CONF_AILIFE_SUPPORT
    svcParam.advType = IOTC_BLE_ADV_TYPE_NEARBY_HALF_MODAL;
    svcParam.onCustomAdv = NULL;
#else
    svcParam.advType = IOTC_BLE_ADV_TYPE_CUSTOM;
    svcParam.onCustomAdv = IotcOhGetBleAdvData;
#endif

    int32_t ret = ServiceProxyStartService(IOTC_SERVICE_ID_BLE, &svcParam);
    if (ret != IOTC_OK) {
        IOTC_LOGE("start ble service error");
    }
    return;
}

static void CloseBleService(uint32_t event, void *param, uint32_t len)
{
    NOT_USED(event);
    NOT_USED(param);
    NOT_USED(len);
    if (!DevSvcProxyGetOnlineStatus()) {
        return;
    }
    (void)EventBusUnsubscribe(CloseBleService);
    (void)ServiceProxyStopService(IOTC_SERVICE_ID_BLE, NULL);
    IOTC_LOGN("stop ble service success");
}

static int32_t IotcOhBleMainInit(void)
{
    int32_t ret = EventBusSubscribe(EventBusStartBleSvcCallback, IOTC_EVENT_INNER_DEVICE_SVC_START);
    if (ret != IOTC_OK) {
        IOTC_LOGW("sub dev svc error %d", ret);
        return ret;
    }
    ret = EventBusSubscribe(CloseBleService, IOTC_CORE_BLE_EVENT_GATT_DISCONNECT);
    if (ret != IOTC_OK) {
        IOTC_LOGW("sub online error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static void IotcOhBleMainDeinit(void)
{
    (void)EventBusUnsubscribe(EventBusStartBleSvcCallback);
    (void)ServiceProxyStopService(IOTC_SERVICE_ID_BLE, NULL);
}

static const FwkInitUnit OH_BLE[] = {
    { FWK_INIT_LVL_BIZ, "ble_main", IotcOhBleMainInit, IotcOhBleMainDeinit },
};

static int32_t OptionSetBleRecvNetcfgCallback(va_list args)
{
    IotcRecvNetCfgInfoCallback cb = va_arg(args, IotcRecvNetCfgInfoCallback);
    CHECK_RETURN_LOGE(cb != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    ProductHooks hooks = {0};
    hooks.onRecvNetCfgInfo = cb;
    int32_t ret = ProductRegisterHooks(&hooks, PROD_HOOK_REG_POLICY_COVER_NON_NULL);
    if (ret != IOTC_OK) {
        IOTC_LOGW("set recv net info cb error %d", ret);
        return ret;
    }

    IOTC_LOGN("set recv net info cb");
    return IOTC_OK;
}

static int32_t OptionSetBleRecvCustomSecDataCallback(va_list args)
{
    IotcRecvCustomSecDataCallback cb = va_arg(args, IotcRecvCustomSecDataCallback);
    CHECK_RETURN_LOGE(cb != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    ProductHooks hooks = {0};
    hooks.onRecvCustomSecData = cb;
    int32_t ret = ProductRegisterHooks(&hooks, PROD_HOOK_REG_POLICY_COVER_NON_NULL);
    if (ret != IOTC_OK) {
        IOTC_LOGW("set custom data cb error %d", ret);
        return ret;
    }

    IOTC_LOGN("set custom data cb");
    return IOTC_OK;
}

static int32_t OptionSetBleStartUpAdvTimeout(va_list args)
{
    uint32_t timeout = va_arg(args, uint32_t);
    SetBleStartUpAdvTimeout(timeout);

    IOTC_LOGN("set start up adv timeout %u", timeout);
    return IOTC_OK;
}

static int32_t OptionSetBleGattProfileSvcList(va_list args)
{
    const IotcBleGattProfileSvcList *svcList = va_arg(args, const IotcBleGattProfileSvcList *);
    CHECK_RETURN_LOGE(svcList != NULL && svcList->svc != NULL && svcList->svcNum != 0,
        IOTC_ERR_PARAM_INVALID, "param invalid");
    int32_t ret;
    for (uint32_t i = 0; i < svcList->svcNum; ++i) {
        ret = BleAddGattSvc(svcList->svc + i);
        if (ret != IOTC_OK) {
            IOTC_LOGW("add gatt svc error %d %u", ret, i);
            return ret;
        }
    }

    IOTC_LOGN("add gatt svc list %u", svcList->svcNum);
    return IOTC_OK;
}

static const OptionItem BLE_OPTION_TABLE[] = {
    { IOTC_OH_OPTION_BLE_EXIT_AFTER_NETCFG, NULL },
    { IOTC_OH_OPTION_BLE_RECV_NETCFG_CALLBACK, OptionSetBleRecvNetcfgCallback },
    { IOTC_OH_OPTION_BLE_RECV_CUSTOM_DATA_CALLBACK, OptionSetBleRecvCustomSecDataCallback },
    { IOTC_OH_OPTION_BLE_START_UP_ADV_TIMEOUT, OptionSetBleStartUpAdvTimeout },
    { IOTC_OH_OPTION_BLE_GATT_PROFILE_SVC_LIST, OptionSetBleGattProfileSvcList },
};

int32_t IotcOhBleEnable(void)
{
    IOTC_LOGN("iotc oh ble enable");
    CHECK_MAIN_RUNNING_RETURN();

    if (IsBleEnabled() == false) {
        if (EnableBle() == false) {
            IOTC_LOGE("enable ble fail");
        } else {
            IOTC_LOGI("enable ble ok");
        }
    }
    /* 此处加延时为了使后面SetLocalName可以成功执行 */
    sleep(BLE_START_WAIT_SLEEP_S);
    
    int32_t ret = IotcOhOptionRegister(BLE_OPTION_TABLE, ARRAY_SIZE(BLE_OPTION_TABLE));
    if (ret != IOTC_OK) {
        IOTC_LOGE("reg ble option error %d", ret);
        return ret;
    }

    ret = BleCoreRegisterInitUnit();
    if (ret != IOTC_OK) {
        IOTC_LOGE("reg ble core unit error %d", ret);
        return ret;
    }

    ret = FwkRegInitUnits(OH_BLE, ARRAY_SIZE(OH_BLE), TO_STR(OH_BLE));
    if (ret != IOTC_OK) {
        IOTC_LOGE("reg oh ble init unit error %d", ret);
        return ret;
    }

    UtilsComboSetBleFlag(true);
    return IOTC_OK;
}

int32_t IotcOhBleDisable(void)
{
    IOTC_LOGN("iotc oh ble disable");
    CHECK_MAIN_RUNNING_RETURN();
    BleCoreUnregisterInitUnit();
    FwkUnregInitUnit(OH_BLE);
    IotcOhOptionUnregister(BLE_OPTION_TABLE);
    UtilsComboSetBleFlag(false);
    return IOTC_OK;
}

static int32_t StopBleServiceExecutor(void *inData, void **outData)
{
    NOT_USED(inData);
    NOT_USED(outData);
    
    int32_t ret = ServiceProxyStopService(IOTC_SERVICE_ID_BLE, NULL);
    if (ret != IOTC_OK) {
        IOTC_LOGW("stop ble service error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

int32_t IotcOhBleRelease(void)
{
    int32_t errcode = IOTC_OK;
    int32_t ret = SchedAsyncExecutorWait(StopBleServiceExecutor, NULL, NULL, &errcode, IOTC_CONF_API_WAIT_MAX_TIME);
    if (ret != IOTC_OK) {
        IOTC_LOGF("add executor error %d", ret);
        return ret;
    }
    if (errcode != IOTC_OK) {
        IOTC_LOGF("stop ble svc error %d", errcode);
        return errcode;
    }
    return IOTC_OK;
}

static int32_t StartAdvExecutorCallback(void *inData, void **outData)
{
    NOT_USED(outData);
    CHECK_RETURN_LOGE(inData != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    uint32_t ms = *((uint32_t *)inData);

    return BleSvcProxyStartAdv(ms);
}

int32_t IotcOhBleStartAdv(uint32_t ms)
{
    int32_t errcode = IOTC_OK;
    uint32_t time = ms;
    int32_t ret = SchedAsyncExecutorWait(StartAdvExecutorCallback, &time, NULL, &errcode, IOTC_CONF_API_WAIT_MAX_TIME);
    if (ret != IOTC_OK) {
        IOTC_LOGF("executor start adv error %d", ret);
        return ret;
    }
    if (errcode != IOTC_OK) {
        IOTC_LOGF("start adv error %d", errcode);
        return errcode;
    }
    return IOTC_OK;
}

static int32_t StopAdvExecutorCallback(void *inData, void **outData)
{
    NOT_USED(inData);
    NOT_USED(outData);

    return BleSvcProxyStopAdv();
}

int32_t IotcOhBleStopAdv(void)
{
    int32_t errcode = IOTC_OK;
    int32_t ret = SchedAsyncExecutorWait(StopAdvExecutorCallback, NULL, NULL, &errcode, IOTC_CONF_API_WAIT_MAX_TIME);
    if (ret != IOTC_OK) {
        IOTC_LOGF("executor stop adv error %d", ret);
        return ret;
    }
    if (errcode != IOTC_OK) {
        IOTC_LOGF("stop adv error %d", errcode);
        return errcode;
    }
    return IOTC_OK;
}

typedef struct {
    const char *svcUuid;
    const char *charUuid;
    const uint8_t *value;
    uint32_t valueLen;
} OhBleSendIndicateParam;

static int32_t SendIndicateExecutorCallback(void *inData, void **outData)
{
    NOT_USED(outData);
    CHECK_RETURN_LOGE(inData != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    OhBleSendIndicateParam *param = (OhBleSendIndicateParam *)inData;

    return BleSvcProxySendIndicateData(param->svcUuid, param->charUuid, param->value, param->valueLen);
}

int32_t IotcOhBleSendIndicateData(const char *svcUuid, const char *charUuid,
    const uint8_t *value, uint32_t valueLen)
{
    CHECK_RETURN_LOGW((svcUuid != NULL) && (charUuid != NULL) &&  (value != NULL) && (valueLen != 0),
        IOTC_ERR_PARAM_INVALID, "invalid param");
    int32_t errcode = IOTC_OK;
    OhBleSendIndicateParam param = {svcUuid, charUuid, value, valueLen};
    int32_t ret = SchedAsyncExecutorWait(SendIndicateExecutorCallback, &param, NULL,
        &errcode, IOTC_CONF_API_WAIT_MAX_TIME);
    if (ret != IOTC_OK) {
        IOTC_LOGF("executor send ind error %d", ret);
        return ret;
    }
    if (errcode != IOTC_OK) {
        IOTC_LOGF("send ind error %d", errcode);
        return errcode;
    }
    return IOTC_OK;
}

typedef struct {
    const uint8_t *data;
    uint32_t len;
} OhBleCustomDataParam;

static int32_t ReportSvcExecutorCallback(void *inData, void **outData)
{
    NOT_USED(outData);
    CHECK_RETURN_LOGE(inData != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    OhBleCustomDataParam *param = (OhBleCustomDataParam *)inData;

    return BleSvcProxySendCustomSecData(param->data, param->len);
}

int32_t IotcOhBleSendCustomSecData(const uint8_t *data, uint32_t len)
{
    CHECK_RETURN_LOGE(data != NULL && len != 0, IOTC_ERR_PARAM_INVALID, "param invalid");

    int32_t errcode = IOTC_OK;
    OhBleCustomDataParam param = {data, len};
    int32_t ret = SchedAsyncExecutorWait(ReportSvcExecutorCallback, &param, NULL,
        &errcode, IOTC_CONF_API_WAIT_MAX_TIME);
    if (ret != IOTC_OK) {
        IOTC_LOGF("executor send data error %d", ret);
        return ret;
    }
    if (errcode != IOTC_OK) {
        IOTC_LOGF("send data error %d", errcode);
        return errcode;
    }
    return IOTC_OK;
}