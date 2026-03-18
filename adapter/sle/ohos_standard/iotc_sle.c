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
#include "iotc_sle.h"
#include <time.h>
#include <sched.h>
#include <pthread.h>
#include "securec.h"
#include "iotc_mem.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "iotc_sle_tem.h"
#include "iotc_sle_vendor.h"
#include "sle_errcode.h"
#include "sle_ssap_server.h"
#include "sle_device_discovery.h"
#include "sle_connection_manager.h"

static bool g_isBond = false;
#define SLE_ADV_HANDLE_DEFAULT 1
static uint8_t g_announceId = SLE_ADV_HANDLE_DEFAULT;
static IotcAdptSleSsapCallback g_gattEventHandler = NULL;

static int32_t SleSetAnnounceData(uint8_t announceId, const IotcAdptSleAdvData *advData)
{
    sle_announce_data_t data_sdk = {
        .announce_data_len = advData->announceDataLen,
        .seek_rsp_data_len = advData->seekRspDataLen,
        .announce_data = advData->announceData,
        .seek_rsp_data = advData->seekRspData,
    };
    return sle_set_announce_data(announceId, &data_sdk);
}

static int32_t SleSetAnnounceParam(uint8_t announceId, const IotcAdptSleAdvParam *param)
{
    sle_announce_param_t param_sdk;
    param_sdk.announce_handle = param->announceHandle,
    param_sdk.announce_mode = param->announceMode,
    param_sdk.announce_gt_role = param->announceGtRole,
    param_sdk.announce_level = param->announceLevel,
    param_sdk.announce_interval_min = param->announceIntervalMin,
    param_sdk.announce_interval_max = param->announceIntervalMax,
    param_sdk.announce_channel_map = param->announceChannelMap,
    param_sdk.announce_tx_power = param->announce_tx_power,
    param_sdk.conn_interval_min = param->connIntervalMin,
    param_sdk.conn_interval_max = param->connIntervalMax,
    param_sdk.conn_max_latency = param->connMaxLatency,
    param_sdk.conn_supervision_timeout = param->connSupervisionTimeout,
    param_sdk.ext_param = (void*)param->extParam,
    param_sdk.own_addr.type = (uint8_t)param->ownAddr.type;
    if (memcpy_s(param_sdk.own_addr.addr, sizeof(param_sdk.own_addr.addr),
        param->ownAddr.addr, sizeof(param_sdk.own_addr.addr)) != EOK) {
        return IOTC_ERROR;
    }
    param_sdk.peer_addr.type = (uint8_t)param->peerAddr.type;
    if (memcpy_s(param_sdk.peer_addr.addr, sizeof(param_sdk.peer_addr.addr),
        param->peerAddr.addr, sizeof(param_sdk.peer_addr.addr)) != EOK) {
        return IOTC_ERROR;
    }
    return sle_set_announce_param(announceId, &param_sdk);
}

static int32_t SleStartAnnounce(uint8_t announceId)
{
    return sle_start_announce(announceId);
}

static int32_t SleStopAnnounce(uint8_t announceId)
{
    return sle_stop_announce(announceId);
}

static int32_t SsapsStartService(uint8_t serverId, uint16_t serviceHandle)
{
    return ssaps_start_service(serverId, serviceHandle);
}

#define SLE_CONNECT_UPDATE_INTERVAL_HDI 20

uint64_t SleClockGettimeUsKhLite(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL);
}

