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
#include "sle_conn_device_info.h"
#include "config_authinfo.h"
#include "sle_print_data.h"
#include "event_bus_pub.h"
#include "iotc_event.h"


// 获取设备信息响应 clent
static int32_t GetSleSvcDeviceInfoRsp(const uint16_t connId, uint8_t **out, uint32_t *outLen)
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
        //给sevser返回收到
        if (IotcJsonAddNum2Obj(root, PROFILE_DATA_MESSAGE_JSON, MSG_PROFILE_TYPE_NONE) != IOTC_OK) {
            IOTC_LOGE("add msg type err ret=%d", ret);
            break;
        }
        if (IotcJsonAddNum2Obj(root, PROFILE_DATA_ERRCODE, 0) != IOTC_OK) {
            IOTC_LOGE("add prod id err");
            return IOTC_ADAPTER_JSON_ERR_ADD;
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

    return ret;
}

static int32_t SaveDeviceInfoDetails(const IotcJson *devInfoJson, IotcConDeviceInfo *deviceConnInfo)
{
    if (SleJsonGetString(devInfoJson, STR_JSON_MODEL, &deviceConnInfo->devInfo->model) != IOTC_OK) {
        IOTC_LOGE("model copy err");
        return IOTC_CORE_COMM_UTILS_ERR_MALLOC_COPY;
    }
    if (SleJsonGetString(devInfoJson, STR_JSON_DEV_TYPE, &deviceConnInfo->devInfo->devTypeId) != IOTC_OK) {
        IOTC_LOGE("devType copy err");
        return IOTC_CORE_COMM_UTILS_ERR_MALLOC_COPY;
    }
    if (SleJsonGetString(devInfoJson, STR_JSON_MANU, &deviceConnInfo->devInfo->manuId) != IOTC_OK) {
        IOTC_LOGE("manu copy err");
        return IOTC_CORE_COMM_UTILS_ERR_MALLOC_COPY;
    }
    if (SleJsonGetString(devInfoJson, STR_JSON_FWV, &deviceConnInfo->devInfo->fwv) != IOTC_OK) {
        IOTC_LOGE("fwv copy err");
        return IOTC_CORE_COMM_UTILS_ERR_MALLOC_COPY;
    }
    if (SleJsonGetString(devInfoJson, STR_JSON_HWV, &deviceConnInfo->devInfo->hwv) != IOTC_OK) {
        IOTC_LOGE("hwv copy err");
        return IOTC_CORE_COMM_UTILS_ERR_MALLOC_COPY;
    }
    if (SleJsonGetString(devInfoJson, STR_JSON_SWV, &deviceConnInfo->devInfo->swv) != IOTC_OK) {
        IOTC_LOGE("swv copy err");
        return IOTC_CORE_COMM_UTILS_ERR_MALLOC_COPY;
    }
    return IOTC_OK;
}

