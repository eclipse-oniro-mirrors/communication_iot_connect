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
#include "iotc_oh_device.h"
#include <string.h>
#include "dev_info.h"
#include "svc_info.h"
#include "utils_assert.h"
#include "core_prof_init.h"
#include "profile_report.h"
#include "utils_common.h"
#include "sched_executor.h"
#include "fwk_main.h"
#include "iotc_svc_dev.h"
#include "iotc_errcode.h"
#include "iotc_oh_option.h"
#include "product_adapter.h"
#include "dev_info.h"
#include "svc_info.h"
#include "fwk_init.h"
#include "iotc_svc.h"
#include "iotc_event.h"
#include "service_proxy.h"
#include "event_bus.h"

#define SET_PRODUCT_CALLBACK_RETURN(cb, member) \
    do { \
        ProductHooks hooks = {0}; \
        hooks.member = (cb); \
        int32_t ret = ProductRegisterHooks(&hooks, PROD_HOOK_REG_POLICY_COVER_NON_NULL); \
        if (ret != IOTC_OK) { \
            IOTC_LOGW(#member "set error %d", ret); \
            return ret; \
        } \
    } while (0)

static int32_t OptionSetDevPutCharCallback(va_list args)
{
    IotcDevProfPutCharState cb = va_arg(args, IotcDevProfPutCharState);
    CHECK_RETURN_LOGE(cb != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    SET_PRODUCT_CALLBACK_RETURN(cb, onProfPutCharState);
    IOTC_LOGN("set put char cb");
    return IOTC_OK;
}

static int32_t OptionSetDevGetCharCallback(va_list args)
{
    IotcDevProfGetCharState cb = va_arg(args, IotcDevProfGetCharState);
    CHECK_RETURN_LOGE(cb != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    SET_PRODUCT_CALLBACK_RETURN(cb, onProfGetCharState);
    IOTC_LOGN("set get char cb");
    return IOTC_OK;
}

static int32_t OptionSetDevReportAllCallback(va_list args)
{
    IotcDevProfReportAll cb = va_arg(args, IotcDevProfReportAll);
    CHECK_RETURN_LOGE(cb != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    SET_PRODUCT_CALLBACK_RETURN(cb, onProfReportAll);
    IOTC_LOGN("set report all cb");
    return IOTC_OK;
}

static int32_t OptionSetDevGetPincodeCallback(va_list args)
{
    IotcDevProfGetPincode cb = va_arg(args, IotcDevProfGetPincode);
    CHECK_RETURN_LOGE(cb != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    SET_PRODUCT_CALLBACK_RETURN(cb, onProfGetPincode);
    IOTC_LOGN("set get pin cb");
    return IOTC_OK;
}

static int32_t OptionSetDevGetAcKeyCallback(va_list args)
{
    IotcDevProfGetAcKey cb = va_arg(args, IotcDevProfGetAcKey);
    CHECK_RETURN_LOGE(cb != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    SET_PRODUCT_CALLBACK_RETURN(cb, onProfGetAcKey);
    IOTC_LOGN("set get ac cb");
    return IOTC_OK;
}

static int32_t OptionSetDevDataFreeCallback(va_list args)
{
    IotcDevProfFree cb = va_arg(args, IotcDevProfFree);
    CHECK_RETURN_LOGE(cb != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    SET_PRODUCT_CALLBACK_RETURN(cb, onProfFree);
    IOTC_LOGN("set get ac cb");
    return IOTC_OK;
}

static int32_t OptionSetDevRebootCallback(va_list args)
{
    IotcDevReboot cb = va_arg(args, IotcDevReboot);
    CHECK_RETURN_LOGE(cb != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    SET_PRODUCT_CALLBACK_RETURN(cb, onDevReboot);
    IOTC_LOGN("set reboot cb");
    return IOTC_OK;
}

static int32_t OptionSetDevTrngCallback(va_list args)
{
    IotcDevTrng cb = va_arg(args, IotcDevTrng);
    CHECK_RETURN_LOGE(cb != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    SET_PRODUCT_CALLBACK_RETURN(cb, onDevTrng);
    IOTC_LOGN("set trng cb");
    return IOTC_OK;
}

static bool CheckDevInfo(const IotcDeviceInfo *devInfo)
{
    UtilsStrCheckItem devInfoCheck[] = {
        {devInfo->sn, UTILS_MIN_STR_LEN, IOTC_OH_SN_STR_MAX_LEN},
#if IOTC_CONF_AILIFE_SUPPORT
        {devInfo->prodId, IOTC_OH_PRO_ID_STR_LEN - 1, IOTC_OH_PRO_ID_STR_LEN - 1},
        {devInfo->devTypeId, IOTC_OH_DEV_TYPE_ID_STR_LEN - 1, IOTC_OH_DEV_TYPE_ID_STR_LEN - 1},
#else
        {devInfo->prodId, IOTC_OH_PRO_ID_STR_LEN, IOTC_OH_PRO_ID_STR_LEN},
        {devInfo->devTypeId, IOTC_OH_DEV_TYPE_ID_STR_LEN, IOTC_OH_DEV_TYPE_ID_STR_LEN},
#endif
        {devInfo->model, UTILS_MIN_STR_LEN, IOTC_OH_MODEL_STR_MAX_LEN},
        {devInfo->devTypeName, UTILS_MIN_STR_LEN, IOTC_OH_DEV_TYPE_NAME_STR_MAX_LEN},
        {devInfo->manuId, IOTC_OH_MANU_ID_STR_LEN, IOTC_OH_MANU_ID_STR_LEN},
        {devInfo->manuName, UTILS_MIN_STR_LEN, IOTC_OH_MANU_NAME_STR_MAX_LEN},
        {devInfo->fwv, UTILS_MIN_STR_LEN, IOTC_OH_FIRMWARE_VER_STR_MAX_LEN},
        {devInfo->hwv, UTILS_MIN_STR_LEN, IOTC_OH_FIRMWARE_VER_STR_MAX_LEN},
        {devInfo->swv, UTILS_MIN_STR_LEN, IOTC_OH_FIRMWARE_VER_STR_MAX_LEN},
    };

    if (!UtilsIsValidStrArrayCheck(devInfoCheck, ARRAY_SIZE(devInfoCheck))) {
        IOTC_LOGW("dev info invalid");
        return false;
    }

    if (devInfo->subProdId != NULL &&
        !(strlen(devInfo->subProdId) == 0 || strlen(devInfo->subProdId) == IOTC_OH_SUB_PRO_ID_STR_LEN)) {
        IOTC_LOGW("sub pro id invalid");
        return false;
    }
    return true;
}

static bool CheckSvcInfo(const IotcServiceInfo *svcInfo, uint32_t num)
{
    for (uint32_t i = 0; i < num; i++) {
        CHECK_RETURN_LOGE(UtilsIsValidStr(svcInfo[i].svcId, UTILS_MIN_STR_LEN, IOTC_OH_SVC_ID_STR_MAX_LEN),
            false, "sid %s too long", svcInfo[i].svcId);
        CHECK_RETURN_LOGE(UtilsIsValidStr(svcInfo[i].svcType, UTILS_MIN_STR_LEN, IOTC_OH_SVC_TYPE_STR_MAX_LEN),
            false, "st %s too long", svcInfo[i].svcType);
    }
    return true;
}

static int32_t OptionSetDevInfo(va_list args)
{
    const IotcDeviceInfo *devInfo = va_arg(args, const IotcDeviceInfo *);
    CHECK_RETURN_LOGE(devInfo != NULL && CheckDevInfo(devInfo), IOTC_ERR_PARAM_INVALID, "param invalid");
    
    int32_t ret = ModelDevInfoInit(devInfo);
    if (ret != IOTC_OK) {
        IOTC_LOGE("dev info init error %d", ret);
    }
    return ret;
}

static int32_t OptionSetSvcInfo(va_list args)
{
    const IotcServiceInfo *svcInfo = va_arg(args, const IotcServiceInfo *);
    uint32_t num = va_arg(args, uint32_t);
    CHECK_RETURN_LOGE(svcInfo != NULL && num != 0 && CheckSvcInfo(svcInfo, num),
        IOTC_ERR_PARAM_INVALID, "param invalid");
    
    int32_t ret = ModelSvcInfoInit(svcInfo, num);
    if (ret != IOTC_OK) {
        IOTC_LOGE("svc info init error %d", ret);
    }
    return ret;
}

static const OptionItem DEVICE_OPTION_TABLE[] = {
    { IOTC_OH_OPTION_DEVICE_PUT_CHAR_STATE_CALLBACK, OptionSetDevPutCharCallback },
    { IOTC_OH_OPTION_DEVICE_GET_CHAR_STATE_CALLBACK, OptionSetDevGetCharCallback },
    { IOTC_OH_OPTION_DEVICE_REPORT_ALL_CALLBACK, OptionSetDevReportAllCallback },
    { IOTC_OH_OPTION_DEVICE_GET_PINCODE_CALLBACK, OptionSetDevGetPincodeCallback },
    { IOTC_OH_OPTION_DEVICE_GET_AC_KEY_CALLBACK, OptionSetDevGetAcKeyCallback },
    { IOTC_OH_OPTION_DEVICE_DATA_FREE_CALLBACK, OptionSetDevDataFreeCallback },
    { IOTC_OH_OPTION_DEVICE_REBOOT_CALLBACK, OptionSetDevRebootCallback },
    { IOTC_OH_OPTION_DEVICE_TRNG_CALLBACK, OptionSetDevTrngCallback },
    { IOTC_OH_OPTION_DEVICE_DEV_INFO, OptionSetDevInfo },
    { IOTC_OH_OPTION_DEVICE_SVC_INFO, OptionSetSvcInfo },
};

static void EventBusStartDevSvcCallback(uint32_t event, void *param, uint32_t len)
{
    NOT_USED(event);
    NOT_USED(param);
    NOT_USED(len);

    int32_t ret = ServiceProxyStartService(IOTC_SERVICE_ID_DEVICE, &param);
    if (ret != IOTC_OK) {
        IOTC_LOGE("start device service error %d", ret);
    }
    return;
}

static int32_t IotcOhDevMainInit(void)
{
    int32_t ret = EventBusSubscribe(EventBusStartDevSvcCallback, IOTC_CORE_COMM_EVENT_MAIN_INITIALIZED);
    if (ret != IOTC_OK) {
        IOTC_LOGW("sub event error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static void IotcOhDevMainDeinit(void)
{
    (void)EventBusUnsubscribe(EventBusStartDevSvcCallback);
    (void)ServiceProxyStopService(IOTC_SERVICE_ID_DEVICE, NULL);
}

static const FwkInitUnit OH_DEVICE_INIT[] = {
    { FWK_INIT_LVL_BIZ, "dev_main", IotcOhDevMainInit, IotcOhDevMainDeinit },
};

int32_t IotcOhDevInit(void)
{
    int32_t ret = IotcOhOptionRegister(DEVICE_OPTION_TABLE, ARRAY_SIZE(DEVICE_OPTION_TABLE));
    if (ret != IOTC_OK) {
        IOTC_LOGW("set dev option error %d", ret);
        return ret;
    }

    ret = FwkRegInitUnitArray(OH_DEVICE_INIT);
    if (ret != IOTC_OK) {
        IOTC_LOGW("reg dev init unit error %d", ret);
        return ret;
    }

    ret = CoreDeviceRegisterInitUnit();
    if (ret != IOTC_OK) {
        IOTC_LOGW("reg prf init error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

int32_t IotcOhDevDeinit(void)
{
    CHECK_MAIN_RUNNING_RETURN();
    CoreDeviceUnregisterInitUnit();
    FwkUnregInitUnit(OH_DEVICE_INIT);
    IotcOhOptionUnregister(DEVICE_OPTION_TABLE);
    ModelDevInfoDeinit();
    ModelSvcInfoDeinit();
    return IOTC_OK;
}

typedef struct {
    const IotcCharState *states;
    uint32_t num;
} ReportExecutorParam;

static int32_t ReportExecutorWaitCallback(void *inData, void **outData)
{
    NOT_USED(outData);
    CHECK_RETURN_LOGE(inData != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    const ReportExecutorParam *param = (const ReportExecutorParam *)inData;

    int32_t ret = DevSvcProxyProfReportCharState(param->states, param->num);
    if (ret != IOTC_OK) {
        IOTC_LOGW("report char state executor error %d/%u", ret, param->num);
    }
    return ret;
}

int32_t IotcOhDevReportCharState(const IotcCharState state[], uint32_t num)
{
    CHECK_RETURN_LOGE(state != NULL && num != 0, IOTC_ERR_PARAM_INVALID, "param invalid");
    IOTC_LOGI("report char state %u", num);

    int32_t errcode;
    ReportExecutorParam param = {state, num};
    int32_t ret = SchedAsyncExecutorWait(ReportExecutorWaitCallback, &param, NULL,
        &errcode, IOTC_CONF_API_WAIT_MAX_TIME);
    if (ret != IOTC_OK) {
        IOTC_LOGE("async executor error %d", ret);
        return ret;
    }
    IOTC_LOGI("report char state finish %d", errcode);
    return errcode;
}