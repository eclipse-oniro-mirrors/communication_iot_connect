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
#include "iotc_sle_server.h"
#include "iotc_sle_announce.h"
#include <time.h>
#include <sched.h>
#include <pthread.h>
#include "securec.h"
#include "iotc_mem.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "iotc_sle_tem.h"
#include "sle_errcode.h"
#include "sle_ssap_server.h"
#include "sle_device_discovery.h"
#include "sle_connection_manager.h"

static bool g_isBond = false;
#define SLE_ADV_HANDLE_DEFAULT 1
static uint8_t g_announceId = SLE_ADV_HANDLE_DEFAULT;
static IotcAdptSleSsapCallback g_sleSsapEventHandler = NULL;
static IotcAdptSleAnnounceSeekCallback g_sleAnnounceSeekEventHandler = NULL;
static IotcAdptSleConnectionCallback g_sleConnectionEventHandler = NULL;

static int32_t SleSetAnnounceData(uint8_t announceId, const IotcAdptSleAnnounceData *advData)
{

    sle_announce_data_t data_sdk = {
        .announce_data_len = advData->announceLength,
        .seek_rsp_data_len = advData->responceLength,
        .announce_data = advData->announceData,
        .seek_rsp_data = advData->responceData,
    };
    return sle_set_announce_data(announceId, &data_sdk);

}

