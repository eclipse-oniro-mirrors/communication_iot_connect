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
#include "iotc_json.h"
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
#include "ble_svc_device_info.h"

static int32_t GetSeq(IotcJson *root, uint32_t *seq)
{
    IotcJson *seqItem = IotcJsonGetObj(root, STR_JSON_SEQ);
    if (seqItem == NULL) {
        IOTC_LOGE("get seq item err");
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    int64_t num = 0;
    int32_t ret = IotcJsonGetNum(seqItem, &num);
    if (ret != IOTC_OK) {
        IOTC_LOGE("get seq err=%d", ret);
        return ret;
    }
    *seq = (uint32_t)num;
    return IOTC_OK;
}

static bool IsAllServicesSid(IotcJson *vendorItem)
{
    uint32_t size = 0;
    int32_t ret = IotcJsonGetArraySize(vendorItem, &size);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get size error %d", ret);
        return false;
    }
    /* 查询所有服务时只会有1个sid */
    if (size != 1) {
        return false;
    }
    const char *sid = IotcJsonGetStr(IotcJsonGetObj(IotcJsonGetArrayItem(vendorItem, 0), STR_JSON_SID));
    if (sid == NULL || strcmp(sid, STR_JSON_ALL_SERVICE) != 0) {
        return false;
    }
    return true;
}

static int32_t DelDevIdAndMsgId(IotcJson* msg)
{
    if (msg == NULL) {
        return IOTC_ERR_INVALID_PARAM;
    }

    uint32_t arraySize = 0;
    IotcJsonGetArraySize(msg, &arraySize);

    for (int32_t i = 0; i < arraySize; i++) {
        IotcJson *item = IotcJsonGetArrayItem(msg, i);
        if (item == NULL) {
            continue;
        }

        // 删除 devid
        if (IotcJsonHasObj(item, STR_JSON_DEVID)) {
            IotcJsonDeleteItem(item, STR_JSON_DEVID);
        }

        // 删除msg_id
        if (IotcJsonHasObj(item, STR_JSON_MSG_ID)) {
            IotcJsonDeleteItem(item, STR_JSON_MSG_ID);
        }
    }

    return IOTC_OK;
}

int32_t BuildBleCustomSecDataService(IotcJson *item, uint8_t **out, uint32_t *outLen)
{
    IotcJson *root = IotcJsonCreate();
    if (root == NULL) {
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }

    int32_t ret = IotcJsonAddNum2Obj(root, STR_JSON_SEQ, (int64_t)BleSessSendSeqGet());
    if (ret != IOTC_OK) {
        IOTC_LOGW("add seq error %d", ret);
        IotcJsonDelete(root);
        return ret;
    }

    // 添加 errcode 字段，解决get指令手机侧解析报错问题
    ret = IotcJsonAddNum2Obj(root, STR_JSON_ERR_CODE, 0);
    if (ret != IOTC_OK) {
        IOTC_LOGW("add errcode error %d", ret);
        IotcJsonDelete(root);
        return ret;
    }
    DelDevIdAndMsgId(item);
    IotcJson *dupItem = NULL;
    if (IotcJsonGetObj(item, STR_JSON_VENDOR)) {
        dupItem = IotcDuplicateJson(IotcJsonGetObj(item, STR_JSON_VENDOR), true);
    } else {
        dupItem = IotcDuplicateJson(item, true);
    }

    if (dupItem == NULL) {
        IOTC_LOGW("duplicate item err");
        IotcJsonDelete(root);
        return IOTC_ADAPTER_JSON_ERR_DUPLICATE;
    }

    ret = IotcJsonAddItem2Obj(root, STR_JSON_VENDOR, dupItem);
    if (ret != IOTC_OK) {
        IotcJsonDelete(root);
        IotcJsonDelete(dupItem);
        return IOTC_ADAPTER_JSON_ERR_ADD;
    }

    *out = (uint8_t *)UtilsJsonPrintByMalloc(root);
    IotcJsonDelete(root);
    if (*out == NULL) {
        return IOTC_CORE_COMM_UTILS_ERR_JSON_MALLOC_PRINT;
    }
    *outLen = strlen((const char *)*out);
    BleSessSendSeqUpdate();
    return IOTC_OK;
}

