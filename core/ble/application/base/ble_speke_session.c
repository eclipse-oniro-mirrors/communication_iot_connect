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
#include "ble_speke_session.h"
#include "utils_common.h"
#include "utils_assert.h"
#include "security_speke.h"
#include "event_bus_sub.h"
#include "ble_linklayer.h"
#include "iotc_errcode.h"
#include "iotc_event.h"
#include "iotc_svc_dev.h"
#include "iotc_svc.h"
#include "product_adapter.h"

static SpekeSession *g_bleSpekeSess = NULL;
static int32_t g_bleSpekeErrCode = IOTC_OK;

static int32_t GetPinCode(SpekeSession *session, void *user, uint8_t *pinCode, uint32_t *len)
{
    NOT_USED(session);
    NOT_USED(user);

    *len = IOTC_PINCODE_LEN;
    return ProductProfGetPincode(pinCode, IOTC_PINCODE_LEN);
}

static int32_t NotifySpekeFinished(SpekeSession *session, void *user, int32_t errorCode)
{
    NOT_USED(session);
    NOT_USED(user);
    g_bleSpekeErrCode = errorCode;
    IOTC_LOGE("speke errcode:%d", errorCode);
    return IOTC_OK;
}

SpekeSession *GetBleSpekeSess(void)
{
    return g_bleSpekeSess;
}

static void BleSpekeSessClear(uint32_t event, void *param, uint32_t len)
{
    NOT_USED(event);
    NOT_USED(param);
    NOT_USED(len);
    DestroyBleSpekeSess();
}

int32_t CreateBleSpekeSess(void)
{
    if (g_bleSpekeSess != NULL) {
        return IOTC_OK;
    }

    int32_t ret = EventBusSubscribe(BleSpekeSessClear, IOTC_CORE_BLE_EVENT_GATT_DISCONNECT);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "subscribe gatt disconn err:%d", ret);
    ret = EventBusSubscribe(BleSpekeSessClear, IOTC_CORE_COMM_EVENT_MAIN_RESET);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "subscribe sdk reset err:%d", ret);
    ret = EventBusSubscribe(BleSpekeSessClear, IOTC_CORE_COMM_EVENT_MAIN_QUIT);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "subscribe sdk quit err:%d", ret);

    SpekeCallback cb = {
        .getPinCode = GetPinCode,
        .notifySpekeFinished = NotifySpekeFinished,
    };
    g_bleSpekeSess = SpekeInitSession(SPEKE_TYPE_SERVER, &cb, NULL);
    if (g_bleSpekeSess == NULL) {
        IOTC_LOGE("create speke session");
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_CREATE;
    }
    return LinkLayerRegisterSpekeSessionGetCb(GetBleSpekeSess);
}

void DestroyBleSpekeSess(void)
{
    if (g_bleSpekeSess == NULL) {
        return;
    }
    SpekeFreeSession(g_bleSpekeSess);
    g_bleSpekeSess = NULL;
}

int32_t GetBleSpekeErrCode(void)
{
    return g_bleSpekeErrCode;
}