static int32_t SaveDeviceInfo(uint16_t connId, IotcJson *root)
{
    IotcConDeviceInfo *deviceConnInfo = SleGetConnectionInfoByConnId(connId);
    if (deviceConnInfo == NULL) {
        IOTC_LOGE("malloc err");
        return IOTC_ERR_NOT_INIT;
    }

    IotcJson *vendor = IotcJsonGetObj(root, STR_JSON_VENDOR);
    if (vendor == NULL) {
        IOTC_LOGE("Failed to get JSON object for key '%s' ", STR_JSON_VENDOR);
        return IOTC_ADAPTER_JSON_ERR_GET_OBJ;
    }

    int32_t ret = SleJsonGetString(root, STR_JSON_PRODUCT_ID, &deviceConnInfo->devInfo->prodId);
    if (ret != IOTC_OK) {
        IOTC_LOGE("prodId copy err %d", ret);
        return IOTC_CORE_COMM_UTILS_ERR_MALLOC_COPY;
    }

    ret = SleJsonGetString(root, STR_JSON_SN, &deviceConnInfo->devInfo->sn);
    if (ret != IOTC_OK) {
        IOTC_LOGE("sn copy err %d", ret);
        return IOTC_CORE_COMM_UTILS_ERR_MALLOC_COPY;
    }

    IotcJson *devInfoJson = IotcJsonGetObj(vendor, STR_JSON_DEVICE_INFO);
    if (devInfoJson == NULL) {
        IOTC_LOGE("Failed to get JSON object for key '%s'", STR_JSON_DEVICE_INFO);
        return IOTC_ADAPTER_JSON_ERR_GET_OBJ;
    }

    // 添加服务列表
    IotcJson *services = IotcJsonGetObj(vendor, STR_JSON_SERVICES);
    if (services == NULL) {
        IOTC_LOGE("Failed to get JSON object for key '%s' ", STR_JSON_SERVICES);
        return IOTC_ADAPTER_JSON_ERR_GET_OBJ;
    }

    deviceConnInfo->services = IotcDuplicateJson(services, true);

    IOTC_LOGD("get devInfoJson '%s' ", IotcJsonPrint(devInfoJson));

    if(SleJsonGetString(devInfoJson,  STR_JSON_MODEL , &deviceConnInfo->devInfo->model) != IOTC_OK)
    {
        IOTC_LOGE("model copy err");
        return IOTC_CORE_COMM_UTILS_ERR_MALLOC_COPY;
    }

    if(SleJsonGetString(devInfoJson,  STR_JSON_DEV_TYPE ,&deviceConnInfo->devInfo->devTypeId) != IOTC_OK)
    {
        IOTC_LOGE("devType copy err");
        return IOTC_CORE_COMM_UTILS_ERR_MALLOC_COPY;
    }

    if(SleJsonGetString(devInfoJson,  STR_JSON_MANU ,&deviceConnInfo->devInfo->manuId) != IOTC_OK)
    {
        IOTC_LOGE("manu copy err");
        return IOTC_CORE_COMM_UTILS_ERR_MALLOC_COPY;
    }

    if(SleJsonGetString(devInfoJson,  STR_JSON_FWV ,&deviceConnInfo->devInfo->fwv) != IOTC_OK)
    {
        IOTC_LOGE("vendor copy err");
        return IOTC_CORE_COMM_UTILS_ERR_MALLOC_COPY;
    }

    if(SleJsonGetString(devInfoJson,  STR_JSON_HWV ,&deviceConnInfo->devInfo->hwv) != IOTC_OK)
    {
        IOTC_LOGE("hwv copy err");
        return IOTC_CORE_COMM_UTILS_ERR_MALLOC_COPY;
    }

    if(SleJsonGetString(devInfoJson,  STR_JSON_SWV ,&deviceConnInfo->devInfo->swv) != IOTC_OK)
    {
        IOTC_LOGE("swv copy err");
        return IOTC_CORE_COMM_UTILS_ERR_MALLOC_COPY;
    }

    IOTC_LOGI("SaveDeviceInfo end!! ");
    return IOTC_OK;
}

int32_t GetSleSvcDeviceInfo(const SleCmdParam *param, uint8_t **out, uint32_t *outLen)
{
    CHECK_RETURN_LOGW((param != NULL) && (out != NULL) && (outLen != NULL), IOTC_ERR_PARAM_INVALID, "invalid param");
    IotcJson *root = IotcJsonParse((const char *)param->request);
    if (root == NULL) {
        IOTC_LOGE("DeviceInfo proc reqPayload err");
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }

    int32_t ret = SaveDeviceInfo(param->connId, root);
    if (ret != IOTC_OK) {
        IOTC_LOGE("save device info fail");
    }
    ret = GetSleSvcDeviceInfoRsp(param->connId, out, outLen);
    if (ret != IOTC_OK) {
        IOTC_LOGE("device info rsp fail");
    }
    IotcJsonDelete(root);
    return ret;
}

int32_t CreateSvcDeviceInfoReq(uint8_t **out, uint32_t *outLen)
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
        ret = IotcJsonAddNum2Obj(root, PROFILE_DATA_MESSAGE_JSON, MSG_PROFILE_TYPE_REQ);
        if (ret != IOTC_OK) {
            IOTC_LOGE("add msg type err ret=%d", ret);
            break;
        }
        ret = IotcJsonAddStr2Obj(root, STR_JSON_PRODUCT_ID, ModelGetDevProId());
        if (ret != IOTC_OK) {
            IOTC_LOGE("add prod id err ret=%d", ret);
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