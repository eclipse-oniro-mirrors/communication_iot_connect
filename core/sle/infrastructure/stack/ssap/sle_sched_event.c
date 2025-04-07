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
#include "sle_sched_event.h"
#include "sched_msg_queue.h"
#include "sle_ssap_mgt.h"
#include "sle_adv_ctrl.h"
#include "securec.h"
#include "iotc_sle_server.h"
#include "comm_def.h"
#include "iotc_log.h"
#include "utils_assert.h"
#include "event_bus.h"
#include "utils_common.h"
#include "iotc_event.h"
#include "iotc_errcode.h"

typedef struct {
    int32_t event;
    void (*eventHandler)(int32_t event, void *param);
} SleSchedMsgHandler;

static void SleEventStartHandler(int32_t event, void *param)
{
    (void)event;
    (void)param;
    IOTC_LOGI("sle event start");
}

static void SleEventConnectHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    if (GetSleSsapMgtApp()->connNum >= SLE_DEFAULT_MAX_CONN_NUM) {
        IOTC_LOGE("connect num overflow=%u", GetSleSsapMgtApp()->connNum);
        return;
    }
    if (GetSleSsapMgtApp()->peerDevInfo == NULL) {
        IOTC_LOGE("no init peer dev info");
        return;
    }
    SlePeerDevInfo *peerDevInfoList = GetSleSsapMgtApp()->peerDevInfo;
    IotcAdptSleSsapEventParam *eventParam = (IotcAdptSleSsapEventParam *)param;
    peerDevInfoList[GetSleSsapMgtApp()->connNum].connId = eventParam->connSvc.connId;
    peerDevInfoList[GetSleSsapMgtApp()->connNum].serverId = eventParam->connSvc.serverId;
    if (memcpy_s(peerDevInfoList[GetSleSsapMgtApp()->connNum].peerAddr,
        sizeof(peerDevInfoList[GetSleSsapMgtApp()->connNum].peerAddr),
        eventParam->connSvc.devAddr, sizeof(eventParam->connSvc.devAddr)) != EOK) {
        IOTC_LOGE("memepy");
        return;
    }
    GetSleSsapMgtApp()->connNum++;
    EventBusPublishSync(IOTC_CORE_SLE_EVENT_SSAP_CONNECT, NULL, 0);
    IOTC_LOGN("connect success connNum=%u, connId=%u, serverId=%u",
        GetSleSsapMgtApp()->connNum, eventParam->connSvc.connId,  eventParam->connSvc.serverId);
}

static void SleEventSendIndicatetHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN(param != NULL);
    SleIndicateParam *indParam = (SleIndicateParam *)param;
    int32_t ret = SleSendIndicateDataInner(indParam->svcUuid, indParam->charUuid, indParam->value, indParam->valueLen);
    IOTC_LOGN("send indicate msg ret=%d, valueLen=%u", ret, indParam->valueLen);
}

static void SleEventIndicateConftHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptSleSsapEventParam *eventParam = (IotcAdptSleSsapEventParam *)param;
    IOTC_LOGN("indicate conf connId=%u, handle=%u, status=%u",
        eventParam->indicateConf.connId, eventParam->indicateConf.handle, eventParam->indicateConf.status);
}

static void SleEventDisconnectHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    if (GetSleSsapMgtApp()->peerDevInfo == NULL) {
        IOTC_LOGE("no init peer dev info");
        return;
    }

    if (GetSleSsapMgtApp()->connNum == 0) {
        IOTC_LOGE("connect num=%u", GetSleSsapMgtApp()->connNum);
        return;
    }
    SlePeerDevInfo *peerDevInfoList = GetSleSsapMgtApp()->peerDevInfo;
    IotcAdptSleSsapEventParam *eventParam = (IotcAdptSleSsapEventParam *)param;
    for (uint8_t i = 0; i < GetSleSsapMgtApp()->connNum; i++) {
        if (peerDevInfoList[i].connId != eventParam->disconnSvc.connId) {
            continue;
        }
        uint8_t maxIndex = GetSleSsapMgtApp()->connNum - 1;
        for (uint8_t j = i; j < maxIndex; j++) {
            if (memcpy_s(&peerDevInfoList[j], sizeof(SlePeerDevInfo),
                &peerDevInfoList[j + 1], sizeof(SlePeerDevInfo)) != EOK) {
                IOTC_LOGE("memcpy_s");
                return;
            }
        }
        (void)memset_s(&peerDevInfoList[maxIndex], sizeof(SlePeerDevInfo), 0, sizeof(SlePeerDevInfo));
        GetSleSsapMgtApp()->connNum--;
        break;
    }

    int32_t ret = SleAdvCtrlResume();
    if (ret != IOTC_OK) {
        IOTC_LOGE("start adv err %d", ret);
        return;
    }
    EventBusPublishSync(IOTC_CORE_SLE_EVENT_SSAP_DISCONNECT, NULL, 0);
    IOTC_LOGN("disconnect success connNum=%u, connId=%u, reason=%d",
        GetSleSsapMgtApp()->connNum, eventParam->disconnSvc.connId, eventParam->disconnSvc.reason);
}

