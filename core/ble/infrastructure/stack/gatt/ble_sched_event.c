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
#include "ble_sched_event.h"
#include "sched_msg_queue.h"
#include "ble_gatt_mgt.h"
#include "ble_adv_ctrl.h"
#include "securec.h"
#include "iotc_ble.h"
#include "comm_def.h"
#include "iotc_log.h"
#include "utils_assert.h"
#include "event_bus.h"
#include "utils_common.h"
#include "iotc_event.h"
#include "iotc_errcode.h"
#include "ble_common.h"
#include "iotc_prof_def.h"
#include "product_adapter.h"

typedef struct {
    int32_t event;
    void (*eventHandler)(int32_t event, void *param);
} BleSchedMsgHandler;

static void BleEventStartHandler(int32_t event, void *param)
{
    (void)event;
    (void)param;
    IOTC_LOGI("ble event start");
}

static void BleEventConnectHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    if (GetBleGattMgtApp()->connNum >= BLE_DEFAULT_MAX_CONN_NUM) {
        IOTC_LOGE("connect num overflow=%u", GetBleGattMgtApp()->connNum);
        return;
    }
    if (GetBleGattMgtApp()->peerDevInfo == NULL) {
        IOTC_LOGE("no init peer dev info");
        return;
    }
    BlePeerDevInfo *peerDevInfoList = GetBleGattMgtApp()->peerDevInfo;
    IotcAdptBleGattEventParam *eventParam = (IotcAdptBleGattEventParam *)param;
    peerDevInfoList[GetBleGattMgtApp()->connNum].connId = eventParam->connSvc.connId;
    peerDevInfoList[GetBleGattMgtApp()->connNum].serverId = eventParam->connSvc.serverId;
    if (memcpy_s(peerDevInfoList[GetBleGattMgtApp()->connNum].peerAddr,
        sizeof(peerDevInfoList[GetBleGattMgtApp()->connNum].peerAddr),
        eventParam->connSvc.devAddr, sizeof(eventParam->connSvc.devAddr)) != EOK) {
        IOTC_LOGE("memepy");
        return;
    }
    GetBleGattMgtApp()->connNum++;
    EventBusPublishSync(IOTC_CORE_BLE_EVENT_GATT_CONNECT, NULL, 0);
    IOTC_LOGN("connect success connNum=%u, connId=%u, serverId=%u",
        GetBleGattMgtApp()->connNum, eventParam->connSvc.connId,  eventParam->connSvc.serverId);
}

static void BleEventSendIndicatetHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN(param != NULL);
    BleIndicateParam *indParam = (BleIndicateParam *)param;
    int32_t ret = BleSendIndicateDataInner(indParam->svcUuid, indParam->charUuid, indParam->value, indParam->valueLen);
    IOTC_LOGN("send indicate msg ret=%d, valueLen=%u", ret, indParam->valueLen);
}

static void BleEventIndicateConftHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptBleGattEventParam *eventParam = (IotcAdptBleGattEventParam *)param;
    IOTC_LOGN("indicate conf connId=%u, handle=%u, status=%u",
        eventParam->indicateConf.connId, eventParam->indicateConf.handle, eventParam->indicateConf.status);
}

