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
#include "adapter_wifi.h"
#include <stdbool.h>
#include <stddef.h>
#include "securec.h"
#include "wifi_device.h"
#include "wifi_event.h"
#include "wifi_device_config.h"
#include "iotc_errcode.h"
#include "adapter_network.h"
#include "adapter_mem.h"
#include "adapter_os.h"
#include "wifi_hotspot.h"
#include "adapter_log.h"

#define WIFI_STATE_NOT_AVAILABLE 0
#define WIFI_STATE_AVAILABLE 1

#define WIFI_SCANNING 1
#define MAX_SCAN_TIMES 4
#define DEF_SCAN_TIMEOUT 15
#define MS_PER_SECOND 1000

static bool g_isRegisterWifiEvent = false;
static bool g_isStaScanSuccess = false;
static WifiEvent g_eventHandler;
static uint16_t g_disconnectReason = 0;
static bool g_isReasonRefresh = false;
static AdapterWifiEvent g_adapterEventHandler = {0};

int32_t AdapterGetWifiInfo(uint8_t *ssidBuf, uint32_t *ssidBufLen, uint8_t *pwdBuf, uint32_t *pwdBufLen)
{
    if ((ssidBuf == NULL) || (ssidBufLen == NULL) || (pwdBuf == NULL) || (pwdBufLen ==NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    if ((*ssidBufLen == 0) || (*ssidBufLen > ADAPTER_WIFI_SSID_MAX_LEN) ||
        (*pwdBufLen == 0) || (*pwdBufLen > ADAPTER_WIFI_PWD_MAX_LEN)) {
        ADAPTER_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    uint32_t size = WIFI_MAX_CONFIG_SIZE;
    WifiDeviceConfig wifiConfig[WIFI_MAX_CONFIG_SIZE];
    (void)memset_s(wifiConfig, sizeof(wifiConfig), 0, sizeof(wifiConfig));

    if (GetDeviceConfigs(wifiConfig, &size) != WIFI_SUCCESS) {
        ADAPTER_LOGI("get device config fail");
        return IOTC_ADAPTER_WIFI_ERR_GET_INFO;
    }
    if (wifiConfig[0].ssid[0] == '\0') {
        ADAPTER_LOGE("GetDeviceConfigs fail, no ssid");
        return IOTC_ADAPTER_WIFI_ERR_GET_INFO;
    }

    if (strcpy_s((char *)ssidBuf, *ssidBufLen, (char *)wifiConfig[0].ssid) != EOK) {
        ADAPTER_LOGE("strcpy fail");
        (void)memset_s(wifiConfig, sizeof(wifiConfig), 0, sizeof(wifiConfig));
        return IOTC_ERR_SECUREC_STRCPY;
    }
    *ssidBufLen = strlen(wifiConfig[0].ssid);

    if (wifiConfig[0].preSharedKey[0] == '\0') {
        (void)memset_s(wifiConfig, sizeof(wifiConfig), 0, sizeof(wifiConfig));
        *pwdBufLen = 0;
        return IOTC_OK;
    }
    if (strcpy_s((char *)pwdBuf, *pwdBufLen, (char *)wifiConfig[0].preSharedKey) != EOK) {
        ADAPTER_LOGE("strcpy fail");
        (void)memset_s(wifiConfig, sizeof(wifiConfig), 0, sizeof(wifiConfig));
        return IOTC_ERR_SECUREC_STRCPY;
    }
    *pwdBufLen = strlen(wifiConfig[0].preSharedKey);
    (void)memset_s(wifiConfig, sizeof(wifiConfig), 0, sizeof(wifiConfig));
    return IOTC_OK;
}

static int32_t GetWifiConfigFromOhos(WifiDeviceConfig *config)
{
    uint32_t size = WIFI_MAX_CONFIG_SIZE;
    WifiDeviceConfig wifiConfig[WIFI_MAX_CONFIG_SIZE];
    (void)memset_s(wifiConfig, sizeof(wifiConfig), 0, sizeof(wifiConfig));

    static bool isPrint = false;
    if (GetDeviceConfigs(wifiConfig, &size) != WIFI_SUCCESS) {
        if (!isPrint) {
            ADAPTER_LOGE("get device config fail");
            isPrint = true;
        }
        return IOTC_ADAPTER_WIFI_ERR_GET_INFO;
    }
    isPrint = false;

    if (memcpy_s(config, sizeof(WifiDeviceConfig), &wifiConfig[0], sizeof(WifiDeviceConfig)) != EOK) {
        ADAPTER_LOGE("memcpy error");
        (void)memset_s(wifiConfig, sizeof(wifiConfig), 0, sizeof(wifiConfig));
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    (void)memset_s(wifiConfig, sizeof(wifiConfig), 0, sizeof(wifiConfig));
    return IOTC_OK;
}

static int32_t GetWifiNetworkIdFromOhos(int32_t *netId)
{
    WifiDeviceConfig config;
    (void)memset_s(&config, sizeof(WifiDeviceConfig), 0, sizeof(WifiDeviceConfig));

    if (GetWifiConfigFromOhos(&config) != IOTC_OK) {
        return IOTC_ADAPTER_WIFI_ERR_GET_INFO;
    }
    *netId = config.netId;
    (void)memset_s(&config, sizeof(WifiDeviceConfig), 0, sizeof(WifiDeviceConfig));
    return IOTC_OK;
}

static void ClearWifiConfig(void)
{
    int32_t netId = 0;
    (void)GetWifiNetworkIdFromOhos(&netId);

    if (RemoveDevice(netId) != WIFI_SUCCESS) {
        ADAPTER_LOGW("remove wifi config error, netId [%d]", netId);
    }
}

int32_t AdapterSetWifiInfo(const uint8_t *ssid, uint32_t ssidLen, const uint8_t *pwd, uint32_t pwdLen)
{
    WifiDeviceConfig config;
    (void)memset_s(&config, sizeof(WifiDeviceConfig), 0, sizeof(WifiDeviceConfig));

    ClearWifiConfig();

    if ((ssidLen != 0) && (ssid != NULL)) {
        if (memcpy_s(config.ssid, sizeof(config.ssid), ssid, ssidLen) != EOK) {
            ADAPTER_LOGE("memcpy error");
            return IOTC_ERR_SECUREC_MEMCPY;
        }
    } else {
        ADAPTER_LOGW("clear wifi info");
        return IOTC_OK;
    }

    if ((pwdLen != 0) && (pwd != NULL)) {
        if (memcpy_s(config.preSharedKey, sizeof(config.preSharedKey), pwd, pwdLen) != EOK) {
            ADAPTER_LOGE("memcpy error");
            return IOTC_ERR_SECUREC_MEMCPY;
        }
        config.securityType = ADAPTER_WIFI_SEC_TYPE_PSK;
    } else {
        config.securityType = ADAPTER_WIFI_SEC_TYPE_OPEN;
    }
    config.ipType = DHCP;

    int32_t netId = 0;
    int32_t ret = AddDeviceConfig(&config, &netId);
    (void)memset_s(&config, sizeof(WifiDeviceConfig), 0, sizeof(WifiDeviceConfig));
    if (ret != IOTC_OK) {
        ADAPTER_LOGE("add device config error %d", ret);
        return IOTC_ADAPTER_WIFI_ERR_SET_INFO;
    }
    return IOTC_OK;
}

int32_t AdapterRestartWifi(void)
{
    int32_t ret;
    if (IsWifiActive() != WIFI_STA_NOT_ACTIVE) {
        ret = DisableWifi();
        if ((ret != IOTC_OK) && (ret != ERROR_WIFI_NOT_AVAILABLE)) {
            ADAPTER_LOGE("disable wifi error %d", ret);
            return IOTC_ADAPTER_WIFI_ERR_DISABLE;
        }
    }

    ret = EnableWifi();
    if (ret != IOTC_OK) {
        ADAPTER_LOGE("enable wifi error %d", ret);
        return IOTC_ADAPTER_WIFI_ERR_ENABLE;
    }

    return IOTC_OK;
}

static void OnWifiScanStateChangedCallback(int32_t state, int32_t size)
{
    ADAPTER_LOGI("scan state change %d/%u", state, size);
    g_isStaScanSuccess = true;
    int32_t iotcState = state == WIFI_STA_ACTIVE ? ADAPTER_WIFI_EVENT_STATE_OK :
        ADAPTER_WIFI_EVENT_STATE_ERROR;
    
    if (g_adapterEventHandler.onScanFinished != NULL) {
        g_adapterEventHandler.onScanFinished(iotcState, size);
    }
}

static void OnWifiConnectionChangedCallback(int32_t state, WifiLinkedInfo *info)
{
    if (info == NULL) {
        ADAPTER_LOGE("invalid param");
        return;
    }
    ADAPTER_LOGI("state: %d, disconnectedReason %u", state, info->disconnectedReason);
    g_disconnectReason = info->disconnectedReason;
    g_isReasonRefresh = true;
    int32_t iotcState = state == WIFI_STA_ACTIVE ? ADAPTER_WIFI_EVENT_STATE_OK :
        ADAPTER_WIFI_EVENT_STATE_ERROR;
    if (g_adapterEventHandler.onWifiStateChanged != NULL) {
        g_adapterEventHandler.onWifiStateChanged(iotcState);
    }
}

static void OnHotspotStaJoinCallback(StationInfo *info)
{
    if (info == NULL) {
        ADAPTER_LOGE("invalid param");
        return;
    }
    ADAPTER_LOGI("hotspot station join");
    AdapterStationInfo iotcInfo = {0};
    iotcInfo.ip = info->ipAddress;
    int32_t ret = memcpy_s(iotcInfo.mac, sizeof(iotcInfo.mac), info->macAddress, sizeof(info->macAddress));
    if (ret != EOK) {
        ADAPTER_LOGW("memcpy error %u/%u", sizeof(iotcInfo.mac), sizeof(info->macAddress));
        return;
    }
    if (g_adapterEventHandler.onSoftapStationChanged != NULL) {
        g_adapterEventHandler.onSoftapStationChanged(ADAPTER_SOFTAP_STA_EVENT_TYPE_JOIN, &iotcInfo);
    }
}

static void OnHotspotStaLeaveCallback(StationInfo *info)
{
    if (info == NULL) {
        ADAPTER_LOGE("invalid param");
        return;
    }
    ADAPTER_LOGI("hotspot station leave");
    AdapterStationInfo iotcInfo = {0};
    iotcInfo.ip = info->ipAddress;
    int32_t ret = memcpy_s(iotcInfo.mac, sizeof(iotcInfo.mac), info->macAddress, sizeof(info->macAddress));
    if (ret != EOK) {
        ADAPTER_LOGW("memcpy error %u/%u", sizeof(iotcInfo.mac), sizeof(info->macAddress));
        return;
    }
    if (g_adapterEventHandler.onSoftapStationChanged != NULL) {
        g_adapterEventHandler.onSoftapStationChanged(ADAPTER_SOFTAP_STA_EVENT_TYPE_LEAVE, &iotcInfo);
    }
}

static void OnHotspotStateChangedCallback(int32_t state)
{
    int32_t iotcState = state == WIFI_STA_ACTIVE ? ADAPTER_WIFI_EVENT_STATE_OK :
        ADAPTER_WIFI_EVENT_STATE_ERROR;
    
    if (g_adapterEventHandler.onSoftapStateChanged != NULL) {
        g_adapterEventHandler.onSoftapStateChanged(iotcState);
    }
}

static int32_t RegisterWifiEventToOhos(void)
{
    if (g_isRegisterWifiEvent) {
        ADAPTER_LOGW("wifievent has been registered");
        return IOTC_OK;
    }

    g_eventHandler.OnWifiScanStateChanged = OnWifiScanStateChangedCallback;
    g_eventHandler.OnWifiConnectionChanged = OnWifiConnectionChangedCallback;
    g_eventHandler.OnHotspotStaJoin = OnHotspotStaJoinCallback;
    g_eventHandler.OnHotspotStaLeave = OnHotspotStaLeaveCallback;
    g_eventHandler.OnHotspotStateChanged = OnHotspotStateChangedCallback;

    if (RegisterWifiEvent(&g_eventHandler) != WIFI_SUCCESS) {
        ADAPTER_LOGE("Register wifi event fail");
        return IOTC_ADAPTER_WIFI_ERR_REGISTER_WIFI_EVENT;
    }
    g_isRegisterWifiEvent = true;
    return IOTC_OK;
}

static int32_t AddBssidTiWifiConfig(AdapterWifiSecurityType type, const uint8_t *bssid,
    uint32_t len, int32_t *netId)
{
    int32_t ret = IOTC_OK;
    WifiDeviceConfig config;
    (void)memset_s(&config, sizeof(config), 0, sizeof(config));
    do {
        if ((GetWifiConfigFromOhos(&config) != IOTC_OK) || (config.ssid[0] == '\0')) {
            ADAPTER_LOGE("get wifi config fail");
            ret = IOTC_ADAPTER_WIFI_ERR_GET_INFO;
            break;
        }

        if (memcpy_s(config.ssid, sizeof(config.ssid), bssid, len) != EOK) {
            ADAPTER_LOGE("memcpy bssid failed");
            ret = IOTC_ERR_SECUREC_MEMCPY;
            break;
        }

        if (type != ADAPTER_WIFI_SEC_TYPE_INVALID) {
            config.securityType = type;
        }

        if (RemoveDevice(config.netId) != WIFI_SUCCESS) {
            ADAPTER_LOGE("remove config erro");
            ret = IOTC_ADAPTER_WIFI_ERR_REMOVE;
            break;
        }

        if (AddDeviceConfig(&config, netId) != WIFI_SUCCESS) {
            ADAPTER_LOGE("add config erro");
            ret = IOTC_ADAPTER_WIFI_ERR_ADD;
            break;
        }
    } while (0);
    (void)memset_s(&config, sizeof(config), 0, sizeof(config));
    return ret;
}

static int32_t RemoveBssidFromWifiConfig(void)
{
    int32_t ret = IOTC_OK;
    WifiDeviceConfig config;
    (void)memset_s(&config, sizeof(config), 0, sizeof(config));
    do {
        if ((GetWifiConfigFromOhos(&config) != IOTC_OK) || (config.ssid[0] == '\0')) {
            ADAPTER_LOGE("get wifi config fail");
            ret = IOTC_ADAPTER_WIFI_ERR_GET_INFO;
            break;
        }

        (void)memset_s(config.bssid, sizeof(config.bssid), 0, sizeof(config.bssid));

        if (RemoveDevice(config.netId) != WIFI_SUCCESS) {
            ADAPTER_LOGE("remove config error");
            ret = IOTC_ADAPTER_WIFI_ERR_REMOVE;
            break;
        }

        int32_t netId = -1;
        if (AddDeviceConfig(&config, &netId) != WIFI_SUCCESS) {
            ADAPTER_LOGE("add config error");
            ret = IOTC_ADAPTER_WIFI_ERR_ADD;
            break;
        }
    } while (0);
    (void)memset_s(&config, sizeof(config), 0, sizeof(config));
    return ret;
}

int32_t AdapterConnectWifiByBssid(int32_t type, const uint8_t *bssid, uint32_t bssidLen)
{
    if ((bssid == NULL) || (bssidLen == 0)) {
        ADAPTER_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    if (IsWifiActive() != WIFI_STA_ACTIVE) {
        if (EnableWifi() != WIFI_SUCCESS) {
            ADAPTER_LOGE("enable wifi fail");
            return IOTC_ADAPTER_WIFI_ERR_ENABLE;
        }
    }

    if (RegisterWifiEventToOhos() != IOTC_OK) {
        ADAPTER_LOGE("register wifi event fail");
        return IOTC_ADAPTER_WIFI_ERR_SET_INFO;
    }
    g_isReasonRefresh = false;

    int32_t netId;
    if (AddBssidTiWifiConfig(type, bssid, bssidLen, &netId) != IOTC_OK) {
        ADAPTER_LOGE("add bssid to config error");
        return IOTC_ADAPTER_WIFI_ERR_ADD;
    }

    if (AdapterGetNetworkState() == ADAPTER_NETWORK_CONNECTED) {
        if (Disconnect() != WIFI_SUCCESS) {
            ADAPTER_LOGE("disconnect wifi error");
            return IOTC_ADAPTER_WIFI_ERR_DISCONNECT;
        }
    }

    int32_t ret = ConnectTo(netId);
    if (ret != IOTC_OK) {
        ADAPTER_LOGE("connect wifi fail, ret:%d", ret);
        return IOTC_ADAPTER_WIFI_ERR_CONNECT;
    }

    if (RemoveBssidFromWifiConfig() != IOTC_OK) {
        ADAPTER_LOGE("remove bssid to config failed");
        return IOTC_ADAPTER_WIFI_ERR_REMOVE;
    }
    return IOTC_OK;
}

int32_t AdapterGetLastConnectResult(void)
{
    if (!g_isReasonRefresh) {
        return ADAPTER_WIFI_CONNECT_ERROR;
    }

    g_isReasonRefresh = false;
    return ADAPTER_WIFI_CONNECT_OK;
}

static int32_t BuildScanParam(const AdapterWifiScanParam *param, WifiScanParams *scanParams)
{
    AdapterWifiScanType scanType = param->scanType;
    ADAPTER_LOGD("scan type: %d", scanType);

    switch (scanType) {
        case WIFI_SCAN_TYPE_SSID: {
            uint32_t ssidLen = strlen(param->ssid);
            if ((param->ssid[0] == '\0') || (ssidLen != param->ssidLen)) {
                ADAPTER_LOGE("invalied ssid param, len:%u, strlen:%u", param->ssidLen, ssidLen);
                return IOTC_ERR_PARAM_INVALID;
            }
            if (memcpy_s(scanParams->ssid, sizeof(scanParams->ssid), param->ssid, param->ssidLen) != EOK) {
                ADAPTER_LOGE("memcpy error");
                return IOTC_ERR_SECUREC_MEMCPY;
            }
            scanParams->scanType = WIFI_SSID_SCAN;
            scanParams->ssidLen = param->ssidLen;
            break;
        }
        case WIFI_SCAN_TYPE_BSSID: {
            if (memcpy_s(scanParams->bssid, sizeof(scanParams->bssid), param->bssid, sizeof(param->bssid)) != EOK) {
                ADAPTER_LOGE("memcpy error");
                return IOTC_ERR_SECUREC_MEMCPY;
            }
            break;
        }
        case WIFI_SCAN_TYPE_FREQ: {
            scanParams->freqs = param->freqs;
            scanParams->scanType = WIFI_FREQ_SCAN;
            break;
        }
        default:
            ADAPTER_LOGE("not support scan type, type:%d", scanType);
    }
    return IOTC_OK;
}

int32_t AdapterScanWifi(const AdapterWifiScanParam *param)
{
    if (param == NULL) {
        ADAPTER_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    if (IsWifiActive() != WIFI_STA_ACTIVE) {
        if (EnableWifi() != WIFI_SUCCESS) {
            ADAPTER_LOGE("enable wifi fail");
            return IOTC_ADAPTER_WIFI_ERR_ENABLE;
        }
    }

    if (RegisterWifiEventToOhos() != IOTC_OK) {
        ADAPTER_LOGE("register wifi event fail");
        return IOTC_ADAPTER_WIFI_ERR_SET_INFO;
    }

    g_isStaScanSuccess = false;

    WifiScanParams scanParams;
    (void)memset_s(&scanParams, sizeof(scanParams), 0, sizeof(scanParams));
    int32_t ret = BuildScanParam(param, &scanParams);
    if (ret != IOTC_OK) {
        ADAPTER_LOGE("build scan param fail");
        return ret;
    }
    if (AdvanceScan(&scanParams) != WIFI_SUCCESS) {
        ADAPTER_LOGE("wifi advance param fail");
        return IOTC_ADAPTER_WIFI_ERR_SCAN;
    }
    (void)memset_s(&scanParams, sizeof(scanParams), 0, sizeof(scanParams));
    return IOTC_OK;
}

static int32_t GetScanWifiResultList(WifiScanInfo **list, uint32_t *listSize)
{
    uint32_t size = WIFI_SCAN_HOTSPOT_LIMIT;
    WifiScanInfo *result = (WifiScanInfo *)AdapterMalloc(sizeof(WifiScanInfo) * size);
    if (result == NULL) {
        ADAPTER_LOGE("malloc error");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(result, sizeof(WifiScanInfo) * size, 0, sizeof(WifiScanInfo) * size);
    if (GetScanInfoList(result, &size) != WIFI_SUCCESS) {
        ADAPTER_LOGE("get wifi scan info fail");
        AdapterFree(result);
        return IOTC_ADAPTER_WIFI_ERR_SCAN;
    }
    *list = result;
    *listSize = size;
    return IOTC_OK;
}

static int32_t CopyScanWifiResultList(AdapterWifiList **scanList, const WifiScanInfo *result, uint32_t resSize)
{
    uint32_t size = sizeof(AdapterWifiInfo) * resSize + sizeof(AdapterWifiList);
    *scanList = (AdapterWifiList *)AdapterMalloc(size);
    if (*scanList == NULL) {
        ADAPTER_LOGE("malloc error");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(*scanList, size, 0, size);
    AdapterWifiInfo *info = (*scanList)->wifiList;
    for (uint32_t i = 0; i < resSize; i++) {
        if ((strcpy_s(info[i].ssid, sizeof(info[i].ssid), result[i].ssid) != EOK) ||
            (memcpy_s(info[i].bssid, sizeof(info[i].bssid), result[i].bssid, sizeof(result[i].bssid)) != EOK)) {
            AdapterFree(info);
            return IOTC_ERR_SECUREC_MEMCPY;
        }
        info[i].rssi = result[i].rssi;
        info[i].band = result[i].band;
        info[i].securityType = result[i].securityType;
        info[i].frequency = result[i].frequency;
    }
    (*scanList)->num = resSize;
    return IOTC_OK;
}

int32_t AdapterGetWifiScanResult(AdapterWifiList **scanList)
{
    if (scanList == NULL || *scanList != NULL) {
        ADAPTER_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    if (!g_isStaScanSuccess) {
        ADAPTER_LOGE("AP scanning is not complete");
        return IOTC_ADAPTER_WIFI_ERR_SCAN;
    }

    int32_t ret = IOTC_ERROR;
    WifiScanInfo *scanInfo = NULL;
    do {
        uint32_t size = 0;
        ret = GetScanWifiResultList(&scanInfo, &size);
        if (ret != IOTC_OK) {
            ADAPTER_LOGE("get scan wifi list fail, ret: %d", ret);
            ret = IOTC_ADAPTER_WIFI_ERR_SCAN;
            break;
        }
        ADAPTER_LOGN("scan result size: %u", size);
        if (size == 0) {
            AdapterFree(scanInfo);
            return IOTC_OK;
        }

        ret = CopyScanWifiResultList(scanList, scanInfo, size);
        if (ret != IOTC_OK) {
            ADAPTER_LOGE("copy wifi list fail, ret: %d", ret);
            ret = IOTC_ADAPTER_WIFI_ERR_SCAN;
            break;
        }
        (void)memset_s(scanInfo, size * sizeof(WifiScanInfo), 0, size * sizeof(WifiScanInfo));
        AdapterFree(scanInfo);
        return IOTC_OK;
    } while (0);

    AdapterFree(scanInfo);
    AdapterFree(*scanList);
    *scanList  = NULL;
    return ret;
}

int32_t AdapterFreeWifiScanResult(AdapterWifiList *scanList)
{
    if (scanList == NULL) {
        ADAPTER_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    AdapterFree(scanList);
    return IOTC_OK;
}

static int32_t AdvanceScanWifiByOhos(const WifiDeviceConfig *config)
{
    int32_t scanTimeout = DEF_SCAN_TIMEOUT;
    g_isStaScanSuccess = false;

    WifiScanParams scanParams;
    (void)memset_s(&scanParams, sizeof(scanParams), 0, sizeof(scanParams));
    if (memcpy_s(scanParams.ssid, sizeof(scanParams.ssid), config->ssid, strlen(config->ssid)) != EOK) {
        ADAPTER_LOGE("memcpy error");
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    scanParams.scanType = WIFI_SSID_SCAN;
    scanParams.ssidLen = (int8_t)strlen(config->ssid);
    if (AdvanceScan(&scanParams) != WIFI_SUCCESS) {
        ADAPTER_LOGE("wifi advance scan fail");
        return IOTC_ADAPTER_WIFI_ERR_SCAN;
    }

    while (scanTimeout > 0) {
        AdapterSleepMs(MS_PER_SECOND);
        scanTimeout--;
        if (g_isStaScanSuccess) {
            break;
        }
    }
    if (scanTimeout == 0) {
        ADAPTER_LOGE("wifi advance scan timeout");
        return IOTC_ADAPTER_WIFI_ERR_SCAN;
    }
    return IOTC_OK;
}

/* TODO 优化代码深度 */
static bool GetScanWifiResultFromOhos(const WifiDeviceConfig *config, WifiScanInfo *info)
{
    bool ret = false;
    uint32_t size = WIFI_SCAN_HOTSPOT_LIMIT;
    WifiScanInfo *result = (WifiScanInfo *)AdapterMalloc(sizeof(WifiScanInfo) * size);
    if (result == NULL) {
        ADAPTER_LOGE("malloc error");
        return false;
    }
    (void)memset_s(result, sizeof(WifiScanInfo) * size, 0, sizeof(WifiScanInfo) * size);
    if (GetScanInfoList(result, &size) != WIFI_SUCCESS) {
        ADAPTER_LOGE("malloc error");
        AdapterFree(result);
        return false;
    }

    if ((size == 0) || (size > WIFI_SCAN_HOTSPOT_LIMIT)) {
        ADAPTER_LOGW("can not scan any wifi or scan size over limit, size: %u", size);
    } else {
        for (uint32_t i = 0; i < size; i++) {
            if ((strcmp(result[i].ssid, config->ssid) == 0) &&
                ((config->securityType != WIFI_SEC_TYPE_OPEN && result[i].securityType != WIFI_SEC_TYPE_OPEN) ||
                (config->securityType == WIFI_SEC_TYPE_OPEN && result[i].securityType == WIFI_SEC_TYPE_OPEN))) {
                ADAPTER_LOGN("find target ssid success");
                if (memcpy_s(info, sizeof(WifiScanInfo), &result[i], sizeof(WifiScanInfo)) != 0) {
                    ADAPTER_LOGE("memcpy error");
                    break;
                }
                ret = true;
                break;
            }
        }
    }
    AdapterFree(result);
    return ret;
}

static void SetSecurityTypeByScanInfo(WifiDeviceConfig *config, const WifiScanInfo *info)
{
    WifiDeviceConfig tempConfig;
    (void)memset_s(&tempConfig, sizeof(WifiDeviceConfig), 0, sizeof(WifiDeviceConfig));
    if (memcpy_s(&tempConfig, sizeof(WifiDeviceConfig), config, sizeof(WifiDeviceConfig)) != EOK) {
        ADAPTER_LOGE("memcpy error");
        return;
    }
    (void)memset_s(config->bssid, sizeof(config->bssid), 0, sizeof(config->bssid));

    if (info->securityType == ADAPTER_WIFI_SEC_TYPE_INVALID) {
        config->securityType = (config->preSharedKey[0] == '\0') ? ADAPTER_WIFI_SEC_TYPE_OPEN :
            ADAPTER_WIFI_SEC_TYPE_PSK;
    } else {
        config->securityType = info->securityType;
    }

    if (config->securityType == ADAPTER_WIFI_SEC_TYPE_WEP) {
        uint8_t pwdLen = strlen(config->preSharedKey);
        /* WEP 的第5和13卫ASCII密码需要用双引号包起来 */
        if ((pwdLen == 5) || (pwdLen == 13)) {
            char tmpKey[WIFI_MAX_KEY_LEN] = {0};
            tmpKey[0] = '\"';
            /* 2 colon */
            if (memcpy_s(tmpKey + 1, sizeof(tmpKey) - 2, config->preSharedKey, pwdLen) != EOK) {
                (void)memset_s(&tempConfig, sizeof(WifiDeviceConfig), 0, sizeof(WifiDeviceConfig));
                return;
            }
            tmpKey[pwdLen + 1] = '\"';
            if (memcpy_s(config->preSharedKey, sizeof(config->preSharedKey), tmpKey, WIFI_MAX_KEY_LEN) != EOK) {
                (void)memset_s(&tempConfig, sizeof(WifiDeviceConfig), 0, sizeof(WifiDeviceConfig));
                (void)memset_s(tmpKey, sizeof(tmpKey), 0, sizeof(tmpKey));
                return;
            }
            (void)memset_s(tmpKey, sizeof(tmpKey), 0, sizeof(tmpKey));
        }
    }
    int32_t netId;
    if (memcmp(config, &tempConfig, sizeof(WifiDeviceConfig)) != 0) {
        ADAPTER_LOGN("ohos config need refresh");
        if (RemoveDevice(tempConfig.netId) != WIFI_SUCCESS) {
            ADAPTER_LOGE("remove config error");
            (void)memset_s(&tempConfig, sizeof(WifiDeviceConfig), 0, sizeof(WifiDeviceConfig));
            return;
        }
        if (AddDeviceConfig(config, &netId) != WIFI_SUCCESS) {
            ADAPTER_LOGE("add config fail");
            (void)memset_s(&tempConfig, sizeof(WifiDeviceConfig), 0, sizeof(WifiDeviceConfig));
            return;
        }
        config->netId = netId;
    }
    (void)memset_s(&tempConfig, sizeof(WifiDeviceConfig), 0, sizeof(WifiDeviceConfig));
    return;
}

int32_t AdapterConnectWifi(void)
{
    int32_t ret;
    WifiDeviceConfig config;
    (void)memset_s(&config, sizeof(config), 0, sizeof(config));
    WifiScanInfo info;
    (void)memset_s(&info, sizeof(info), 0, sizeof(info));

    if (IsWifiActive() != WIFI_STA_ACTIVE) {
        if (EnableWifi() != WIFI_SUCCESS) {
            ADAPTER_LOGE("enable wifi fail");
            return IOTC_ADAPTER_WIFI_ERR_SET_INFO;
        }
    }
    if (RegisterWifiEventToOhos() != IOTC_OK) {
        ADAPTER_LOGE("register wifi event fail");
        return IOTC_ADAPTER_WIFI_ERR_SET_INFO;
    }
    if (GetWifiConfigFromOhos(&config) != IOTC_OK) {
        ADAPTER_LOGE("get wifi config fail");
        return IOTC_ADAPTER_WIFI_ERR_GET_INFO;
    }

    if (config.ssid[0] == '\0') {
        ADAPTER_LOGE("ssid null");
        return IOTC_ADAPTER_WIFI_ERR_GET_INFO;
    }

    for (uint32_t i = 0; i < MAX_SCAN_TIMES; ++i) {
        if (AdvanceScanWifiByOhos(&config) != IOTC_OK) {
            ADAPTER_LOGE("advance scan wifi fail");
            (void)memset_s(&config, sizeof(config), 0, sizeof(config));
            return IOTC_ADAPTER_WIFI_ERR_SCAN;
        }
        if (GetScanWifiResultFromOhos(&config, &info)) {
            SetSecurityTypeByScanInfo(&config, &info);
            break;
        }
        ADAPTER_LOGN("not find target wifi, tyr again");
    }
    ret = ConnectTo(config.netId);
    (void)memset_s(&config, sizeof(config), 0, sizeof(config));
    if (ret != IOTC_OK) {
        ADAPTER_LOGE("connect to wifi fail, ret: %d", ret);
        return IOTC_ADAPTER_WIFI_ERR_CONNECT;
    }
    return IOTC_OK;
}

int32_t AdapterReconnectWifi(void)
{
    (void)Disconnect();
    if (AdapterConnectWifi() != IOTC_OK) {
        ADAPTER_LOGE("connect wifi error");
        return IOTC_ADAPTER_WIFI_ERR_CONNECT;
    }
    return IOTC_OK;
}

int32_t AdapterGetWifiBssid(uint8_t *buf, uint32_t len)
{
    if ((buf == NULL) || (len == 0)) {
        ADAPTER_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    WifiLinkedInfo info;
    (void)memset_s(&info, sizeof(info), 0, sizeof(info));

    if (GetLinkedInfo(&info) != WIFI_SUCCESS) {
        ADAPTER_LOGE("get wifi linked info fail");
        return IOTC_ADAPTER_WIFI_ERR_GET_INFO;
    }

    if (memcpy_s(buf, len, info.bssid, sizeof(info.bssid)) != EOK) {
        ADAPTER_LOGE("memcpy error");
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    return IOTC_OK;
}

int32_t AdapterGetWifiRssi(int8_t *rssi)
{
    if (rssi == NULL) {
        ADAPTER_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    WifiLinkedInfo info;
    (void)memset_s(&info, sizeof(info), 0, sizeof(info));
    if (GetLinkedInfo(&info) != WIFI_SUCCESS) {
        ADAPTER_LOGE("get wifi linked info fail");
        return IOTC_ADAPTER_WIFI_ERR_GET_INFO;
    }

    *rssi = (int8_t)info.rssi;
    return IOTC_OK;
}

int32_t AdapterDeleteWifiInfo(void)
{
    ClearWifiConfig();
    return IOTC_OK;
}

int32_t AdapterRegisterEventCallback(const AdapterWifiEvent *cb)
{
    if (cb == NULL) {
        return IOTC_ERR_PARAM_INVALID;
    }
    int32_t ret = RegisterWifiEventToOhos();
    if (ret != IOTC_OK) {
        return ret;
    }

    ret = memcpy_s(&g_adapterEventHandler, sizeof(AdapterWifiEvent), cb, sizeof(AdapterWifiEvent));
    if (ret != IOTC_OK) {
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    return IOTC_OK;
}

int32_t AdapterDisconnectWifi(void)
{
    int32_t ret = Disconnect();
    if (ret != WIFI_SUCCESS) {
        ADAPTER_LOGW("disconnect wifi error %d", ret);
        return IOTC_ADAPTER_WIFI_ERR_DISCONNECT;
    }
    return IOTC_OK;
}

static void CheckWifiStateBeforeStartSoftap(void)
{
    int32_t ret;
    if (IsWifiActive() == WIFI_STA_ACTIVE) {
        ADAPTER_LOGI("sta active, try stop!");
        ret = DisableWifi();
        if (ret != WIFI_SUCCESS) {
            ADAPTER_LOGW("stop sta fail %d", ret);
        }
    }

    if (IsHotspotActive() == WIFI_HOTSPOT_ACTIVE) {
        ADAPTER_LOGI("ap active, try stop!");
        ret = DisableHotspot();
        if (ret != WIFI_SUCCESS) {
            ADAPTER_LOGW("stop ap fail %d", ret);
        }
    }
}

int32_t AdapterStartSoftAp(const uint8_t *ssid, uint32_t ssidLen, const uint8_t *pwd, uint32_t pwdLen)
{
    (void)pwd;
    (void)pwdLen;
    if ((ssid == NULL) || (ssidLen == 0)) {
        ADAPTER_LOGE("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    CheckWifiStateBeforeStartSoftap();

    HotspotConfig config;
    (void)memset_s(&config, sizeof(HotspotConfig), 0, sizeof(HotspotConfig));

    int32_t ret = strncpy_s(config.ssid, sizeof(config.ssid), (const char *)ssid, ssidLen);
    if (ret != EOK) {
        ADAPTER_LOGE("strcpy error %u", ssidLen);
        return IOTC_ERR_SECUREC_STRCPY;
    }

    if (pwd != NULL && pwdLen != 0) {
        ret = strncpy_s(config.preSharedKey, sizeof(config.preSharedKey), (const char *)pwd, pwdLen);
        if (ret != EOK) {
            ADAPTER_LOGE("strcpy error %u", pwdLen);
            return IOTC_ERR_SECUREC_STRCPY;
        }
        config.securityType = WIFI_SEC_TYPE_PSK;
    } else {
        config.securityType = WIFI_SEC_TYPE_OPEN;
    }

    config.channelNum = HOTSPOT_DEFAULT_CHANNEL;
    config.band = HOTSPOT_BAND_TYPE_2G;
    ret = SetHotspotConfig(&config);
    (void)memset_s(&config, sizeof(HotspotConfig), 0, sizeof(HotspotConfig));
    if (ret != WIFI_SUCCESS) {
        ADAPTER_LOGE("set hotspot config fail %d", ret);
        return IOTC_ADAPTER_SOFTAP_ERR_START;
    }

    ADAPTER_LOGI("start softap");
    ret = EnableHotspot();
    if (ret != WIFI_SUCCESS) {
        ADAPTER_LOGE("enable hotspot fail %d", ret);
        return IOTC_ADAPTER_SOFTAP_ERR_START;
    }

    return IOTC_OK;
}

int32_t AdapterStopSoftAp(void)
{
    if (IsHotspotActive() == WIFI_HOTSPOT_ACTIVE) {
        ADAPTER_LOGI("stop softap");
        if (DisableHotspot() != WIFI_SUCCESS) {
            ADAPTER_LOGE("stop ap fail");
            return IOTC_ADAPTER_SOFTAP_ERR_STOP;
        }
    }
    return IOTC_OK;
}

void AdapterUnregEventCallback(void)
{
    (void)memset_s(&g_adapterEventHandler, sizeof(AdapterWifiEvent), 0, sizeof(AdapterWifiEvent));
}

int32_t AdapterGetSoftapStationInfo(AdapterStationList **staList)
{
    if (staList == NULL || *staList != NULL) {
        ADAPTER_LOGW("param invalid");
        return IOTC_ERR_PARAM_INVALID;
    }

    StationInfo infos[WIFI_MAX_STA_NUM] = {0};
    uint32_t size = WIFI_MAX_STA_NUM;
    int32_t ret = GetStationList(infos, &size);
    if (ret != WIFI_SUCCESS || size > WIFI_MAX_STA_NUM) {
        ADAPTER_LOGE("get station list error %d/%u", ret, size);
        return IOTC_ADAPTER_WIFI_ERR_SOFTAP_GET_STA_LIST;
    }

    uint32_t len = sizeof(AdapterStationList) + sizeof(AdapterStationInfo) * size;
    *staList = (AdapterStationList *)AdapterMalloc(len);
    if (*staList == NULL) {
        ADAPTER_LOGW("malloc error %u", len);
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    AdapterStationInfo *staInfo = (*staList)->stationList;
    for (uint32_t i = 0; i < size; ++i) {
        staInfo[i].ip = infos[i].ipAddress;
        ret = memcpy_s(staInfo[i].mac, sizeof(staInfo[i].mac), infos[i].macAddress, sizeof(infos[i].macAddress));
        if (ret != EOK) {
            AdapterFree(*staList);
            *staList = NULL;
            return IOTC_ERR_SECUREC_MEMCPY;
        }
    }
    (*staList)->num = size;
    return IOTC_OK;
}

void AdapterFreeSoftapStationInfo(AdapterStationList *staList)
{
    if (staList == NULL) {
        ADAPTER_LOGW("param invalid");
        return;
    }
    AdapterFree(staList);
    return;
}

int32_t AdapterSoftapDisassociateSta(uint8_t *mac, uint32_t len)
{
    if (mac == NULL || len != ADAPTER_MAC_ADDRESS_LEN) {
        ADAPTER_LOGW("param invalid");
        return IOTC_ERR_PARAM_INVALID;
    }

    int32_t ret = DisassociateSta(mac, len);
    if (ret != WIFI_SUCCESS) {
        ADAPTER_LOGE("disassociate sta error %d", ret);
        return IOTC_ADAPTER_WIFI_ERR_SOFTAP_DISASSOCIATE_STA;
    }
    return IOTC_OK;
}

int32_t AdapterSoftapAddTxPower(int power)
{
    int32_t ret = AddTxPowerInfo(power);
    if (ret != WIFI_SUCCESS) {
        ADAPTER_LOGE("add tx power error %d", ret);
        return IOTC_ADAPTER_WIFI_ERR_SOFTAP_ADD_TX_POWER;
    }
    return IOTC_OK;
}

int32_t AdapterGetWifiMode(void)
{
    if (IsHotspotActive() == WIFI_HOTSPOT_ACTIVE) {
        return ADAPTER_WIFI_MODE_SOFTAP;
    }
    if (IsWifiActive() == WIFI_STA_ACTIVE) {
        return ADAPTER_WIFI_MODE_STATION;
    }
    return ADAPTER_WIFI_MODE_INVALID;
}