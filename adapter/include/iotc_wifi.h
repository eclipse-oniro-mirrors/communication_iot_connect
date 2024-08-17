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
#ifndef ADAPTER_WIFI_H
#define ADAPTER_WIFI_H

#include <stdint.h>
#include "iotc_wifi_def.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IOTC_WIFI_CONNECT_ERROR = 0,
    IOTC_WIFI_CONNECT_OK,
} IotcWifiConnectResult;

typedef enum {
    IOTC_WIFI_MODE_INVALID = 0,
    IOTC_WIFI_MODE_STATION,
    IOTC_WIFI_MODE_SOFTAP,
} IotcWifiMode;

typedef enum {
    IOTC_WIFI_EVENT_STATE_ERROR = 0,
    IOTC_WIFI_EVENT_STATE_OK,
} IotcWifiEventState;

typedef enum {
    IOTC_SOFTAP_STA_EVENT_TYPE_LEAVE = 0,
    IOTC_SOFTAP_STA_EVENT_TYPE_JOIN,
} IotcSoftapStaEventType;

typedef enum {
    IOTC_WIFI_SEC_TYPE_INVALID = -1,
    IOTC_WIFI_SEC_TYPE_OPEN,
    IOTC_WIFI_SEC_TYPE_WEP,
    IOTC_WIFI_SEC_TYPE_PSK,
    IOTC_WIFI_SEC_TYPE_SAE,
} IotcWifiSecurityType;

typedef enum {
    IOTC_WIFI_SCAN_TYPE_SSID = 0,
    IOTC_WIFI_SCAN_TYPE_BSSID,
    IOTC_WIFI_SCAN_TYPE_FREQ,
} IotcWifiScanType;

typedef struct {
    char ssid[IOTC_WIFI_SSID_MAX_LEN];
    uint8_t ssidLen;
    uint8_t bssid[IOTC_WIFI_BSSID_LEN];
    int32_t freqs;
    IotcWifiScanType scanType;
} IotcWifiScanParam;

typedef struct {
    char ssid[IOTC_WIFI_SSID_MAX_LEN];
    uint8_t bssid[IOTC_WIFI_BSSID_LEN];
    IotcWifiSecurityType securityType;
    uint8_t rssi;
    uint32_t band;
    uint32_t frequency;
} IotcWifiInfo;

typedef struct {
    uint32_t num;
    IotcWifiInfo wifiList[];
} IotcWifiList;

typedef struct {
    uint32_t ip;
    uint8_t mac[IOTC_MAC_ADDRESS_LEN];
} IotcStationInfo;

typedef struct {
    uint32_t num;
    IotcStationInfo stationList[];
} IotcStationList;

typedef struct {
    void (*onWifiStateChanged)(int state);
    void (*onScanFinished)(int state, uint32_t size);
    void (*onSoftapStateChanged)(int state);
    void (*onSoftapStationChanged)(int type, const IotcStationInfo *info);
} IotcWifiEvent;

int32_t IotcGetWifiInfo(uint8_t *ssidBuf, uint32_t *ssidBufLen, uint8_t *pwdBuf, uint32_t *pwdBufLen);

int32_t IotcSetWifiInfo(const uint8_t *ssid, uint32_t ssidLen, const uint8_t *pwd, uint32_t pwdLen);

int32_t IotcDeleteWifiInfo(void);

int32_t IotcReconnectWifi(void);

int32_t IotcConnectWifi(void);

int32_t IotcDisconnectWifi(void);

int32_t IotcRestartWifi(void);

int32_t IotcConnectWifiByBssid(int32_t type, const uint8_t *bssid, uint32_t bssidLen);

int32_t IotcGetLastConnectResult(void);

int32_t IotcScanWifi(const IotcWifiScanParam *param);

int32_t IotcGetWifiScanResult(IotcWifiList **scanList);

int32_t IotcFreeWifiScanResult(IotcWifiList *scanList);

int32_t IotcGetWifiBssid(uint8_t *buf, uint32_t len);

int32_t IotcGetWifiRssi(int8_t *rssi);

int32_t IotcRegisterEventCallback(const IotcWifiEvent *cb);

void IotcUnregEventCallback(void);

int32_t IotcStartSoftAp(const uint8_t *ssid, uint32_t ssidLen, const uint8_t *pwd, uint32_t pwdLen);

int32_t IotcStopSoftAp(void);

int32_t IotcGetSoftapStationInfo(IotcStationList **staList);

void IotcFreeSoftapStationInfo(IotcStationList *staList);

int32_t IotcSoftapDisassociateSta(uint8_t *mac, uint32_t macLen);

int32_t IotcSoftapAddTxPower(int power);

int32_t IotcGetWifiMode(void);

#ifdef __cplusplus
}
#endif

#endif /* ADAPTER_WIFI_H */
