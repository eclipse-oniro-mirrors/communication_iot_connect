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
#include "iotc_svc_dev.h"
#include "iotc_svc.h"
#include "service_proxy.h"
#include "iotc_errcode.h"
#include "iotc_log.h"

static const DevSvcApi *GetDevSvcApi(void)
{
    const DevSvcApi *devApi = NULL;
    int32_t ret = ServiceProxyGetApiHandler(IOTC_SERVICE_ID_DEVICE, (const void **)&devApi);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get dev api error %d", ret);
        return NULL;
    }
    return devApi;
}

int32_t DevSvcProxyRecvBindInfo(const IotcJson *json)
{
    const DevSvcApi *devApi = GetDevSvcApi();
    if (devApi == NULL || devApi->onRecvBindInfo == NULL) {
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_NO_API;
    }

    return devApi->onRecvBindInfo(json);
}

int32_t DevSvcProxyCtlGetCharStates(const IotcJson *inArray, IotcJson **outArray)
{
    const DevSvcApi *devApi = GetDevSvcApi();
    if (devApi == NULL || devApi->onGetChar == NULL) {
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_NO_API;
    }

    return devApi->onGetChar(inArray, outArray);
}

int32_t DevSvcProxyCtlReportAll(DevReportType type)
{
    const DevSvcApi *devApi = GetDevSvcApi();
    if (devApi == NULL || devApi->onReportAll == NULL) {
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_NO_API;
    }

    return devApi->onReportAll(type);
}

int32_t DevSvcProxyCtlReportByDevId(DevReportType type, const char *devId)
{
    const DevSvcApi *devApi = GetDevSvcApi();
    if (devApi == NULL || devApi->onReportByDevId == NULL) {
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_NO_API;
    }

    return devApi->onReportByDevId(type, devId);
}

int32_t DevSvcProxyCtlPutCharStates(const IotcJson *inArray, IotcJson **outArray)
{
    const DevSvcApi *devApi = GetDevSvcApi();
    if (devApi == NULL || devApi->onPutChar == NULL) {
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_NO_API;
    }

    return devApi->onPutChar(inArray, outArray);
}

DevBindStatus DevSvcProxyGetBindStatus(void)
{
    const DevSvcApi *devApi = GetDevSvcApi();
    if (devApi == NULL || devApi->onGetBindStatus == NULL) {
        return DEV_BIND_STATUS_INVALID;
    }

    return devApi->onGetBindStatus();
}

int32_t DevSvcProxyProfReportCharState(const IotcCharState state[], uint32_t num)
{
    const DevSvcApi *devApi = GetDevSvcApi();
    if (devApi == NULL || devApi->onReportChar == NULL) {
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_NO_API;
    }

    return devApi->onReportChar(state, num);
}

int32_t DevSvcProxyGetAuthInfo(bool *isExist, DevAuthInfo *info)
{
    const DevSvcApi *devApi = GetDevSvcApi();
    if (devApi == NULL || devApi->onGetAuthInfo == NULL) {
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_NO_API;
    }

    return devApi->onGetAuthInfo(isExist, info);
}

int32_t DevSvcProxyGetRegisterInfo(bool *isExist, DevRegInfo *info)
{
    const DevSvcApi *devApi = GetDevSvcApi();
    if (devApi == NULL || devApi->onGetRegInfo == NULL) {
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_NO_API;
    }

    return devApi->onGetRegInfo(isExist, info);
}

int32_t DevSvcProxyGetLoginInfo(bool *isExist, DevLoginInfo *info)
{
    const DevSvcApi *devApi = GetDevSvcApi();
    if (devApi == NULL || devApi->onGetLoginInfo == NULL) {
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_NO_API;
    }

    return devApi->onGetLoginInfo(isExist, info);
}

int32_t DevSvcProxyRecvAuthInfo(const IotcJson *json)
{
    const DevSvcApi *devApi = GetDevSvcApi();
    if (devApi == NULL || devApi->onRecvAuthInfo == NULL) {
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_NO_API;
    }

    return devApi->onRecvAuthInfo(json);
}

int32_t DevSvcProxyRecvLoginInfo(const IotcJson *json)
{
    const DevSvcApi *devApi = GetDevSvcApi();
    if (devApi == NULL || devApi->onRecvLoginInfo == NULL) {
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_NO_API;
    }

    return devApi->onRecvLoginInfo(json);
}

int32_t DevSvcProxyCleanLoginInfo(void)
{
    const DevSvcApi *devApi = GetDevSvcApi();
    if (devApi == NULL || devApi->onCleanLoginInfo == NULL) {
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_NO_API;
    }

    return devApi->onCleanLoginInfo();
}

int32_t DevSvcProxyCleanRevokeFlag(void)
{
    const DevSvcApi *devApi = GetDevSvcApi();
    if (devApi == NULL || devApi->onCleanRevokeFlag == NULL) {
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_NO_API;
    }

    return devApi->onCleanRevokeFlag();
}

bool DevSvcProxyGetOnlineStatus(void)
{
    const DevSvcApi *devApi = GetDevSvcApi();
    if (devApi == NULL || devApi->onGetOnlineStatus == NULL) {
        return false;
    }

    return devApi->onGetOnlineStatus();
}

void DevSvcProxySetOnlineStatus(bool isOnline)
{
    const DevSvcApi *devApi = GetDevSvcApi();
    if (devApi == NULL || devApi->onSetOnlineStatus == NULL) {
        return;
    }

    devApi->onSetOnlineStatus(isOnline);
}