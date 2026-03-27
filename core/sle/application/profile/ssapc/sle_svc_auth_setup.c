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
#include <string.h>
#include "sle_svc_auth_setup.h"
#include "utils_assert.h"
#include "utils_json.h"
#include "iotc_svc_dev.h"
#include "iotc_errcode.h"
#include "iotc_sle_def.h"
#include "sle_conn_device_info.h"
#include "securec.h"
#include "event_bus_pub.h"
#include "sle_comm_status.h"
#include "iotc_event.h"
#include "sle_print_data.h"
#include "comm_def.h"
#include "dev_info.h"

typedef struct DeviceInfoReport {
    char deviceId[DEVICE_ID_MAX_STR_LEN + 1];
    uint8_t online;
    char *lastChange;
} DeviceInfoReport;

typedef enum {
    M2M_CLOUD_DEV_STATE_OFFLINE = 0,
    M2M_CLOUD_DEV_STATE_ONLINE = 1,
    M2M_CLOUD_DEV_STATE_PENDING = 2,
    M2M_CLOUD_DEV_STATE_SYNC = 3,
} M2MCloudDevState;

static int32_t ConstCharCopy(const char *src, const char **outStr)
{
    if (*outStr != NULL) {
        IotcFree((char *)*outStr);
    }

    char *dup = strdup(src);
    if (dup == NULL) {
        IOTC_LOGE("strdup(%s) failed", src);
        *outStr = NULL;
        return IOTC_ERROR;
    }

    *outStr = dup;
    return IOTC_OK;
}

int32_t AuthSetupAndDevInfo()
{
    SleConnRetDeviceInfo retDev = {0};
    retDev.connID = GATEWAY_CONNID; //桥设备的假连接ID
    retDev.status = IOTC_CONN_SLE_STATE_CONNECTED;
    if (SleAddConnDev(&retDev) != IOTC_OK) {
        return IOTC_ERROR;
    }

    IotcConDeviceInfo *deviceInfo = SleGetConnectionInfoByConnId(retDev.connID);
    if (deviceInfo == NULL) {
        IOTC_LOGE("malloc err");
        return IOTC_ERR_NOT_INIT;
    }

    IOTC_LOGI("deviceInfo->connID:%d", deviceInfo->connID);

    DevAuthInfo authInfo = {0};
    bool isAuthInfoExist = false;
    if (DevSvcProxyGetAuthInfo(&isAuthInfoExist, &authInfo) != IOTC_OK) {
        IOTC_LOGE("get auth info err");
        return IOTC_ERR_SECUREC_SPRINTF;
    }

    if (!isAuthInfoExist) {
        IOTC_LOGE("get auth info err");
        return IOTC_ERR_SECUREC_SPRINTF;
    }

    int32_t ret = PopulateDeviceInfo(deviceInfo, &authInfo);
    if (ret != IOTC_OK) {
        return ret;
    }

    IOTC_LOGI("AuthSetupAndDevInfo success ");
    return IOTC_OK;
}

