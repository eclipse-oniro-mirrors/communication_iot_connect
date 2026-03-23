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
#include "iotc_oh_sle.h"
#include "iotc_errcode.h"
#include "utils_common.h"
#include "utils_combo.h"
#include "sle_profile.h"
#include "sle_linklayer.h"
#include "utils_assert.h"
#include "iotc_conf.h"
#include "sle_adv.h"
#include "sle_adv_ctrl.h"
#include "sle_ssap_mgt.h"
#include "iotc_oh_sle_adv_data.h"
#include "sched_executor.h"
#include "fwk_main.h"
#include "fwk_init.h"
#include "iotc_svc_sle.h"
#include "iotc_svc.h"
#include "netcfg_service.h"
#include "service_proxy.h"
#include "event_bus.h"
#include "core_sle_init.h"
#include "iotc_oh_sdk.h"
#include "iotc_oh_option.h"
#include "product_adapter.h"
#include "sle_common.h"
#include "iotc_event_inner.h"
#include "iotc_svc_dev.h"
#include "securec.h"

static void EventBusStartSleSvcCallback(uint32_t event, void *param, uint32_t len)
{
    NOT_USED(event);
    NOT_USED(param);
    NOT_USED(len);
    if (UtilsGetComboType() == COMBO_TYPE_COMBO &&
        DevSvcProxyGetBindStatus() == DEV_BIND_STATUS_BIND) {
        IOTC_LOGN("binded");
        return;
    }

    SleSvcInitParam svcParam = {0};
#if IOTC_CONF_AILIFE_SUPPORT
    svcParam.advType = IOTC_SLE_ADV_TYPE_NEARBY_HALF_MODAL;
    svcParam.onCustomAdv = NULL;
#else
    svcParam.advType = IOTC_SLE_ADV_TYPE_CUSTOM;
    svcParam.onCustomAdv = IotcOhGetSleAdvData;
#endif

    int32_t ret = ServiceProxyStartService(IOTC_SERVICE_ID_SLE, &svcParam);
    if (ret != IOTC_OK) {
        IOTC_LOGE("start sle service error");
    }
    return;
}

static void CloseSleService(uint32_t event, void *param, uint32_t len)
{
    NOT_USED(event);
    NOT_USED(param);
    NOT_USED(len);
    if (!DevSvcProxyGetOnlineStatus()) {
        return;
    }
    (void)EventBusUnsubscribe(CloseSleService);
    (void)ServiceProxyStopService(IOTC_SERVICE_ID_SLE, NULL);
    IOTC_LOGN("stop sle service success");
}

