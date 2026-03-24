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
#include "sle_svc_device_info.h"
#include "securec.h"
#include "iotc_json.h"
#include "utils_common.h"
#include "utils_assert.h"
#include "sle_common.h"
#include "utils_json.h"
#include "iotc_errcode.h"
#include "comm_def.h"
#include "iotc_svc_dev.h"
#include "service_proxy.h"
#include "iotc_svc.h"
#include "dev_info.h"
#include "config_authinfo.h"
#include "sle_print_data.h"

static int32_t BuildDeviceInfo(IotcJson *root)
{
    char protTypeBuf[PROT_TYPE_MAX_LEN + 1] = {0};
    int32_t protType = ModelGetDevProtType();
    int32_t ret = sprintf_s(protTypeBuf, sizeof(protTypeBuf), "%d", protType);
    if (ret <= 0) {
        IOTC_LOGW("sprintf error %d", protType);
        return IOTC_ERR_SECUREC_SPRINTF;
    }

    UtilsJsonStrItem strItem[] = {
        {STR_JSON_SN, ModelGetDevSn()},
        {STR_JSON_MODEL, ModelGetDevModel()},
        {STR_JSON_DEV_TYPE, ModelGetDevTypeId()},
        {STR_JSON_MANU, ModelGetDevManuId()},
        {STR_JSON_PROD_ID, ModelGetDevProId()},
        {STR_JSON_SLE_MAC, GetSleMacStr()},
        {STR_JSON_FWV, ModelGetDevFwv()},
        {STR_JSON_HWV, ModelGetDevHwv()},
        {STR_JSON_SWV, ModelGetDevSwv()},
        {STR_JSON_PROT_TYPE, protTypeBuf},
        {STR_JSON_SUB_PROD_ID, ModelGetDevSubProId()},
    };

    ret = UtilsJsonAddStrTable(root, strItem, ARRAY_SIZE(strItem));
    if (ret != IOTC_OK) {
        IOTC_LOGE("add dev info err %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static int32_t BuildVendor(IotcJson *root)
{
    IotcJson *devInfo = IotcJsonCreate();
    if (devInfo == NULL) {
        IOTC_LOGE("create vendor err");
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }

    DevAuthInfo authInfo = {0};
    bool isAuthInfoExist = false;
    if (DevSvcProxyGetAuthInfo(&isAuthInfoExist, &authInfo) != IOTC_OK) {
        IOTC_LOGE("get auth info err");
        return IOTC_ERR_SECUREC_SPRINTF;
    }

    int32_t ret = IotcJsonAddStr2Obj(root, STR_JSON_DEVID, authInfo.devId);
    if (ret != IOTC_OK) {
        IOTC_LOGE("get auth info err");
        return IOTC_ERR_SECUREC_SPRINTF;
    }

    ret = BuildDeviceInfo(devInfo);
    if (ret != IOTC_OK) {
        IOTC_LOGE("build vendor err ret=%d", ret);
        IotcJsonDelete(devInfo);
        return ret;
    }

    ret = IotcJsonAddItem2Obj(root, STR_JSON_DEVICE_INFO, devInfo);
    if (ret != IOTC_OK) {
        IOTC_LOGE("add device info err ret=%d", ret);
        IotcJsonDelete(devInfo);
        return ret;
    }
    return IOTC_OK;
}

static int32_t BuildAll(IotcJson *root)
{
    if (IotcJsonAddStr2Obj(root, STR_JSON_PRODUCT_ID, ModelGetDevProId()) != IOTC_OK) {
        IOTC_LOGE("add prod id err");
        return IOTC_ADAPTER_JSON_ERR_ADD;
    }
    if (IotcJsonAddStr2Obj(root, STR_JSON_SN, ModelGetDevSn()) != IOTC_OK) {
        IOTC_LOGE("add sn err");
        return IOTC_ADAPTER_JSON_ERR_ADD;
    }
    IotcJson *vendor = IotcJsonCreate();
    if (vendor == NULL) {
        IOTC_LOGE("create vendor err");
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }
    int32_t ret = BuildVendor(vendor);
    if (ret != IOTC_OK) {
        IOTC_LOGE("build vendor ret=%d", ret);
        IotcJsonDelete(vendor);
        return ret;
    }
    if (IotcJsonAddItem2Obj(root, STR_JSON_VENDOR, vendor) != IOTC_OK) {
        IOTC_LOGE("add vendor err");
        IotcJsonDelete(vendor);
        return IOTC_ADAPTER_JSON_ERR_ADD;
    }

    return IOTC_OK;
}

static int32_t GetSleSvcDeviceInfoReq(const uint16_t connId, uint8_t **out, uint32_t *outLen)
{
    CHECK_RETURN_LOGW((out != NULL) && (outLen != NULL), IOTC_ERR_PARAM_INVALID, "invalid param");
    *out = NULL;
    *outLen = 0;
    IotcJson *root = IotcJsonCreate();
    if (root == NULL) {
        IOTC_LOGE("create err");
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }
    int32_t ret = 0;
    do {
        if (IotcJsonAddNum2Obj(root, PROFILE_DATA_MESSAGE_JSON, MSG_PROFILE_TYPE_RSP) != IOTC_OK) {
            IOTC_LOGE("add msg type err ret=%d", ret);
            break;
        }

        ret = BuildAll(root);
        if (ret != IOTC_OK) {
            IOTC_LOGE("build err ret=%d", ret);
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

int32_t GetSleSvcDeviceInfo(const SleCmdParam *param, uint8_t **out, uint32_t *outLen)
{
    CHECK_RETURN_LOGW((param != NULL) && (out != NULL) && (outLen != NULL), IOTC_ERR_PARAM_INVALID, "invalid param");
    IotcJson *root = IotcJsonParse((const char *)param->request);
    if (root == NULL) {
        IOTC_LOGE("DeviceInfo proc reqPayload err");
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }

    int32_t ret = GetSleSvcDeviceInfoReq(param->connId, out, outLen);
    if (ret != IOTC_OK) {
        IOTC_LOGE("GetSleSvcDeviceInfoReq err");
    }

    IotcJsonDelete(root);
    return ret;
}
