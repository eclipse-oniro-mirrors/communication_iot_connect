
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

#include "iotc_sle_announce.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "kh_sle_announce.h"
#include "securec.h"


static IotcAdptSleAnnounceCallback g_announceEventHandler = NULL;

uint8_t IotcInitSleAnnounceService(void)
{
    uint8_t ret = InitSleAnnounceService();
    if (ret != IOTC_OK) {
        IOTC_LOGE("IotcInitSleAnnounceService=%d", ret);
        return ret;
    }
    return IOTC_OK;
}

uint8_t IotcAddAnnounce(uint8_t *announceId)
{
    if (announceId == NULL) {
        IOTC_LOGE("IotcAddAnnounce invalid param");
        return IOTC_ERROR;
    }

    uint8_t ret = AddAnnounce(announceId);
    if (ret != IOTC_OK) {
        IOTC_LOGE("IotcAddAnnounce=%d", ret);
        return ret;
    }
    return IOTC_OK;
}

static void iotcAnnounceStateChangeCB(uint8_t announceId, uint8_t announceState, bool isTerminaled)
{
    IOTC_LOGD("iotAnnounceStateChangeCB %d", announceState);
    IotcAdptSleAnnounceEventParam eventParam;
    eventParam.announceStateChnage.announceId = announceId;
    eventParam.announceStateChnage.announceState = announceState;
    eventParam.announceStateChnage.isTerminaled = isTerminaled;
    if (g_announceEventHandler != NULL &&
        g_announceEventHandler(IOTC_ADPT_SLE_ANNOUNCE_STATE_CHANGE, &eventParam) != IOTC_OK) {
        IOTC_LOGE("SLE MtuChangeCb handle error");
    }
}

static SleAnnounceCallbacks g_sleAnnounceUtCbs = {
    .OnSleAnnounceStateChangeCb = iotcAnnounceStateChangeCB,
};

uint8_t IotcRegisterAnnounceCallbacks(IotcAdptSleAnnounceCallback callback)
{
    g_announceEventHandler = callback;
    uint8_t ret = RegisterAnnounceCallbacks(&g_sleAnnounceUtCbs);
    if (ret != IOTC_OK) {
        IOTC_LOGE("IotcRegisterAnnounceCallbacks ret=%d", ret);
        return ret;
    }
    return IOTC_OK;
}

uint8_t IotcStartAnnounce(
    uint8_t announceId,
    const IotcAdptSleAnnounceData *advData,
    const IotcAdptSleAnnounceParam *advParam
)
{
    if ((advData == NULL)) {
        IOTC_LOGE("invalid param");
        return IOTC_ERROR;
    }
    SleAnnounceData iotcAdvData = {0};
    iotcAdvData.announceData = (uint8_t *)advData->announceData;
    iotcAdvData.announceLength = advData->announceLength;
    iotcAdvData.responceData = (uint8_t *)advData->responceData;
    iotcAdvData.responceLength = advData->responceLength;

    int32_t ret = 0;
    if (advParam != NULL) {
        SleAnnounceParam iotcAdvParam = {0};
        iotcAdvParam.handle = advParam->handle;
        iotcAdvParam.mode = advParam->mode;
        iotcAdvParam.role = advParam->role;
        iotcAdvParam.level = advParam->level;
        iotcAdvParam.channelMap = advParam->channelMap;
        iotcAdvParam.annonceIntervalMin = advParam->annonceIntervalMin;
        iotcAdvParam.annonceIntervalMax = advParam->annonceIntervalMax;
        iotcAdvParam.txPower = advParam->txPower;
        iotcAdvParam.connectIntervalMin = advParam->connectIntervalMin;
        iotcAdvParam.connectIntervalMax = advParam->connectIntervalMax;
        iotcAdvParam.connectLatency = advParam->connectLatency;
        iotcAdvParam.connectTimeout = advParam->connectTimeout;
        SleDeviceAddress ownAddr = {0};
        ownAddr.addrType = advParam->ownAddr.type;
        if (memcpy_s(
            ownAddr.addr, 
            sizeof(ownAddr.addr), 
            advParam->ownAddr.addr, 
            sizeof(advParam->ownAddr.addr)
        ) != EOK) {
            IOTC_LOGE("IotcStartAnnounce memcpy_s failed: UUID data copy error");
        }
        iotcAdvParam.ownAddr = ownAddr;
        SleDeviceAddress peerAddr = {0};
        peerAddr.addrType = advParam->peerAddr.type;
        if (memcpy_s(
            peerAddr.addr, 
            sizeof(peerAddr.addr), 
            advParam->peerAddr.addr, 
            sizeof(advParam->peerAddr.addr)
        ) != EOK) {
            IOTC_LOGE("IotcStartAnnounce memcpy_s failed: UUID data copy error");
        }
        iotcAdvParam.peerAddr = peerAddr;
        ret = StartAnnounce(announceId, &iotcAdvData, &iotcAdvParam);
    } else {
        ret = StartAnnounce(announceId, &iotcAdvData, NULL);
    }
    if (ret != IOTC_OK) {
        IOTC_LOGE("start adv ret=%d", ret);
        return ret;
    }
    return IOTC_OK;
}

uint8_t IotcRemoveAnnounce(uint8_t announceId)
{
    uint8_t ret = RemoveAnnounce(announceId);
    if (ret != IOTC_OK) {
        IOTC_LOGE("IotcRemoveAnnounce ret=%d", ret);
        return ret;
    }
    return IOTC_OK;
}


uint8_t IotcStopAnnounce(uint8_t announceId)
{
    /* 由于当前设备仅有一个广播，暂时不涉及多路广播 */
    uint8_t ret = StopAnnounce(announceId);
    if (ret != IOTC_OK) {
        IOTC_LOGE("IotcStopAnnounce ret=%d", ret);
        return ret;
    }
    return IOTC_OK;
}

uint8_t IotcUnregisterAnnounceCallbacks(IotcAdptSleAnnounceCallback callback)
{
    g_announceEventHandler = callback;
    uint8_t ret =  ret = UnregisterAnnounceCallbacks(&g_sleAnnounceUtCbs);;

    if (ret != IOTC_OK) {
        IOTC_LOGE("IotcUnregisterAnnounceCallbacks ret=%d", ret);
        return ret;
    }
    return IOTC_OK;
}

uint8_t IotcDeinitSleAnnounceService(void)
{
    uint8_t ret = DeinitSleAnnounceService();
    if (ret != IOTC_OK) {
        IOTC_LOGE("IotcDeinitSleAnnounceService=%d", ret);
        return ret;
    }
   return IOTC_OK;
}