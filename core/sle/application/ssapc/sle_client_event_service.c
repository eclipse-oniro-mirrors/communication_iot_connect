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

#include "sle_client_event_service.h"
#include "sle_svc_device_info.h"
#include "security_speke.h"
#include "sle_profile.h"
#include "sle_speke_session.h"
#include "iotc_errcode.h"
#include "utils_assert.h"
#include "iotc_event.h"
#include "sle_disc_ctrl.h"
#include "sle_ssapc_ctrl.h"
#include "iotc_sle_server.h"
#include "sle_ssap_mgt.h"
#include "event_bus.h"
#include "iotc_log.h"
#include "securec.h"
#include <stddef.h>

/* 部分星闪扫描参数 */
#define IOTC_SLE_CLOSE_FILTER           0
#define SLE_SEEK_INTERVAL_DEFAULT       30000
#define SLE_SEEK_WINDOW_DEFAULT         1500
#define SLE_DEFAULT_MTU_SIZE            1500
#define SLE_DEFAULT_MTU_VERSION         1
#define SLE_LOG_ADDR_FMT                "%02x:%02x:%02x:%02x:%02x:%02x"
#define SLE_ADDR_HEX_WIDTH              2
#define SLE_ADDR_STR_LEN                18

typedef enum {
    SLE_ADDR_BYTE_IDX_0 = 0,
    SLE_ADDR_BYTE_IDX_1,
    SLE_ADDR_BYTE_IDX_2,
    SLE_ADDR_BYTE_IDX_3,
    SLE_ADDR_BYTE_IDX_4,
    SLE_ADDR_BYTE_IDX_5
} SleAddrByteIdx;

static inline void SleAddrToStr(const uint8_t *addr, char *buf, uint32_t bufLen)
{
    if (addr == NULL || buf == NULL || bufLen < SLE_ADDR_STR_LEN) {
        return;
    }
    (void)snprintf_s(buf, bufLen, bufLen - 1, SLE_LOG_ADDR_FMT,
        addr[SLE_ADDR_BYTE_IDX_0], addr[SLE_ADDR_BYTE_IDX_1], addr[SLE_ADDR_BYTE_IDX_2],
        addr[SLE_ADDR_BYTE_IDX_3], addr[SLE_ADDR_BYTE_IDX_4], addr[SLE_ADDR_BYTE_IDX_5]);
}

typedef enum {
    IOTC_SLE_SEEK_FILTER_ALLOW_ALL   = 0x00,
    IOTC_SLE_SEEK_FILTER_ALLOW_WLST  = 0x01,
} SoftbusSleSeekFilter;

typedef enum {
    IOTC_SLE_SEEK_PHY_1M = 0x1,
    IOTC_SLE_SEEK_PHY_2M = 0x2,
    IOTC_SLE_SEEK_PHY_4M = 0x4,
} SoftbusSleSeekPhy;

typedef enum {
    IOTC_SLE_SEEK_PASSIVE = 0x00,
    IOTC_SLE_SEEK_ACTIVE  = 0x01,
} SoftbusSleSeekType;