static void SleEventStartSvcResultHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptSleSsapService *svcList = GetSleSsapMgtApp()->svc;
    uint8_t svcNum = GetSleSsapMgtApp()->svcNum;
    if (svcList == NULL) {
        IOTC_LOGE("no init svc list");
        return;
    }
    IotcAdptSleSsapEventParam *eventParam = (IotcAdptSleSsapEventParam *)param;
    if (eventParam->startSvc.status != IOTC_ADPT_SLE_STATUS_SUCCESS) {
        IOTC_LOGE("start svc fail");
        for (uint8_t i = 0; i < svcNum; i++) {
            if (svcList[i].svcHandle != eventParam->startSvc.svcHandle) {
                continue;
            }
            
            if (IotcSleSsapsStartService(&svcList[i], 1) != IOTC_OK) {
                IOTC_LOGE("start svc err");
            }
            return;
        }
    }
    GetSleSsapMgtApp()->startedSvcNum++;
    if (GetSleSsapMgtApp()->startedSvcNum == GetSleSsapMgtApp()->svcNum) {
        IOTC_LOGN("all svc start success");
        PrintSleSsapServiceList(GetSleSsapMgtApp()->svc, GetSleSsapMgtApp()->svcNum);
    }
    IOTC_LOGN("start svc result success startedSvcNum=%u, svcHandle=%d, serverId=%d,",
        GetSleSsapMgtApp()->startedSvcNum, eventParam->startSvc.svcHandle, eventParam->startSvc.serverId);
}

static void SleEventStopSvcResultHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptSleSsapEventParam *eventParam = (IotcAdptSleSsapEventParam *)param;
    if (eventParam->stopSvc.status != IOTC_ADPT_SLE_STATUS_SUCCESS) {
        IOTC_LOGE("stop svc fail");
        // if (IotcSleStopSsapsService(eventParam->stopSvc.serverId, eventParam->stopSvc.svcHandle) != IOTC_OK) {
        //     IOTC_LOGE("stop svc err");
        // }
        return;
    }
    GetSleSsapMgtApp()->startedSvcNum--;
    if (GetSleSsapMgtApp()->startedSvcNum == 0) {
        IOTC_LOGN("all svc stop success");
    }
    IOTC_LOGN("stop svc result success svcHandle=%d, serverId=%d",
        eventParam->stopSvc.svcHandle, eventParam->stopSvc.serverId);
}

static void SleEventSetMtuResultHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptSleSsapEventParam *eventParam = (IotcAdptSleSsapEventParam *)param;
    if (eventParam->setMtu.status != IOTC_ADPT_SLE_STATUS_SUCCESS) {
        IOTC_LOGE("set mtu fail");
        return;
    }
    IOTC_LOGN("set mtu success connId=%u, mtu =%u", eventParam->setMtu.connectId, eventParam->setMtu.version);
}

static void SleEventStartAdvResultHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptSleSsapEventParam *eventParam = (IotcAdptSleSsapEventParam *)param;
    // if (eventParam->startAdv.status != IOTC_ADPT_SLE_STATUS_SUCCESS) {
    //     IOTC_LOGE("start adv fail");
    //     return;
    // }
    IOTC_LOGN("start adv success");
}

static void SleEventReqReadHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptSleSsapEventParam *eventParam = (IotcAdptSleSsapEventParam *)param;
    int32_t ret = SleSsapReqRead(eventParam->reqRead.serverId,
        eventParam->reqRead.connectId, eventParam->reqRead.handle,eventParam->reqRead.requestId);
    IOTC_LOGN("req read ret=%d", ret);
}

static void SleEventReqWriteHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptSleSsapEventParam *eventParam = (IotcAdptSleSsapEventParam *)param;
    SleSsapWriteParam writeParam = {
        .serverId = eventParam->reqWrite.serverId,
        .connectId = eventParam->reqWrite.connectId,
        .requestId = eventParam->reqWrite.requestId,
        .type = eventParam->reqWrite.type,
        .value = eventParam->reqWrite.value,
        .valueLen = eventParam->reqWrite.valueLen,
        .handle = eventParam->reqWrite.handle
    };
    int32_t ret = SleSsapReqWrite(&writeParam);
    IOTC_LOGN("req write ret=%d", ret);
}

