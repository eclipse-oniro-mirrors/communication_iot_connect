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

#include "kh_sle_host.h"
#include "iotc_sle_host.h"
#include "iotc_errcode.h"
#include "iotc_log.h"

uint8_t IotcInitSleHostService(void)
{
    uint8_t ret = InitSleHostService();
    if (ret != IOTC_OK) {
        IOTC_LOGE("IotcInitSleHostServiceret=%d", ret);
        return ret;
    }
    return IOTC_OK;
}


uint8_t IotcDeinitSleHostService(void)
{
    uint8_t ret = DeinitSleHostService();
    if (ret != IOTC_OK) {
        IOTC_LOGE("IotcInitSleHostServiceret=%d", ret);
        return ret;
    }
    return IOTC_OK;
}

uint8_t IotcSleEnable(void)
{
    uint8_t ret = SleEnable();
    if (ret != IOTC_OK) {
        IOTC_LOGE("IotcSleEnable=%d", ret);
        return ret;
    }
    return IOTC_OK;
}

uint8_t IotcSleDisable(void)
{
    uint8_t ret = SleDisable();
    if (ret != IOTC_OK) {
        IOTC_LOGE("IotcSleEnable=%d", ret);
        return ret;
    }
    return IOTC_OK;
}

uint8_t IotcSleSetSleName(const char *name)
{
    if (name == NULL) {
        IOTC_LOGE("IotcSleSetSleName invalid param");
        return IOTC_ERROR;
    }
    uint8_t ret = SetHostName(&name, sizeof(*name));
    if (ret != IOTC_OK) {
        IOTC_LOGE("set name ret=%d", ret);
        return ret;
    }
    return IOTC_OK;
}

IotcAdptSleDeviceAddr* IotcGetHostAddress(void)
{
    SleDeviceAddress* sleAddr = (SleDeviceAddress*)malloc(sizeof(SleDeviceAddress));
    uint8_t ret = GetHostAddress(sleAddr);
    if (ret != IOTC_OK) {
        IOTC_LOGE("set name ret=%d", ret);
        free(sleAddr);
        return NULL;
    }
    IotcAdptSleDeviceAddr* addr = (IotcAdptSleDeviceAddr*)malloc(sizeof(IotcAdptSleDeviceAddr));
    if (memcpy_s(addr, sizeof(IotcAdptSleDeviceAddr), sleAddr, sizeof(IotcAdptSleDeviceAddr)) != EOK) {
        return NULL;
    }
    return addr;
}


static void SleFlowMonitorEventCb(float flow)
{
}

static void sleHostStateChangeCb(SleDeviceState state)
{
    IOTC_LOGI("sle OnSleHostStateChangeCb ret=%d", state);
}

static SleHostCallbacks  g_host_cb = {
    .OnSleHostStateChangeCb= sleHostStateChangeCb,
    .OnSleFlowMonitorEventCb = sleFlowMonitorEventCb,
};


uint8_t IotcSleRegisterHostCallbacks()
{
    int32_t ret = RegisterHostCallbacks(&g_host_cb);
    if (!ret) {
        IOTC_LOGE("IotcSleRegisterHostCallbacks ret=%d", ret);
        return ret;
    }
    return IOTC_OK;
}

uint8_t IotcUnregisterHostCallbacks(IotcSleHostCallbacks *hostCallback)
{
    if (hostCallback == NULL) {
        IOTC_LOGE("UnregisterHostCallbacks invalid param");
        return IOTC_ERROR;
    }
    uint8_t ret = UnregisterHostCallbacks(&hostCallback);
    if (ret != IOTC_OK) {
        IOTC_LOGE("IotcUnregisterHostCallbacksret=%d", ret);
        return ret;
    }
    return IOTC_OK;
}