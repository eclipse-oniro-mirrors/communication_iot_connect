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
#include "ble_adv_ctrl.h"
#include "adapter_ble.h"
#include "securec.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "sched_timer.h"
#include "utils_assert.h"
#include "utils_mutex_global.h"
#include "adapter_os.h"
#include "utils_common.h"

static int32_t g_advTimerId = EVENT_SOURCE_INVALID_TIMER_FD;
static BleGetAdvInfoCallback g_bleAdvCtrlCb = NULL;
static uint32_t g_advStartTime = 0;
static uint32_t g_advDuration = 0;

static void AdvCtrlTimerCb(int32_t id, void *userData)
{
    (void)id;
    (void)userData;
    (void)BleAdvCtrlStop();
    SchedTimerRemove(g_advTimerId);
    g_advTimerId = EVENT_SOURCE_INVALID_TIMER_FD;
    IOTC_LOGN("ble adv ctrl timer cb finish");
}

static int32_t StartBleAdvTimer(uint32_t ms)
{
    g_advStartTime = AdapterGetSysTimeMs();
    g_advDuration = ms;
    if (ms == IOTC_BLE_ADV_ALWAYS) {
        return IOTC_OK;
    }
    int32_t id = SchedTimerAdd(EVENT_SOURCE_TIMER_TYPE_ONCE, AdvCtrlTimerCb, ms, NULL);
    if (id < 0) {
        IOTC_LOGW("start adv timer error %d", id);
        return id;
    }

    g_advTimerId = id;
    return IOTC_OK;
}

static int32_t CopyAdvInfo2Adapter(const IotcBleAdvParam *advPara, const IotcBleAdvData *advData,
    AdapterBleAdvParam *adapterAdvParam, AdapterBleAdvData *adapterAdvData)
{
    CHECK_RETURN_LOGW(advData->advData != NULL && advData->advDataLen != 0 &&
        advData->rspData && advData->rspDataLen != 0,
        IOTC_ERR_PARAM_INVALID, "param invalid");
    adapterAdvParam->advType = (AdapterBleAdvType)advPara->advType;
    adapterAdvParam->advMinInt = advPara->minInterval;
    adapterAdvParam->advMaxInt = advPara->maxInterval;
    adapterAdvParam->channelMap = advPara->channelMap;

    int32_t ret = memcpy_s(adapterAdvData->advData, sizeof(adapterAdvData->advData),
        advData->advData, advData->advDataLen);
    if (ret != EOK) {
        IOTC_LOGW("memcpy error %d/%u", ret, advData->advDataLen);
        return IOTC_ERR_SECUREC_MEMCPY;
    }

    ret = memcpy_s(adapterAdvData->rspData, sizeof(adapterAdvData->rspData), advData->rspData, advData->rspDataLen);
    if (ret != EOK) {
        IOTC_LOGW("memcpy error %d/%u", ret, advData->rspDataLen);
        return IOTC_ERR_SECUREC_MEMCPY;
    }

    adapterAdvData->advDataLen = advData->advDataLen;
    adapterAdvData->rspDataLen = advData->rspDataLen;
    return IOTC_OK;
}

int32_t BleRegAdvAdvInfoCallback(BleGetAdvInfoCallback cb)
{
    CHECK_RETURN_LOGW(cb != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    (void)UtilsGlobalMutexLock();
    g_bleAdvCtrlCb = cb;
    UtilsGlobalMutexUnlock();
    return IOTC_OK;
}

static void StopBleAdvTimer(void)
{
    if (g_advTimerId >= 0) {
        SchedTimerRemove(g_advTimerId);
        g_advTimerId = EVENT_SOURCE_INVALID_TIMER_FD;
    }
}

static int32_t BleAdapterAdvCtrlStart(const AdapterBleAdvParam *advPara, const AdapterBleAdvData *advData, uint32_t ms)
{
    StopBleAdvTimer();
    int32_t ret = AdapterBleStartAdv(advPara, advData);
    if (ret != IOTC_OK) {
        IOTC_LOGE("start ble adv err %d", ret);
        return ret;
    }

    return StartBleAdvTimer(ms);
}

int32_t BleAdvCtrlStartSpecific(const IotcBleAdvParam *advPara, const IotcBleAdvData *advData, uint32_t ms)
{
    CHECK_RETURN_LOGW(advPara != NULL && advData != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    AdapterBleAdvData adapterAdvData = {0};
    AdapterBleAdvParam adapterAdvParam = {0};
    int32_t ret = CopyAdvInfo2Adapter(advPara, advData, &adapterAdvParam, &adapterAdvData);
    if (ret != IOTC_OK) {
        IOTC_LOGW("copy adv date error %d", ret);
        return ret;
    }

    return BleAdapterAdvCtrlStart(&adapterAdvParam, &adapterAdvData, ms);
}

int32_t BleAdvCtrlStart(uint32_t ms)
{
    (void)UtilsGlobalMutexLock();
    BleGetAdvInfoCallback advInfoCb = g_bleAdvCtrlCb;
    UtilsGlobalMutexUnlock();

    if (advInfoCb == NULL) {
        IOTC_LOGW("adv info cb null");
        return IOTC_ERR_CALLBACK_NULL;
    }

    AdapterBleAdvData advData = {0};
    AdapterBleAdvParam advPara = {0};

    int32_t ret = advInfoCb(&advPara, &advData);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get adv info error %d", ret);
        return ret;
    }

    return BleAdapterAdvCtrlStart(&advPara, &advData, ms);
}

int32_t BleAdvCtrlStop(void)
{
    StopBleAdvTimer();
    int32_t ret = AdapterBleStopAdv();
    if (ret != IOTC_OK) {
        IOTC_LOGW("stop adv error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

int32_t BleAdvCtrlResume(void)
{
    if (g_advDuration == IOTC_BLE_ADV_ALWAYS) {
        return BleAdvCtrlStart(IOTC_BLE_ADV_ALWAYS);
    }
    if (g_advDuration == 0) {
        return IOTC_OK;
    }

    uint32_t delta = UtilsDeltaTime(AdapterGetSysTimeMs(), g_advStartTime);
    if (delta >= g_advDuration) {
        g_advDuration = 0;
        return IOTC_OK;
    }
    return BleAdvCtrlStart(g_advDuration - delta);
}

int32_t BleAdvCtrlUpdate(void)
{
    int32_t ret = AdapterBleStopAdv();
    if (ret != IOTC_OK) {
        IOTC_LOGW("stop adv error %d", ret);
        return ret;
    }
    return BleAdvCtrlResume();
}