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
#include <stddef.h>
#include <unistd.h>
#include "sle_client_event_service.h"
#include "sle_svc_device_info.h"
#include "security_speke.h"
#include "sle_profile.h"
#include "sle_speke_session.h"
#include "iotc_errcode.h"
#include "utils_assert.h"
#include "iotc_event.h"
#include "sle_disc_ctrl.h"
#include "iotc_sle_client.h"
#include "sle_profile.h"
#include "sle_ssap_mgt.h"
#include "event_bus.h"
#include "iotc_log.h"
#include "securec.h"
#include "sle_ssap_service.h"
#include "sle_svc_ctx.h"

#include "sle_conn_device_info.h"
#include "sle_client_auth_setup.h"
#include "sle_client_auth_setup.h"
#include "sle_comm_status.h"

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

static uint8_t g_clientId = 1;


int32_t ClientSleSpekeStartSession(uint16_t connId){
    int32_t ret = CreateSleSpekeSess(SPEKE_TYPE_CLIENT,connId);
    if(ret != IOTC_OK){
        IOTC_LOGE("[uuid client] create speke failed ret = %d!", ret);
        return ret;
    }

    uint8_t *msg = NULL;
    uint32_t len = 0;
    ret = SpekeStartSession(GetSleSpekeSess(connId), &msg, &len);
    if (ret != IOTC_OK || len == 0) {
        IOTC_LOGE("[uuid client] start speke failed!");
        return ret;
    }

    ret = SleLinkLayerReportSvcData(connId, SLE_SVC_SPEKE, msg, len, SLE_OPTYPE_PUT);
    if (ret != IOTC_OK) {
        IOTC_LOGE("[uuid client] report msg failed!");
        return ret;
    }
    return ret;
}

