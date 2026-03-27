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
#include "iotc_svc_sle.h"
#include "iotc_log.h"
#include "iotc_errcode.h"
#include "iotc_svc.h"
#include "service_proxy.h"

static const SleSvcApi *GetSleSvcApi(void)
{
    const SleSvcApi *sleApi = NULL;
    int32_t ret = ServiceProxyGetApiHandler(IOTC_SERVICE_ID_SLE, (const void **)&sleApi);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get sle api error %d", ret);
        /* service 1792 无效等异常场景下，尝试轻量级自恢复：先拉起 SLE 服务再重试一次 */
        (void)ServiceProxyStartService(IOTC_SERVICE_ID_SLE, NULL);
        ret = ServiceProxyGetApiHandler(IOTC_SERVICE_ID_SLE, (const void **)&sleApi);
        if (ret != IOTC_OK) {
            IOTC_LOGW("retry get sle api error %d", ret);
        return NULL;
    }
    }
    return sleApi;
}

int32_t SleSvcProxyStartSeek(void)
{
    const SleSvcApi *sleApi = GetSleSvcApi();
    if (sleApi == NULL || sleApi->onStartScan == NULL) {
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_NO_API;
    }

    return sleApi->onStartScan();
}

int32_t SleSvcProxyStartAdv(uint32_t ms)
{
    const SleSvcApi *sleApi = GetSleSvcApi();
    if (sleApi == NULL || sleApi->onStartAdv == NULL) {
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_NO_API;
    }

    return sleApi->onStartAdv(ms);
}

int32_t SleSvcProxyStopAdv(void)
{
    const SleSvcApi *sleApi = GetSleSvcApi();
    if (sleApi == NULL || sleApi->onStopAdv == NULL) {
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_NO_API;
    }

    return sleApi->onStopAdv();
}

int32_t SleSvcProxySendCustomSecData(const char *devId, const uint8_t *data, uint32_t len)
{
    const SleSvcApi *sleApi = GetSleSvcApi();
    if (sleApi == NULL || sleApi->onSendCustomSecData == NULL) {
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_NO_API;
    }

    return sleApi->onSendCustomSecData(devId,  data, len);
}

int32_t SleSvcProxySendIndicateData(const char *svcUuid, const char *charUuid,
    const uint8_t *value, uint32_t valueLen)
{
    const SleSvcApi *sleApi = GetSleSvcApi();
    if (sleApi == NULL || sleApi->onSendIndicateData == NULL) {
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_NO_API;
    }

    return sleApi->onSendIndicateData(svcUuid, charUuid, value, valueLen);
}

int32_t SleSvcProxyFindDeviceInfo(const char *devId, void **info)
{
    const SleSvcApi *sleApi = GetSleSvcApi();
    if (sleApi == NULL || sleApi->onFindDeviceInfo == NULL) {
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_NO_API;
    }

    return sleApi->onFindDeviceInfo(devId, info);
}

int32_t SleSvcProxyFindDeviceInfoByName(const char *name, IotcConDeviceInfo **info)
{
    const SleSvcApi *sleApi = GetSleSvcApi();
    if (sleApi == NULL || sleApi->onFindDeviceInfoByName == NULL) {
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_NO_API;
    }

    return sleApi->onFindDeviceInfoByName(name, info);
}