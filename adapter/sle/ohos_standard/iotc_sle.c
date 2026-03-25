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
#include "iotc_sle_ssap_common.h"
#include "iotc_sle_client.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <pthread.h>
#include "securec.h"
#include "iotc_mem.h"
#include "iotc_errcode.h"
#include "iotc_log.h"

#include "sle_errcode.h"
#include "sle_device_discovery.h"
#include "sle_connection_manager.h"
#include "sle_ssap_client.h"

#define SLE_ADDR_LEN 6
#define SLE_WRITE_PARAM_HANLDE 2

static bool g_isBond = false;
static IotcAdptSleSsapCallback g_sleSsapEventHandler = NULL;
static IotcAdptSleAnnounceSeekCallback g_sleAnnounceSeekEventHandler = NULL;
static IotcAdptSleConnectionCallback g_sleConnectionEventHandler = NULL;
static IotcAdptSleSsapClientCallback g_sleSsapClientEventHandler = NULL;

typedef enum {
    IOTC_SLE_ENABLE = 0,
    IOTC_SLE_SEEK_ENABLE,
    IOTC_SLE_SEEK_STOP,
    IOTC_SLE_STOP,
} IotcClientState;

static IotcClientState g_clientState = IOTC_SLE_STOP;

#define SPEED_DEFAULT_CONN_INTERVAL 0x09
#define SPEED_DEFAULT_TIMEOUT_MULTIPLIER 0x1f4
#define SPEED_DEFAULT_SCAN_INTERVAL 400
#define SPEED_DEFAULT_SCAN_WINDOW 20

static int32_t IotcSleStartServiceEx(uint8_t *serverId)
{
    int32_t ret;
    sleUUID appUuid = {0};
    appUuid.len = SLE_UUID_LEN;

    const uint8_t fixedUuid[SLE_UUID_LEN] = {
        0x39, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA, 0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    if (memcpy_s(appUuid.id, SLE_UUID_LEN, fixedUuid, SLE_UUID_LEN) != EOK) {
        return IOTC_ERROR;
    }

    ret = IotcSleSsapcRegister(&appUuid, serverId);
    IOTC_LOGI("[FIXED UUID] ssapc_register ret:0x%08X", ret);
    return ret;
}

int32_t IotcSleSsapsStartServiceExt(IotcAdptSleSsapService *svc, uint8_t svcNum)
{
    if ((svc == NULL) || (svcNum == 0)) {
        IOTC_LOGE("IotcSleSsapsStartService invalid param");
        return IOTC_ERROR;
    }

    for (uint8_t i = 0; i < svcNum; i++) {
        int32_t ret = IotcSleStartServiceEx(&svc[i].serverId);
        if (ret != IOTC_OK) {
            IOTC_LOGE("sle start service ret=%d, serverId = %d", ret, svc[i].serverId);
            return ret;
        }
    }

    return IOTC_OK;
}

#define SLE_CONNECT_UPDATE_INTERVAL_HDI 20

static uint64_t IotcSleClockGettimeUsLite(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL);
}

uint8_t gle_tx_acb_data_num_get(void);

pthread_mutex_t g_sendMutexClient;

int32_t SlePairRemoteDevice(const IotcAdptSleDeviceAddr *addr)
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

static int32_t SleSetLocalName(const char *name, uint8_t len)
{
    return sle_set_local_name((uint8_t *)name, len);
}

static IotcAdptSleStatus OhosStatusToAdapterStatus(int32_t status)
{
    return (status == IOTC_ADPT_SLE_STATUS_SUCCESS) ? IOTC_ADPT_SLE_STATUS_SUCCESS : IOTC_ADPT_SLE_STATUS_FAIL;
}

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