int32_t ClientGetSleDeviceInfo(uint16_t connId){

    uint8_t *msg = NULL;
    uint32_t len = 0;
    int32_t ret = CreateSvcDeviceInfoReq(&msg, &len);
    if(ret != IOTC_OK)
    {
        IOTC_LOGE("[uuid client] get device info failed!");
        return ret;
    }

    ret = SleLinkLayerReportSvcDataEnc(connId,SLE_SVC_DEVICE_INFO, msg, len, SLE_OPTYPE_GET);
    if(ret != IOTC_OK)
    {
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


static void IotcSlePrintData(const uint16_t valueLen,const uint8_t *value) {
    size_t buf_len = (size_t)valueLen * 3 + 1;
    char *hex_str = malloc(buf_len);
    if (!hex_str) {
        IOTC_LOGI("IotcSlePrintData malloc failed");
        return;
    }
    char *p = hexStr;
    size_t remaining = bufLen;

    for (uint32_t i = 0; i < valueLen; i++) {
        int n = snprintf_s(p, remaining, remaining - 1, "%02X ", value[i]);
        if (n < 0 || (size_t)n >= remaining) {
            break;
        }
        p += n;
        remaining -= n;
    }
    IOTC_LOGI("IotcSlePrintData First %u bytes: %s", valueLen, hexStr);
    free(hexStr);
}

static void SleClientSeekResultCallBack(uint32_t event, void *param, uint32_t len)
{
    if (param == NULL) {
        IOTC_LOGE("[uuid client] %s: param error.", __func__);
        return;
    }
    IOTC_LOGI("[uuid client] %s: event = %d, param = %p, len = %d", __func__, event, param, len);
    IotcAdptSleAnnounceSeekEventParam *eventParam = (IotcAdptSleAnnounceSeekEventParam *)param;

    IotcAdptSleDeviceAddr g_sle_remote_addr;
    IOTC_LOGI("[uuid client] seekResult.data: %s", (const char *)eventParam->seekResult.data);

    IotcSlePrintData(eventParam->seekResult.dataLength, eventParam->seekResult.data);

    if (eventParam->seekResult.data[SLE_ADV_FLAGS_LEN_IDX] == SLE_ADV_FLAGS_LEN_VAL &&
        eventParam->seekResult.data[SLE_ADV_FLAGS_TYPE_IDX] == SLE_ADV_FLAGS_AD_TYPE &&
        eventParam->seekResult.data[SLE_ADV_FLAGS_VAL_IDX] == SLE_ADV_FLAGS_VALUE) {
        IOTC_LOGI("[uuid client] %s target found, addr: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
            eventParam->seekResult.addr.addr[SLE_ADDR_BYTE_IDX_0],
            eventParam->seekResult.addr.addr[SLE_ADDR_BYTE_IDX_1],
            eventParam->seekResult.addr.addr[SLE_ADDR_BYTE_IDX_2],
            eventParam->seekResult.addr.addr[SLE_ADDR_BYTE_IDX_3],
            eventParam->seekResult.addr.addr[SLE_ADDR_BYTE_IDX_4],
            eventParam->seekResult.addr.addr[SLE_ADDR_BYTE_IDX_5]);

        memcpy_s(&g_sle_remote_addr, sizeof(IotcAdptSleDeviceAddr),
            &(eventParam->seekResult.addr), sizeof(IotcAdptSleDeviceAddr));
        int32_t ret = IotcSleConnectRemoteDevice(&g_sle_remote_addr);
        IOTC_LOGI("[uuid client]  connect remote device %d!", ret);
    }

    IOTC_LOGI("[uuid client] seek result, addr:%02x:%02x:%02x:%02x:%02x:%02x, addrtype:%d, rssi:%d, len = %d",
        eventParam->seekResult.addr.addr[SLE_ADDR_BYTE_IDX_0],
        eventParam->seekResult.addr.addr[SLE_ADDR_BYTE_IDX_1],
        eventParam->seekResult.addr.addr[SLE_ADDR_BYTE_IDX_2],
        eventParam->seekResult.addr.addr[SLE_ADDR_BYTE_IDX_3],
        eventParam->seekResult.addr.addr[SLE_ADDR_BYTE_IDX_4],
        eventParam->seekResult.addr.addr[SLE_ADDR_BYTE_IDX_5],
        eventParam->seekResult.eventType, eventParam->seekResult.rssi, len);
}

static void SleClientConnectStateCallback(uint32_t event, void *param, uint32_t len)
{
    if (param == NULL) {
        IOTC_LOGE("[uuid client] %s: param error.", __func__);
        return;
    }
    IOTC_LOGI("[uuid client] %s: event = %d, param = %p, len = %d", __func__, event, param, len);
    IotcAdptSleConnectionEventParam *eventParam = (IotcAdptSleConnectionEventParam *)param;

    SlePeerDevInfo *devInfo = GetSleSsapMgtPeerDevInfo(eventParam->sleConnectStateChanged.connId);
    if (devInfo != NULL) {
        IOTC_LOGI("[uuid client]  connect remote device %d!", eventParam->sleConnectStateChanged.connId);
        memcpy_s(&devInfo->devAddr, sizeof(devInfo->devAddr),
            &(eventParam->sleConnectStateChanged.addr), sizeof(devInfo->devAddr));
        devInfo->connState = eventParam->sleConnectStateChanged.connState;
    }
    IOTC_LOGI("[uuid client]  connect remote device Success connId = %d!", devInfo->connId);
    char addrStr[SLE_ADDR_STR_LEN];
    SleAddrToStr(devInfo->devAddr.addr, addrStr, sizeof(addrStr));
    IOTC_LOGI("[uuid client] %s Success, addr: %s\r\n", __func__, addrStr);

    if (eventParam->sleConnectStateChanged.connState == IOTC_SLE_SSAP_CONNECT_STATE_NONE) {
        IOTC_LOGI("[uuid client]  connect remote device state none connId = %d!", devInfo->connId);
        return;
    }
    if(eventParam->sleConnectStateChanged.conn_state == IOTC_SLE_SSAP_CONNECT_STATE_DISCONNECTED)
    {
        //断开连接后把连接信息删除
        SleDeleteConnDev(devInfo->connId);
        IOTC_LOGI("[uuid client]  connect remote device state disconnected connId = %d!", devInfo->connId);
        return;
    }

    if (eventParam->sleConnectStateChanged.connState == IOTC_SLE_SSAP_CONNECT_STATE_CONNECTED) {
        IotcAdptSsapExchangeInfo info = {0};
        info.mtuSize = SLE_DEFAULT_MTU_SIZE;
        info.version = SLE_DEFAULT_MTU_VERSION;
        if (IotcSleSsapcExchangeInfoReq(g_clientId, devInfo->connId, &info) != IOTC_OK) {
            IOTC_LOGE("[uuid client] %s: exchange info failed.", __func__);
        }

        SleConnRetDeviceInfo retDevInfo;
        retDevInfo.connID = eventParam->sleConnectStateChanged.connId;
        memset_s(retDevInfo.devAddr, IOTC_ADPT_SLE_ADDR_LEN, 0, IOTC_ADPT_SLE_ADDR_LEN);
        memcpy_s(retDevInfo.devAddr, IOTC_ADPT_SLE_ADDR_LEN,
            eventParam->sleConnectStateChanged.addr.addr,
            sizeof(eventParam->sleConnectStateChanged.addr.addr));
        retDevInfo.status = (uint16_t)eventParam->sleConnectStateChanged.connState;

        int32_t ret = SleConnDevMgt(&retDevInfo);
        if (ret != IOTC_OK) {
            IOTC_LOGE("sle conn dev mgt fail, %u", ret);
        }
    }
}

static void SleClientExchangeInfoCallBack(uint32_t event, void *param, uint32_t len)
{
    IOTC_LOGI("[uuid client] [SleClientExchangeInfoCallBack] %s: event %d.", __func__, event);
    if (param == NULL) {
        IOTC_LOGE("[uuid client] %s: param error.", __func__);
        return;
    }
    IOTC_LOGI("[uuid client] %s: event = %d, param = %p, len = %d", __func__, event, param, len);

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
    if (param == NULL) {
        IOTC_LOGE("[uuid client] %s: param error.", __func__);
        return;
    }
    IOTC_LOGI("[uuid client] %s: event = %d, param = %p, len = %d", __func__, event, param, len);
    IotcAdptSleSsapClientEventParam *eventParam = (IotcAdptSleSsapClientEventParam *)param;

    int32_t ret = SleSetServiceAtt(eventParam->ssapcFindServiceResult.clientId,
        (eventParam->ssapcFindServiceResult.service.startHdl) + 1,
        eventParam->ssapcFindServiceResult.service.endHdl);
    if (ret != IOTC_OK) {
        IOTC_LOGE("[uuid client] %s: set service att failed.", __func__);
        return;
    }
}

static void SleClientFindStructureCompleteCallback(uint32_t event, void *param, uint32_t len)
{
    if (param == NULL) {
        IOTC_LOGE("[uuid client] %s: param is NULL.", __func__);
        return;
    }
    IotcAdptSleSsapClientEventParam *eventParam = (IotcAdptSleSsapClientEventParam *)param;
    IOTC_LOGI("[uuid client] %s: status = %d", __func__, eventParam->ssapcFindStructureResult.status);

    if (eventParam->ssapcFindStructureResult.status == IOTC_ADPT_SLE_STATUS_SUCCESS) {
        SleDeviceInfo *info = SleGetSleConnRetDeviceInfo(eventParam->ssapcFindStructureResult.connId);
        if (info == NULL) {
            IOTC_LOGE("[uuid client] %s: info is NULL.", __func__);
            return;
        }
        if (info->isSecure == IOTC_SPEKE_SLE_STATE_INIT) {
            IOTC_LOGE("[uuid client] %s: isSecure is IOTC_SPEKE_SLE_STATE_INIT.", __func__);
            return;
        }
        int32_t ret = ClientSleSpekeStartSession(eventParam->ssapcFindStructureResult.connId);
        if (ret != IOTC_OK) {
            IOTC_LOGE("[uuid client] %s: ClientSleSpekeStartSession failed, ret:%d\r\n", __func__, ret);
        }
        info->isSecure = IOTC_SPEKE_SLE_STATE_INIT;
        SleScanServiceStop();
    }
}

static void SleClientConnectParamUpdateCallback(uint32_t event, void *param, uint32_t len)
{
    if (param == NULL) {
        IOTC_LOGE("[uuid client] %s: param is NULL.", __func__);
        return;
    }
    IotcAdptSleConnectionEventParam *eventParam = (IotcAdptSleConnectionEventParam *)param;
    IOTC_LOGI("[uuid client] %s: conn_id:%d status:%d \r\n", __func__,
        eventParam->sleConnectParamUpdateReq.connId, eventParam->sleConnectParamUpdateReq.status);
}

static void SleClientSpekeFinishedCallback(uint32_t event, void *param, uint32_t len)
{
    NotifyFinishedStatus * spekeStatus = (NotifyFinishedStatus *)param;
    if(spekeStatus->errorCode == IOTC_OK)
    {
        IOTC_LOGI("[uuid client] %s: SleClientSpekeFinishedCallback success.", __func__);
        uint8_t *msg = NULL;
        uint32_t len = 0;
        //接通ble下发的authcode的时候就把这个测试注释掉
        if(CreateSvcAuthSetupIssueReq(&msg, &len)!= IOTC_OK)
        {
            IOTC_LOGE("[uuid client] %s: CreateSvcAuthSetupIssueReq failed.", __func__);
            return ;
        }

        if(SleLinkLayerReportSvcDataEnc(spekeStatus->connSessionId,SLE_SVC_AUTH_SETUP, msg, len, SLE_OPTYPE_GET) != IOTC_OK)
        {
            IOTC_LOGE("[uuid client] report msg failed!");
            return ;
        }
        IotcFree(msg);

        sleep(1000); //延迟查询devid
        if(CreateSvcAuthSetupGetReq(&msg, &len) != IOTC_OK)
        {
            IOTC_LOGE("[uuid client] %s: CreateSvcAuthSetupGetReq failed.", __func__);
            return ;
        }

        if(SleLinkLayerReportSvcDataEnc(spekeStatus->connSessionId, SLE_SVC_AUTH_SETUP, msg, len, SLE_OPTYPE_GET)!= IOTC_OK)
        {
            IOTC_LOGE("[uuid client] report msg failed!");
            return ;
        }
    }
}

//Auth Setup 完成之后查询设备信息
static void SleClientAuthSetupFinishedCallback(uint32_t event, void *param, uint32_t len)
{
    NotifyFinishedStatus * spekeStatus = (NotifyFinishedStatus *)param;
    if(spekeStatus->errorCode == IOTC_OK)
    {
        IOTC_LOGI("[uuid client] %s: SleClientAuthSetupFinishedCallback success.", __func__);
        uint8_t *msg = NULL;
        uint32_t len = 0;
        int32_t ret = CreateSvcDeviceInfoReq(&msg, &len);
        if(ret != IOTC_OK)
        {
            IOTC_LOGE("[uuid client] get device info failed!");
            return ;
        }

        ret = SleLinkLayerReportSvcDataEnc(spekeStatus->connSessionId,SLE_SVC_DEVICE_INFO, msg, len, SLE_OPTYPE_GET);
        if(ret != IOTC_OK)
        {
            IOTC_LOGE("[uuid client] report msg failed!");
        }
    }
}

static int32_t SleClientBusinessCallbackInit(void)
{
    int32_t ret;

    ret = EventBusSubscribe(SleClientSeekResultCallBack, IOTC_CORE_SLE_EVENT_SEEK_RESULT);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "[uuid client] subscribe gatt ClientSleSeek err:%d", ret);

    ret = EventBusSubscribe(SleClientConnectStateCallback, IOTC_CORE_SLE_EVENT_CONNECT_STATE_CHANGED);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "[uuid client] subscribe gatt ClientSleConnect err:%d", ret);

    ret = EventBusSubscribe(SleClientExchangeInfoCallBack, IOTC_CORE_SLE_EVENT_SSAPC_EXCHANGE_INFO);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "[uuid client] subscribe gatt ClientSleConnect err:%d", ret);

    ret = EventBusSubscribe(SleClientFindStructureCallBack, IOTC_CORE_SLE_EVENT_SSAPC_FIND_STRUCTURE);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "[uuid client] subscribe gatt FindStructure err:%d", ret);

    ret = EventBusSubscribe(SleClientFindStructureCompleteCallback,
        IOTC_CORE_SLE_EVENT_SSAPC_FIND_STRUCTURE_COMPLETE);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret,
        "[uuid client] subscribe gatt SleClientFindStructureCompleteCallback err:%d", ret);

    ret = EventBusSubscribe(SleClientConnectParamUpdateCallback, IOTC_CORE_SLE_EVENT_CONNECT_PARAM_UPDATE_REQ);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret,
        "[uuid client] subscribe gatt SleClientConnectParamUpdateCallback err:%d", ret);

    ret = EventBusSubscribe(SleClientSpekeFinishedCallback, IOTC_CORE_SLE_EVENT_SPEKE_FINISHED);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "[uuid client] subscribe gatt SleClientSpekeFinishedCallback err:%d", ret);

    ret = EventBusSubscribe(SleClientAuthSetupFinishedCallback, IOTC_CORE_SLE_EVENT_AUTH_SETUP_FINISHED);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "[uuid client] subscribe gatt SleClientAuthSetupFinishedCallback err:%d", ret);


    return ret;
}

