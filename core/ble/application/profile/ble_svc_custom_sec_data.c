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
#include "ble_svc_custom_sec_data.h"
#include "adapter_json.h"
#include "utils_common.h"
#include "utils_assert.h"
#include "utils_json.h"
#include "iotc_conf.h"
#include "sched_executor.h"
#include "ble_session_mngr.h"
#include "ble_profile.h"
#include "ble_linklayer.h"
#include "iotc_svc_dev.h"
#include "iotc_errcode.h"
#include "product_adapter.h"

static int32_t GetSeq(AdapterJson *root, uint32_t *seq)
{
    AdapterJson *seqItem = AdapterJsonGetObj(root, STR_JSON_SEQ);
    if (seqItem == NULL) {
        IOTC_LOGE("get seq item err");
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    int64_t num = 0;
    int32_t ret = AdapterJsonGetNum(seqItem, &num);
    if (ret != IOTC_OK) {
        IOTC_LOGE("get seq err=%d", ret);
        return ret;
    }
    *seq = (uint32_t)num;
    return IOTC_OK;
}

static bool IsAllServicesSid(AdapterJson *vendorItem)
{
    uint32_t size = 0;
    int32_t ret = AdapterJsonGetArraySize(vendorItem, &size);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get size error %d", ret);
        return false;
    }
    /* 查询所有服务时只会有1个sid */
    if (size != 1) {
        return false;
    }
    const char *sid = AdapterJsonGetStr(AdapterJsonGetObj(AdapterJsonGetArrayItem(vendorItem, 0), STR_JSON_SID));
    if (sid == NULL || strcmp(sid, STR_JSON_ALL_SERVICE) != 0) {
        return false;
    }
    return true;
}

static int32_t BuildBleCustomSecDataService(AdapterJson *item, uint8_t **out, uint32_t *outLen)
{
    AdapterJson *root = AdapterCreateJson();
    if (root == NULL) {
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }

    int32_t ret = AdapterJsonAddNum2Obj(root, STR_JSON_SEQ, (int64_t)BleSessSendSeqGet());
    if (ret != IOTC_OK) {
        IOTC_LOGW("add seq error %d", ret);
        AdapterJsonDelete(root);
        return ret;
    }

    AdapterJson *dupItem = AdapterDuplicateJson(item, true);
    if (dupItem == NULL) {
        IOTC_LOGW("duplicate item err");
        AdapterJsonDelete(root);
        return IOTC_ADAPTER_JSON_ERR_DUPLICATE;
    }

    ret = AdapterJsonAddItem2Obj(root, STR_JSON_VENDOR, dupItem);
    if (ret != IOTC_OK) {
        AdapterJsonDelete(root);
        AdapterJsonDelete(dupItem);
        return IOTC_ADAPTER_JSON_ERR_ADD;
    }

    *out = (uint8_t *)UtilsJsonPrintByMalloc(root);
    AdapterJsonDelete(root);
    if (*out == NULL) {
        return IOTC_CORE_COMM_UTILS_ERR_JSON_MALLOC_PRINT;
    }
    *outLen = strlen((const char *)*out);
    BleSessSendSeqUpdate();
    return IOTC_OK;
}

#if IOTC_CONF_AILIFE_SUPPORT
static int32_t BuildBleCustomSecDataSingleService(AdapterJson *array, uint8_t **out, uint32_t *outLen)
{
    uint32_t size = 0;
    int32_t ret = AdapterJsonGetArraySize(array, &size);
    CHECK_RETURN_LOGW(ret == IOTC_OK, ret, "get arr size err:%d", ret);
    CHECK_RETURN_LOGW(size == 1, IOTC_CORE_BLE_ERR_CUSTOM_DATA_SIZE, "get arr size:%u err", size);
    AdapterJson *item = AdapterJsonGetArrayItem(array, 0);
    CHECK_RETURN_LOGW(item != NULL, IOTC_ADAPTER_JSON_ERR_GET_ARRAY_ITEM, "get arr item err");
    return BuildBleCustomSecDataService(item, out, outLen);
}
#endif