static int32_t IotcOhSleMainInit(void)
{
    int32_t ret = EventBusSubscribe(EventBusStartSleSvcCallback, IOTC_EVENT_INNER_DEVICE_SVC_START);
    if (ret != IOTC_OK) {
        IOTC_LOGW("sub dev svc error %d", ret);
        return ret;
    }
    ret = EventBusSubscribe(CloseSleService, IOTC_CORE_SLE_EVENT_SSAP_DISCONNECT);
    if (ret != IOTC_OK) {
        IOTC_LOGW("sub online error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static void IotcOhSleMainDeinit(void)
{
    (void)EventBusUnsubscribe(EventBusStartSleSvcCallback);
    (void)ServiceProxyStopService(IOTC_SERVICE_ID_SLE, NULL);
}

static const FwkInitUnit OH_SLE[] = {
    { FWK_INIT_LVL_BIZ, "sle_main", IotcOhSleMainInit, IotcOhSleMainDeinit },
};

static int32_t OptionSetSleRecvNetcfgCallback(va_list args)
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

static int32_t OptionSetSleRecvCustomSecDataCallback(va_list args)
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

static int32_t OptionSetSleStartUpAdvTimeout(va_list args)
{
    uint32_t timeout = va_arg(args, uint32_t);
    SetSleStartUpAdvTimeout(timeout);

    IOTC_LOGN("set start up adv timeout %u", timeout);
    return IOTC_OK;
}

static int32_t OptionSetSleSsapProfileSvcList(va_list args)
{
    const IotcSleSsapProfileSvcList *svcList = va_arg(args, const IotcSleSsapProfileSvcList *);
    CHECK_RETURN_LOGE(svcList != NULL && svcList->svc != NULL && svcList->svcNum != 0,
        IOTC_ERR_PARAM_INVALID, "param invalid");
    int32_t ret;
    for (uint32_t i = 0; i < svcList->svcNum; ++i) {
        ret = SleAddSsapSvc(svcList->svc + i);
        if (ret != IOTC_OK) {
            IOTC_LOGW("add gatt svc error %d %u", ret, i);
            return ret;
        }
    }

    IOTC_LOGN("add gatt svc list %u", svcList->svcNum);
    return IOTC_OK;
}

static const OptionItem SLE_OPTION_TABLE[] = {
    { IOTC_OH_OPTION_SLE_EXIT_AFTER_NETCFG, NULL },
    { IOTC_OH_OPTION_SLE_RECV_NETCFG_CALLBACK, OptionSetSleRecvNetcfgCallback },
    { IOTC_OH_OPTION_SLE_RECV_CUSTOM_DATA_CALLBACK, OptionSetSleRecvCustomSecDataCallback },
    { IOTC_OH_OPTION_SLE_START_UP_ADV_TIMEOUT, OptionSetSleStartUpAdvTimeout },
    { IOTC_OH_OPTION_SLE_GATT_PROFILE_SVC_LIST, OptionSetSleSsapProfileSvcList },
};

int32_t IotcOhSleEnable(uint8_t mode)
{
    IOTC_LOGN("iotc oh sle enable");
    CHECK_MAIN_RUNNING_RETURN();

    int32_t ret = IotcOhOptionRegister(SLE_OPTION_TABLE, ARRAY_SIZE(SLE_OPTION_TABLE));
    if (ret != IOTC_OK) {
        IOTC_LOGE("reg sle option error %d", ret);
        return ret;
    }

    ret = SleCoreRegisterInitUnit();
    if (ret != IOTC_OK) {
        IOTC_LOGE("reg sle core unit error %d", ret);
        return ret;
    }

    ret = FwkRegInitUnits(OH_SLE, ARRAY_SIZE(OH_SLE), TO_STR(OH_SLE));
    if (ret != IOTC_OK) {
        IOTC_LOGE("reg oh sle init unit error %d", ret);
        return ret;
    }

    UtilsComboSetSleFlag(true);
    return IOTC_OK;
}

int32_t IotcOhSleDisable(void)
{
    IOTC_LOGN("iotc oh sle disable");
    CHECK_MAIN_RUNNING_RETURN();
    SleCoreUnregisterInitUnit();
    FwkUnregInitUnit(OH_SLE);
    IotcOhOptionUnregister(SLE_OPTION_TABLE);
    UtilsComboSetSleFlag(false);
    return IOTC_OK;
}

static int32_t StopSleServiceExecutor(void *inData, void **outData)
{
    NOT_USED(inData);
    NOT_USED(outData);

    int32_t ret = ServiceProxyStopService(IOTC_SERVICE_ID_SLE, NULL);
    if (ret != IOTC_OK) {
        IOTC_LOGW("stop sle service error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

int32_t IotcOhSleRelease(void)
{
    int32_t errcode = IOTC_OK;
    int32_t ret = SchedAsyncExecutorWait(StopSleServiceExecutor, NULL, NULL, &errcode, IOTC_CONF_API_WAIT_MAX_TIME);
    if (ret != IOTC_OK) {
        IOTC_LOGF("add executor error %d", ret);
        return ret;
    }
    if (errcode != IOTC_OK) {
        IOTC_LOGF("stop sle svc error %d", errcode);
        return errcode;
    }
    return IOTC_OK;
}

static int32_t StartAdvExecutorCallback(void *inData, void **outData)
{
    NOT_USED(outData);
    CHECK_RETURN_LOGE(inData != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    uint32_t ms = *((uint32_t *)inData);

    return SleSvcProxyStartAdv(ms);
}

int32_t IotcOhSleStartAdv(uint32_t ms)
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

    return SleSvcProxyStopAdv();
}

int32_t IotcOhSleStopAdv(void)
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
} OhSleSendIndicateParam;

static int32_t SendIndicateExecutorCallback(void *inData, void **outData)
{
    NOT_USED(outData);
    CHECK_RETURN_LOGE(inData != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    OhSleSendIndicateParam *param = (OhSleSendIndicateParam *)inData;

    return SleSvcProxySendIndicateData(param->svcUuid, param->charUuid, param->value, param->valueLen);
}

int32_t IotcOhSleSendIndicateData(const char *svcUuid, const char *charUuid,
    const uint8_t *value, uint32_t valueLen)
{
    CHECK_RETURN_LOGW((svcUuid != NULL) && (charUuid != NULL) &&  (value != NULL) && (valueLen != 0),
        IOTC_ERR_PARAM_INVALID, "invalid param");
    int32_t errcode = IOTC_OK;
    OhSleSendIndicateParam param = {svcUuid, charUuid, value, valueLen};
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

#define SLE_DEVICE_ID_MAX_STR_LEN 40
typedef struct {
    char devId[SLE_DEVICE_ID_MAX_STR_LEN + 1];
    uint8_t protType;
    const uint8_t *data;
    uint32_t len;
} OhSleCustomDataParam;

static int32_t ReportSvcExecutorCallback(void *inData, void **outData)
{
    NOT_USED(outData);
    CHECK_RETURN_LOGE(inData != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    OhSleCustomDataParam *param = (OhSleCustomDataParam *)inData;

    return SleSvcProxySendCustomSecData(param->devId, param->protType, param->data, param->len);
}

int32_t IotcOhSleSendCustomSecData(const char *devId, uint8_t portType, uint8_t *data, uint32_t len)
{
    CHECK_RETURN_LOGE(data != NULL && len != 0, IOTC_ERR_PARAM_INVALID, "param invalid");

    int32_t errcode = IOTC_OK;
    OhSleCustomDataParam param = {};
    if (memcpy_s(param.devId, SLE_DEVICE_ID_MAX_STR_LEN+1, devId, SLE_DEVICE_ID_MAX_STR_LEN + 1) != IOTC_OK) {
        IOTC_LOGE(" sle ext devId cpy fail");
        return -1;
    }
    param.protType = portType;
    param.data = data;
    param.len = len;

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

static int32_t FindDeviceInfoCallback(void* inData, void** outData)
{
    const char* devId = (const char*)inData;
    return SleSvcProxyFindDeviceInfo(devId, outData);
}

typedef struct {
    char devId[SLE_DEVICE_ID_MAX_STR_LEN + 1];
    void *info;
} OhSleFindDeviceInfoParam;

int32_t IotcOhFindDeviceInfo(const char *devId, void **info)
{
    CHECK_RETURN_LOGE(devId && info, IOTC_ERR_PARAM_INVALID, "null pointer");
    CHECK_RETURN_LOGE(*info == NULL, IOTC_ERR_PARAM_INVALID, "info not null initialized");

    int32_t errcode = IOTC_OK;
    int32_t ret = SchedAsyncExecutorWait(
        FindDeviceInfoCallback,
        (void*)devId,
        info,
        &errcode,
        IOTC_CONF_API_WAIT_MAX_TIME);
    if (ret != IOTC_OK) {
        IOTC_LOGF("executor find device info error %d", ret);
        return ret;
    }
    return IOTC_OK;
}