static int32_t sle_notify_indicate_sync(uint8_t serverId, uint16_t connectId, ssaps_ntf_ind_t *param)
{
    int32_t ret = -1;
    int sflag = 0;
    uint64_t khtBeforeGetJiffies = 0;
    uint64_t khtAfterGetJiffies = 0;
    uint64_t afterUs = 0;
    uint32_t waitMinTimeUs = (SLE_CONNECT_UPDATE_INTERVAL_HDI * 125);  // 最少等待1个发送间隔(可以是连接间隔)
    int retry = 2000; // 最多等待2000个连接间隔时间
    khtBeforeGetJiffies = SleClockGettimeUsKhLite();
    afterUs = khtBeforeGetJiffies + waitMinTimeUs; // 防止溢出回卷, 等待超时时间

    while (retry > 0) {
        if (sflag == 0 && GleTxAcbDataNumGet() > 0) {
            ret = ssaps_notify_indicate(serverId, connectId, param);
            sflag = 1;
        }

        khtAfterGetJiffies = SleClockGettimeUsKhLite();
        if ((int)(khtAfterGetJiffies - afterUs) < 0) { // 这里至少等待1个连接间隔(或发送间隔)
            sched_yield();
        } else {
            if (sflag == 0 && (--retry) > 0) {
                afterUs += waitMinTimeUs; // 没有发出去包的话 这里要继续等待下一个连接间隔(或发送间隔)
                sched_yield();
                continue;
            }
            // 超过最大等待时间,没有发送出去, 失败退出
            // 或者发送出去了, 成功返回
            break;
        }
    }
    return ret;
}

pthread_mutex_t g_sendMutexServer;
static int32_t SsapsNotifyIndicate(uint8_t serverId, uint16_t connectId, const SsapsNtfInd *param)
{
    pthread_mutex_lock(&g_sendMutexServer);
    ssaps_ntf_ind_t param_sdk = {
        .handle = param->handle,
        .type = (uint8_t)param->type,
        .value_len = param->value_len,
        .value = param->value,
    };
    int32_t ret = sle_notify_indicate_sync(serverId, connectId, &param_sdk);
    pthread_mutex_unlock(&g_sendMutexServer);
    return ret;
}

static int32_t SlePairRemoteDevice(const IotcAdptSleAddr *addr)
{
    sle_addr_t addr_sdk = {
        .type = (uint8_t)addr->type,
    };
    if (memcpy_s(&addr_sdk.addr, sizeof(addr_sdk.addr), addr->addr, sizeof(addr_sdk.addr)) != EOK) {
        return IOTC_ERROR;
    }
    return sle_pair_remote_device(&addr_sdk);
}

static int32_t EnableSle(void)
{
    return enable_sle();
}

static int32_t DisableSle(void)
{
    return disable_sle();
}

static int32_t SleSetLocalName(const uint8_t *name, uint8_t len)
{
    return sle_set_local_name(name, len);
}

static void AddServiceCb(uint8_t serverId, sle_uuid_t *uuid, uint16_t handle, uint32_t status)
{
    IOTC_LOGD("SLE AddServiceCb:status:%d,serverId:%d,Handle=%d", status, serverId, handle);
}

static void AddPropertyCb(uint8_t serverId, sle_uuid_t *uuid, uint16_t serviceHandle,
    uint16_t handle, uint32_t status)
{
    IOTC_LOGD("SLE AddPropertyCb:status:%d,serverId:%d,Handle=%d", status, serverId, handle);
}

static void AddDescriptorCb(uint8_t serverId, sle_uuid_t *uuid, uint16_t serviceHandle,
    uint16_t propertyHandle, uint32_t status)
{
    IOTC_LOGD("SLE AddDescriptorCb:status:%d,serverId:%d,Handle=%d", status, serverId, serviceHandle);
}

static IotcAdptSleStatus OhosStatusToAdapterStatus(int32_t status)
{
    return (status == IOTC_ADPT_SLE_STATUS_SUCCESS) ? IOTC_ADPT_SLE_STATUS_SUCCESS : IOTC_ADPT_SLE_STATUS_FAIL;
}

static void ServiceStartCb(uint8_t serverId, uint16_t handle, uint32_t status)
{
    IOTC_LOGD("SLE service start cb:status:%d,serverId:%d,Handle=%d", status, serverId, handle);
    IotcAdptSleSsapEventParam eventParam;
    eventParam.startSvc.status = OhosStatusToAdapterStatus(status);
    eventParam.startSvc.serverId = serverId;
    eventParam.startSvc.svcHandle = handle;
    if (g_gattEventHandler != NULL &&
        g_gattEventHandler(IOTC_ADPT_SLE_SSAP_EVENT_START_SVC_RESULT, &eventParam) != IOTC_OK) {
        IOTC_LOGE("doing gatt event");
    }
}