int32_t SleScanServiceStart(void)
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

int32_t SleSendCustomSecDataService(const char *devId, uint8_t protType, const uint8_t *data, uint32_t len)
{
    // 停止扫描
    if (SleSeekCtrlStop() != IOTC_OK) {
        IOTC_LOGE("%s: do SleStopSeek failed. Error.", __func__);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t SleSendCustomSecDataService(const char *devId, const uint8_t *data, uint32_t len)
{
    if (devId == NULL || data == NULL) {
        IOTC_LOGE("%s: Invalid input parameters (devId=%p, data=%p)", __func__, devId, data);
        return IOTC_ERR_PARAM_INVALID;
    }

    uint16_t connId = 0;
    if(!SleFindConnDevByDevId(devId, &connId))
    {
        IOTC_LOGE("%s: devId=%s not found", __func__, devId);
        return IOTC_ERROR;
    }
    return SleLinkLayerReportSvcDataEnc(connId, SLE_SVC_CUSTOM_SEC_DATA, data, len, SLE_OPTYPE_GET);
}

int32_t IotcOhSleFindDeviceInfoService(const char *devId, void **info)
{
    if (devId == NULL || info == NULL) {
        IOTC_LOGE("%s: Invalid input parameters (devId=%p, info=%p)", __func__, devId, info);
        return IOTC_ERR_PARAM_INVALID;
    }

    SleConnDeviceInfo *node = SleFindRetDeviceInfoNode(devId);
    SleConnDeviceInfo *external_copy = (SleConnDeviceInfo *)IotcMalloc(sizeof(SleConnDeviceInfo));
    if (external_copy == NULL) {
        IOTC_LOGE("Memory allocation failed | Size:%zu", sizeof(SleConnDeviceInfo));
        return IOTC_ERR_PARAM_INVALID;
    }
    memcpy_s(external_copy, sizeof(SleConnDeviceInfo), node, sizeof(SleConnDeviceInfo));

    *info = external_copy;
    return IOTC_OK;
}

int32_t IotcClientFindConnIdAndAddrList(void)
{
    PrintSleSsapConnidAndAddr();
    return IOTC_OK;
}

int32_t SleSsapServiceSvcInit(SleSvcCtx *ctx)
{
    int32_t ret = SleClientBusinessCallbackInit();
    if (ret != IOTC_OK) {
        IOTC_LOGE("sle ssap client register err ret=%d", ret);
        return ret;
    }

    return IOTC_OK;
}