#if IOTC_CONF_AILIFE_SUPPORT
static int32_t BuildBleCustomSecDataSingleService(IotcJson *array, uint8_t **out, uint32_t *outLen)
{
    uint32_t size = 0;
    int32_t ret = IotcJsonGetArraySize(array, &size);
    CHECK_RETURN_LOGW(ret == IOTC_OK, ret, "get arr size err:%d", ret);
    CHECK_RETURN_LOGW(size == 1, IOTC_CORE_BLE_ERR_CUSTOM_DATA_SIZE, "get arr size:%u err", size);
    IotcJson *item = IotcJsonGetArrayItem(array, 0);
    CHECK_RETURN_LOGW(item != NULL, IOTC_ADAPTER_JSON_ERR_GET_ARRAY_ITEM, "get arr item err");
    return BuildBleCustomSecDataService(item, out, outLen);
}
#endif

static bool IsDeviceInfoSid(IotcJson *vendorItem)
{
    uint32_t size = 0;
    int32_t ret = IotcJsonGetArraySize(vendorItem, &size);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get size error %d", ret);
        return false;
    }
    /* 查询所有服务时只会有1个sid */
    if (size != 1) {
        return false;
    }
    const char *sid = IotcJsonGetStr(IotcJsonGetObj(IotcJsonGetArrayItem(vendorItem, 0), STR_JSON_SID));
    if ((sid == NULL) || (strcmp(sid, STR_JSON_DEVICE_INFO) != 0)) {
        return false;
    }
    return true;
}

static int32_t BleCustomSecDataGetChar(IotcJson *vendorItem, uint8_t **out, uint32_t *outLen,
    DevCtlGetCharStates getChar)
{
    IotcJson *arrayObj = NULL;
    int32_t ret = IOTC_OK;
    if (IsDeviceInfoSid(vendorItem)) {
    ret = GetBleCustomSecDeviceInfo(&arrayObj);
    } else {
        ret = getChar(vendorItem, &arrayObj);
    }
    if (ret != IOTC_OK) {
        IOTC_LOGW("get char control error %d", ret);
        return ret;
    }

#if IOTC_CONF_AILIFE_SUPPORT
    ret = BuildBleCustomSecDataSingleService(arrayObj, out, outLen);
#else
    ret = BuildBleCustomSecDataService(arrayObj, out, outLen);
    ret = LinkLayerReportSvcDataEnc(BLE_SVC_CUSTOM_SEC_DATA, (const uint8_t *)*out, *outLen);
#endif
    IotcJsonDelete(arrayObj);
    if (ret != IOTC_OK) {
        IOTC_LOGW("build get char json array %d", ret);
        return ret;
    }

    return IOTC_OK;
}

static void PutCharReportExecutorCallback(void *userData)
{
    CHECK_V_RETURN_LOGW(userData != NULL, "param invalid");
    IotcJson *vendor = (IotcJson *)userData;

    IotcJson *respVendor = NULL;
    int32_t ret = DevSvcProxyCtlGetCharStates(vendor, &respVendor);
    IotcJsonDelete(vendor);
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
    IotcJsonDelete(respVendor);
    if (ret != IOTC_OK || data == NULL || len == 0) {
        IOTC_LOGW("trans rpt json err:%d", ret);
        return;
    }
    ret = LinkLayerReportSvcDataEnc(BLE_SVC_CUSTOM_SEC_DATA, data, len);
    IotcFree(data);
    if (ret != IOTC_OK) {
        IOTC_LOGW("rpt custom sec data err:%d", ret);
    }
}

