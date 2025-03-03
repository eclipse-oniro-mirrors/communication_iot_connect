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
#include "sle_speke_session.h"
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

static SpekeSession *g_sleSpekeSess = NULL;
static int32_t g_sleSpekeErrCode = IOTC_OK;

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
    g_sleSpekeErrCode = errorCode;
    IOTC_LOGN("speke errcode:%d", errorCode);
    return IOTC_OK;
}

SpekeSession *GetSleSpekeSess(void)
{
    return g_sleSpekeSess;
}

static void SleSpekeSessClear(uint32_t event, void *param, uint32_t len)
{
    NOT_USED(event);
    NOT_USED(param);
    NOT_USED(len);
    DestroySleSpekeSess();
}

int32_t CreateSleSpekeSess(void)
{
    if (g_sleSpekeSess != NULL) {
        return IOTC_OK;
    }

    int32_t ret = EventBusSubscribe(SleSpekeSessClear, IOTC_CORE_SLE_EVENT_SSAP_DISCONNECT);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "subscribe ssap disconn err:%d", ret);
    ret = EventBusSubscribe(SleSpekeSessClear, IOTC_CORE_COMM_EVENT_MAIN_RESET);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "subscribe sdk reset err:%d", ret);
    ret = EventBusSubscribe(SleSpekeSessClear, IOTC_CORE_COMM_EVENT_MAIN_QUIT);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "subscribe sdk quit err:%d", ret);

    SpekeCallback cb = {
        .getPinCode = GetPinCode,
        .notifySpekeFinished = NotifySpekeFinished,
    };
    g_sleSpekeSess = SpekeInitSession(SPEKE_TYPE_SERVER, &cb, NULL);
    if (g_sleSpekeSess == NULL) {
        IOTC_LOGE("create speke session");
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_CREATE;
    }
    return LinkLayerRegisterSpekeSessionGetCb(GetSleSpekeSess);
}

void DestroySleSpekeSess(void)
{
    if (g_sleSpekeSess == NULL) {
        return;
    }
    SpekeFreeSession(g_sleSpekeSess);
    g_sleSpekeSess = NULL;
}

int32_t GetSleSpekeErrCode(void)
{
    return g_sleSpekeErrCode;
}