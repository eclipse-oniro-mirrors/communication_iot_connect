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
#include "iotc_oh_sdk.h"
#include <stdarg.h>
#include "core_infrastructure_init.h"
#include "iotc_errcode.h"
#include "event_bus_public.h"
#include "utils_common.h"
#include "iotc_oh_config.h"
#include "utils_assert.h"
#include "fwk_main.h"
#include "sched_executor.h"
#include "iotc_svc_dev.h"
#include "dfx_watch_dog.h"
#include "iotc_oh_option.h"
#include "product_adapter.h"
#include "config_revoke_flag.h"

#define IOTC_OH_RESTORE_TIMEOUT UTILS_SEC_TO_MS(10)

static const FwkInitUnit OH_COMM[] = {
    {FWK_INIT_LVL_BIZ, "config", IotcOhStoreDataInit, IotcOhStoreDataDeinit},
};
static uint32_t g_taskSize = IOTC_CONF_OH_DEFAULT_TASK_SIZE;
static bool g_isCommOptionReg = false;

static int32_t OptionSetLogLevel(va_list args)
{
    uint32_t level = va_arg(args, uint32_t);
    if (level > UINT8_MAX) {
        IOTC_LOGE("invalid log level %u", level);
        return IOTC_ERR_PARAM_INVALID;
    }
    IotcSetLogLevel(level);
    IOTC_LOGN("set log level %u", level);
    return IOTC_OK;
}

static int32_t OptionSetMainTaskSize(va_list args)
{
    uint32_t taskSize = va_arg(args, uint32_t);
    if (taskSize == 0) {
        IOTC_LOGE("set main task size error %u", taskSize);
        return IOTC_ERR_PARAM_INVALID;
    }
    g_taskSize = taskSize;
    IOTC_LOGN("set main task size %u", taskSize);
    return IOTC_OK;
}

static int32_t OptionSetMonitorTaskSize(va_list args)
{
    uint32_t taskSize = va_arg(args, uint32_t);
    if (taskSize == 0) {
        IOTC_LOGE("set monitor task size error %u", taskSize);
        return IOTC_ERR_PARAM_INVALID;
    }
    DfxSetWatchDogTaskSize(taskSize);
    IOTC_LOGN("set monitor task size %u", taskSize);
    return IOTC_OK;
}

static int32_t OptionSetSetConfigPath(va_list args)
{
    const char *path = va_arg(args, const char *);
    CHECK_RETURN_LOGE(path != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    return IotcOhStorePathSet(path);
}

static int32_t OptionSetRegEventListener(va_list args)
{
    IotcOhEventCallback listener = va_arg(args, IotcOhEventCallback);
    CHECK_RETURN_LOGE(listener != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    return IotcRegPublicEventListener(listener);
}

static int32_t OptionSetUnregEventListener(va_list args)
{
    IotcOhEventCallback listener = va_arg(args, IotcOhEventCallback);
    CHECK_RETURN_LOGE(listener != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    return IotcUnregPublicEventListener(listener);
}

static const OptionItem COMM_OPTION_TABLE[] = {
    { IOTC_OH_OPTION_SDK_LOG_LEVEL, OptionSetLogLevel },
    { IOTC_OH_OPTION_SDK_MAIN_TASK_SIZE, OptionSetMainTaskSize },
    { IOTC_OH_OPTION_SDK_MONITOR_TASK_SIZE, OptionSetMonitorTaskSize },
    { IOTC_OH_OPTION_SDK_CONFIG_PATH, OptionSetSetConfigPath },
    { IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, OptionSetRegEventListener },
    { IOTC_OH_OPTION_SDK_UNREG_EVENT_LISTENER, OptionSetUnregEventListener },
};

int32_t IotcOhMain(void)
{
    IOTC_LOGI("iotc main in ----");
    CHECK_MAIN_RUNNING_RETURN();
    IOTC_LOGI("iotc main in ---- OH_COMM");
    int32_t ret = FwkRegInitUnitArray(OH_COMM);
    if (ret != IOTC_OK) {
        IOTC_LOGE("reg init unit error %d", ret);
        return ret;
    }
    IOTC_LOGI("iotc main in ---- core");
    ret = FwkRegInitUnits(CoreInfrastructureGetInitUnit(),
        CoreInfrastructureGetInitUnitSize(), CoreInfrastructureGetInitName());
    if (ret != IOTC_OK) {
        FwkUnregInitUnit(OH_COMM);
        IOTC_LOGE("reg comm init unit error %d", ret);
        return ret;
    }
    IOTC_LOGI("iotc main g_taskSize ----%d", g_taskSize);
    ret = IotcFwkMain(g_taskSize);
    if (ret != IOTC_OK) {
        FwkUnregInitUnit(CoreInfrastructureGetInitUnit());
        FwkUnregInitUnit(OH_COMM);
        IOTC_LOGE("iotc oh main error %d", ret);
        return ret;
    }

    return IOTC_OK;
}

int32_t IotcOhReset(void)
{
    return IotcFwkReset();
}

int32_t IotcOhStop(void)
{
    int32_t ret = IotcFwkStop();
    if (ret != IOTC_OK) {
        IOTC_LOGE("iotc stop error %d", ret);
        return ret;
    }
    FwkUnregInitUnit(OH_COMM);
    FwkUnregInitUnit(CoreInfrastructureGetInitUnit());
    IotcOhOptionUnregister(COMM_OPTION_TABLE);
    g_isCommOptionReg = false;
    return IOTC_OK;
}

static int32_t RestoreExecutorCallback(void *inData, void **outData)
{
    NOT_USED(inData);
    NOT_USED(outData);
    int32_t ret = IotcFwkRestore();
    if (ret != IOTC_OK) {
        IOTC_LOGW("restore error %d", ret);
        return ret;
    }

    return IOTC_OK;
}

int32_t IotcOhRestore(void)
{
    int32_t ret = SetRevokeFlag();
    if (ret != IOTC_OK) {
        IOTC_LOGF("set revoke error %d", ret);
        return ret;
    }

    int32_t errcode;
    ret = SchedAsyncExecutorWait(RestoreExecutorCallback, NULL, NULL, &errcode, IOTC_CONF_API_WAIT_MAX_TIME);
    if (ret != IOTC_OK) {
        IOTC_LOGF("executor restore error %d", ret);
        return ret;
    }
    if (errcode != IOTC_OK) {
        IOTC_LOGF("restore error %d", errcode);
        return errcode;
    }
    return IOTC_OK;
}

int32_t IotcOhSetOption(int32_t option, ...)
{
    CHECK_MAIN_RUNNING_RETURN();

    int32_t ret;
    if (!g_isCommOptionReg) {
        ret = IotcOhOptionRegister(COMM_OPTION_TABLE, ARRAY_SIZE(COMM_OPTION_TABLE));
        if (ret != IOTC_OK) {
            IOTC_LOGW("reg comm option error %d", ret);
            return ret;
        }
        g_isCommOptionReg = true;
    }

    va_list args;
    va_start(args, option);
    ret = IotcOhSetOptionInner(option, args);
    va_end(args);
    if (ret != IOTC_OK) {
        IOTC_LOGE("set option [%d] err [%d]", option, ret);
    }
    return ret;
}