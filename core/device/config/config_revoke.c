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
#include "config_revoke.h"
#include "event_bus_sub.h"
#include "iotc_event.h"
#include "iotc_errcode.h"
#include "config_revoke_flag.h"
#include "config_register_info.h"
#include "config_authinfo.h"
#include "iotc_log.h"
#include "utils_combo.h"

static void RevokeEventConfigCallback(uint32_t event, void *param, uint32_t len)
{
    int32_t ret = IOTC_ERROR;
#ifdef IOTC_CONNECT_WIFI_CLOUD_SUPPORT
    /* After the cloud function is complete, add the factory reset flag */
    ret = ConfigClearRegisterInfo();
    if (ret != IOTC_OK) {
        IOTC_LOGW("clear register info error %d", ret);
    }
#endif
#if IOTC_CONF_KV_SUPPORT
    if (UtilsGetComboType() == COMBO_TYPE_BLE_ONLY) {
        ret = ConfigClearAuthInfo();
        if (ret != IOTC_OK) {
            IOTC_LOGW("clear auth info error %d", ret);
        }
    }
#endif
}

int32_t ConfigRevokeInit(void)
{
    int32_t ret = EventBusSubscribe(RevokeEventConfigCallback, IOTC_CORE_COMM_EVENT_MAIN_RESTORE);
    if (ret != IOTC_OK) {
        IOTC_LOGE("sub revoke event error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

void ConfigRevokeDeinit(void)
{
    (void)EventBusUnsubscribe(RevokeEventConfigCallback);
}