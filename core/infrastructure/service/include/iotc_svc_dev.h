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
#ifndef IOTC_SERVICE_DEVICE_H
#define IOTC_SERVICE_DEVICE_H
#include <stdint.h>
#include <stdbool.h>
#include "iotc_prof_def.h"
#include "adapter_json.h"
#include "iotc_def.h"
#include "comm_def.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DEV_REPORT_TYPE_SYNC = 0,
    DEV_REPORT_TYPE_ASYNC
} DevReportType;

typedef enum {
    DEV_BIND_STATUS_INVALID = 0,
    DEV_BIND_STATUS_NOT_BIND,
    DEV_BIND_STATUS_BIND,
    DEV_BIND_STATUS_REVOKE,
} DevBindStatus;

typedef struct {
    char devId[DEVICE_ID_MAX_STR_LEN + 1];
    char secret[CLOUD_SECRET_MAX_STR_LEN + 1];
    uint8_t psk[CLOUD_PSK_MAX_LEN];
    char url[CLOUD_MAX_URL_LEN + 1];
    char backupUrl[CLOUD_MAX_URL_LEN + 1];
} DevLoginInfo;

typedef struct {
    char devId[DEVICE_ID_MAX_STR_LEN + 1];
    uint8_t psk[CLOUD_PSK_MAX_LEN];
    char code[CLOUD_ACTIVATE_CODE_MAX_STR_LEN + 1];
    char url[CLOUD_MAX_URL_LEN + 1];
    char backupUrl[CLOUD_MAX_URL_LEN + 1];
} DevRegInfo;

typedef struct {
    char devId[DEVICE_ID_MAX_STR_LEN + 1];
    char uidHash[BLE_UID_HASH_LEN + 1];
    char authCodeId[BLE_AUTHCODE_ID_LEN + 1];
    uint8_t authCode[BLE_AUTHCODE_LEN];
} DevAuthInfo;

typedef int32_t (*DevProfReportCharState)(const IotcCharState state[], uint32_t num);
typedef int32_t (*DevCtlReportAll)(DevReportType type);
typedef int32_t (*DevCtlGetCharStates)(const AdapterJson *inArray, AdapterJson **outArray);
typedef int32_t (*DevCtlPutCharStates)(const AdapterJson *inArray, AdapterJson **outArray);
typedef const IotcDeviceInfo *(*DevMdlGetDevInfo)(void);
typedef const IotcServiceInfo *(*DevMdlGetSvcInfo)(uint32_t *num);
typedef DevBindStatus (*DevGetBindStatus)(void);
typedef int32_t (*DevGetLoginInfo)(bool *isExist, DevLoginInfo *info);
typedef int32_t (*DevGetRegisterInfo)(bool *isExist, DevRegInfo *info);
typedef int32_t (*DevGetAuthInfo)(bool *isExist, DevAuthInfo *info);
typedef int32_t (*DevRecvBindInfo)(const AdapterJson *json);
typedef int32_t (*DevRecvAuthInfo)(const AdapterJson *json);
typedef int32_t (*DevRecvLoginInfo)(const AdapterJson *json);
typedef int32_t (*DevCleanLoginInfo)(void);
typedef bool (*DevGetOnlineStatus)(void);

typedef struct {
    DevCtlReportAll onReportAll;
    DevCtlGetCharStates onGetChar;
    DevCtlPutCharStates onPutChar;
    DevProfReportCharState onReportChar;
    DevGetBindStatus onGetBindStatus;
    DevGetLoginInfo onGetLoginInfo;
    DevGetRegisterInfo onGetRegInfo;
    DevGetAuthInfo onGetAuthInfo;
    DevRecvBindInfo onRecvBindInfo;
    DevRecvBindInfo onRecvAuthInfo;
    DevRecvLoginInfo onRecvLoginInfo;
    DevCleanLoginInfo onCleanLoginInfo;
    DevGetOnlineStatus onGetOnlineStatus;
} DevSvcApi;

typedef enum {
    DEVICE_SERVICE_MSG_ID_REPORT = 0,
    DEVICE_SERVICE_MSG_ID_MAX,
} DevSvcMsgId;

int32_t DevSvcProxyRecvBindInfo(const AdapterJson *json);
int32_t DevSvcProxyCtlGetCharStates(const AdapterJson *inArray, AdapterJson **outArray);
int32_t DevSvcProxyCtlReportAll(DevReportType type);
int32_t DevSvcProxyCtlPutCharStates(const AdapterJson *inArray, AdapterJson **outArray);
DevBindStatus DevSvcProxyGetBindStatus(void);
int32_t DevSvcProxyProfReportCharState(const IotcCharState state[], uint32_t num);
int32_t DevSvcProxyGetAuthInfo(bool *isExist, DevAuthInfo *info);
int32_t DevSvcProxyGetRegisterInfo(bool *isExist, DevRegInfo *info);
int32_t DevSvcProxyGetLoginInfo(bool *isExist, DevLoginInfo *info);
int32_t DevSvcProxyRecvAuthInfo(const AdapterJson *json);
int32_t DevSvcProxyRecvLoginInfo(const AdapterJson *json);
int32_t DevSvcProxyCleanLoginInfo(void);
bool DevDevSvcProxyGetOnlineStatus(void);

#ifdef __cplusplus
}
#endif
#endif /* IOTC_SERVICE_DEVICE_H */