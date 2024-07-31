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
#include "iotc_svc_ble.h"
#include "iotc_log.h"
#include "iotc_errcode.h"
#include "iotc_svc.h"
#include "service_proxy.h"

static const BleSvcApi *GetBleSvcApi(void)
{
    const BleSvcApi *bleApi = NULL;
    int32_t ret = ServiceProxyGetApiHandler(IOTC_SERVICE_ID_BLE, (const void **)&bleApi);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get ble api error %d", ret);
        return NULL;
    }
    return bleApi;
}

int32_t BleSvcProxyStartAdv(uint32_t ms)
{
    const BleSvcApi *bleApi = GetBleSvcApi();
    if (bleApi == NULL || bleApi->onStartAdv == NULL) {
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_NO_API;
    }

    return bleApi->onStartAdv(ms);
}

int32_t BleSvcProxyStopAdv(void)
{
    const BleSvcApi *bleApi = GetBleSvcApi();
    if (bleApi == NULL || bleApi->onStopAdv == NULL) {
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_NO_API;
    }

    return bleApi->onStopAdv();
}

int32_t BleSvcProxySendCustomSecData(const uint8_t *data, uint32_t len)
{
    const BleSvcApi *bleApi = GetBleSvcApi();
    if (bleApi == NULL || bleApi->onSendCustomSecData == NULL) {
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_NO_API;
    }

    return bleApi->onSendCustomSecData(data, len);
}

int32_t BleSvcProxySendIndicateData(const char *svcUuid, const char *charUuid,
    const uint8_t *value, uint32_t valueLen)
{
    const BleSvcApi *bleApi = GetBleSvcApi();
    if (bleApi == NULL || bleApi->onSendIndicateData == NULL) {
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_NO_API;
    }

    return bleApi->onSendIndicateData(svcUuid, charUuid, value, valueLen);
}
