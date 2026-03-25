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
#include "utils_common.h"
#include "sle_conn_device_info.h"
#include "sle_svc_auth_setup.h"
#include "sle_comm_status.h"
#include "sle_svc_create_session.h"
#include "iotc_json.h"
#include "sle_linklayer_encrypt_sesskey.h"

// 部分星闪扫描参数
#define IOTC_SLE_CLOSE_FILTER 0 // 关闭过滤
#define SLE_SEEK_INTERVAL_DEFAULT 30000
#define SLE_SEEK_WINDOW_DEFAULT 1500

#define PROTOCOL_HEADER_BYTE1 0x02
#define PROTOCOL_HEADER_BYTE2 0x01
#define PROTOCOL_HEADER_BYTE3 0x06

#define PROTOCOL_HEADER_INDEX_0 0
#define PROTOCOL_HEADER_INDEX_1 1
#define PROTOCOL_HEADER_INDEX_2 2

typedef enum {
    IOTC_SLE_SEEK_FILTER_ALLOW_ALL = 0x00,
    IOTC_SLE_SEEK_FILTER_ALLOW_WLST = 0x01,
} SoftbusSleSeekFilter;

typedef enum {
    IOTC_SLE_SEEK_PHY_1M = 0x1,
    IOTC_SLE_SEEK_PHY_2M = 0x2,
    IOTC_SLE_SEEK_PHY_4M = 0x4,
} SoftbusSleSeekPhy;

typedef enum {
    IOTC_SLE_SEEK_PASSIVE = 0x00,
    IOTC_SLE_SEEK_ACTIVE = 0x01,
} SoftbusSleSeekType;

typedef struct {
    char devId[DEVICE_ID_MAX_STR_LEN + 1];
    char authCodeId[BLE_AUTHCODE_ID_LEN + 1];
    char authCode[BLE_AUTHCODE_LEN];
} SleIssueAuthCode;

typedef struct {
    int32_t errcode;
    char devId[64];
} StatusSyncResult;

static uint8_t g_clientId = 1;
static int32_t BuildIotcDevInfo(IotcJson *devInfoJson, IotcDeviceInfo *info)
{
    IOTC_LOGE("BuildIotcDevInfo, sn = %s", info->sn);
    const struct {
        const char *key;
        const char *value;
        bool isNum;
    } fields[] = {{"sn", info->sn, false},
                  {"model", info->model, false},
                  {"devType", info->devTypeId, false},
                  {"manuId", info->manuId, false},
                  {"prodId", info->prodId, false},
                  {"mac", "", false},
                  {"hiv", "1.0", false},
                  {"fwv", info->fwv, false},
                  {"hwv", info->hwv, false},
                  {"swv", info->swv, false},
                  {"proType", (const char *)&info->protType, true}};

    for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i) {
        int32_t ret = fields[i].isNum ? IotcJsonAddNum2Obj(devInfoJson, fields[i].key, *(int *)fields[i].value) :
                                        IotcJsonAddStr2Obj(devInfoJson, fields[i].key, fields[i].value);
        if (ret != IOTC_OK) {
            IOTC_LOGE("add %s err ret=%d", fields[i].key, ret);
            return ret;
        }
    }
    return IOTC_OK;
}

