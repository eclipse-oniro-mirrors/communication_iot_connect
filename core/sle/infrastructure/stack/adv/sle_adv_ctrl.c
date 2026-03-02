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
#include "sle_adv_ctrl.h"
#include "iotc_sle.h"
#include "securec.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "sched_timer.h"
#include "utils_assert.h"
#include "utils_mutex_global.h"
#include "iotc_os.h"
#include "utils_common.h"

static int32_t g_advTimerId = EVENT_SOURCE_INVALID_TIMER_FD;
static SleGetAdvInfoCallback g_sleAdvCtrlCb = NULL;
static uint32_t g_advStartTime = 0;
static uint32_t g_advDuration = 0;

static void AdvCtrlTimerCb(int32_t id, void *userData)
{
    (void)id;
    (void)userData;
    (void)SleAdvCtrlStop();
    SchedTimerRemove(g_advTimerId);
    g_advTimerId = EVENT_SOURCE_INVALID_TIMER_FD;
    IOTC_LOGN("sle adv ctrl timer cb finish");
}

static int32_t StartSleAdvTimer(uint32_t ms)
{
    g_advStartTime = IotcGetSysTimeMs();
    g_advDuration = ms;
    if (ms == IOTC_SLE_ADV_ALWAYS) {
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

static int32_t CopyAdvInfo2Adapter(const IotcSleAdvParam *advPara, const IotcSleAdvData *advData,
    IotcAdptSleAdvParam *adapterAdvParam, IotcAdptSleAdvData *adapterAdvData)
{
    CHECK_RETURN_LOGW(advData->announceData != NULL && advData->announceDataLen != 0 &&
        advData->seekRspData && advData->seekRspDataLen != 0,
        IOTC_ERR_PARAM_INVALID, "param invalid");
    adapterAdvParam->announceMode = (IotcAdptSleAdvType)advPara->announceMode;
    adapterAdvParam->announceIntervalMin = advPara->announceIntervalMin;
    adapterAdvParam->announceIntervalMax = advPara->announceIntervalMax;
    adapterAdvParam->announceChannelMap = advPara->announceChannelMap;

    int32_t ret = memcpy_s(adapterAdvData->announceData, sizeof(adapterAdvData->announceData),
        advData->announceData, advData->announceDataLen);
    if (ret != EOK) {
        IOTC_LOGW("memcpy error %d/%u", ret, advData->announceDataLen);
        return IOTC_ERR_SECUREC_MEMCPY;
    }

    ret = memcpy_s(adapterAdvData->seekRspData, sizeof(adapterAdvData->seekRspData),
        advData->seekRspData, advData->seekRspDataLen);
    if (ret != EOK) {
        IOTC_LOGW("memcpy error %d/%u", ret, advData->seekRspDataLen);
        return IOTC_ERR_SECUREC_MEMCPY;
    }

    adapterAdvData->announceDataLen = advData->announceDataLen;
    adapterAdvData->seekRspDataLen = advData->seekRspDataLen;
    return IOTC_OK;
}

int32_t SleRegAdvAdvInfoCallback(SleGetAdvInfoCallback cb)
{
    CHECK_RETURN_LOGW(cb != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    (void)UtilsGlobalMutexLock();
    g_sleAdvCtrlCb = cb;
    UtilsGlobalMutexUnlock();
    return IOTC_OK;
}

static void StopSleAdvTimer(void)
{
    if (g_advTimerId >= 0) {
        SchedTimerRemove(g_advTimerId);
        g_advTimerId = EVENT_SOURCE_INVALID_TIMER_FD;
    }
}

static int32_t SleAdapterAdvCtrlStart(const IotcAdptSleAdvParam *advPara,
    const IotcAdptSleAdvData *advData, uint32_t ms)
{
    StopSleAdvTimer();
    int32_t ret = IotcSleStartAdv(advPara, advData);
    if (ret != IOTC_OK) {
        IOTC_LOGE("start sle adv err %d", ret);
        return ret;
    }

    return StartSleAdvTimer(ms);
}

int32_t SleAdvCtrlStartSpecific(const IotcSleAdvParam *advPara, const IotcSleAdvData *advData, uint32_t ms)
{
    CHECK_RETURN_LOGW(advPara != NULL && advData != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    IotcAdptSleAdvData adapterAdvData = {0};
    IotcAdptSleAdvParam adapterAdvParam = {0};
    int32_t ret = CopyAdvInfo2Adapter(advPara, advData, &adapterAdvParam, &adapterAdvData);
    if (ret != IOTC_OK) {
        IOTC_LOGW("copy adv date error %d", ret);
        return ret;
    }

    return SleAdapterAdvCtrlStart(&adapterAdvParam, &adapterAdvData, ms);
}

int32_t SleAdvCtrlStart(uint32_t ms)
{
    (void)UtilsGlobalMutexLock();
    SleGetAdvInfoCallback advInfoCb = g_sleAdvCtrlCb;
    UtilsGlobalMutexUnlock();

    if (advInfoCb == NULL) {
        IOTC_LOGW("adv info cb null");
        return IOTC_ERR_CALLBACK_NULL;
    }

    IotcAdptSleAdvData advData = {0};
    IotcAdptSleAdvParam advPara = {0};

    int32_t ret = advInfoCb(&advPara, &advData);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get adv info error %d", ret);
        return ret;
    }

    return SleAdapterAdvCtrlStart(&advPara, &advData, ms);
}

int32_t SleAdvCtrlStop(void)
{
    StopSleAdvTimer();
    int32_t ret = IotcSleStopAdv();
    if (ret != IOTC_OK) {
        IOTC_LOGW("stop adv error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

int32_t SleAdvCtrlResume(void)
{
    if (g_advDuration == IOTC_SLE_ADV_ALWAYS) {
        return SleAdvCtrlStart(IOTC_SLE_ADV_ALWAYS);
    }
    if (g_advDuration == 0) {
        return IOTC_OK;
    }

    uint32_t delta = UtilsDeltaTime(IotcGetSysTimeMs(), g_advStartTime);
    if (delta >= g_advDuration) {
        g_advDuration = 0;
        return IOTC_OK;
    }
    return SleAdvCtrlStart(g_advDuration - delta);
}

int32_t SleAdvCtrlUpdate(void)
{
    int32_t ret = IotcSleStopAdv();
    if (ret != IOTC_OK) {
        IOTC_LOGW("stop adv error %d", ret);
        return ret;
    }
    return SleAdvCtrlResume();
}