static void BleEventDisconnectHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    if (GetBleGattMgtApp()->peerDevInfo == NULL) {
        IOTC_LOGE("no init peer dev info");
        return;
    }

    if (GetBleGattMgtApp()->connNum == 0) {
        IOTC_LOGE("connect num=%u", GetBleGattMgtApp()->connNum);
        return;
    }
    BlePeerDevInfo *peerDevInfoList = GetBleGattMgtApp()->peerDevInfo;
    IotcAdptBleGattEventParam *eventParam = (IotcAdptBleGattEventParam *)param;
    for (uint8_t i = 0; i < GetBleGattMgtApp()->connNum; i++) {
        if (peerDevInfoList[i].connId != eventParam->disconnSvc.connId) {
            continue;
        }
        uint8_t maxIndex = GetBleGattMgtApp()->connNum - 1;
        for (uint8_t j = i; j < maxIndex; j++) {
            if (memcpy_s(&peerDevInfoList[j], sizeof(BlePeerDevInfo),
                &peerDevInfoList[j + 1], sizeof(BlePeerDevInfo)) != EOK) {
                IOTC_LOGE("memcpy_s");
                return;
            }
        }
        (void)memset_s(&peerDevInfoList[maxIndex], sizeof(BlePeerDevInfo), 0, sizeof(BlePeerDevInfo));
        GetBleGattMgtApp()->connNum--;
        break;
    }

    IotcOhCloudRegisterState state = IOTC_OH_CLOUD_UNREGISTER;
    int32_t ret = ProductProfGetCloudRegisterState(&state);
    if (ret != IOTC_OK) {
        if (ret == IOTC_ERR_CALLBACK_NULL) {
            IOTC_LOGW("The get cloud register state callback function is null, err %d", ret);
        } else {
            IOTC_LOGE("get cloud register state err %d", ret);
        }
    }
    if (state == IOTC_OH_CLOUD_UNREGISTER) {
        ret = BleAdvCtrlResume();
        if (ret != IOTC_OK) {
            IOTC_LOGE("start adv err %d", ret);
            return;
        }
    }
    EventBusPublishSync(IOTC_CORE_BLE_EVENT_GATT_DISCONNECT, NULL, 0);
    IOTC_LOGN("disconnect success connNum=%u, connId=%u, reason=%d",
        GetBleGattMgtApp()->connNum, eventParam->disconnSvc.connId, eventParam->disconnSvc.reason);
}

static void BleEventStartSvcResultHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptBleGattService *svcList = GetBleGattMgtApp()->svc;
    uint32_t svcNum = GetBleGattMgtApp()->svcNum;
    if (svcList == NULL) {
        IOTC_LOGE("no init svc list");
        return;
    }
    IotcAdptBleGattEventParam *eventParam = (IotcAdptBleGattEventParam *)param;
    if (eventParam->startSvc.status != IOTC_ADPT_BLE_STATUS_SUCCESS) {
        IOTC_LOGE("start svc fail");
        for (uint32_t i = 0; i < svcNum; i++) {
            if (svcList[i].svcHandle != eventParam->startSvc.svcHandle) {
                continue;
            }
            if (IotcBleStartGattsService(&svcList[i], 1) != IOTC_OK) {
                IOTC_LOGE("start svc err");
            }
            return;
        }
    }
    GetBleGattMgtApp()->startedSvcNum++;
    if (GetBleGattMgtApp()->startedSvcNum == GetBleGattMgtApp()->svcNum) {
        IOTC_LOGN("all svc start success");
        PrintBleGattServiceList(GetBleGattMgtApp()->svc, GetBleGattMgtApp()->svcNum);
    }
    IOTC_LOGN("start svc result success startedSvcNum=%u, svcHandle=%d, serverId=%d, svcNum=%d",
        GetBleGattMgtApp()->startedSvcNum, eventParam->startSvc.svcHandle, eventParam->startSvc.serverId,
        GetBleGattMgtApp()->svcNum);
}

static void BleEventStopSvcResultHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptBleGattEventParam *eventParam = (IotcAdptBleGattEventParam *)param;
    if (eventParam->stopSvc.status != IOTC_ADPT_BLE_STATUS_SUCCESS) {
        IOTC_LOGE("stop svc fail");
        if (IotcBleStopGattsService(eventParam->stopSvc.serverId, eventParam->stopSvc.svcHandle) != IOTC_OK) {
            IOTC_LOGE("stop svc err");
        }
        return;
    }
    if (GetBleGattMgtApp()->startedSvcNum > 0) {
        GetBleGattMgtApp()->startedSvcNum--;
    }
    if (GetBleGattMgtApp()->startedSvcNum == 0) {
        IOTC_LOGN("all svc stop success");
    }
    IOTC_LOGN("stop svc result success svcHandle=%d, serverId=%d",
        eventParam->stopSvc.svcHandle, eventParam->stopSvc.serverId);
}

static void BleEventSetMtuResultHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptBleGattEventParam *eventParam = (IotcAdptBleGattEventParam *)param;
    if (eventParam->setMtu.status != IOTC_ADPT_BLE_STATUS_SUCCESS) {
        IOTC_LOGE("set mtu fail");
        return;
    }
    IOTC_LOGN("set mtu success connId=%u, mtu=%u", eventParam->setMtu.connId, eventParam->setMtu.mtu);
}

static void BleEventStartAdvResultHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptBleGattEventParam *eventParam = (IotcAdptBleGattEventParam *)param;
    if (eventParam->startAdv.status != IOTC_ADPT_BLE_STATUS_SUCCESS) {
        IOTC_LOGE("start adv fail");
        return;
    }
    IOTC_LOGN("start adv success");
}