int32_t CreateSvcSessionSuccess(uint16_t connId)
{
    IotcConDeviceInfo *iotcConDevInfo = SleGetConnectionInfoByConnId(connId);
    if (iotcConDevInfo == NULL) {
        IOTC_LOGE("iotcConDevInfo is NULL");
        return IOTC_ERROR;
    }

    IotcConDeviceInfo *gatewayInfo = SleGetConnectionInfoByConnId(GATEWAY_CONNID);
    if (gatewayInfo == NULL) {
        IOTC_LOGE("gatewayInfo is NULL");
        return IOTC_ERROR;
    }

    int32_t ret = -1;
    IotcJson *root = IotcJsonCreate();
    if (root == NULL) {
        IOTC_LOGE("create err");
        return ret;
    }
    ret = IotcJsonAddStr2Obj(root, "devId", iotcConDevInfo->devId);
    if (ret != IOTC_OK) {
        IOTC_LOGE("add devId err ret=%d", ret);
        return ret;
    }

    ret = IotcJsonAddStr2Obj(root, "gatewayId", gatewayInfo->devId);
    if (ret != IOTC_OK) {
        IOTC_LOGE("add gatewayId err ret=%d", ret);
        return ret;
    }

    IotcJson *devInfoJson = IotcJsonCreate();
    if (devInfoJson == NULL) {
        IOTC_LOGE("create err");
        return ret;
    }

    ret = BuildIotcDevInfo(devInfoJson, iotcConDevInfo->devInfo);
    if (ret != IOTC_OK) {
        IOTC_LOGE("build devInfo err ret=%d", ret);
        return ret;
    }

    ret = IotcJsonAddItem2Obj(root, "devInfo", devInfoJson);
    if (ret != IOTC_OK) {
        IOTC_LOGE("add devInfo err ret=%d", ret);
        return ret;
    }

    ret = IotcJsonAddItem2Obj(root, "services", iotcConDevInfo->services);
    if (ret != IOTC_OK) {
        IOTC_LOGE("add services err ret=%d", ret);
        return ret;
    }

    EventBusPublishSync(IOTC_EVENT_INNER_CLOUD_SLE_DEVINFO_SYNC, (void *)root, 0);
    return ret;
}

