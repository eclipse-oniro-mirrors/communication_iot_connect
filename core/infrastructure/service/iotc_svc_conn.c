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
#include "iotc_svc_conn.h"
#include "iotc_svc.h"
#include "service_proxy.h"
#include "iotc_errcode.h"
#include "iotc_log.h"

static const ConnSvcApi *GetConnSvcApi(void)
{
    const ConnSvcApi *connApi = NULL;
    int32_t ret = ServiceProxyGetApiHandler(IOTC_SERVICE_ID_WIFI_CONNECT, (const void **)&connApi);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get conn api error %d", ret);
        return NULL;
    }
    return connApi;
}

bool ConnSvcProxyIsNetConnected(void)
{
    const ConnSvcApi *connApi = GetConnSvcApi();
    if (connApi == NULL || connApi->isNetConnected == NULL) {
        return false;
    }

    return connApi->isNetConnected();
}

bool ConnSvcProxyIsWifiInfoExits(void)
{
    const ConnSvcApi *connApi = GetConnSvcApi();
    if (connApi == NULL || connApi->isWifiExist == NULL) {
        return false;
    }

    return connApi->isWifiExist();
}

int32_t ConnSvcProxySetNetCfgInfo(const AdapterJson *json)
{
    const ConnSvcApi *connApi = GetConnSvcApi();
    if (connApi == NULL || connApi->onSetNetInfo == NULL) {
        return false;
    }

    return connApi->onSetNetInfo(json);
}