static int32_t BleCustomSecDataGetChar(AdapterJson *vendorItem, uint8_t **out, uint32_t *outLen,
    DevCtlGetCharStates getChar)
{
    AdapterJson *arrayObj = NULL;
    int32_t ret = getChar(vendorItem, &arrayObj);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get char control error %d", ret);
        return ret;
    }

#if IOTC_CONF_AILIFE_SUPPORT
    ret = BuildBleCustomSecDataSingleService(arrayObj, out, outLen);
#else
    ret = BuildBleCustomSecDataService(arrayObj, out, outLen);
#endif
    AdapterJsonDelete(arrayObj);
    if (ret != IOTC_OK) {
        IOTC_LOGW("build get char json array %d", ret);
        return ret;
    }

    return IOTC_OK;
}

static void PutCharReportExecutorCallback(void *userData)
{
    CHECK_V_RETURN_LOGW(userData != NULL, "param invalid");
    AdapterJson *vendor = (AdapterJson *)userData;

    AdapterJson *respVendor = NULL;
    int32_t ret = DevSvcProxyCtlGetCharStates(vendor, &respVendor);
    AdapterJsonDelete(vendor);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get char control error %d", ret);
        return;
    }

    uint8_t *data = NULL;
    uint32_t len = 0;
#if IOTC_CONF_AILIFE_SUPPORT
    ret = BuildBleCustomSecDataSingleService(respVendor, &data, &len);
#else
    ret = BuildBleCustomSecDataService(respVendor, &data, &len);
#endif
    AdapterJsonDelete(respVendor);
    if (ret != IOTC_OK || data == NULL || len == 0) {
        IOTC_LOGW("trans rpt json err:%d", ret);
        return;
    }
    ret = LinkLayerReportSvcDataEnc(BLE_SVC_CUSTOM_SEC_DATA, data, len);
    AdapterFree(data);
    if (ret != IOTC_OK) {
        IOTC_LOGW("rpt custom sec data err:%d", ret);
    }
}

static int32_t BleCustomSecDataProcess(AdapterJson *vendorItem, uint8_t **out, uint32_t *outLen)
{
    uint32_t size = 0;
    int32_t ret = AdapterJsonGetArraySize(vendorItem, &size);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get array size error %d", ret);
        return ret;
    }

    if (size > IOTC_CONF_PROF_MAX_SVC_NUM || size == 0) {
        IOTC_LOGW("svc size error %u", size);
        return IOTC_SDK_AILIFE_BLE_ERR_CUSTOM_SEC_DATA_SVC_NUM;
    }

    /* 基于是否有data字段来判断是否为控制指令 */
    bool isGetCmd = (AdapterJsonGetObj(AdapterJsonGetArrayItem(vendorItem, 0), STR_JSON_DATA) == NULL);
    if (isGetCmd) {
        if (IsAllServicesSid(vendorItem)) {
            return DevSvcProxyCtlReportAll(DEV_REPORT_TYPE_ASYNC);
        } else {
            return BleCustomSecDataGetChar(vendorItem, out, outLen, DevSvcProxyCtlGetCharStates);
        }
    }

    ret = DevSvcProxyCtlPutCharStates(vendorItem, NULL);
    if (ret != IOTC_OK) {
        IOTC_LOGW("put char control error %d", ret);
        return ret;
    }
    if (ret == IOTC_CORE_PROF_SVC_ERR_ASYNC_REPORT) {
        return IOTC_OK;
    }

    AdapterJson *vendorCopy = AdapterDuplicateJson(vendorItem, true);
    if (vendorCopy == NULL) {
        return IOTC_ADAPTER_JSON_ERR_DUPLICATE;
    }

    /* 控制成功后异步执行查询并上报 */
    ret = SchedAsyncExecutor(PutCharReportExecutorCallback, vendorCopy);
    if (ret != IOTC_OK) {
        IOTC_LOGW("add executor error %d", ret);
        AdapterJsonDelete(vendorCopy);
        return ret;
    }
    return IOTC_OK;
}