int32_t ClientSleSpekeStartSession(uint16_t connId)
{
    int32_t ret = CreateSleSpekeSess(SPEKE_TYPE_CLIENT, connId);
    if (ret != IOTC_OK) {
        IOTC_LOGE("[uuid client] create speke failed ret = %d!", ret);
        return ret;
    }

    uint8_t *msg = NULL;
    uint32_t len = 0;
    // 创建 msg1 消息
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

int32_t ClientGetIotcConDeviceInfo(uint16_t connId)
{
    uint8_t *msg = NULL;
    uint32_t len = 0;
    int32_t ret = CreateSvcDeviceInfoReq(&msg, &len);
    if (ret != IOTC_OK) {
        IOTC_LOGE("[uuid client] get device info failed!");
        return ret;
    }

    ret = SleLinkLayerReportSvcDataEnc(connId, SLE_SVC_DEVICE_INFO, msg, len, SLE_OPTYPE_GET);
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

// 设备断开连接
int32_t SleDisconnectService(const char *devId)
{
    IOTC_LOGI("%s: devId=%s", __func__, devId);
    IotcConDeviceInfo *node = SleGetConnectionInfoByDevId(devId);
    if (node == NULL) {
        IOTC_LOGE("%s: devId=%s not found", __func__, devId);
        return IOTC_ERROR;
    }

    IotcAdptSleDeviceAddr devAddr = {0};
    devAddr.type = node->type;
    if (memcpy_s(devAddr.addr, IOTC_ADPT_SLE_ADDR_LEN, node->devAddr, IOTC_ADPT_SLE_ADDR_LEN)) {
        IOTC_LOGE("%s: memcpy_s failed", __func__);
        return IOTC_ERROR;
    }

    // 断开连接
    if (IotcSleDisconnectRemoteDevice(&devAddr) != IOTC_OK) {
        IOTC_LOGE("%s: do SleSsapDisconnect failed. Error.", __func__);
        return IOTC_ERROR;
    }

    // 删除管理信息
    if (DelSleSsapMgtPeerDevInfo(node->connID) != IOTC_OK) {
        IOTC_LOGE("%s: DelSleSsapMgtPeerDevInfo failed", __func__);
        return IOTC_ERROR;
    }
    // 删除连接管理信息
    SleDeleteConnDev(node->connID);
    return IOTC_OK;
}

static void IotcSlePrintData(const uint16_t valueLen, const uint8_t *value)
{
    size_t bufLen = (size_t)valueLen * 3 + 1;
    char *hexStr = malloc(bufLen);
    if (!hexStr) {
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
    IotcSlePrintData(eventParam->seekResult.dataLength, eventParam->seekResult.data);

    if (eventParam->seekResult.data[PROTOCOL_HEADER_INDEX_0] == PROTOCOL_HEADER_BYTE1 &&
        eventParam->seekResult.data[PROTOCOL_HEADER_INDEX_1] == PROTOCOL_HEADER_BYTE2 &&
        eventParam->seekResult.data[PROTOCOL_HEADER_INDEX_2] == PROTOCOL_HEADER_BYTE3) {
        memcpy_s(&g_sle_remote_addr, sizeof(IotcAdptSleDeviceAddr), &(eventParam->seekResult.addr),
                 sizeof(IotcAdptSleDeviceAddr));
        int32_t ret = IotcSleConnectRemoteDevice(&g_sle_remote_addr); // 链接
        IOTC_LOGI("[uuid client]  connect remote device %d!", ret);
    }
}

static void SleClientConnectStateCallback(uint32_t event, void *param, uint32_t len)
{
    if (param == NULL) {
        IOTC_LOGE("[uuid client] %s: param error.", __func__);
        return;
    }
    IOTC_LOGI("[uuid client] %s: event = %d, param = %p, len = %d", __func__, event, param, len);
    IotcAdptSleConnectionEventParam *eventParam = (IotcAdptSleConnectionEventParam *)param;

    SlePeerDevInfo *devInfo = GetSleSsapMgtPeerDevInfo(eventParam->sleConnectStateChanged.conn_id);
    if (devInfo != NULL) {
        IOTC_LOGI("[uuid client]  connect remote device %d!", eventParam->sleConnectStateChanged.conn_id);
        memcpy_s(&devInfo->devAddr, sizeof(devInfo->devAddr), &(eventParam->sleConnectStateChanged.addr),
                 sizeof(devInfo->devAddr));
        devInfo->connState = eventParam->sleConnectStateChanged.conn_state;
    }
    IOTC_LOGI("[uuid client]  connect remote device Success connId = %d!", devInfo->connId);

    if (eventParam->sleConnectStateChanged.conn_state == IOTC_SLE_SSAP_CONNECT_STATE_NONE) {
        IOTC_LOGI("[uuid client]  connect remote device state none connId = %d!", devInfo->connId);
        return;
    }
    if (eventParam->sleConnectStateChanged.conn_state == IOTC_SLE_SSAP_CONNECT_STATE_DISCONNECTED) {
        // 断开连接后把连接信息删除
        SleDeleteConnDev(devInfo->connId);
        SleLinkLayerSessDelete(devInfo->connId);
        IOTC_LOGI("[uuid client]  connect remote device state disconnected connId = %d!", devInfo->connId);
        return;
    }

    if (eventParam->sleConnectStateChanged.conn_state == IOTC_SLE_SSAP_CONNECT_STATE_CONNECTED) {
        IotcAdptSsapExchangeInfo info = {0};
        info.mtuSize = SLE_MTU_SIZE;
        info.version = 1;
        if (IotcSleSsapcExchangeInfoReq(g_clientId, devInfo->connId, &info) != IOTC_OK) {
            IOTC_LOGE("[uuid client] %s: exchange info failed.", __func__);
        }

        SleConnRetDeviceInfo retDevInfo;
        retDevInfo.connID = eventParam->sleConnectStateChanged.connId;
        memset_s(retDevInfo.devAddr, IOTC_ADPT_SLE_ADDR_LEN, 0, IOTC_ADPT_SLE_ADDR_LEN);
        memcpy_s(retDevInfo.devAddr, IOTC_ADPT_SLE_ADDR_LEN, eventParam->sleConnectStateChanged.addr.addr,
                 sizeof(eventParam->sleConnectStateChanged.addr.addr));
        retDevInfo.status = (uint16_t)eventParam->sleConnectStateChanged.conn_state;
        retDevInfo.type = eventParam->sleConnectStateChanged.addr.type;

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

    if (IotcSleSsapcFindStructure(eventParam->ssapExchangeInfo.clientId, eventParam->ssapExchangeInfo.connId,
                                  &findParam) != IOTC_OK) {
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
        IotcConDeviceInfo *info = SleGetConnectionInfoByConnId(eventParam->ssapcFindStructureResult.connId);
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
    }
}

static void SleClientConnectParamUpdateCallback(uint32_t event, void *param, uint32_t len)
{
    if (param == NULL) {
        IOTC_LOGE("[uuid client] %s: param is NULL.", __func__);
        return;
    }
    IotcAdptSleConnectionEventParam *eventParam = (IotcAdptSleConnectionEventParam *)param;
    IOTC_LOGI("[uuid client] %s: conn_id:%d status:%d \r\n", __func__, eventParam->sleConnectParamUpdateReq.conn_id,
              eventParam->sleConnectParamUpdateReq.status);
}

static void SleClientSpekeFinishedCallback(uint32_t event, void *param, uint32_t len)
{
    NotifyFinishedStatus *spekeStatus = (NotifyFinishedStatus *)param;
    if (spekeStatus->errorCode == IOTC_OK) {
        IOTC_LOGI("[uuid client] %s: SleClientSpekeFinishedCallback success.", __func__);
        uint8_t *msg = NULL;
        uint32_t len = 0;
        if (CreateSvcAuthSetupGet(&msg, &len) != IOTC_OK) {
            IOTC_LOGE("[uuid client] %s: CreateSvcAuthSetupGetReq failed.", __func__);
            return;
        }

        if (SleLinkLayerReportSvcDataEnc(spekeStatus->connSessionId, SLE_SVC_AUTH_SETUP, msg, len, SLE_OPTYPE_GET) !=
            IOTC_OK) {
            IOTC_LOGE("[uuid client] report msg failed!");
            return;
        }

        IotcFree(msg);
        msg = NULL;
    }
}

// 云下发的authcode拿到之后就开始创建sessionKey上线
static void SleClientCloudIssueAuthCodeCallback(uint32_t event, void *param, uint32_t len)
{
    SleIssueAuthCode *authCodeInfo = (SleIssueAuthCode *)param;
    IotcConDeviceInfo *devInfo = SleGetConnectionInfoByDevId(authCodeInfo->devId);
    if (devInfo == NULL) {
        IOTC_LOGE("[uuid client] cloud issue auth code callback, devId:%s", (const char *)authCodeInfo->devId);
        return;
    }

    IOTC_LOGD("[uuid client] cloud issue auth code callback, authcode: %s, authcodeId: %s", authCodeInfo->authCode,
              authCodeInfo->authCodeId);
    if (memcpy_s(devInfo->authCode, sizeof(devInfo->authCode), authCodeInfo->authCode, sizeof(devInfo->authCode)) !=
        EOK) {
        IOTC_LOGE("[uuid client] cloud issue auth code callback, memcpy_s failed");
        return;
    }

    if (memcpy_s(devInfo->authCodeId, sizeof(devInfo->authCodeId), authCodeInfo->authCodeId,
                 sizeof(devInfo->authCodeId)) != EOK) {
        IOTC_LOGE("[uuid client] cloud issue auth code callback, memcpy_s failed");
        return;
    }

    IOTC_LOGI("start CreateSession");
    uint8_t *msg = NULL;
    len = 0;

    IOTC_LOGI("[uuid client] %s: CreateSvcAuthSetupGet success.", __func__);
    if (CreateSvcSessionIssue(devInfo->connID, &msg, &len) != IOTC_OK) {
        IOTC_LOGE("[uuid client] %s: CreateSvcSessionIssueReq failed.", __func__);
        return;
    }

    if (SleLinkLayerReportSvcData(devInfo->connID, SLE_SVC_CREATE_SESSION, msg, len, SLE_OPTYPE_GET) != IOTC_OK) {
        IOTC_LOGE("[uuid client] report msg failed!");
        return;
    }
    IotcFree(msg);
    IOTC_LOGI("[uuid client] %s: CreateSvcSessionIssueReq success.", __func__);
}

// Auth Setup 完成之后查询设备信息
static void SleClientAuthSetupFinishedCallback(uint32_t event, void *param, uint32_t len)
{
    NotifyFinishedStatus *spekeStatus = (NotifyFinishedStatus *)param;
    if (spekeStatus->errorCode == IOTC_OK) {
        IOTC_LOGI("[uuid client] %s: SleClientAuthSetupFinishedCallback success.", __func__);

        uint8_t *msg = NULL;
        len = 0;
        IOTC_LOGI("[uuid client] %s: CreateSvcSessionIssue success.", __func__);

        int32_t ret = CreateSvcDeviceInfoReq(&msg, &len);
        if (ret != IOTC_OK) {
            IOTC_LOGE("[uuid client] get device info failed!");
            return;
        }

        ret = SleLinkLayerReportSvcDataEnc(spekeStatus->connSessionId, SLE_SVC_DEVICE_INFO, msg, len, SLE_OPTYPE_GET);
        if (ret != IOTC_OK) {
            IOTC_LOGE("[uuid client] report msg failed!");
        }

        IotcFree(msg);
        msg = NULL;
    }
}

int32_t SleScanServiceStart(void)
{
    // 调用扫描，说明已经登录了云，拿到了authcode
    if (AuthSetupAndDevInfo() != IOTC_OK) {
        return IOTC_ERROR;
    }

    // 打包扫描参数
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
    if (!SleFindConnDevByDevId(devId, &connId)) {
        IOTC_LOGE("%s: devId=%s not found", __func__, devId);
        return IOTC_ERROR;
    }
    return SleLinkLayerReportSvcDataEnc(connId, SLE_SVC_CUSTOM_SEC_DATA, data, len, SLE_OPTYPE_PUT);
}

int32_t IotcOhSleFindDeviceInfoService(const char *devId, void **info)
{
    if (devId == NULL || info == NULL) {
        IOTC_LOGE("%s: Invalid input parameters (devId=%p, info=%p)", __func__, devId, info);
        return IOTC_ERR_PARAM_INVALID;
    }

    IotcDeviceInfo *node = SleGetDeviceInfoByDevId(devId);
    IotcDeviceInfo *external_copy = (IotcDeviceInfo *)IotcMalloc(sizeof(IotcDeviceInfo));
    if (external_copy == NULL) {
        IOTC_LOGE("Memory allocation failed | Size:%zu", sizeof(IotcDeviceInfo));
        return IOTC_ERR_PARAM_INVALID;
    }
    memcpy_s(external_copy, sizeof(IotcDeviceInfo), node, sizeof(IotcDeviceInfo));

    *info = external_copy;
    return IOTC_OK;
}

int32_t IotcOhSleFindDeviceInfoByNameService(const char *name, IotcConDeviceInfo **info)
{
    IOTC_LOGD("IotcOhSleFindDeviceInfoByNameService in");
    if (name == NULL || info == NULL) {
        IOTC_LOGE("%s: Invalid input parameters (devId=%p, info=%p)", __func__, name, info);
        return IOTC_ERR_PARAM_INVALID;
    }

    IotcConDeviceInfo *node = SleGetDeviceInfoByDevTypeName(name);
    if (node == NULL) {
        IOTC_LOGE("%s: Device info not found for name: %s", __func__, name);
        return IOTC_ERR_PARAM_INVALID;
    }
    IotcConDeviceInfo *external_copy = (IotcConDeviceInfo *)IotcMalloc(sizeof(IotcConDeviceInfo));
    if (external_copy == NULL) {
        IOTC_LOGE("Memory allocation failed | Size:%zu", sizeof(IotcConDeviceInfo));
        return IOTC_ERR_PARAM_INVALID;
    }

    external_copy->type = node->type;
    external_copy->connID = node->connID;
    memcpy_s(external_copy->devId, sizeof(external_copy->devId), node->devId, sizeof(node->devId));
    memcpy_s(external_copy->secret, sizeof(external_copy->secret), node->secret, sizeof(node->secret));
    memcpy_s(external_copy->psk, sizeof(external_copy->psk), node->psk, sizeof(node->psk));
    memcpy_s(external_copy->uidHash, sizeof(external_copy->uidHash), node->uidHash, sizeof(node->uidHash));
    memcpy_s(external_copy->authCodeId, sizeof(external_copy->authCodeId), node->authCodeId, sizeof(node->authCodeId));
    memcpy_s(external_copy->authCode, sizeof(external_copy->authCode), node->authCode, sizeof(node->authCode));
    memcpy_s(external_copy->devAddr, sizeof(external_copy->devAddr), node->devAddr, sizeof(node->devAddr));
    external_copy->isSecure = node->isSecure;

    if (node->devInfo != NULL) {
        external_copy->devInfo = IotcMalloc(sizeof(IotcDeviceInfo));
        if (external_copy->devInfo != NULL) {
            memcpy_s(external_copy->devInfo, sizeof(IotcDeviceInfo), node->devInfo, sizeof(IotcDeviceInfo));
        } else {
            external_copy->devInfo = NULL;
        }
    } else {
        external_copy->devInfo = NULL;
    }

    if (node->services != NULL) {
        const char *dataStr = IotcJsonPrint(node->services);
        if (dataStr != NULL) {
            external_copy->services = IotcJsonParse(dataStr);
            IotcFree((void *)dataStr);
        } else {
            external_copy->services = NULL;
        }
    } else {
        external_copy->services = NULL;
    }

    *info = external_copy;
    return IOTC_OK;
}

static void SleClientCloudStartSeekCallback(uint32_t event, void *param, uint32_t len)
{
    NOT_USED(event);
    NOT_USED(len);
    NOT_USED(param);
    IOTC_LOGI("[uuid client] %s: SleClientCloudStartSeekCallback success.", __func__);
    if (SleScanServiceStart() != IOTC_OK) {
        IOTC_LOGE("[uuid client] %s: SleScanServiceStart failed.", __func__);
        return;
    }
}

static void SleClientCloudStatusCallback(uint32_t event, StatusSyncResult *param, uint32_t len)
{
    NOT_USED(event);
    StatusSyncResult *status = param;
    if (status == NULL) {
        IOTC_LOGE("statusResu is NULL");
        return;
    }
    NOT_USED(len);
    if (SleScanServiceStart() != IOTC_OK) {
        IOTC_LOGE("[uuid client] %s: SleScanServiceStart failed.", __func__);
        return;
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

    ret = EventBusSubscribe(SleClientFindStructureCompleteCallback, IOTC_CORE_SLE_EVENT_SSAPC_FIND_STRUCTURE_COMPLETE);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret,
                      "[uuid client] subscribe gatt SleClientFindStructureCompleteCallback err:%d", ret);

    ret = EventBusSubscribe(SleClientConnectParamUpdateCallback, IOTC_CORE_SLE_EVENT_CONNECT_PARAM_UPDATE_REQ);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "[uuid client] subscribe gatt SleClientConnectParamUpdateCallback err:%d",
                      ret);

    ret = EventBusSubscribe(SleClientSpekeFinishedCallback, IOTC_CORE_SLE_EVENT_SPEKE_FINISHED);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret,
        "[uuid client] subscribe gatt SleClientSpekeFinishedCallback err:%d", ret);

    ret = EventBusSubscribe(SleClientAuthSetupFinishedCallback, IOTC_CORE_SLE_EVENT_AUTH_SETUP_FINISHED);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "[uuid client] subscribe gatt SleClientAuthSetupFinishedCallback err:%d",
                      ret);

    ret = EventBusSubscribe(SleClientCloudIssueAuthCodeCallback, IOTC_CORE_SLE_EVENT_AUTH_CODE_SETUP);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "[uuid client] subscribe gatt SleClientCloudIssueAuthCodeCallback err:%d",
                      ret);

    ret = EventBusSubscribe(SleClientCloudStartSeekCallback, IOTC_CORE_SLE_EVENT_START_SEEK_SETUP);

    ret = EventBusSubscribe((EventBusCallback)SleClientCloudStatusCallback, IOTC_EVENT_INNER_CLOUD_SLE_STATUS_RESULT);
    return ret;
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