static int32_t BleCustomSecDataProcess(IotcJson *vendorItem, uint8_t **out, uint32_t *outLen)
{
    uint32_t size = 0;
    int32_t ret = IotcJsonGetArraySize(vendorItem, &size);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get array size error %d", ret);
        return ret;
    }

    if (size > IOTC_CONF_PROF_MAX_SVC_NUM || size == 0) {
        IOTC_LOGW("svc size error %u", size);
        return IOTC_SDK_AILIFE_BLE_ERR_CUSTOM_SEC_DATA_SVC_NUM;
    }

    /* 基于是否有data字段来判断是否为控制指令 */
    bool isGetCmd = (IotcJsonGetObj(IotcJsonGetArrayItem(vendorItem, 0), STR_JSON_DATA) == NULL);
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

    IotcJson *vendorCopy = IotcDuplicateJson(vendorItem, true);
    if (vendorCopy == NULL) {
        return IOTC_ADAPTER_JSON_ERR_DUPLICATE;
    }

    /* 控制成功后异步执行查询并上报 */
    ret = SchedAsyncExecutor(PutCharReportExecutorCallback, vendorCopy);
    if (ret != IOTC_OK) {
        IOTC_LOGW("add executor error %d", ret);
        IotcJsonDelete(vendorCopy);
        return ret;
    }
    return IOTC_OK;
}

/* 对端下发服务非array时先进行处理 */
static int32_t BuildVendorArrayFromItem(IotcJson *vendorItem, IotcJson **vendorArray)
{
    IotcJson *array = IotcJsonCreateArray();
    CHECK_RETURN_LOGE(array != NULL, IOTC_ADAPTER_JSON_ERR_CREATE, "create vendor arr err");

    IotcJson *duplicateItem = IotcDuplicateJson(vendorItem, true);
    if (duplicateItem == NULL) {
        IOTC_LOGE("copy vendor item err");
        IotcJsonDelete(array);
        return IOTC_ADAPTER_JSON_ERR_DUPLICATE;
    }

    int32_t ret = IotcJsonAddItem2Array(array, duplicateItem);
    if (ret != IOTC_OK) {
        IOTC_LOGE("add item copy err");
        IotcJsonDelete(array);
        IotcJsonDelete(duplicateItem);
        return IOTC_ADAPTER_JSON_ERR_ADD;
    }

    *vendorArray = array;
    return IOTC_OK;
}

static int32_t ForwardCustomSecData(IotcJson *root, uint8_t **out, uint32_t *outLen)
{
    IotcJson *vendorItem = IotcJsonGetObj(root, STR_JSON_VENDOR);
    if (vendorItem == NULL) {
        IOTC_LOGE("get vendor item err");
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    char *vendor = IotcJsonPrint(vendorItem);
    if (vendor == NULL) {
        IOTC_LOGE("print vendor item err");
        return IOTC_ADAPTER_JSON_ERR_PRINT;
    }

    /* custom sec data 优先由外部回调处理 */
    int32_t ret = ProductRecvCustomSecData((const uint8_t *)vendor, strlen(vendor));
    IotcJsonFreePrint(vendor);
    if (ret == IOTC_OK) {
        return IOTC_OK;
    } else if (ret != IOTC_ERR_CALLBACK_NULL) {
        IOTC_LOGW("custom sec data product error %d", ret);
        return ret;
    }

    if (!IotcJsonIsArray(vendorItem)) {
        IotcJson *vendorArray = NULL;
        ret = BuildVendorArrayFromItem(vendorItem, &vendorArray);
        CHECK_RETURN(ret == IOTC_OK, ret);
        ret = BleCustomSecDataProcess(vendorArray, out, outLen);
        IotcJsonDelete(vendorArray);
    } else {
        ret = BleCustomSecDataProcess(vendorItem, out, outLen);
    }
    return ret;
}

static int32_t PutCustomSecData(const char *jsonStr, uint8_t **out, uint32_t *outLen)
{
    IotcJson *root = IotcJsonParse(jsonStr);
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
    IotcJsonDelete(root);
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