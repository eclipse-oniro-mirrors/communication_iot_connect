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
#include "iotc_dev_svc.h"
#include "service_manager.h"
#include "iotc_svc.h"
#include "iotc_errcode.h"
#include "securec.h"
#include "utils_assert.h"
#include "utils_common.h"
#include "service_proxy.h"
#include "svc_cloud_setup.h"
#include "profile_report.h"
#include "char_state_mdl.h"
#include "iotc_svc_dev.h"
#include "iotc_svc_softap.h"
#include "iotc_svc_ble.h"
#include "device_control.h"
#include "dev_info.h"
#include "svc_info.h"
#include "config_info.h"
#include "config_login_info.h"
#include "config_revoke_flag.h"
#include "config_register_info.h"
#include "config_authinfo.h"
#include "iotc_event_inner.h"
#include "event_bus.h"
#include "dev_svc_auth_setup.h"
#include "config_online_status.h"
#include "dev_mngr_ctx.h"

static const char *DEVICE_SERVICE_NAME = "DEVICE";

static int32_t DeviceReportCallback(const IotcCharState state[], uint32_t num)
{
    CHECK_RETURN_LOGW(state != NULL && num != 0, IOTC_ERR_PARAM_INVALID, "param invalid");

    AdapterJson *array = NULL;
    int32_t ret = MdlCharStatesToJson(state, num, &array);
    CHECK_RETURN_LOGW(ret == IOTC_OK, ret, "build report json array error %d", ret);

    DeviceContext *ctx = GetDeviceManagerContext();
    ServiceRequestInfo req = {
        .instanceId = ctx->instanceId,
        .msgId = DEVICE_SERVICE_MSG_ID_REPORT,
        .req = array,
    };
    ret = ServiceProxySendMessage(&req, NULL, NULL);
    AdapterJsonDelete(array);
    if (ret != IOTC_OK) {
        IOTC_LOGW("send report msg error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static int32_t DeviceServiceStart(int32_t instanceId, ServiceFinishCallback onFinish, void *param)
{
    CHECK_RETURN_LOGW(onFinish != NULL && param != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    DeviceContext *ctx = GetDeviceManagerContext();
    ctx->instanceId = instanceId;
    ctx->onFinish = onFinish;

    int32_t ret = ProfileReportRegUplink(DeviceReportCallback, DEVICE_SERVICE_NAME);
    if (ret != IOTC_OK) {
        IOTC_LOGW("reg device uplink error %d", ret);
        return ret;
    }

    ret = ConfigInfoInit();
    if (ret != IOTC_OK) {
        IOTC_LOGW("config info init %d", ret);
        return ret;
    }

    EventBusPublishSync(IOTC_EVENT_INNER_DEVICE_SVC_START, NULL, 0);
    return IOTC_OK;
}

static int32_t DeviceServiceStop(int32_t instanceId, void *param)
{
    NOT_USED(param);
    DeviceContext *ctx = GetDeviceManagerContext();
    if (ctx->instanceId != instanceId) {
        IOTC_LOGW("invalid id %d/%d", instanceId, ctx->instanceId);
        return IOTC_ERR_PARAM_INVALID;
    }

    ConfigInfoDeinit();
    ProfileReportUnregUplink(DeviceReportCallback);

    if (ctx->onFinish != NULL) {
        ctx->onFinish(instanceId);
    }
    return IOTC_OK;
}

static DevBindStatus GetBindStatus(void)
{
    if (!IsDeviceBinded()) {
        return DEV_BIND_STATUS_NOT_BIND;
    }
    if (IsRevokeFlagExist()) {
        return DEV_BIND_STATUS_REVOKE;
    }
    return DEV_BIND_STATUS_BIND;
}

static int32_t GetRegisterInfo(bool *isExist, DevRegInfo *info)
{
    CHECK_RETURN_LOGW(isExist != NULL && info != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    if (!IsRegisterInfoExist()) {
        *isExist = false;
        return IOTC_OK;
    }
    *isExist = true;
    return ConfigGetRegisterInfo(info);
}

static int32_t GetLoginInfo(bool *isExist, DevLoginInfo *info)
{
    CHECK_RETURN_LOGW(isExist != NULL && info != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    if (!IsDeviceBinded()) {
        *isExist = false;
        return IOTC_OK;
    }
    *isExist = true;
    return ConfigGetLoginInfo(info);
}

static int32_t GetAuthInfo(bool *isExist, DevAuthInfo *info)
{
    CHECK_RETURN_LOGW(isExist != NULL && info != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    if (!IsDeviceBinded()) {
        *isExist = false;
        return IOTC_OK;
    }
    *isExist = true;
    return ConfigGetAuthInfo(info);
}

int32_t DeviceServiceInit(void)
{
    static const ServiceHandler DEVICE_SERVICE_HANDLER = {
        .onStart = DeviceServiceStart,
        .onStop = DeviceServiceStop,
        .onReset = NULL,
    };

    int32_t msgs[DEVICE_SERVICE_MSG_ID_MAX] = {0};
    for (uint32_t i = 0 ; i < DEVICE_SERVICE_MSG_ID_MAX; ++i) {
        msgs[i] = i;
    }

    static const DevSvcApi DEVICE_SERVICE_API = {
        .onReportAll = DeviceControlReportAll,
        .onGetChar = DeviceControlGetCharStates,
        .onPutChar = DeviceControlPutCharStates,
        .onReportChar = ProfileReportCharState,
        .onGetBindStatus = GetBindStatus,
        .onGetLoginInfo = GetLoginInfo,
        .onGetRegInfo = GetRegisterInfo,
        .onGetAuthInfo = GetAuthInfo,
        .onRecvBindInfo = DeviceServiceRecvBindingInfo,
        .onRecvAuthInfo = DeviceServiceRecvAuthInfo,
        .onGetOnlineStatus = ConfigGetOnlineStatus,
    };

    ServiceInstance instance = {
        .serviceId = IOTC_SERVICE_ID_DEVICE,
        .name = DEVICE_SERVICE_NAME,
        .handler = &DEVICE_SERVICE_HANDLER,
        .msgNum = DEVICE_SERVICE_MSG_ID_MAX,
        .msgIds = msgs,
        .apiHandler = &DEVICE_SERVICE_API,
    };

    int32_t ret = ServiceManagerRegisterInstance(&instance);
    if (ret != IOTC_OK) {
        IOTC_LOGE("reg device service instance error %d", ret);
        return ret;
    }
    ret = DeviceManagerContextInit();
    if (ret != IOTC_OK) {
        IOTC_LOGE("dev ctx init error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

void DeviceServiceDeinit(void)
{
    ServiceManagerUnregisterInstance(IOTC_SERVICE_ID_DEVICE);
}