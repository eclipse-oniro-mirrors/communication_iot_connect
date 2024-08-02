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
#include <stdbool.h>
#include <string.h>
#include "security_speke_common.h"
#include "security_speke_defs.h"
#include "iotc_log.h"
#include "securec.h"
#include "adapter_mem.h"
#include "utils_common.h"
#include "utils_json.h"
#include "iotc_errcode.h"

int32_t SpekeCommonAddVerInfoToJson(AdapterJson *secDataPayload)
{
    if (secDataPayload == NULL) {
        return IOTC_ERR_PARAM_INVALID;
    }
    AdapterJson *version = AdapterCreateJson();
    if (version == NULL) {
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }

    int32_t ret = AdapterJsonAddStr2Obj(version, SPEKE_SEC_DATA_CUR_VER_JSON, SPEKE_VERSION);
    if (ret != IOTC_OK) {
        AdapterJsonDelete(version);
        return ret;
    }
    ret = AdapterJsonAddStr2Obj(version, SPEKE_SEC_DATA_MIN_VER_JSON, SPEKE_VERSION);
    if (ret != IOTC_OK) {
        AdapterJsonDelete(version);
        return ret;
    }
    ret = AdapterJsonAddItem2Obj(secDataPayload, SPEKE_SEC_DATA_VER_JSON, version);
    if (ret != IOTC_OK) {
        AdapterJsonDelete(version);
        return ret;
    }

    return IOTC_OK;
}

int32_t SpekeCommonVerifyVersion(const AdapterJson *secDataPayload)
{
    if (secDataPayload == NULL) {
        return IOTC_ERR_PARAM_INVALID;
    }

    AdapterJson *verObj = AdapterJsonGetObj(secDataPayload, SPEKE_SEC_DATA_VER_JSON);
    if (verObj == NULL) {
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    AdapterJson *curVerObj = AdapterJsonGetObj(verObj, SPEKE_SEC_DATA_CUR_VER_JSON);
    if (curVerObj == NULL) {
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    const char *curVerStr = AdapterJsonGetStr(curVerObj);
    if (curVerStr == NULL) {
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    if (strcmp(curVerStr, SPEKE_VERSION) != 0) {
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_VER_NOT_SUPP;
    }

    return IOTC_OK;
}

static AdapterJson *CreateSecDataObj(int32_t msgType, const AdapterJson *secDataPayload)
{
    AdapterJson *secData = AdapterCreateJson();
    if (secData == NULL) {
        IOTC_LOGE("Speke create secData JSON err");
        return NULL;
    }
    int32_t ret = AdapterJsonAddNum2Obj(secData, SPEKE_SEC_DATA_MESSAGE_JSON, msgType);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke add msgType to secData err:%d", ret);
        AdapterJsonDelete(secData);
        return NULL;
    }
    AdapterJson *duplicatePayload = AdapterDuplicateJson(secDataPayload, true);
    if (duplicatePayload == NULL) {
        IOTC_LOGE("Speke duplicate payload err");
        AdapterJsonDelete(secData);
        return NULL;
    }
    ret = AdapterJsonAddItem2Obj(secData, SPEKE_SEC_DATA_PAYLOAD_JSON, duplicatePayload);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke add payload to secData err:%d", ret);
        AdapterJsonDelete(secData);
        AdapterJsonDelete(duplicatePayload);
        return NULL;
    }

    return secData;
}

static AdapterJson *CreateNegoMsgObj(const char *sessionId, AdapterJson *secData)
{
    AdapterJson *root = AdapterCreateJson();
    if (root == NULL) {
        IOTC_LOGE("Speke create nego msg root JSON err");
        return NULL;
    }
    int32_t ret = AdapterJsonAddStr2Obj(root, SPEKE_SESSION_ID_JSON, sessionId);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke create nego msg add sessionId err:%d", ret);
        AdapterJsonDelete(root);
        return NULL;
    }
    ret = AdapterJsonAddItem2Obj(root, SPEKE_SEC_DATA_JSON, secData);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke create nego msg add secData err:%d", ret);
        AdapterJsonDelete(root);
        return NULL;
    }

    return root;
}

int32_t SpekeCommonCreateNegoMsg(const char *sessionId, int32_t msgType, const AdapterJson *secDataPayload,
    uint8_t **msg, uint32_t *len)
{
    if ((sessionId == NULL) || (secDataPayload == NULL) || (msg == NULL) || (len == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }

    AdapterJson *secData = CreateSecDataObj(msgType, secDataPayload);
    if (secData == NULL) {
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }
    AdapterJson *root = CreateNegoMsgObj(sessionId, secData);
    if (root == NULL) {
        AdapterJsonDelete(secData);
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }

    char *outMsg = UtilsJsonPrintByMalloc(root);
    AdapterJsonDelete(root);
    if (outMsg == NULL) {
        IOTC_LOGE("Speke create nego msg print JSON err");
        AdapterJsonDelete(root);
        return IOTC_CORE_COMM_UTILS_ERR_JSON_MALLOC_PRINT;
    }

    *msg = (uint8_t *)outMsg;
    *len = strlen(outMsg);
    return IOTC_OK;
}

int32_t SpekeCommonAddDataToJson(AdapterJson *target, const char *name, const uint8_t *input, uint32_t inputLen)
{
    if ((target == NULL) || (name == NULL) || (input == NULL) || (inputLen == 0)) {
        return IOTC_ERR_PARAM_INVALID;
    }

    uint32_t dataLen = HEXIFY_LEN(inputLen) + 1;
    char *data = (char *)AdapterMalloc(dataLen);
    if (data == NULL) {
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(data, dataLen, 0, dataLen);

    if (!UtilsHexify(input, inputLen, data, dataLen)) {
        AdapterFree(data);
        return IOTC_CORE_COMM_UTILS_ERR_HEXIFY;
    }

    int32_t ret = AdapterJsonAddStr2Obj(target, name, data);
    if (ret != IOTC_OK) {
        AdapterFree(data);
        return ret;
    }

    AdapterFree(data);
    return IOTC_OK;
}

int32_t SpekeCommonParseDataFromJson(const AdapterJson *src, const char *name, uint8_t **output, uint32_t *outputLen)
{
    if ((src == NULL) || (name == NULL) || (output == NULL) || (outputLen == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }

    AdapterJson *obj = AdapterJsonGetObj(src, name);
    if (obj == NULL) {
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    const char *srcStr = AdapterJsonGetStr(obj);
    if (srcStr == NULL) {
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }

    uint32_t dataLen = UNHEXIFY_LEN(strlen(srcStr));
    if (dataLen == 0) {
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    uint8_t *data = (uint8_t *)AdapterMalloc(dataLen);
    if (data == NULL) {
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(data, dataLen, 0, dataLen);

    if (!UtilsUnhexify(srcStr, strlen(srcStr), data, dataLen)) {
        AdapterFree(data);
        return IOTC_CORE_COMM_UTILS_ERR_UNHEXIFY;
    }

    *output = data;
    *outputLen = dataLen;
    return IOTC_OK;
}