static void SleConnectStateChangedCallback(uint16_t conn_id, const sle_addr_t *addr, sle_acb_state_t conn_state,
    sle_pair_state_t pair_state, sle_disc_reason_t disc_reason)
{
    if (addr == NULL) {
        IOTC_LOGE("invalid param");
        return;
    }

    IOTC_LOGD("SLE ConnectStateChanged cb:conn_id:%d,conn_state:%d,pair_state:%d,disc_reason:%d", conn_id, conn_state,
        pair_state, disc_reason);
    IotcAdptSleConnectionEventParam param = {0};
    (void)memset_s(&param, sizeof(param), 0, sizeof(IotcAdptSleConnectionEventParam));
    param.sleConnectStateChanged.connId = conn_id;
    param.sleConnectStateChanged.addr.type = addr->type;
    (void)memcpy_s(param.sleConnectStateChanged.addr.addr, IOTC_ADPT_SLE_ADDR_LEN, addr->addr, SLE_ADDR_LEN);
    param.sleConnectStateChanged.connState = (IotcAdptSleAcbState)conn_state;
    param.sleConnectStateChanged.pairState = (IotcAdptSlePairState)pair_state;
    param.sleConnectStateChanged.discReason = (IotcAdptSleDiscReason)disc_reason;
    if (g_sleConnectionEventHandler != NULL) {
        int32_t ret = g_sleConnectionEventHandler(IOTC_ADPT_SLE_CONNECT_STATE_CHANGED_EVENT, &param);
        IOTC_LOGD("SLE ConnectStateChanged cb:ret:%d", ret);
    }
}

static void SleConnectParamUpdateCallback(
    uint16_t conn_id, errcode_t status, const sle_connection_param_update_evt_t *param)
{
    if (param == NULL) {
        IOTC_LOGE("invalid param");
        return;
    }
    IOTC_LOGD("SLE ConnectParamUpdate cb:conn_id:%d,status:%d,param->interval:%d,param->latency:%d", conn_id, status,
        param->interval, param->latency);
    IotcAdptSleConnectionEventParam updateParam = {0};
    (void)memset_s(&updateParam, sizeof(updateParam), 0, sizeof(IotcAdptSleConnectionEventParam));
    updateParam.sleConnectParamUpdate.connId = conn_id;
    updateParam.sleConnectParamUpdate.status = OhosStatusToAdapterStatus(status);
    updateParam.sleConnectParamUpdate.param.interval = param->interval;
    updateParam.sleConnectParamUpdate.param.latency = param->latency;
    updateParam.sleConnectParamUpdate.param.supervision = param->supervision;
    if (g_sleConnectionEventHandler != NULL) {
        int32_t ret = g_sleConnectionEventHandler(IOTC_ADPT_SLE_CONNECT_PARAM_UPDATE_EVENT, &updateParam);
        IOTC_LOGD("SLE ConnectParamUpdate cb:ret:%d", ret);
    }
}