static void SleEventStopAdvResultHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptSleSsapEventParam *eventParam = (IotcAdptSleSsapEventParam *)param;
    // if (eventParam->stopAdv.status != IOTC_ADPT_SLE_STATUS_SUCCESS) {
    //     IOTC_LOGE("stop adv fail");
    //     return;
    // }
    IOTC_LOGN("stop adv success");
}

static void SleEventEnableHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptSleAnnounceSeekEventParam *eventParam =  (IotcAdptSleAnnounceSeekEventParam *)param;
    if (eventParam->sleEnable.status == IOTC_ADPT_SLE_STATUS_SUCCESS) {
        EventBusPublishSync(IOTC_CORE_SLE_EVENT_ENABLED, NULL, 0);
    } else {
        IOTC_LOGE("sle enable error!");
    }
}

static void SleEventDisableHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptSleAnnounceSeekEventParam *eventParam =  (IotcAdptSleAnnounceSeekEventParam *)param;
    if (eventParam->sleDisable.status == IOTC_ADPT_SLE_STATUS_SUCCESS) {
        EventBusPublishSync(IOTC_CORE_SLE_EVENT_DISABLED, NULL, 0);
    } else {
        IOTC_LOGE("sle disable error!");
    }
}

static void SleEventAnnounceEnableHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptSleAnnounceSeekEventParam *eventParam =  (IotcAdptSleAnnounceSeekEventParam *)param;
    EventBusPublishSync(IOTC_CORE_SLE_EVENT_ANNOUNCE_ENABLED, param, sizeof(eventParam->announceEnable));
}

static void SleEventAnnounceDisableHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptSleAnnounceSeekEventParam *eventParam =  (IotcAdptSleAnnounceSeekEventParam *)param;
    EventBusPublishSync(IOTC_CORE_SLE_EVENT_ANNOUNCE_DISABLED, param, sizeof(eventParam->announceDisable));
}

static void SleEventAnnounceTerminalHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptSleAnnounceSeekEventParam *eventParam =  (IotcAdptSleAnnounceSeekEventParam *)param;
    EventBusPublishSync(IOTC_CORE_SLE_EVENT_ANNOUNCE_TERMINAL, param, sizeof(eventParam->announceTerminal));
}

static void SleEventAnnounceRemoveHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptSleAnnounceSeekEventParam *eventParam =  (IotcAdptSleAnnounceSeekEventParam *)param;
    EventBusPublishSync(IOTC_CORE_SLE_EVENT_ANNOUNCE_REMOVE, param, sizeof(eventParam->announceRemove));
}

static void SleEventSeekEnableHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptSleAnnounceSeekEventParam *eventParam =  (IotcAdptSleAnnounceSeekEventParam *)param;
    if (eventParam->startSeek.status == IOTC_ADPT_SLE_STATUS_SUCCESS) {
        EventBusPublishSync(IOTC_CORE_SLE_EVENT_SEEK_ENABLE, NULL, 0);
    } else {
        IOTC_LOGE("sle seek enable error!");
    }
}

static void SleEventSeekDisableHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptSleAnnounceSeekEventParam *eventParam =  (IotcAdptSleAnnounceSeekEventParam *)param;
    if (eventParam->seekDisable.status == IOTC_ADPT_SLE_STATUS_SUCCESS) {
        EventBusPublishSync(IOTC_CORE_SLE_EVENT_SEEK_DISABLE, NULL, 0);
    } else {
        IOTC_LOGE("sle seek disable error!");
    }
}

static void SleEventSeekResultHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptSleAnnounceSeekEventParam *eventParam =  (IotcAdptSleAnnounceSeekEventParam *)param;
    EventBusPublishSync(IOTC_CORE_SLE_EVENT_SEEK_RESULT, param, sizeof(eventParam->seekResult));
}

static void SleEventConnectStateChangeHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptSleConnectionEventParam *eventParam =  (IotcAdptSleConnectionEventParam *)param;
    EventBusPublishSync(IOTC_CORE_SLE_EVENT_CONNECT_STATE_CHANGED, param, sizeof(eventParam->sleConnectStateChanged));
}

static void SleEventConnectParamUpdateReqHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptSleConnectionEventParam *eventParam =  (IotcAdptSleConnectionEventParam *)param;
    EventBusPublishSync(IOTC_CORE_SLE_EVENT_CONNECT_PARAM_UPDATE_REQ, param, sizeof(eventParam->sleConnectParamUpdateReq));
}

static void SleEventConnectParamUpdateHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptSleConnectionEventParam *eventParam =  (IotcAdptSleConnectionEventParam *)param;
    EventBusPublishSync(IOTC_CORE_SLE_EVENT_CONNECT_PARAM_UPDATE, param, sizeof(eventParam->sleConnectParamUpdate));
}