/* 对端下发服务非array时先进行处理 */
static int32_t BuildVendorArrayFromItem(AdapterJson *vendorItem, AdapterJson **vendorArray)
{
    AdapterJson *array = AdapterJsonCreateArray();
    CHECK_RETURN_LOGE(array != NULL, IOTC_ADAPTER_JSON_ERR_CREATE, "create vendor arr err");

    AdapterJson *duplicateItem = AdapterDuplicateJson(vendorItem, true);
    if (duplicateItem == NULL) {
        IOTC_LOGE("copy vendor item err");
        AdapterJsonDelete(array);
        return IOTC_ADAPTER_JSON_ERR_DUPLICATE;
    }

    int32_t ret = AdapterJsonAddItem2Array(array, duplicateItem);
    if (ret != IOTC_OK) {
        IOTC_LOGE("add item copy err");
        AdapterJsonDelete(array);
        AdapterJsonDelete(duplicateItem);
        return IOTC_ADAPTER_JSON_ERR_ADD;
    }

    *vendorArray = array;
    return IOTC_OK;
}

static int32_t ForwardCustomSecData(AdapterJson *root, uint8_t **out, uint32_t *outLen)
{
    AdapterJson *vendorItem = AdapterJsonGetObj(root, STR_JSON_VENDOR);
    if (vendorItem == NULL) {
        IOTC_LOGE("get vendor item err");
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    char *vendor = AdapterJsonPrint(vendorItem);
    if (vendor == NULL) {
        IOTC_LOGE("print vendor item err");
        return IOTC_ADAPTER_JSON_ERR_PRINT;
    }

    /* custom sec data 优先由外部回调处理 */
    int32_t ret = ProductRecvCustomSecData((const uint8_t *)vendor, strlen(vendor));
    AdapterJsonFreePrint(vendor);
    if (ret == IOTC_OK) {
        return IOTC_OK;
    } else if (ret != IOTC_ERR_CALLBACK_NULL) {
        IOTC_LOGW("custom sec data product error %d", ret);
        return ret;
    }

    if (!AdapterJsonIsArray(vendorItem)) {
        AdapterJson *vendorArray = NULL;
        ret = BuildVendorArrayFromItem(vendorItem, &vendorArray);
        CHECK_RETURN(ret == IOTC_OK, ret);
        ret = BleCustomSecDataProcess(vendorArray, out, outLen);
        AdapterJsonDelete(vendorArray);
    } else {
        ret = BleCustomSecDataProcess(vendorItem, out, outLen);
    }
    return ret;
}

static int32_t PutCustomSecData(const char *jsonStr, uint8_t **out, uint32_t *outLen)
{
    AdapterJson *root = AdapterJsonParse(jsonStr);
    if (root == NULL) {
        IOTC_LOGE("json parse");
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    int32_t ret = IOTC_ERROR;
    do {
        uint32_t seq = 0;
        ret = GetSeq(root, &seq);
        if (ret != IOTC_OK) {
            IOTC_LOGE("get seq err ret=%d", ret);
            break;
        }
        if (!BleSessRecvSeqCheck(seq)) {
            IOTC_LOGE("check seq err");
            ret = IOTC_CORE_BLE_INVALID_SEQ;
            break;
        }
        BleSessRecvSeqUpdate(seq);
        ret = ForwardCustomSecData(root, out, outLen);
        if (ret != IOTC_OK) {
            IOTC_LOGE("forward custom sec data err ret=%d", ret);
            break;
        }
        ret = IOTC_OK;
    } while (false);
    AdapterJsonDelete(root);
    return ret;
}

int32_t PutBleSvcCustomSecData(const BtCmdParam *param, uint8_t **out, uint32_t *outLen)
{
    CHECK_RETURN_LOGW((param != NULL) && (param->request != NULL) && (param->requestLen != 0) &&
        (out != NULL) && (outLen != NULL), IOTC_ERR_PARAM_INVALID, "invalid param");

    *out = NULL;
    *outLen = 0;
    int32_t ret = PutCustomSecData((char*)param->request, out, outLen);
    if (ret != IOTC_OK || *out == NULL) {
        return UtilsGenErrcodeJsonStr(ret, (char **)out, outLen);
    }
    return IOTC_OK;
}