static void SleConnectParamUpdateReqCallback(
    uint16_t conn_id, errcode_t status, const sle_connection_param_update_req_t *param)
{
    if (param == NULL) {
        IOTC_LOGE("invalid param");
        return;
    }
    IOTC_LOGD("SLE ConnectParamUpdateReq cb:conn_id:%d,status:%d", conn_id, status);
    IotcAdptSleConnectionEventParam updateReqParam = {0};
    (void)memset_s(&updateReqParam, sizeof(updateReqParam), 0, sizeof(IotcAdptSleConnectionEventParam));
    updateReqParam.sleConnectParamUpdateReq.connId = conn_id;
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

static void SleAuthCompleteCallback(
    uint16_t conn_id, const sle_addr_t *addr, errcode_t status, const sle_auth_info_evt_t *evt)
{
    if (addr == NULL || evt == NULL) {
        IOTC_LOGE("invalid param");
        return;
    }
    IOTC_LOGD("SLE AuthComplete cb:conn_id:%d,status:%d", conn_id, status);
    IotcAdptSleConnectionEventParam authParam = {0};
    (void)memset_s(&authParam, sizeof(authParam), 0, sizeof(IotcAdptSleConnectionEventParam));
    authParam.sleAuthComplete.connId = conn_id;
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
    pairParam.slePairComplete.connId = conn_id;
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
    rssiParam.sleReadRssi.connId = conn_id;
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
    phyParam.sleSetPhy.connId = conn_id;
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

void SsapcFindStructureCallback(
    uint8_t client_id, uint16_t conn_id, ssapc_find_service_result_t *service, errcode_t status)
{
    if (service == NULL) {
        IOTC_LOGE("invalid param");
        return;
    }
    IOTC_LOGD("Ssapc Find Structure cb:client_id:%d,conn_id:%d,status:%d", client_id, conn_id, status);
    IotcAdptSleSsapClientEventParam param = {0};
    (void)memset_s(&param, sizeof(param), 0, sizeof(IotcAdptSleSsapClientEventParam));
    param.ssapcFindServiceResult.clientId = client_id;
    param.ssapcFindServiceResult.connId = conn_id;
    param.ssapcFindServiceResult.service.startHdl = service->starthdl;
    param.ssapcFindServiceResult.service.endHdl = service->endhdl;
    param.ssapcFindServiceResult.service.uuid.len = service->uuid.len;
    (void)memcpy_s(param.ssapcFindServiceResult.service.uuid.id, SLE_UUID_LEN, service->uuid.uuid, SLE_ADDR_LEN);
    param.ssapcFindServiceResult.status = OhosStatusToAdapterStatus(status);
    if (g_sleSsapClientEventHandler != NULL) {
        int32_t ret = g_sleSsapClientEventHandler(IOTC_ADPT_SLE_SSAPC_FIND_STRUCTURE_EVENT, &param);
        IOTC_LOGD("Ssapc Find Structure cb:ret:%d", ret);
    }
}

void SsapcFindPropertyCallback(
    uint8_t client_id, uint16_t conn_id, ssapc_find_property_result_t *property, errcode_t status)
{
    if (property == NULL) {
        IOTC_LOGE("invalid param");
        return;
    }

    IOTC_LOGD("Ssapc Find Property cb:client_id:%d,conn_id:%d,status:%d, handle:%d", client_id, conn_id, status,
        property->handle);
    IotcAdptSleSsapClientEventParam param = {0};
    (void)memset_s(&param, sizeof(param), 0, sizeof(IotcAdptSleSsapClientEventParam));
    param.ssapcFindPropertyResult.clientId = client_id;
    param.ssapcFindPropertyResult.connId = conn_id;
    param.ssapcFindPropertyResult.property.handle = property->handle;
    param.ssapcFindPropertyResult.property.operateIndication = property->operate_indication;
    param.ssapcFindPropertyResult.property.uuid.len = property->uuid.len;
    (void)memcpy_s(param.ssapcFindPropertyResult.property.uuid.id, SLE_UUID_LEN, property->uuid.uuid, SLE_ADDR_LEN);
    if (property->descriptors_count > 0) {
        param.ssapcFindPropertyResult.property.descriptorsCount = property->descriptors_count;
        param.ssapcFindPropertyResult.property.descriptorsType = IotcMalloc(property->descriptors_count);
        (void)memcpy_s(param.ssapcFindPropertyResult.property.descriptorsType, property->descriptors_count,
            property->descriptors_type, property->descriptors_count);
    }
    param.ssapcFindPropertyResult.status = OhosStatusToAdapterStatus(status);
    if (g_sleSsapClientEventHandler != NULL) {
        int32_t ret = g_sleSsapClientEventHandler(IOTC_ADPT_SLE_SSAPC_FIND_PROPERTY_EVENT, &param);
        IOTC_LOGD("Ssapc Find Property cb:ret:%d", ret);
    }
}

void SsapcFindStructureCompleteCallback(
    uint8_t client_id, uint16_t conn_id, ssapc_find_structure_result_t *structure_result, errcode_t status)
{
    if (structure_result == NULL) {
        IOTC_LOGE("invalid param");
        return;
    }
    IOTC_LOGD("Ssapc Find Structure Complete cb:client_id:%d,conn_id:%d,status:%d", client_id, conn_id, status);
    IotcAdptSleSsapClientEventParam param = {0};
    (void)memset_s(&param, sizeof(param), 0, sizeof(IotcAdptSleSsapClientEventParam));
    param.ssapcFindStructureResult.clientId = client_id;
    param.ssapcFindStructureResult.connId = conn_id;
    param.ssapcFindStructureResult.structureResult.type = structure_result->type;
    param.ssapcFindStructureResult.structureResult.uuid.len = structure_result->uuid.len;
    (void)memcpy_s(param.ssapcFindStructureResult.structureResult.uuid.id, SLE_UUID_LEN, structure_result->uuid.uuid,
        SLE_ADDR_LEN);
    param.ssapcFindStructureResult.status = OhosStatusToAdapterStatus(status);
    if (g_sleSsapClientEventHandler != NULL) {
        int32_t ret = g_sleSsapClientEventHandler(IOTC_ADPT_SLE_SSAPC_FIND_STRUCTURE_COMPLETE_EVENT, &param);
        IOTC_LOGD("Ssapc Find Structure Complete cb:ret:%d", ret);
    }
}

void SsapcReadCfmCallback(uint8_t client_id, uint16_t conn_id, ssapc_handle_value_t *read_data, errcode_t status)
{
    if (read_data == NULL) {
        IOTC_LOGE("invalid param");
        return;
    }
    IOTC_LOGD("Ssapc Read Cfm cb:client_id:%d,conn_id:%d,status:%d", client_id, conn_id, status);
    IotcAdptSleSsapClientEventParam param = {0};
    (void)memset_s(&param, sizeof(param), 0, sizeof(IotcAdptSleSsapClientEventParam));
    param.ssapcHandleValue.clientId = client_id;
    param.ssapcHandleValue.connId = conn_id;
    param.ssapcHandleValue.readData.handle = read_data->handle;
    param.ssapcHandleValue.readData.type = read_data->type;
    if (read_data->data_len > 0) {
        param.ssapcHandleValue.readData.dataLen = read_data->data_len;
        param.ssapcHandleValue.readData.data = IotcMalloc(read_data->data_len);
        (void)memcpy_s(
            param.ssapcHandleValue.readData.data, read_data->data_len, read_data->data, read_data->data_len);
    }
    param.ssapcHandleValue.status = OhosStatusToAdapterStatus(status);
    if (g_sleSsapClientEventHandler != NULL) {
        int32_t ret = g_sleSsapClientEventHandler(IOTC_ADPT_SLE_SSAPC_READ_CFM_EVENT, &param);
        IOTC_LOGD("Ssapc Read Cfm Complete cb:ret:%d", ret);
    }
}

void SsapcReadByUuidCompleteCallback(
    uint8_t client_id, uint16_t conn_id, ssapc_read_by_uuid_cmp_result_t *cmp_result, errcode_t status)
{
    if (cmp_result == NULL) {
        IOTC_LOGE("invalid param");
        return;
    }
    IOTC_LOGD("Ssapc Read By Uuid Complete cb:client_id:%d,conn_id:%d,status:%d", client_id, conn_id, status);
    IotcAdptSleSsapClientEventParam param = {0};
    (void)memset_s(&param, sizeof(param), 0, sizeof(IotcAdptSleSsapClientEventParam));
    param.ssapcReadByUuidCmpResult.clientId = client_id;
    param.ssapcReadByUuidCmpResult.connId = conn_id;
    param.ssapcReadByUuidCmpResult.cmpResult.type = cmp_result->type;
    param.ssapcReadByUuidCmpResult.cmpResult.uuid.len = cmp_result->uuid.len;
    (void)memcpy_s(
        param.ssapcReadByUuidCmpResult.cmpResult.uuid.id, SLE_UUID_LEN, cmp_result->uuid.uuid, SLE_ADDR_LEN);
    param.ssapcReadByUuidCmpResult.status = OhosStatusToAdapterStatus(status);
    if (g_sleSsapClientEventHandler != NULL) {
        int32_t ret = g_sleSsapClientEventHandler(IOTC_ADPT_SSAPC_READ_BY_UUID_COMPLETE_EVENT, &param);
        IOTC_LOGD("Ssapc Read By Uuid Complete cb:ret:%d", ret);
    }
}

void SsapcWriteCfmCallback(uint8_t client_id, uint16_t conn_id, ssapc_write_result_t *write_result, errcode_t status)
{
    if (write_result == NULL) {
        IOTC_LOGE("invalid param");
        return;
    }
    IOTC_LOGI("Ssapc Write Cfm cb:client_id:%d,conn_id:%d,status:%d", client_id, conn_id, status);
    IotcAdptSleSsapClientEventParam param = {0};
    (void)memset_s(&param, sizeof(param), 0, sizeof(IotcAdptSleSsapClientEventParam));
    param.ssapcWriteResult.clientId = client_id;
    param.ssapcWriteResult.connId = conn_id;
    param.ssapcWriteResult.writeResult.type = write_result->type;
    param.ssapcWriteResult.writeResult.handle = write_result->handle;
    param.ssapcWriteResult.status = OhosStatusToAdapterStatus(status);
    if (g_sleSsapClientEventHandler != NULL) {
        int32_t ret = g_sleSsapClientEventHandler(IOTC_ADPT_SSAPC_WRITE_CFM_EVENT, &param);
        IOTC_LOGD("Ssapc Write Cfm  cb:ret:%d", ret);
    }
}
void SsapcExchangeInfoCallback(
    uint8_t client_id, uint16_t conn_id, ssap_exchange_info_t *exchange_info, errcode_t status)
{
    if (exchange_info == NULL) {
        IOTC_LOGE("invalid param");
        return;
    }
    IOTC_LOGI("Ssapc Exchange Info cb:client_id:%d,conn_id:%d,status:%d", client_id, conn_id, status);
    IotcAdptSleSsapClientEventParam param = {0};
    (void)memset_s(&param, sizeof(param), 0, sizeof(IotcAdptSleSsapClientEventParam));
    param.ssapExchangeInfo.clientId = client_id;
    param.ssapExchangeInfo.connId = conn_id;
    param.ssapExchangeInfo.exInfo.mtuSize = exchange_info->mtu_size;
    param.ssapExchangeInfo.exInfo.version = exchange_info->version;
    param.ssapExchangeInfo.status = OhosStatusToAdapterStatus(status);
    if (g_sleSsapClientEventHandler != NULL) {
        int32_t ret = g_sleSsapClientEventHandler(IOTC_ADPT_SSAPC_EXCHANGE_INFO_EVENT, &param);
        IOTC_LOGD("Ssapc Exchange Info  cb:ret:%d", ret);
    }
}

void SsapcNotificationCallback(uint8_t client_id, uint16_t conn_id, ssapc_handle_value_t *data, errcode_t status)
{
    if (data == NULL) {
        IOTC_LOGE("invalid param");
        return;
    }
    IOTC_LOGI("Ssapc Notification cb:client_id:%d,conn_id:%d,status:%d", client_id, conn_id, status);
    IotcAdptSleSsapClientEventParam param = {0};
    (void)memset_s(&param, sizeof(param), 0, sizeof(IotcAdptSleSsapClientEventParam));
    param.ssapcNotification.clientId = client_id;
    param.ssapcNotification.connId = conn_id;
    param.ssapcNotification.data.handle = data->handle;
    param.ssapcNotification.data.type = data->type;
    if (data->data_len > 0) {
        param.ssapcNotification.data.dataLen = data->data_len;
        param.ssapcNotification.data.data = IotcMalloc(data->data_len);
        (void)memcpy_s(param.ssapcNotification.data.data, data->data_len, data->data, data->data_len);
    }
    param.ssapcNotification.status = OhosStatusToAdapterStatus(status);
    if (g_sleSsapClientEventHandler != NULL) {
        int32_t ret = g_sleSsapClientEventHandler(IOTC_ADPT_SSAPC_NOTIFICATION_EVENT, &param);
        IOTC_LOGD("Ssapc Notification Complete cb:ret:%d", ret);
    }
}
void SsapcIndicationCallback(uint8_t client_id, uint16_t conn_id, ssapc_handle_value_t *data, errcode_t status)
{
    if (data == NULL) {
        IOTC_LOGE("invalid param");
        return;
    }
    IOTC_LOGD("Ssapc Indication cb:client_id:%d,conn_id:%d,status:%d", client_id, conn_id, status);
    IotcAdptSleSsapClientEventParam param = {0};
    (void)memset_s(&param, sizeof(param), 0, sizeof(IotcAdptSleSsapClientEventParam));
    param.ssapcIndication.clientId = client_id;
    param.ssapcIndication.connId = conn_id;
    param.ssapcIndication.data.handle = data->handle;
    param.ssapcIndication.data.type = data->type;
    if (data->data_len > 0) {
        param.ssapcIndication.data.dataLen = data->data_len;
        param.ssapcIndication.data.data = IotcMalloc(data->data_len);
        (void)memcpy_s(param.ssapcIndication.data.data, data->data_len, data->data, data->data_len);
    }
    param.ssapcIndication.status = OhosStatusToAdapterStatus(status);
    if (g_sleSsapClientEventHandler != NULL) {
        int32_t ret = g_sleSsapClientEventHandler(IOTC_ADPT_SSAPC_INDICATION_EVENT, &param);
        IOTC_LOGD("Ssapc Indication Complete cb:ret:%d", ret);
    }
}

static ssapc_callbacks_t g_SsapcCallbacks = {
    .find_structure_cb = SsapcFindStructureCallback,
    .ssapc_find_property_cbk = SsapcFindPropertyCallback,
    .find_structure_cmp_cb = SsapcFindStructureCompleteCallback,
    .read_cfm_cb = SsapcReadCfmCallback,
    .read_by_uuid_cmp_cb = SsapcReadByUuidCompleteCallback,
    .write_cfm_cb = SsapcWriteCfmCallback,
    .exchange_info_cb = SsapcExchangeInfoCallback,
    .notification_cb = SsapcNotificationCallback,
    .indication_cb = SsapcIndicationCallback,
};

static int32_t SsapClientRegisterCallbacks(ssapc_callbacks_t *cb)
{
    return ssapc_register_callbacks(cb);
}

static int32_t SsapcRegister(sle_uuid_t *app_uuid, uint8_t *client_id)
{
    return ssapc_register_client(app_uuid, client_id);
}

static int32_t SsapcRegisterUnregister(uint8_t client_id)
{
    return ssapc_unregister_client(client_id);
}

static int32_t SsapcFindStructure(uint8_t client_id, uint16_t conn_id, ssapc_find_structure_param_t *param)
{
    return ssapc_find_structure(client_id, conn_id, param);
}

static int32_t SsapcReadReq(uint8_t client_id, uint16_t conn_id, uint16_t handle, uint8_t type)
{
    return ssapc_read_req(client_id, conn_id, handle, type);
}

static int32_t IotcSleWriteSync(uint8_t client_id, uint16_t conn_id, ssapc_write_param_t *param, bool isReq)
{
    int32_t ret = -1;
    int sflag = 0;
    uint64_t kht_before_get_jiffies = 0, kht_after_get_jiffies = 0;
    uint64_t after_us = 0;
    uint32_t wait_min_time_us = (SLE_CONNECT_UPDATE_INTERVAL_HDI * 125); // 最少等待1个发送间隔(可以是连接间隔)
    int retry = 2000; // 最多等待2000个连接间隔时间
    kht_before_get_jiffies = IotcSleClockGettimeUsLite();
    after_us = kht_before_get_jiffies + wait_min_time_us; // 防止溢出回卷, 等待超时时间

    while (1) {
        if (sflag == 0 && gle_tx_acb_data_num_get() > 0) {
            if (isReq) {
                ret = ssapc_write_req(client_id, conn_id, param);
            } else {
                ret = ssapc_write_cmd(client_id, conn_id, param);
            }
            sflag = 1;
        }

        khtAfterGetJiffies = IotcSleClockGettimeUsLite();
        if ((int)(khtAfterGetJiffies - afterUs) < 0) {
            sched_yield();
        } else {
            if (sflag == 0 && (--retry) > 0) {
                afterUs += waitMinTimeUs;
                sched_yield();
            } else {
                keepLooping = false;
            }
        }
    }
    return ret;
}

static int32_t SsapcWriteReq(uint8_t client_id, uint16_t conn_id, ssapc_write_param_t *param)
{
    int32_t ret = IotcSleWriteSync(client_id, conn_id, param, true);
    return ret;
}

static int32_t SsapcExchangeInfoReq(uint8_t client_id, uint16_t conn_id, ssap_exchange_info_t *param)
{
    return ssapc_exchange_info_req(client_id, conn_id, param);
}

int32_t IotcInitSleHostService(void)
{
    return IOTC_OK;
}

int32_t IotcSleRegisterHostCallbacks(void)
{
    return IOTC_OK;
}

int32_t IotcSleEnable(void)
{
    if (g_clientState != IOTC_SLE_STOP) {
        IOTC_LOGE("sle enable state=%d", g_clientState); // 开启状态
        return IOTC_OK;
    }
    int32_t ret = 0;

    ret = AnnounceSeekRegisterCallbacks(&g_SleAnnounceSeek);
    if (ret != IOTC_ADPT_SLE_STATUS_SUCCESS) {
        IOTC_LOGE("register announce seek callback ret=%d", ret);
        return IOTC_ERROR;
    }

    ret = ConnectionRegisterCallbacks(&g_SleConnectionCb);
    if (ret != IOTC_ADPT_SLE_STATUS_SUCCESS) {
        IOTC_LOGE("register connection callback ret=%d", ret);
        return IOTC_ERROR;
    }

    ret = SsapClientRegisterCallbacks(&g_SsapcCallbacks);
    if (ret != IOTC_ADPT_SLE_STATUS_SUCCESS) {
        IOTC_LOGE("register ssap client callback ret=%d", ret);
        return IOTC_ERROR;
    }

    ret = EnableSle();
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

int32_t IotcSleRegisterAnnounceSeekCallbacks(const IotcAdptSleAnnounceSeekCallback callback)
{
    if (callback == NULL) {
        IOTC_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    g_sleAnnounceSeekEventHandler = callback;
    return IOTC_OK;
}

int32_t IotcSleRegisterConnectionCallbacks(const IotcAdptSleConnectionCallback callback)
{
    if (callback == NULL) {
        IOTC_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    g_sleConnectionEventHandler = callback;

    return IOTC_OK;
}

int32_t IotcSleRegisterSsapClientCallbacks(const IotcAdptSleSsapClientCallback callback)
{
    if (callback == NULL) {
        IOTC_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    g_sleSsapClientEventHandler = callback;

    return IOTC_OK;
}

int32_t IotcSleSsapcRegister(SleUuid *appUuid, uint8_t *clientId)
{
    if (appUuid == NULL) {
        IOTC_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    sle_uuid_t app_uuid = {0};
    uint8_t client_id = 0;
    (void)memset_s(&app_uuid, sizeof(app_uuid), 0, sizeof(sle_uuid_t));
    app_uuid.len = appUuid->len;
    (void)memcpy_s(app_uuid.uuid, SLE_UUID_LEN, appUuid->id, appUuid->len);
    int32_t ret = SsapcRegister(&app_uuid, &client_id);
    if (ret != IOTC_ADPT_SLE_STATUS_SUCCESS) {
        IOTC_LOGE("Iotc Sle Ssapc Register ret=%x", ret);
        return IOTC_ERROR;
    }
    *clientId = client_id;
    return IOTC_OK;
}

int32_t IotcSleSsapcRegisterUnregister(uint8_t clientId)
{
    int32_t ret = SsapcRegisterUnregister(clientId);
    if (ret != IOTC_ADPT_SLE_STATUS_SUCCESS) {
        IOTC_LOGE("Iotc Sle Ssapc Register Unregister ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t IotcSleSsapcFindStructure(uint8_t clientId, uint16_t connId, IotcAdptSsapcFindStructureParam *param)
{
    if (param == NULL) {
        IOTC_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    ssapc_find_structure_param_t ssapcFindParam = {0};
    (void)memset_s(&ssapcFindParam, sizeof(ssapcFindParam), 0, sizeof(ssapc_find_structure_param_t));
    ssapcFindParam.type = param->type;
    ssapcFindParam.start_hdl = param->type;
    ssapcFindParam.end_hdl = param->endHdl;
    ssapcFindParam.uuid.len = param->uuid.len;
    (void)memcpy_s(ssapcFindParam.uuid.uuid, SLE_UUID_LEN, param->uuid.id, param->uuid.len);
    ssapcFindParam.reserve = param->reserve;
    int32_t ret = SsapcFindStructure(clientId, connId, &ssapcFindParam);
    if (ret != IOTC_ADPT_SLE_STATUS_SUCCESS) {
        IOTC_LOGE("Iotc Sle Ssapc Find Structure ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t IotcSleSsapcReadReq(uint8_t clientId, uint16_t connId, uint16_t handle, uint8_t type)
{
    int32_t ret = SsapcReadReq(clientId, connId, handle, type);
    if (ret != IOTC_ADPT_SLE_STATUS_SUCCESS) {
        IOTC_LOGE("Iotc Sle Ssapc Read Req ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t IotcSleSendSsapsIndicate(uint8_t serverId, uint16_t connectId, const IotcAdptSleSendIndicateParam *param)
{
    if (param == NULL) {
        IOTC_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    IOTC_LOGD("IotcSleSendSsapsIndicate handle=%d, type=%d, serverId=%d, connectId=%d", param->handle, param->type,
        serverId, connectId);
    ssapc_write_param_t writeParam = {0};
    (void)memset_s(&writeParam, sizeof(writeParam), 0, sizeof(ssapc_write_param_t));
    writeParam.handle = SLE_WRITE_PARAM_HANLDE;
    writeParam.type = param->type;
    writeParam.data_len = param->valueLen;
    writeParam.data = param->value;
    int32_t ret = SsapcWriteReq(serverId, connectId, &writeParam);
    if (ret != IOTC_ADPT_SLE_STATUS_SUCCESS) {
        IOTC_LOGE("Iotc Sle Ssapc Write Req ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t IotcSleSsapcExchangeInfoReq(uint8_t clientId, uint16_t connId, IotcAdptSsapExchangeInfo *param)
{
    if (param == NULL) {
        IOTC_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    ssap_exchange_info_t excInfo = {0};
    (void)memset_s(&excInfo, sizeof(excInfo), 0, sizeof(ssap_exchange_info_t));
    excInfo.mtu_size = param->mtuSize;
    excInfo.version = param->version;
    int32_t ret = SsapcExchangeInfoReq(clientId, connId, &excInfo);
    if (ret != IOTC_ADPT_SLE_STATUS_SUCCESS) {
        IOTC_LOGE("Iotc Sle Ssapc Exchange Info Req ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t IotcSleSetSleName(const char *name, uint8_t len)
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

int32_t IotcSleSsapsStartService(uint8_t serviceId, uint16_t serviceHandle)
{
    (void)serviceId;
    (void)serviceHandle;
    return IOTC_OK;
}

int32_t IotcSleSendSsapsResponse(uint8_t serverId, uint16_t connectId, const IotcAdptSleResponseParam *param)
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
    int32_t ret = memcpy_s(
        seekParam.seek_interval, sizeof(seekParam.seek_interval), param->seekInterval, sizeof(param->seekInterval));
    if (ret != EOK) {
        IOTC_LOGE("memcpy_s err");
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    seekParam.seek_phys = param->seekphys;
    ret = memcpy_s(seekParam.seek_type, sizeof(seekParam.seek_type), param->seekType, sizeof(param->seekType));
    if (ret != EOK) {
        IOTC_LOGE("memcpy_s err");
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    ret = memcpy_s(seekParam.seek_window, sizeof(seekParam.seek_window), param->seekWindow, sizeof(param->seekWindow));
    if (ret != EOK) {
        IOTC_LOGE("memcpy_s err");
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    return SleSetSeekParam(&seekParam);
}

int32_t IotcSleStartSeek(void)
{
    /*
    if(g_clientState != IOTC_SLE_ENABLE)
    {
        IOTC_LOGE("SLE is not enabled or seek has already started state = %d!", g_clientState);
        return IOTC_ERROR;
    }
        */

    if (SleStartSeek() != IOTC_OK) {
        IOTC_LOGE("SLE start seek failed!");
        return IOTC_ERROR;
    }

    g_clientState = IOTC_SLE_SEEK_ENABLE;
    return IOTC_OK;
}

int32_t IotcSleStoptSeek(void)
{
    if (g_clientState == IOTC_SLE_SEEK_STOP) {
        IOTC_LOGE("sle not enabled seeek state = %d!", g_clientState);
        return IOTC_ERROR;
    }

    if (SleStopSeek() != IOTC_OK) {
        IOTC_LOGE("SLE stop seek failed!");
        return IOTC_ERROR;
    }

    g_clientState = IOTC_SLE_SEEK_STOP;
    return IOTC_OK;
}

int32_t IotcSleConnectRemoteDevice(const IotcAdptSleDeviceAddr *addr)
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

int32_t IotcSleDisconnectRemoteDevice(const IotcAdptSleDeviceAddr *addr)
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