static int32_t SleSetAnnounceParam(uint8_t announceId, const IotcAdptSleAnnounceParam *param)
{
    sle_announce_param_t param_sdk;
    param_sdk.announce_handle = param->handle,
    param_sdk.announce_mode = param->mode,
    param_sdk.announce_gt_role = param->role,
    param_sdk.announce_level = param->level,
    param_sdk.announce_interval_min = param->annonceIntervalMin,
    param_sdk.announce_interval_max = param->annonceIntervalMax,
    param_sdk.announce_channel_map = param->channelMap,
    param_sdk.announce_tx_power = param->txPower,
    param_sdk.conn_interval_min = param->connectIntervalMin,
    param_sdk.conn_interval_max = param->connectIntervalMax,
    param_sdk.conn_max_latency = param->connectLatency,
    param_sdk.conn_supervision_timeout = param->connectTimeout,
    param_sdk.ext_param = (void*)param->extParam,
    param_sdk.own_addr.type = (uint8_t)param->ownAddr.type;
    if (memcpy_s(param_sdk.own_addr.addr, sizeof(param_sdk.own_addr.addr), param->ownAddr.addr, sizeof(param_sdk.own_addr.addr)) != 0) {
        IOTC_LOGE("memcpy_s failed");
        return IOTC_ERROR;
    }
    param_sdk.peer_addr.type = (uint8_t)param->peerAddr.type;
    if (memcpy_s(param_sdk.peer_addr.addr, sizeof(param_sdk.peer_addr.addr), param->peerAddr.addr, sizeof(param_sdk.peer_addr.addr)) != 0) {
        IOTC_LOGE("memcpy_s failed");
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

uint64_t sle_clock_gettime_us_kh_lite(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL);
}


static int32_t sle_notify_indicate_sync(uint8_t serverId, uint16_t connectId, ssaps_ntf_ind_t *param)
{
    int32_t ret = -1;
    int sflag = 0;
    uint64_t kht_before_get_jiffies = 0, kht_after_get_jiffies = 0;
    uint64_t after_us = 0;
    uint32_t wait_min_time_us = (SLE_CONNECT_UPDATE_INTERVAL_HDI * 125);  // 最少等待1个发送间隔(可以是连接间隔)
    int retry = 2000; // 最多等待2000个连接间隔时间
    kht_before_get_jiffies = sle_clock_gettime_us_kh_lite();
    after_us = kht_before_get_jiffies + wait_min_time_us; // 防止溢出回卷, 等待超时时间

    while (1) {
        if (sflag == 0 && gle_tx_acb_data_num_get() > 0) {
            ret = ssaps_notify_indicate(serverId, connectId, param);
            sflag = 1;
        }

        kht_after_get_jiffies = sle_clock_gettime_us_kh_lite();
        if ((int)(kht_after_get_jiffies - after_us) < 0) { // 这里至少等待1个连接间隔(或发送间隔)
            sched_yield();
        } else {
            if (sflag == 0 && (--retry) > 0) {
                after_us += wait_min_time_us; // 没有发出去包的话 这里要继续等待下一个连接间隔(或发送间隔)
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
    if (memcpy_s(&addr_sdk.addr, sizeof(addr_sdk.addr), addr->addr, sizeof(addr_sdk.addr)) != 0) {
        IOTC_LOGE("memcpy_s failed");
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

static void AddServiceCb(uint8_t server_id, sle_uuid_t *uuid, uint16_t handle, uint32_t status)
{
     IOTC_LOGD("SLE AddServiceCb:status:%d,serverId:%d,Handle=%d", status, server_id, handle);
}

static void AddPropertyCb(uint8_t server_id, sle_uuid_t *uuid, uint16_t service_handle,
    uint16_t handle, uint32_t status)
{
    IOTC_LOGD("SLE AddPropertyCb:status:%d,serverId:%d,Handle=%d", status, server_id, handle);
}

static void AddDescriptorCb(uint8_t server_id, sle_uuid_t *uuid, uint16_t service_handle,
    uint16_t property_handle, uint32_t status)
{
    IOTC_LOGD("SLE AddDescriptorCb:status:%d,serverId:%d,Handle=%d", status, server_id, service_handle);
}

static IotcAdptSleStatus OhosStatusToAdapterStatus(int32_t status)
{
    return (status == IOTC_ADPT_SLE_STATUS_SUCCESS) ? IOTC_ADPT_SLE_STATUS_SUCCESS : IOTC_ADPT_SLE_STATUS_FAIL;
}

static void ServiceStartCb(uint8_t server_id, uint16_t handle, uint32_t status)
{
    IOTC_LOGD("SLE service start cb:status:%d,serverId:%d,Handle=%d", status, server_id, handle);
    IotcAdptSleSsapEventParam eventParam;
    eventParam.startSvc.status = OhosStatusToAdapterStatus(status);
    eventParam.startSvc.serverId = server_id;
    eventParam.startSvc.svcHandle = handle;
    if (g_sleSsapEventHandler != NULL &&
        g_sleSsapEventHandler(IOTC_ADPT_SLE_SSAP_EVENT_START_SVC_RESULT, &eventParam) != IOTC_OK) {
        IOTC_LOGE("doing gatt event");
    }
}

static void DeleteAllServiceCb(uint8_t server_id, uint32_t status)
{
    IOTC_LOGD("SLE DeleteAllServiceCb:status:%d,serverId:%d", status, server_id);

}

static void ReadRequestCb(uint8_t server_id, uint16_t conn_id, ssaps_req_read_cb_t *read_cb_para,
    uint32_t status)
{
    IOTC_LOGD("SLE  ReadRequestCb:status:%d,serverId:%d,conn_id=%d", status, server_id, conn_id);
}

static void WriteRequestCb(uint8_t server_id, uint16_t conn_id, ssaps_req_write_cb_t *write_cb_para,
    uint32_t status)
{
    IOTC_LOGD("SLE  WriteRequestCb:status:%d,serverId:%d,conn_id=%d", status, server_id, conn_id);

}

static void MtuChangeCb(uint8_t server_id, uint16_t conn_id, ssap_exchange_info_t *info, uint32_t status)
{
    IOTC_LOGD("SLE  MtuChangeCb:status:%d,serverId:%d,conn_id=%d", status, server_id, conn_id);
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

static void SleAnnounceEnableCallback(uint32_t announce_id, errcode_t status)
{
    IOTC_LOGD("SLE Announce Enable cb:announce_id:%d,status:%d", announce_id, status);
    IotcAdptSleAnnounceSeekEventParam param = {0};
    (void)memset_s(&param, sizeof(param), 0, sizeof(IotcAdptSleAnnounceSeekEventParam));
    param.announceEnable.announceId = announce_id;
    param.announceEnable.status = OhosStatusToAdapterStatus(status);
    if (g_sleAnnounceSeekEventHandler != NULL) {
        int32_t ret = g_sleAnnounceSeekEventHandler(IOTC_ADPT_SLE_ANNOUNCE_ENABLE_EVENT, &param);
        IOTC_LOGD("SLE Announce Enable cb:ret:%d", ret);
    }
}

static void SleAnnounceDisableCallback(uint32_t announce_id, errcode_t status)
{
    IOTC_LOGD("SLE Announce Disable cb:announce_id:%d,status:%d", announce_id, status);
    IotcAdptSleAnnounceSeekEventParam param = {0};
    (void)memset_s(&param, sizeof(param), 0, sizeof(IotcAdptSleAnnounceSeekEventParam));
    param.announceDisable.announceId = announce_id;
    param.announceDisable.status = OhosStatusToAdapterStatus(status);
    if (g_sleAnnounceSeekEventHandler != NULL) {
        int32_t ret = g_sleAnnounceSeekEventHandler(IOTC_ADPT_SLE_ANNOUNCE_DISABLE_EVENT, &param);
        IOTC_LOGD("SLE Announce Disable cb:ret:%d", ret);
    }
}

static void SleAnnounceTerminalCallback(uint32_t announce_id)
{
    IOTC_LOGD("SLE Announce Terminal cb:announce_id:%d", announce_id);
    IotcAdptSleAnnounceSeekEventParam param = {0};
    (void)memset_s(&param, sizeof(param), 0, sizeof(IotcAdptSleAnnounceSeekEventParam));
    param.announceTerminal.announceId = announce_id;
    if (g_sleAnnounceSeekEventHandler != NULL) {
        int32_t ret = g_sleAnnounceSeekEventHandler(IOTC_ADPT_SLE_ANNOUNCE_TERMINAL_EVENT, &param);
        IOTC_LOGD("SLE Announce Terminal cb:ret:%d", ret);
    }
}

static void SleAnnounceRemoveCallback(uint32_t announce_id, errcode_t status)
{
    IOTC_LOGD("SLE Announce Remove cb:announce_id:%d,status:%d", announce_id, status);
    IotcAdptSleAnnounceSeekEventParam param = {0};
    (void)memset_s(&param, sizeof(param), 0, sizeof(IotcAdptSleAnnounceSeekEventParam));
    param.announceRemove.announceId = announce_id;
    param.announceRemove.status = OhosStatusToAdapterStatus(status);
    if (g_sleAnnounceSeekEventHandler != NULL) {
        int32_t ret = g_sleAnnounceSeekEventHandler(IOTC_ADPT_SLE_ANNOUNCE_REMOVE_EVENT, &param);
        IOTC_LOGD("SLE Announce Remove cb:ret:%d", ret);
    }
}

static void SleStartSeekCallback(errcode_t status)
{
    IOTC_LOGD("SLE Start Seek cb:status:%d", status);
    IotcAdptSleAnnounceSeekEventParam param = {0};
    (void)memset_s(&param, sizeof(param), 0, sizeof(IotcAdptSleAnnounceSeekEventParam));
    param.startSeek.status = OhosStatusToAdapterStatus(status);
    if (g_sleAnnounceSeekEventHandler != NULL) {
        int32_t ret = g_sleAnnounceSeekEventHandler(IOTC_ADPT_SLE_SEEK_ENABLE_EVENT, &param);
        IOTC_LOGD("SLE Start Seek cb:ret:%d", ret);
    }
}

static void SleSeekDisableCallback(errcode_t status)
{
    IOTC_LOGD("SLE Seek Disable cb:status:%d", status);
    IotcAdptSleAnnounceSeekEventParam param = {0};
    (void)memset_s(&param, sizeof(param), 0, sizeof(IotcAdptSleAnnounceSeekEventParam));
    param.seekDisable.status = OhosStatusToAdapterStatus(status);
    if (g_sleAnnounceSeekEventHandler != NULL) {
        int32_t ret = g_sleAnnounceSeekEventHandler(IOTC_ADPT_SLE_SEEK_DISABLE_EVENT, &param);
        IOTC_LOGD("SLE Seek Disable cb:ret:%d", ret);
    }
}

static void SleSeekResultCallback(sle_seek_result_info_t *result)
{
    if (result == NULL) {
        IOTC_LOGE("invalid param");
        return;
    }

    IOTC_LOGD("SLE Seek Resul cb:event_type:%d, rssi:%d", result->event_type, result->rssi);
    IotcAdptSleAnnounceSeekEventParam param = {0};
    (void)memset_s(&param, sizeof(param), 0, sizeof(IotcAdptSleAnnounceSeekEventParam));
    param.seekResult.eventType = result->event_type;
    param.seekResult.addr.type = result->addr.type;
    (void)memcpy_s(param.seekResult.addr.addr, IOTC_ADPT_SLE_ADDR_LEN, result->addr.addr, SLE_ADDR_LEN);
    param.seekResult.directAddr.type = result->direct_addr.type;
    (void)memcpy_s(param.seekResult.directAddr.addr, IOTC_ADPT_SLE_ADDR_LEN, result->direct_addr.addr, SLE_ADDR_LEN);
    param.seekResult.rssi = result->rssi;
    param.seekResult.dataStatus = result->data_status;
    param.seekResult.dataLength = result->data_length;
    param.seekResult.data = IotcMalloc(result->data_length);
    if (param.seekResult.data == NULL) {
        IOTC_LOGE("malloc err len=%d", result->data_length);
        return;
    }
    (void)memcpy_s(param.seekResult.data, result->data_length, result->data, result->data_length);
    if (g_sleAnnounceSeekEventHandler != NULL) {
        int32_t ret = g_sleAnnounceSeekEventHandler(IOTC_ADPT_SLE_SEEK_RESULT_EVENT, &param);
        IOTC_LOGD("SLE Seek Resul cb:ret:%d", ret);
    }
}

static void SleEnableCallback(errcode_t status)
{
    IOTC_LOGD("SLE Enable cb:status:%d", status);
    IotcAdptSleAnnounceSeekEventParam param = {0};
    (void)memset_s(&param, sizeof(param), 0, sizeof(IotcAdptSleAnnounceSeekEventParam));
    param.sleEnable.status = OhosStatusToAdapterStatus(status);
    if (g_sleAnnounceSeekEventHandler != NULL) {
        int32_t ret = g_sleAnnounceSeekEventHandler(IOTC_ADPT_SLE_ENABLE_EVENT, &param);
        IOTC_LOGD("SLE Enable cb:ret:%d", ret);
    }
}

static void SleDisableCallback(errcode_t status)
{
    IOTC_LOGD("SLE Disabl cb:status:%d", status);
    IotcAdptSleAnnounceSeekEventParam param = {0};
    (void)memset_s(&param, sizeof(param), 0, sizeof(IotcAdptSleAnnounceSeekEventParam));
    param.sleDisable.status = OhosStatusToAdapterStatus(status);
    if (g_sleAnnounceSeekEventHandler != NULL) {
        int32_t ret = g_sleAnnounceSeekEventHandler(IOTC_ADPT_SLE_DISABLE_EVENT, &param);
        IOTC_LOGD("SLE Disabl cb:ret:%d", ret);
    }
}

static sle_announce_seek_callbacks_t g_SleAnnounceSeek = {
    .sle_enable_cb = SleEnableCallback,
    .sle_disable_cb = SleDisableCallback,
    .announce_enable_cb = SleAnnounceEnableCallback,
    .announce_disable_cb = SleAnnounceDisableCallback,
    .announce_terminal_cb = SleAnnounceTerminalCallback,
    .announce_remove_cb = SleAnnounceRemoveCallback,
    .seek_enable_cb = SleStartSeekCallback,
    .seek_disable_cb = SleSeekDisableCallback,
    .seek_result_cb = SleSeekResultCallback,
};

static void SleConnectStateChangedCallback(uint16_t conn_id, const sle_addr_t *addr,
    sle_acb_state_t conn_state, sle_pair_state_t pair_state, sle_disc_reason_t disc_reason)
{
    if (addr == NULL) {
        IOTC_LOGE("invalid param");
        return;
    }

    IOTC_LOGD("SLE ConnectStateChanged cb:conn_id:%d,conn_state:%d,pair_state:%d,disc_reason:%d",
        conn_id, conn_state, pair_state, disc_reason);
    IotcAdptSleConnectionEventParam param = {0};
    (void)memset_s(&param, sizeof(param), 0, sizeof(IotcAdptSleConnectionEventParam));
    param.sleConnectStateChanged.conn_id = conn_id;
    param.sleConnectStateChanged.addr.type = addr->type;
    (void)memcpy_s(param.sleConnectStateChanged.addr.addr, IOTC_ADPT_SLE_ADDR_LEN, addr->addr, SLE_ADDR_LEN);
    param.sleConnectStateChanged.conn_state = (IotcAdptSleAcbState)conn_state;
    param.sleConnectStateChanged.pair_state = (IotcAdptSlePairState)pair_state;
    param.sleConnectStateChanged.disc_reason = (IotcAdptSleDiscReason)disc_reason;
    if (g_sleConnectionEventHandler != NULL) {
        int32_t ret = g_sleConnectionEventHandler(IOTC_ADPT_SLE_CONNECT_STATE_CHANGED_EVENT, &param);
        IOTC_LOGD("SLE ConnectStateChanged cb:ret:%d", ret);
    }
}

static void SleConnectParamUpdateCallback(uint16_t conn_id, errcode_t status,
    const sle_connection_param_update_evt_t *param)
{
    if (param == NULL) {
        IOTC_LOGE("invalid param");
        return;
    }
    IOTC_LOGD("SLE ConnectParamUpdate cb:conn_id:%d,status:%d,param->interval:%d,param->latency:%d",
        conn_id, status, param->interval, param->latency);
    IotcAdptSleConnectionEventParam updateParam = {0};
    (void)memset_s(&updateParam, sizeof(updateParam), 0, sizeof(IotcAdptSleConnectionEventParam));
    updateParam.sleConnectParamUpdate.conn_id = conn_id;
    updateParam.sleConnectParamUpdate.status = OhosStatusToAdapterStatus(status);
    updateParam.sleConnectParamUpdate.param.interval = param->interval;
    updateParam.sleConnectParamUpdate.param.latency = param->latency;
    updateParam.sleConnectParamUpdate.param.supervision = param->supervision;
    if (g_sleConnectionEventHandler != NULL) {
        int32_t ret = g_sleConnectionEventHandler(IOTC_ADPT_SLE_CONNECT_PARAM_UPDATE_EVENT, &updateParam);
        IOTC_LOGD("SLE ConnectParamUpdate cb:ret:%d", ret);
    }
}

static void SleConnectParamUpdateReqCallback(uint16_t conn_id, errcode_t status,
    const sle_connection_param_update_req_t *param)
{
    if (param == NULL) {
        IOTC_LOGE("invalid param");
        return;
    }
    IOTC_LOGD("SLE ConnectParamUpdateReq cb:conn_id:%d,status:%d", conn_id, status);
    IotcAdptSleConnectionEventParam updateReqParam = {0};
    (void)memset_s(&updateReqParam, sizeof(updateReqParam), 0, sizeof(IotcAdptSleConnectionEventParam));
    updateReqParam.sleConnectParamUpdateReq.conn_id = conn_id;
    updateReqParam.sleConnectParamUpdateReq.status = OhosStatusToAdapterStatus(status);
    updateReqParam.sleConnectParamUpdateReq.param.intervalMin = param->interval_min;
    updateReqParam.sleConnectParamUpdateReq.param.intervalMax = param->interval_max;
    updateReqParam.sleConnectParamUpdateReq.param.maxLatency = param->max_latency;
    updateReqParam.sleConnectParamUpdateReq.param.supervisionTimeout = param->supervision_timeout;
    if (g_sleConnectionEventHandler != NULL) {
        int32_t ret = g_sleConnectionEventHandler(IOTC_ADPT_SLE_CONNECT_PARAM_UPDATE_REQ_EVENT, &updateReqParam);
        IOTC_LOGD("SLE ConnectParamUpdateReq cb:ret:%d", ret);
    }
}

static void SleAuthCompleteCallback(uint16_t conn_id, const sle_addr_t *addr, errcode_t status,
    const sle_auth_info_evt_t* evt)
{
    if (addr == NULL || evt == NULL) {
        IOTC_LOGE("invalid param");
        return;
    }
    IOTC_LOGD("SLE AuthComplete cb:conn_id:%d,status:%d", conn_id, status);
    IotcAdptSleConnectionEventParam authParam = {0};
    (void)memset_s(&authParam, sizeof(authParam), 0, sizeof(IotcAdptSleConnectionEventParam));
    authParam.sleAuthComplete.conn_id = conn_id;
    authParam.sleAuthComplete.status = OhosStatusToAdapterStatus(status);
    authParam.sleAuthComplete.addr.type = addr->type;
    (void)memcpy_s(authParam.sleAuthComplete.addr.addr, IOTC_ADPT_SLE_ADDR_LEN, addr->addr, SLE_ADDR_LEN);
    authParam.sleAuthComplete.evt.cryptoAlgo = evt->crypto_algo;
    authParam.sleAuthComplete.evt.integrChkInd = evt->integr_chk_ind;
    authParam.sleAuthComplete.evt.keyDerivAlgo = evt->key_deriv_algo;
    (void)memcpy_s(authParam.sleAuthComplete.evt.linkKey, IOTC_ADPT_SLE_LINK_KEY_LEN, evt->link_key, SLE_LINK_KEY_LEN);
    if (g_sleConnectionEventHandler != NULL) {
        int32_t ret = g_sleConnectionEventHandler(IOTC_ADPT_SLE_AUTH_COMPLETE_EVENT, &authParam);
        IOTC_LOGD("SLE AuthComplete cb:ret:%d", ret);
    }
}

static void SlePairCompleteCallback(uint16_t conn_id, const sle_addr_t *addr, errcode_t status)
{
    if (addr == NULL) {
        IOTC_LOGE("invalid param");
        return;
    }
    IOTC_LOGD("SLE PairComplete cb:conn_id:%d,status:%d", conn_id, status);
    IotcAdptSleConnectionEventParam pairParam = {0};
    (void)memset_s(&pairParam, sizeof(pairParam), 0, sizeof(IotcAdptSleConnectionEventParam));
    pairParam.slePairComplete.conn_id = conn_id;
    pairParam.slePairComplete.status = OhosStatusToAdapterStatus(status);
    pairParam.slePairComplete.addr.type = addr->type;
    (void)memcpy_s(pairParam.slePairComplete.addr.addr, IOTC_ADPT_SLE_ADDR_LEN, addr->addr, SLE_ADDR_LEN);
    if (g_sleConnectionEventHandler != NULL) {
        int32_t ret = g_sleConnectionEventHandler(IOTC_ADPT_SLE_PAIR_COMPLETE_EVENT, &pairParam);
        IOTC_LOGD("SLE PairComplete cb:ret:%d", ret);
    }
}

static void SleReadRssiCallback(uint16_t conn_id, int8_t rssi, errcode_t status)
{
    IOTC_LOGD("SLE PairComplete cb:conn_id:%d,status:%d, rssi:%d", conn_id, status, rssi);
    IotcAdptSleConnectionEventParam rssiParam = {0};
    (void)memset_s(&rssiParam, sizeof(rssiParam), 0, sizeof(IotcAdptSleConnectionEventParam));
    rssiParam.sleReadRssi.conn_id = conn_id;
    rssiParam.sleReadRssi.rssi = rssi;
    rssiParam.sleReadRssi.status = OhosStatusToAdapterStatus(status);
    if (g_sleConnectionEventHandler != NULL) {
        int32_t ret = g_sleConnectionEventHandler(IOTC_ADPT_SLE_READ_RSSI_EVENT, &rssiParam);
        IOTC_LOGD("SLE PairComplete cb:ret:%d", ret);
    }
}

static void SleLowLatencyCallback(uint8_t status, sle_addr_t *addr, uint8_t rate)
{
    if (addr == NULL) {
        IOTC_LOGE("invalid param");
        return;
    }
    IOTC_LOGD("SLE LowLatency cb:rate:%d,status:%d", rate, status);
    IotcAdptSleConnectionEventParam param = {0};
    (void)memset_s(&param, sizeof(param), 0, sizeof(IotcAdptSleConnectionEventParam));
    param.sleLowLatency.status = OhosStatusToAdapterStatus(status);
    param.sleLowLatency.rate = rate;
    param.sleLowLatency.addr.type = addr->type;
    (void)memcpy_s(param.sleLowLatency.addr.addr, IOTC_ADPT_SLE_ADDR_LEN, addr->addr, SLE_ADDR_LEN);
    if (g_sleConnectionEventHandler != NULL) {
        int32_t ret = g_sleConnectionEventHandler(IOTC_ADPT_SLE_LOW_LATENCY_EVENT, &param);
        IOTC_LOGD("SLE LowLatency cb:ret:%d", ret);
    }
}

static void SleSetPhyCallback(uint16_t conn_id, errcode_t status, const sle_set_phy_t *param)
{
    if (param == NULL) {
        IOTC_LOGE("invalid param");
        return;
    }
    IOTC_LOGD("SLE SetPhy cb:conn_id:%d,status:%d", conn_id, status);
    IotcAdptSleConnectionEventParam phyParam = {0};
    (void)memset_s(&phyParam, sizeof(phyParam), 0, sizeof(IotcAdptSleConnectionEventParam));
    phyParam.sleSetPhy.conn_id = conn_id;
    phyParam.sleSetPhy.status = status;
    phyParam.sleSetPhy.param.gFeedback = param->g_feedback;
    phyParam.sleSetPhy.param.rxFormat = param->rx_format;
    phyParam.sleSetPhy.param.rxPhy = param->rx_phy;
    phyParam.sleSetPhy.param.rxPilotDensity = param->rx_pilot_density;
    phyParam.sleSetPhy.param.tFeedback = param->t_feedback;
    phyParam.sleSetPhy.param.txFormat = param->tx_format;
    phyParam.sleSetPhy.param.txPhy = param->tx_phy;
    phyParam.sleSetPhy.param.txPilotDensity = param->tx_pilot_density;
    if (g_sleConnectionEventHandler != NULL) {
        int32_t ret = g_sleConnectionEventHandler(IOTC_ADPT_SLE_SET_PHY_EVENT, &phyParam);
        IOTC_LOGD("SLE SetPhy cb:ret:%d", ret);
    }
}

static sle_connection_callbacks_t g_SleConnectionCb = {
    .connect_state_changed_cb = SleConnectStateChangedCallback,
    .connect_param_update_req_cb = SleConnectParamUpdateReqCallback,
    .connect_param_update_cb = SleConnectParamUpdateCallback,
    .auth_complete_cb = SleAuthCompleteCallback,
    .pair_complete_cb = SlePairCompleteCallback,
    .read_rssi_cb = SleReadRssiCallback,
    .low_latency_cb = SleLowLatencyCallback,
    .set_phy_cb = SleSetPhyCallback,
};

static int32_t SsapsRegisterCallbacks(ssaps_callbacks_t *cb)
{
    return ssaps_register_callbacks(cb);
}

static int32_t AnnounceSeekRegisterCallbacks(sle_announce_seek_callbacks_t *cb)
{
    return sle_announce_seek_register_callbacks(cb);
}

static int32_t ConnectionRegisterCallbacks(sle_connection_callbacks_t *cb)
{
    return sle_connection_register_callbacks(cb);
}

static int32_t SetGapSecurityParam(bool flag)
{
    (void)flag;
    return ERRCODE_SLE_SUCCESS;
}

static uint32_t SleSetSeekParam(sle_seek_param_t *param)
{
    uint32_t ret = sle_set_seek_param(param);
    IOTC_LOGI("sle_set_seek_param ret = %{public}d", ret);
    return ret;
}

static uint32_t SleStartSeek(void)
{
    uint32_t ret = sle_start_seek();
    IOTC_LOGI("sle_start_seek ret = %{public}d", ret);
    return ret;
}

static uint32_t SleStopSeek(void)
{
    uint32_t ret = sle_stop_seek();
    IOTC_LOGI("sle_stop_seek ret = %{public}d", ret);
    return ret;
}

static uint32_t SleConnectRemoteDevice(const sle_addr_t *addr)
{
    uint32_t ret = sle_connect_remote_device(addr);
    IOTC_LOGI("sle_connect_remote_device ret = %{public}d", ret);
    return ret;
}

static uint32_t SleDisconnectRemoteDevice(const sle_addr_t *addr)
{
    uint32_t ret = sle_disconnect_remote_device(addr);
    IOTC_LOGI("sle_disconnect_remote_device ret = %{public}d", ret);
    return ret;
}

static uint32_t SleDefaultConnectionParamSet(sle_default_connect_param_t *set_param)
{
    uint32_t ret = sle_default_connection_param_set(set_param);
    IOTC_LOGI("sle_default_connection_param_set ret = %{public}d", ret);
    return ret;
}

/***************************************************************************************************/

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
        return IOTC_ERR_PARAM_INVALID;
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
        return IOTC_ERR_PARAM_INVALID;
    }
    g_sleSsapEventHandler = callback;
    //TODO 注册回调
    int32_t ret =  SsapsRegisterCallbacks(&g_sleSsapsCb);
    if (ret != IOTC_ADPT_SLE_STATUS_SUCCESS) {
        IOTC_LOGE("register ssap callback ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t IotcSleRegisterAnnounceSeekCallbacks(const IotcAdptSleAnnounceSeekCallback callback)
{
    if (callback == NULL) {
        IOTC_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    g_sleAnnounceSeekEventHandler = callback;
    int32_t ret = AnnounceSeekRegisterCallbacks(&g_SleAnnounceSeek);
    if (ret != IOTC_ADPT_SLE_STATUS_SUCCESS) {
        IOTC_LOGE("register announce seek callback ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t IotcSleRegisterConnectionCallbacks(const IotcAdptSleConnectionCallback callback)
{
    if (callback == NULL) {
        IOTC_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    g_sleConnectionEventHandler = callback;
    int32_t ret = ConnectionRegisterCallbacks(&g_SleConnectionCb);
    if (ret != IOTC_ADPT_SLE_STATUS_SUCCESS) {
        IOTC_LOGE("register connection callback ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t IotcSleSetSleName(const uint8_t *name, uint8_t len)
{
    if (name == NULL) {
        IOTC_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    int32_t ret = SleSetLocalName(name, len);
    if (ret != IOTC_ADPT_SLE_STATUS_SUCCESS) {
        IOTC_LOGE("set name ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t IotcSleStartAdv(const IotcAdptSleAnnounceParam *advParam, const IotcAdptSleAnnounceData *advData)
{
    if ((advParam == NULL) || (advData == NULL)) {
        IOTC_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    int32_t ret = SleSetAnnounceData(g_announceId, advData);
    if (ret != ERRCODE_SLE_SUCCESS) {
        IOTC_LOGE("start adv set data=%d", ret);
        return ret;
    }

    int32_t paramRet = SleSetAnnounceParam(g_announceId, advParam);
    if (paramRet != ERRCODE_SLE_SUCCESS) {
        IOTC_LOGE("start adv set param=%d", paramRet);
        return paramRet;
    }

    ret = SleStartAnnounce(g_announceId);
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
        return IOTC_ERR_PARAM_INVALID;
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
        return IOTC_ERR_PARAM_INVALID;
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
        return IOTC_ERR_PARAM_INVALID;
    }
    IotcAdptSleAddr iotcAddr = {0};
    if (memcpy_s(iotcAddr.addr, SLE_ADDR_LEN, bdAddr, addrLen) != EOK) {
        IOTC_LOGE("memcpy");
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    //TODO  type?
    int32_t ret = SlePairRemoteDevice(&iotcAddr);
    if (ret != ERRCODE_SLE_SUCCESS) {
        IOTC_LOGE("gatt disconnect ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t IotcSleSetSeekParam(const IotcAdptSleSeekParam *param)
{
    if (param == NULL) {
        IOTC_LOGE("invalid param");
        return IOTC_ERROR;
    }

    sle_seek_param_t seekParam = {0};
    (void)memset_s(&seekParam, sizeof(seekParam), 0, sizeof(seekParam));
    seekParam.filter_duplicates = param->filterduplicates;
    seekParam.own_addr_type = param->ownaddrtype;
    seekParam.seek_filter_policy = param->seekfilterpolicy;
    int32_t ret = memcpy_s(seekParam.seek_interval, sizeof(seekParam.seek_interval),
                        param->seekInterval, sizeof(param->seekInterval));
    if (ret != EOK) {
        IOTC_LOGE("memcpy_s err");
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    seekParam.seek_phys = param->seekphys;
    ret = memcpy_s(seekParam.seek_type, sizeof(seekParam.seek_type),
                        param->seekType, sizeof(param->seekType));
    if (ret != EOK) {
        IOTC_LOGE("memcpy_s err");
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    ret = memcpy_s(seekParam.seek_window, sizeof(seekParam.seek_window),
                        param->seekWindow, sizeof(param->seekWindow));
    if (ret != EOK) {
        IOTC_LOGE("memcpy_s err");
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    return SleSetSeekParam(&seekParam);
}

int32_t IotcSleStartSeek(void)
{
    return SleStartSeek();
}

int32_t IotcSleStoptSeek(void)
{
    return SleStopSeek();
}

int32_t IotcSleConnectRemoteDevice(const IotcAdptSleAddr *addr)
{
    if (addr == NULL) {
        IOTC_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    sle_addr_t sleAddr = {0};
    (void)memset_s(&sleAddr, sizeof(sleAddr), 0, sizeof(sleAddr));
    sleAddr.type = addr->type;
    int32_t ret = memcpy_s(sleAddr.addr, sizeof(sleAddr.addr), addr->addr, sizeof(addr->addr));
    if (ret != EOK) {
        IOTC_LOGE("memcpy_s ret = %{public}d", ret);
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    return SleConnectRemoteDevice(&sleAddr);
}

int32_t IotcSleDisconnectRemoteDevice(const IotcAdptSleAddr *addr)
{
    if (addr == NULL) {
        IOTC_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    sle_addr_t sleAddr = {0};
    (void)memset_s(&sleAddr, sizeof(sleAddr), 0, sizeof(sleAddr));
    sleAddr.type = addr->type;
    int32_t ret = memcpy_s(sleAddr.addr, sizeof(sleAddr.addr), addr->addr, sizeof(addr->addr));
    if (ret != EOK) {
        IOTC_LOGE("memcpy_s ret = %{public}d", ret);
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    return SleDisconnectRemoteDevice(&sleAddr);
}

int32_t IotcSleDefaultConnectionParamSet(const IotcAdptSleDefaultConnectParam *param)
{
    sle_default_connect_param_t connParam = {0};
    (void)memset_s(&connParam, sizeof(connParam), 0, sizeof(sle_default_connect_param_t));
    connParam.enable_filter_policy = param->enableFilterPolicy;
    connParam.gt_negotiate = param->gtNegotiate;
    connParam.initiate_phys = param->initiatePhys;
    connParam.max_interval = param->maxInterval;
    connParam.min_interval = param->minInterval;
    connParam.scan_interval = param->scanInterval;
    connParam.scan_window = param->scanWindow;
    connParam.timeout = param->timeout;
    return SleDefaultConnectionParamSet(&connParam);
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