static void SleEventAuthCompleteHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptSleConnectionEventParam *eventParam =  (IotcAdptSleConnectionEventParam *)param;
    EventBusPublishSync(IOTC_CORE_SLE_EVENT_AUTH_COMPLETE, param, sizeof(eventParam->sleAuthComplete));
}

static void SleEventPairCompleteHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptSleConnectionEventParam *eventParam =  (IotcAdptSleConnectionEventParam *)param;
    EventBusPublishSync(IOTC_CORE_SLE_EVENT_PAIR_COMPLETE, param, sizeof(eventParam->slePairComplete));
}

static void SleEventReadRssiHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptSleConnectionEventParam *eventParam =  (IotcAdptSleConnectionEventParam *)param;
    EventBusPublishSync(IOTC_CORE_SLE_EVENT_READ_RSSI, param, sizeof(eventParam->sleReadRssi));
}

static void SleEventLowLatencyHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptSleConnectionEventParam *eventParam =  (IotcAdptSleConnectionEventParam *)param;
    EventBusPublishSync(IOTC_CORE_SLE_EVENT_LOW_LATENCY, param, sizeof(eventParam->sleLowLatency));
}

    IOTC_LOGN("stop adv success");
}

static SleSchedMsgHandler g_sleSchedEvent[] = {
    {.event = SLE_EVENT_START, .eventHandler = SleEventStartHandler},
    {.event = SLE_EVENT_CONNECT, .eventHandler = SleEventConnectHandler},
    {.event = SLE_EVENT_SEND_INDICATE, .eventHandler = SleEventSendIndicatetHandler},
    {.event = SLE_EVENT_INDICATE_CONF, .eventHandler = SleEventIndicateConftHandler},
    {.event = SLE_EVENT_DISCONNECT, .eventHandler = SleEventDisconnectHandler},
    {.event = SLE_EVENT_START_SVC_RESULT, .eventHandler = SleEventStartSvcResultHandler},
    {.event = SLE_EVENT_STOP_SVC_RESULT, .eventHandler = SleEventStopSvcResultHandler},
    {.event = SLE_EVENT_SET_MTU_RESULT, .eventHandler = SleEventSetMtuResultHandler},
    {.event = SLE_EVENT_START_ADV_RESULT, .eventHandler = SleEventStartAdvResultHandler},
    {.event = SLE_EVENT_STOP_ADV_RESULT, .eventHandler = SleEventStopAdvResultHandler},
    {.event = SLE_EVENT_REQ_READ, .eventHandler = SleEventReqReadHandler},
    {.event = SLE_EVENT_REQ_WRITE, .eventHandler = SleEventReqWriteHandler},
};

int32_t SleScheduleEventInit(void)
{
    SleSchedMsg msg;
    msg.event = SLE_EVENT_START;
    msg.param = NULL;
    msg.freeFunc = NULL;
    int32_t ret = SleSchedMsgQueueSend(&msg, 0);
    if (ret != IOTC_OK) {
        IOTC_LOGF("send start event error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static void SleSchedEventSourceMsgHandler(const uint8_t *msg, uint32_t len)
{
    CHECK_V_RETURN_LOGW(msg != NULL && len == sizeof(SleSchedMsg), "invalid msg %u", len);
    const SleSchedMsg *schedMsg = (const SleSchedMsg *)msg;

    do {
        const SleSchedMsgHandler *hdlTbl = g_sleSchedEvent;
        uint32_t hdlNum = ARRAY_SIZE(g_sleSchedEvent);
        if (hdlTbl == NULL || hdlNum == 0) {
            IOTC_LOGF("no msg handler tbl");
            break;
        }

        for (uint32_t i = 0; i < hdlNum; ++i) {
            const SleSchedMsgHandler *curHandler = hdlTbl + i;
            if (curHandler->event != schedMsg->event) {
                continue;
            }
            if (curHandler->eventHandler == NULL) {
                IOTC_LOGF("no msg handler %d", curHandler->event);
                break;
            }
            curHandler->eventHandler(schedMsg->event, schedMsg->param);
            break;
        }
    } while (0);
    if (schedMsg->param != NULL && schedMsg->freeFunc != NULL) {
        schedMsg->freeFunc(schedMsg->param);
    }
}

int32_t SleSchedMsgQueueSend(const SleSchedMsg *msg, uint32_t timeout)
{
    CHECK_RETURN_LOGW(msg != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");

    return SchedMsgQueueSend((const uint8_t *)msg, sizeof(SleSchedMsg), SleSchedEventSourceMsgHandler, timeout);
}