// 保存从生态设备获取到的设备信息到连接信息中
static int32_t SaveAuthSetupConfigInfo(uint16_t connId, IotcJson *root)
{
    IOTC_LOGI("GetSleSvcAuthSetup 111111");
    IotcConDeviceInfo *deviceInfo = SleGetConnectionInfoByConnId(connId);
    if (deviceInfo == NULL) {
        IOTC_LOGE("malloc err");
        return IOTC_ERR_NOT_INIT;
    }

    if (UtilsJsonGetString(root, STR_JSON_DEVID, deviceInfo->devId, DEVICE_ID_MAX_STR_LEN + 1) != IOTC_OK) {
        IOTC_LOGE("GetSleSvcAuthSetup %s proc reqPayload err", STR_JSON_DEVID);
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }

    if (UtilsJsonGetString(root, STR_JSON_UIDHASH, deviceInfo->uidHash, BLE_UID_HASH_LEN) != IOTC_OK) {
        IOTC_LOGE("GetSleSvcAuthSetup %s proc reqPayload err", STR_JSON_UIDHASH);
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }

    if (UtilsJsonGetString(root, STR_JSON_AUTHCODE_ID, deviceInfo->authCodeId, BLE_AUTHCODE_ID_LEN + 1) != IOTC_OK) {
        IOTC_LOGE("GetSleSvcAuthSetup %s proc reqPayload err", STR_JSON_AUTHCODE_ID);
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }

    DeviceInfoReport devIdReport = {0};
    devIdReport.online = M2M_CLOUD_DEV_STATE_PENDING;
    if (strncpy_s(devIdReport.deviceId, DEVICE_ID_MAX_STR_LEN + 1, deviceInfo->devId, sizeof(deviceInfo->devId)) !=
        EOK) {
        IOTC_LOGE("GetSleSvcAuthSetup strncpy_s %s proc deviceId err", devIdReport.deviceId);
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    // 完成通知
    NotifyFinishedStatus status = {0};
    status.errorCode = IOTC_OK;
    status.connSessionId = connId;
    EventBusPublishSync(IOTC_CORE_SLE_EVENT_AUTH_SETUP_FINISHED, (void *)&status, sizeof(NotifyFinishedStatus));
    // 通知云测该设备id上线，获取authcode再下一步
    EventBusPublishSync(IOTC_EVENT_INNER_CLOUD_SLE_SETUP, (void *)&devIdReport, sizeof(DeviceInfoReport));

    return IOTC_OK;
}

int32_t CreateSvcAuthSetupIssue(uint8_t **out, uint32_t *outLen)
{
    CHECK_RETURN_LOGW((out != NULL) && (outLen != NULL), IOTC_ERR_PARAM_INVALID, "invalid param");
    *out = NULL;
    *outLen = 0;
    IotcJson *root = IotcJsonCreate();
    if (root == NULL) {
        IOTC_LOGE("create err");
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }

    int32_t ret = IOTC_OK;
    do {
        ret = IotcJsonAddNum2Obj(root, PROFILE_DATA_MESSAGE_JSON, (int64_t)MSG_PROFILE_TYPE_ISSUE);
        if (ret != IOTC_OK) {
            IOTC_LOGE("add msg type err ret=%d", ret);
            break;
        }

        ret = IotcJsonAddStr2Obj(root, STR_JSON_DEVID, "1234567890123456789012345678901234567890");
        if (ret != IOTC_OK) {
            IOTC_LOGE("add devId err ret=%d", ret);
            break;
        }

        const char hexStr[] = "48656C6C6F48656C6C6F48656C6C6F";
        ret = IotcJsonAddStr2Obj(root, STR_JSON_AUTHCODE, hexStr);
        if (ret != IOTC_OK) {
            IOTC_LOGE("add devType err ret=%d", ret);
            break;
        }
        ret = IotcJsonAddStr2Obj(root, STR_JSON_AUTHCODE_ID, "12345678901234567890123456789");
        if (ret != IOTC_OK) {
            IOTC_LOGE("add sn err ret=%d", ret);
            break;
        }

        ret = IotcJsonAddStr2Obj(root, STR_JSON_UIDHASH, "12345678901234567890123456789012345678900");
        if (ret != IOTC_OK) {
            IOTC_LOGE("add model err ret=%d", ret);
            break;
        }

        char *outStr = UtilsJsonPrintByMalloc(root);
        if (outStr == NULL) {
            IOTC_LOGE("json print err");
            ret = IOTC_CORE_COMM_UTILS_ERR_JSON_MALLOC_PRINT;
            break;
        }
        *out = (uint8_t *)outStr;
        *outLen = strlen(outStr);
        ret = IOTC_OK;
    } while (false);
    IotcJsonDelete(root);
    return ret;
}

int32_t CreateSvcAuthSetupGet(uint8_t **out, uint32_t *outLen)
{
    CHECK_RETURN_LOGW((out != NULL) && (outLen != NULL), IOTC_ERR_PARAM_INVALID, "invalid param");
    *out = NULL;
    *outLen = 0;
    IotcJson *root = IotcJsonCreate();
    if (root == NULL) {
        IOTC_LOGE("create err");
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }

    int32_t ret = IOTC_OK;
    do {
        ret = IotcJsonAddNum2Obj(root, PROFILE_DATA_MESSAGE_JSON, (int64_t)MSG_PROFILE_TYPE_REQ);
        if (ret != IOTC_OK) {
            IOTC_LOGE("add msg type err ret=%d", ret);
            break;
        }

        char *outStr = UtilsJsonPrintByMalloc(root);
        if (outStr == NULL) {
            IOTC_LOGE("json print err");
            ret = IOTC_CORE_COMM_UTILS_ERR_JSON_MALLOC_PRINT;
            break;
        }
        *out = (uint8_t *)outStr;
        *outLen = strlen(outStr);
        ret = IOTC_OK;
    } while (false);
    IotcJsonDelete(root);
    return ret;
}

int32_t GetSleSvcAuthSetup(const SleCmdParam *param, uint8_t **out, uint32_t *outLen)
{
    CHECK_RETURN_LOGW((param != NULL) && (out != NULL) && (outLen != NULL), IOTC_ERR_PARAM_INVALID, "invalid param");
    IotcJson *root = IotcJsonParse((const char *)param->request);
    if (root == NULL) {
        IOTC_LOGE("GetSleSvcAuthSetup proc reqPayload err");
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }

    int32_t ret = SaveAuthSetupConfigInfo(param->connId, root);
    if (ret != IOTC_OK) {
        IOTC_LOGE("save device info fail");
    }

    IotcJsonDelete(root);
    return ret;
}