static void BleEventReqReadHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptBleGattEventParam *eventParam = (IotcAdptBleGattEventParam *)param;
    int32_t ret = BleGattReqRead(eventParam->reqRead.connId,
        eventParam->reqRead.attrHandle, eventParam->reqRead.transId);
    IOTC_LOGN("req read ret=%d", ret);
}

static void BleEventReqWriteHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptBleGattEventParam *eventParam = (IotcAdptBleGattEventParam *)param;
    int32_t ret = BleGattReqWrite(eventParam->reqWrite.connId, eventParam->reqWrite.attrHandle,
        eventParam->reqWrite.transId, eventParam->reqWrite.value, eventParam->reqWrite.valueLen);
    IOTC_LOGN("req write ret=%d", ret);
}

static void BleEventStopAdvResultHandler(int32_t event, void *param)
{
    (void)event;
    CHECK_V_RETURN_LOGW(param != NULL, "invalid param");
    IotcAdptBleGattEventParam *eventParam = (IotcAdptBleGattEventParam *)param;
    if (eventParam->stopAdv.status != IOTC_ADPT_BLE_STATUS_SUCCESS) {
        IOTC_LOGE("stop adv fail");
        return;
    }
    IOTC_LOGN("stop adv success");
}

static BleSchedMsgHandler g_bleSchedEvent[] = {
    {.event = BLE_EVENT_START, .eventHandler = BleEventStartHandler},
    {.event = BLE_EVENT_CONNECT, .eventHandler = BleEventConnectHandler},
    {.event = BLE_EVENT_SEND_INDICATE, .eventHandler = BleEventSendIndicatetHandler},
    {.event = BLE_EVENT_INDICATE_CONF, .eventHandler = BleEventIndicateConftHandler},
    {.event = BLE_EVENT_DISCONNECT, .eventHandler = BleEventDisconnectHandler},
    {.event = BLE_EVENT_START_SVC_RESULT, .eventHandler = BleEventStartSvcResultHandler},
    {.event = BLE_EVENT_STOP_SVC_RESULT, .eventHandler = BleEventStopSvcResultHandler},
    {.event = BLE_EVENT_SET_MTU_RESULT, .eventHandler = BleEventSetMtuResultHandler},
    {.event = BLE_EVENT_START_ADV_RESULT, .eventHandler = BleEventStartAdvResultHandler},
    {.event = BLE_EVENT_STOP_ADV_RESULT, .eventHandler = BleEventStopAdvResultHandler},
    {.event = BLE_EVENT_REQ_READ, .eventHandler = BleEventReqReadHandler},
    {.event = BLE_EVENT_REQ_WRITE, .eventHandler = BleEventReqWriteHandler},
};

int32_t BleScheduleEventInit(void)
{
    BleSchedMsg msg;
    msg.event = BLE_EVENT_START;
    msg.param = NULL;
    msg.free = NULL;
    int32_t ret = BleSchedMsgQueueSend(&msg, 0);
    if (ret != IOTC_OK) {
        IOTC_LOGF("send start event error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static void BleSchedEventSourceMsgHandler(const uint8_t *msg, uint32_t len)
{
    CHECK_V_RETURN_LOGW(msg != NULL && len == sizeof(BleSchedMsg), "invalid msg %u", len);
    const BleSchedMsg *schedMsg = (const BleSchedMsg *)msg;

    do {
        const BleSchedMsgHandler *hdlTbl = g_bleSchedEvent;
        uint32_t hdlNum = ARRAY_SIZE(g_bleSchedEvent);
        if (hdlTbl == NULL || hdlNum == 0) {
            IOTC_LOGF("no msg handler tbl");
            break;
        }

        for (uint32_t i = 0; i < hdlNum; ++i) {
            const BleSchedMsgHandler *curHandler = hdlTbl + i;
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
    if (schedMsg->param != NULL && schedMsg->free != NULL) {
        schedMsg->free(schedMsg->param);
    }
}

int32_t BleSchedMsgQueueSend(const BleSchedMsg *msg, uint32_t timeout)
{
    CHECK_RETURN_LOGW(msg != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");

    return SchedMsgQueueSend((const uint8_t *)msg, sizeof(BleSchedMsg), BleSchedEventSourceMsgHandler, timeout);
}