static char g_sleUuidAppUuid[] = {0x39, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static uint8_t g_client_id = 0;

int32_t ClientSleSpekeStartSession(uint32_t connId)
{
    int32_t ret = CreateSleSpekeSess(SPEKE_TYPE_CLIENT, connId);
    if (ret != IOTC_OK) {
        IOTC_LOGE("[uuid client] create speke failed!");
        return ret;
    }

    uint8_t *msg = NULL;
    uint32_t len = 0;
    ret = SpekeStartSession(GetSleSpekeSess(connId), &msg, &len);
    if (ret != IOTC_OK || len == 0) {
        IOTC_LOGE("[uuid client] start speke failed!");
        return ret;
    }

    ret = SleLinkLayerReportSvcData(connId, SLE_SVC_SPEKE, msg, len);
    if (ret != IOTC_OK) {
        IOTC_LOGE("[uuid client] report msg failed!");
        return ret;
    }
    return ret;
}

int32_t ClientSleSpekeProcessMsg(uint32_t connId)
{
    uint8_t *msg = NULL;
    uint32_t len = 0;
    int32_t ret = GetSleSvcDeviceInfoReq(&msg, &len);
    if (ret != IOTC_OK) {
        IOTC_LOGE("[uuid client] get device info failed!");
        return ret;
    }

    ret = SleLinkLayerReportSvcDataEnc(connId, SLE_SVC_DEVICE_INFO, msg, len);
    if (ret != IOTC_OK) {
        IOTC_LOGE("[uuid client] report msg failed!");
        return ret;
    }
    return ret;
}

static int32_t BuildIotcSleSeekParam(IotcAdptSleSeekParam *param)
{
    if (param == NULL) {
        IOTC_LOGE("[uuid client] %s: param error.", __func__);
        return IOTC_ERROR;
    }
    (void)memset_s(param, sizeof(IotcAdptSleSeekParam), 0, sizeof(IotcAdptSleSeekParam));
    param->ownaddrtype = IOTC_SLE_ADDRESS_TYPE_PUBLIC;

    param->filterduplicates = IOTC_SLE_CLOSE_FILTER;
    param->seekfilterpolicy = IOTC_SLE_SEEK_FILTER_ALLOW_ALL;
    param->seekphys = IOTC_SLE_SEEK_PHY_1M;
    param->seekType[0] = IOTC_SLE_SEEK_PASSIVE;
    param->seekInterval[0] = SLE_SEEK_INTERVAL_DEFAULT;
    param->seekWindow[0] = SLE_SEEK_WINDOW_DEFAULT;
    return IOTC_OK;
}

static void SleClientSeekResultCallBack(uint32_t event, void *param, uint32_t len)
{
    CHECK_V_RETURN_LOGE(event != IOTC_CORE_SLE_EVENT_SEEK_RESULT,
        "[uuid client] not IOTC_CORE_SLE_EVENT_SEEK_RESULT!");
    IotcAdptSleAnnounceSeekEventParam *eventParam = (IotcAdptSleAnnounceSeekEventParam *)param;

    IotcAdptSleDeviceAddr g_sle_remote_addr;
    if (strstr((const char *)eventParam->seekResult.data, "aaa") != NULL) {
        char addrStr[SLE_ADDR_STR_LEN];
        SleAddrToStr(eventParam->seekResult.addr.addr, addrStr, sizeof(addrStr));
        IOTC_LOGI("[uuid client] %s target found, addr: %s\r\n", __func__, addrStr);

        memcpy_s(&g_sle_remote_addr, sizeof(IotcAdptSleDeviceAddr),
            &(eventParam->seekResult.addr), sizeof(IotcAdptSleDeviceAddr));

        int32_t ret = IotcSleConnectRemoteDevice(&g_sle_remote_addr);
        IOTC_LOGI("[uuid client]  connect remote device %d!", ret);
    }

    IOTC_LOGI("[uuid client] seek result, addr:%s, addrtype:%d, rssi:%d, len = %d",
        eventParam->seekResult.addr, eventParam->seekResult.eventType,
        eventParam->seekResult.rssi, len);
}

static void SleClientConnectStateCallback(uint32_t event, void *param, uint32_t len)
{
    CHECK_V_RETURN_LOGE(event != IOTC_CORE_SLE_EVENT_CONNECT_STATE_CHANGED,
        "not IOTC_CORE_SLE_EVENT_SEEK_RESULT!");
    IotcAdptSleConnectionEventParam *eventParam = (IotcAdptSleConnectionEventParam *)param;

    SlePeerDevInfo *devInfo = GetSleSsapMgtPeerDevInfo(eventParam->sleConnectStateChanged.conn_id);
    if (devInfo != NULL) {
        IOTC_LOGI("[uuid client]  connect remote device %d!", eventParam->sleConnectStateChanged.conn_id);
        memcpy_s(&devInfo->devAddr, sizeof(devInfo->devAddr),
            &(eventParam->sleConnectStateChanged.addr), sizeof(devInfo->devAddr));
        devInfo->connState = eventParam->sleConnectStateChanged.conn_state;
    }
    IOTC_LOGI("[uuid client]  connect remote device Success connId = %d!", devInfo->connId);
    char addrStr[SLE_ADDR_STR_LEN];
    SleAddrToStr(devInfo->devAddr.addr, addrStr, sizeof(addrStr));
    IOTC_LOGI("[uuid client] %s Success, addr: %s\r\n", __func__, addrStr);

    IotcAdptSsapExchangeInfo info = {0};
    info.mtuSize = SLE_DEFAULT_MTU_SIZE;
    info.version = SLE_DEFAULT_MTU_VERSION;
    if (IotcSleSsapcExchangeInfoReq(g_client_id, devInfo->connId, &info) != IOTC_OK) {
        IOTC_LOGE("[uuid client] %s: exchange info failed.", __func__);
    }
}

static void SleClientExchangeInfoCallBack(uint32_t event, void *param, uint32_t len)
{
    CHECK_V_RETURN_LOGE(event != IOTC_ADPT_SSAPC_EXCHANGE_INFO_EVENT,
        "not IOTC_CORE_SLE_EVENT_SEEK_RESULT!");

    IotcAdptSleSsapClientEventParam *eventParam = (IotcAdptSleSsapClientEventParam *)param;
    IotcAdptSsapcFindStructureParam findParam = {0};
    findParam.type = 1;
    findParam.startHdl = 1;
    findParam.endHdl = 0xFFFF;

    if (IotcSleSsapcFindStructure(eventParam->ssapExchangeInfo.clientId,
        eventParam->ssapExchangeInfo.connId, &findParam) != IOTC_OK) {
        IOTC_LOGE("[uuid client] %s: find structure failed.", __func__);
        return;
    }
}

static void SleClientFindStructureCallBack(uint32_t event, void *param, uint32_t len)
{
    CHECK_V_RETURN_LOGE(event != IOTC_ADPT_SLE_SSAPC_FIND_STRUCTURE_EVENT,
        "not IOTC_CORE_SLE_EVENT_SEEK_RESULT!");

    IotcAdptSleSsapClientEventParam *eventParam = (IotcAdptSleSsapClientEventParam *)param;
    int32_t ret = SleSetServiceAtt(eventParam->ssapcFindServiceResult.clientId,
        eventParam->ssapcFindServiceResult.service.startHdl,
        eventParam->ssapcFindServiceResult.service.endHdl);
    if (ret != IOTC_OK) {
        IOTC_LOGE("[uuid client] %s: set service att failed.", __func__);
        return;
    }
}

static int32_t SleUuidClientRegister(void)
{
    int32_t ret;

    SleUuid app_uuid = {0};
    IOTC_LOGI("[uuid client] ssapc_register_client \r\n");
    app_uuid.len = sizeof(g_sleUuidAppUuid);
    if (memcpy_s(app_uuid.id, app_uuid.len, g_sleUuidAppUuid, sizeof(g_sleUuidAppUuid)) != EOK) {
        return IOTC_ERROR;
    }

    ret = EventBusSubscribe(SleClientSeekResultCallBack, IOTC_CORE_SLE_EVENT_SEEK_RESULT);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "[uuid client] subscribe gatt ClientSleSeek err:%d", ret);

    ret = EventBusSubscribe(SleClientConnectStateCallback, IOTC_CORE_SLE_EVENT_CONNECT_STATE_CHANGED);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "[uuid client] subscribe gatt ClientSleConnect err:%d", ret);

    ret = EventBusSubscribe(SleClientExchangeInfoCallBack, IOTC_ADPT_SSAPC_EXCHANGE_INFO_EVENT);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "[uuid client] subscribe gatt ClientSleConnect err:%d", ret);

    ret = EventBusSubscribe(SleClientFindStructureCallBack, IOTC_ADPT_SLE_SSAPC_FIND_STRUCTURE_EVENT);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "[uuid client] subscribe gatt ClientSleConnect err:%d", ret);

    ret = SleCtrlSsapcRegister(&app_uuid, &g_client_id);
    IOTC_LOGI("[uuid client] ssapc_register_client ret:%d", ret);
    return ret;
}

int32_t IotcDiscStartSeek(void)
{
    IotcAdptSleSeekParam param;
    if (BuildIotcSleSeekParam(&param) != IOTC_OK) {
        return IOTC_ERROR;
    }

    if (SleSeekCtrlParamSet(&param) != IOTC_OK) {
        return IOTC_ERROR;
    }

    if (SleSeekCtrlStart() != IOTC_OK) {
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t IotcClientFindConnIdAndAddrList(void)
{
    PrintSleSsapConnidAndAddr();
    return IOTC_OK;
}

int32_t IotcClientInit(void)
{
    (void)SleUuidClientRegister();
    return IOTC_OK;
}