static void DeleteAllServiceCb(uint8_t serverId, uint32_t status)
{
    IOTC_LOGD("SLE DeleteAllServiceCb:status:%d,serverId:%d", status, serverId);
}

static void ReadRequestCb(uint8_t serverId, uint16_t connId, ssaps_req_read_cb_t *read_cb_para,
    uint32_t status)
{
    IOTC_LOGD("SLE  ReadRequestCb:status:%d,serverId:%d,conn_id=%d", status, serverId, connId);
}

static void WriteRequestCb(uint8_t serverId, uint16_t connId, ssaps_req_write_cb_t *write_cb_para,
    uint32_t status)
{
    IOTC_LOGD("SLE  WriteRequestCb:status:%d,serverId:%d,conn_id=%d", status, serverId, connId);
}

static void MtuChangeCb(uint8_t serverId, uint16_t connId, ssap_exchange_info_t *info, uint32_t status)
{
    IOTC_LOGD("SLE  MtuChangeCb:status:%d,serverId:%d,conn_id=%d", status, serverId, connId);
}

static ssaps_callbacks_t g_sleSsapsCb = {
    .add_service_cb = AddServiceCb,
    .add_property_cb = AddPropertyCb,
    .add_descriptor_cb = AddDescriptorCb,
    .start_service_cb = ServiceStartCb,
    .delete_all_service_cb = DeleteAllServiceCb,
    .read_request_cb = ReadRequestCb,
    .write_request_cb = WriteRequestCb,
    .mtu_changed_cb = MtuChangeCb,
};

static int32_t SsapsRegisterCallbacks(ssaps_callbacks_t *cb)
{
    return ssaps_register_callbacks(cb);
}

static int32_t SetGapSecurityParam(bool flag)
{
    (void)flag;
    return ERRCODE_SLE_SUCCESS;
}

