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
#include "adapter_wifi_def.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ADAPTER_WIFI_CONNECT_ERROR = 0,
    ADAPTER_WIFI_CONNECT_OK,
} AdapterWifiConnectResult;

typedef enum {
    ADAPTER_WIFI_MODE_INVALID = 0,
    ADAPTER_WIFI_MODE_STATION,
    ADAPTER_WIFI_MODE_SOFTAP,
} AdapterWifiMode;

typedef enum {
    ADAPTER_WIFI_EVENT_STATE_ERROR = 0,
    ADAPTER_WIFI_EVENT_STATE_OK,
} AdapterWifiEventState;

typedef enum {
    ADAPTER_SOFTAP_STA_EVENT_TYPE_LEAVE = 0,
    ADAPTER_SOFTAP_STA_EVENT_TYPE_JOIN,
} AdapterSoftapStaEventType;

typedef enum {
    ADAPTER_WIFI_SEC_TYPE_INVALID = -1,
    ADAPTER_WIFI_SEC_TYPE_OPEN,
    ADAPTER_WIFI_SEC_TYPE_WEP,
    ADAPTER_WIFI_SEC_TYPE_PSK,
    ADAPTER_WIFI_SEC_TYPE_SAE,
} AdapterWifiSecurityType;

typedef enum {
    WIFI_SCAN_TYPE_SSID = 0,
    WIFI_SCAN_TYPE_BSSID,
    WIFI_SCAN_TYPE_FREQ,
} AdapterWifiScanType;

typedef struct {
    char ssid[ADAPTER_WIFI_SSID_MAX_LEN];
    uint8_t ssidLen;
    uint8_t bssid[ADAPTER_WIFI_BSSID_LEN];
    int32_t freqs;
    AdapterWifiScanType scanType;
} AdapterWifiScanParam;

typedef struct {
    char ssid[ADAPTER_WIFI_SSID_MAX_LEN];
    uint8_t bssid[ADAPTER_WIFI_BSSID_LEN];
    AdapterWifiSecurityType securityType;
    uint8_t rssi;
    uint32_t band;
    uint32_t frequency;
} AdapterWifiInfo;

typedef struct {
    uint32_t num;
    AdapterWifiInfo wifiList[];
} AdapterWifiList;

typedef struct {
    uint32_t ip;
    uint8_t mac[ADAPTER_MAC_ADDRESS_LEN];
} AdapterStationInfo;

typedef struct {
    uint32_t num;
    AdapterStationInfo stationList[];
} AdapterStationList;

typedef struct {
    void (*onWifiStateChanged)(int state);
    void (*onScanFinished)(int state, uint32_t size);
    void (*onSoftapStateChanged)(int state);
    void (*onSoftapStationChanged)(int type, const AdapterStationInfo *info);
} AdapterWifiEvent;

int32_t AdapterGetWifiInfo(uint8_t *ssidBuf, uint32_t *ssidBufLen, uint8_t *pwdBuf, uint32_t *pwdBufLen);

int32_t AdapterSetWifiInfo(const uint8_t *ssid, uint32_t ssidLen, const uint8_t *pwd, uint32_t pwdLen);

int32_t AdapterDeleteWifiInfo(void);

int32_t AdapterReconnectWifi(void);

int32_t AdapterConnectWifi(void);

int32_t AdapterDisconnectWifi(void);

int32_t AdapterRestartWifi(void);

int32_t AdapterConnectWifiByBssid(int32_t type, const uint8_t *bssid, uint32_t bssidLen);

int32_t AdapterGetLastConnectResult(void);

int32_t AdapterScanWifi(const AdapterWifiScanParam *param);

int32_t AdapterGetWifiScanResult(AdapterWifiList **scanList);

int32_t AdapterFreeWifiScanResult(AdapterWifiList *scanList);

int32_t AdapterGetWifiBssid(uint8_t *buf, uint32_t len);

int32_t AdapterGetWifiRssi(int8_t *rssi);

int32_t AdapterRegisterEventCallback(const AdapterWifiEvent *cb);

void AdapterUnregEventCallback(void);

int32_t AdapterStartSoftAp(const uint8_t *ssid, uint32_t ssidLen, const uint8_t *pwd, uint32_t pwdLen);

int32_t AdapterStopSoftAp(void);

int32_t AdapterGetSoftapStationInfo(AdapterStationList **staList);

void AdapterFreeSoftapStationInfo(AdapterStationList *staList);

int32_t AdapterSoftapDisassociateSta(uint8_t *mac, uint32_t macLen);

int32_t AdapterSoftapAddTxPower(int power);

int32_t AdapterGetWifiMode(void);

#ifdef __cplusplus
}
#endif

#endif /* ADAPTER_WIFI_H */