int32_t IotcSleInitStack(void)
{
    int32_t ret = EnableSle();
    if (ret != IOTC_ADPT_SLE_STATUS_SUCCESS) {
        IOTC_LOGE("init stack ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t IotcSleSetConnectParam(const IotcAdptSleConnectParam *param)
{
    if (param == NULL) {
        IOTC_LOGE("invalid param");
        return IOTC_ERROR;
    }

    g_isBond = param->isBond;
    if (SetGapSecurityParam(param->isBond) != IOTC_OK) {
        IOTC_LOGE("set gap security param");
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t IotcSleRegisterSsapCb(const IotcAdptSleSsapCallback callback)
{
    if (callback == NULL) {
        IOTC_LOGE("invalid param");
        return IOTC_ERROR;
    }
    g_gattEventHandler = callback;
    IOTC_LOGD("gatts reg start");
    int32_t ret =  SsapsRegisterCallbacks(&g_sleSsapsCb);
    if (ret != IOTC_ADPT_SLE_STATUS_SUCCESS) {
        IOTC_LOGE("register gatt callback ret=%d", ret);
        return IOTC_ERROR;
    }
    IOTC_LOGI("gatts reg success");
    return IOTC_OK;
}

int32_t IotcSleSetSleName(const uint8_t *name, uint8_t len)
{
    if (name == NULL) {
        IOTC_LOGE("invalid param");
        return IOTC_ERROR;
    }
    int32_t ret = SleSetLocalName(name, len);
    if (ret != IOTC_ADPT_SLE_STATUS_SUCCESS) {
        IOTC_LOGE("set name ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t IotcSleStartAdv(const IotcAdptSleAdvParam *advParam, const IotcAdptSleAdvData *advData)
{
    if ((advParam == NULL) || (advData == NULL)) {
        IOTC_LOGE("invalid param");
        return IOTC_ERROR;
    }

    int32_t retData = SleSetAnnounceData(g_announceId, advData);
    if (retData != ERRCODE_SLE_SUCCESS) {
        IOTC_LOGE("start adv set data=%d", retData);
    }

    int32_t paramRet = SleSetAnnounceParam(g_announceId, advParam);
    if (paramRet != ERRCODE_SLE_SUCCESS) {
        IOTC_LOGE("start adv set param=%d", paramRet);
    }

    int32_t ret = SleStartAnnounce(g_announceId);
    if (ret != ERRCODE_SLE_SUCCESS) {
        IOTC_LOGE("start adv ret=%d", ret);
        return ret;
    }
    IOTC_LOGD("start adv success");
    return ERRCODE_SLE_SUCCESS;
}

int32_t IotcSleStopAdv()
{
    /* 由于当前设备仅有一个广播，暂时不涉及多路广播 */
    int32_t ret = SleStopAnnounce(g_announceId);
    if (ret != ERRCODE_SLE_SUCCESS) {
        IOTC_LOGE("stop adv ret=%d", ret);
        return ret;
    }
    IOTC_LOGD("stop adv success");
    return ERRCODE_SLE_SUCCESS;
}

static void RefreshHandle(IotcAdptSleSsapService *svc)
{
    for (uint8_t i = 0; i < svc->charNum; i++) {
        svc->character[i].charHandle += svc->svcHandle;
        for (uint8_t j = 0; j < svc->character[i].descNum; j++) {
            svc->character[i].desc[j].descHandle += svc->svcHandle;
        }
    }
}

int32_t IotcSleStartSsapsService(IotcAdptSleSsapService *svc, uint8_t svcNum)
{
    if ((svc == NULL) || (svcNum == 0)) {
        IOTC_LOGE("invalid param");
        return IOTC_ERROR;
    }
    for (uint8_t i = 0; i < svcNum; i++) {
        int32_t ret = SsapsStartService(svc[i].serverId, svc[i].svcHandle);
        if (ret != IOTC_ADPT_SLE_STATUS_SUCCESS) {
            IOTC_LOGE("gatt start service ret=%d", ret);
            return IOTC_ERROR;
        }
        RefreshHandle(svc + i);
    }
    return IOTC_OK;
}


int32_t IotcSleSendSsapsIndicate(const IotcAdptSleSendIndicateParam *param)
{
    if ((param == NULL) || (param->value == NULL) || (param->valueLen == 0)) {
        IOTC_LOGE("invalid param");
        return IOTC_ERROR;
    }

    SsapsNtfInd indParam = {0};
    (void)memset_s(&indParam, sizeof(indParam), 0, sizeof(indParam));
    indParam.handle = param->handle;
    indParam.type = param->type;
    indParam.value_len = param->valueLen;
    indParam.value = param->value;
    int32_t ret = SsapsNotifyIndicate(param->serverId, param->connId, &indParam);
    if (ret != IOTC_ADPT_SLE_STATUS_SUCCESS) {
        IOTC_LOGE("sle send indicate ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t IotcSleSendSsapsResponse(const IotcAdptSleResponseParam *param)
{
    (void)param;

    return IOTC_OK;
}

int32_t IotcSleDisconnectSsap(const uint8_t *bdAddr, uint32_t addrLen)
{
    if ((bdAddr == NULL) || (addrLen > SLE_ADDR_LEN)) {
        IOTC_LOGE("invalid param");
        return IOTC_ERROR;
    }
    IotcAdptSleAddr iotcAddr = {0};
    if (memcpy_s(iotcAddr.addr, SLE_ADDR_LEN, bdAddr, addrLen) != EOK) {
        IOTC_LOGE("memcpy");
        return IOTC_ERROR;
    }
    //  type: default 0 (public address)
    int32_t ret = SlePairRemoteDevice(&iotcAddr);
    if (ret != ERRCODE_SLE_SUCCESS) {
        IOTC_LOGE("gatt disconnect ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t IotcSleDeInitStack(void)
{
    bool ret = DisableSle();
    if (!ret) {
        IOTC_LOGE("disable bt stack ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}