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
#include "utils_json.h"
#include "comm_def.h"
#include "iotc_log.h"
#include "utils_assert.h"
#include "securec.h"
#include "utils_common.h"
#include "iotc_conf.h"
#include "iotc_errcode.h"

IotcJson *UtilsJsonCreateErrcode(int32_t errcode)
{
    IotcJson *obj = IotcJsonCreate();
    if (obj == NULL) {
        return NULL;
    }

    int32_t ret = IotcJsonAddNum2Obj(obj, STR_ERRCODE, errcode);
    if (ret != IOTC_OK) {
        IOTC_LOGW("add num to obj err %d", ret);
        IotcJsonDelete(obj);
        return NULL;
    }
    return obj;
}

int32_t UtilsJsonGetNum(const IotcJson *json, const char *name, int32_t *num)
{
    CHECK_RETURN(json != NULL && name != NULL && num != NULL, IOTC_ERR_PARAM_INVALID);

    int64_t err;
    int32_t ret = IotcJsonGetNum(IotcJsonGetObj(json, name), &err);
    if (ret != IOTC_OK) {
        return ret;
    }
    if (err > INT32_MAX) {
        IOTC_LOGW("invalid num");
        return IOTC_CORE_COMM_UTILS_ERR_JSON_INVALID_ERRCODE;
    }
    *num = err;
    return IOTC_OK;
}

int32_t UtilsJsonGetUint(const IotcJson *json, const char *name, uint32_t *num)
{
    CHECK_RETURN(json != NULL && name != NULL && num != NULL, IOTC_ERR_PARAM_INVALID);

    int64_t jsonNum;
    int32_t ret = IotcJsonGetNum(IotcJsonGetObj(json, name), &jsonNum);
    if (ret != IOTC_OK) {
        return ret;
    }
    if (jsonNum > UINT32_MAX || jsonNum < 0) {
        IOTC_LOGW("invalid num %lld", jsonNum);
        return IOTC_CORE_COMM_UTILS_ERR_JSON_INVALID_ERRCODE;
    }
    *num = jsonNum;
    return IOTC_OK;
}

int32_t UtilsJsonGetString(const IotcJson *json, const char *key, char *buf, uint32_t len)
{
    CHECK_RETURN(json != NULL && key != NULL && buf != NULL && len != 0, IOTC_ERR_PARAM_INVALID);

    const char *str = IotcJsonGetStr(IotcJsonGetObj(json, key));
    if (str == NULL) {
        IOTC_LOGW("get json str error %s", key);
        return IOTC_ADAPTER_JSON_ERR_GET_STRING;
    }
    int32_t ret = strcpy_s(buf, len, str);
    if (ret != EOK) {
        IOTC_LOGW("strcpy error %u/%u", len, strlen(str));
        return IOTC_ERR_SECUREC_STRCPY;
    }
    return IOTC_OK;
}

int32_t UtilsGenErrcodeJsonStr(int32_t errcode, char **out, uint32_t *outLen)
{
    CHECK_RETURN_LOGW((out != NULL) && (outLen != NULL), IOTC_ERR_PARAM_INVALID, "invalid param");

    IotcJson *obj = UtilsJsonCreateErrcode(errcode);
    if (obj == NULL) {
        IOTC_LOGW("create json error");
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }

    *out = UtilsJsonPrintByMalloc(obj);
    IotcJsonDelete(obj);
    if (*out == NULL) {
        IOTC_LOGW("json print err");
        return IOTC_CORE_COMM_UTILS_ERR_JSON_MALLOC_PRINT;
    }
    *outLen = strlen(*out);

    return IOTC_OK;
}

int32_t UtilsJsonAddHexify(IotcJson *json, const char *name, const uint8_t *input, uint32_t inputLen)
{
    uint32_t dataLen = HEXIFY_LEN(inputLen) + 1;
    char *data = (char *)IotcCalloc(dataLen, sizeof(char));
    if (data == NULL) {
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }

    if (!UtilsHexify(input, inputLen, data, dataLen)) {
        IotcFree(data);
        return IOTC_CORE_COMM_UTILS_ERR_HEXIFY;
    }

    int32_t ret = IotcJsonAddStr2Obj(json, name, data);
    IotcFree(data);
    return ret;
}

int32_t UtilsJsonAddStrTable(IotcJson *json, const UtilsJsonStrItem *tbl, uint32_t size)
{
    CHECK_RETURN_LOGW((json != NULL) && (tbl != NULL) && (size != 0), IOTC_ERR_PARAM_INVALID, "invalid param");

    int32_t ret;
    for (uint32_t i = 0; i < size; ++i) {
        ret = IotcJsonAddStr2Obj(json, tbl[i].name, tbl[i].value);
        if (ret != IOTC_OK) {
            IOTC_LOGW("json add %s error", NON_NULL_STR(tbl[i].name));
            return ret;
        }
    }
    return IOTC_OK;
}

int32_t UtilsJsonParseStrTable(IotcJson *json, UtilsJsonStrBufItem *tbl, uint32_t size)
{
    CHECK_RETURN_LOGW((json != NULL) && (tbl != NULL) && (size != 0), IOTC_ERR_PARAM_INVALID, "invalid param");

    int32_t ret;
    for (uint32_t i = 0; i < size; ++i) {
        ret = UtilsJsonGetString(json, tbl[i].name, tbl[i].buf, tbl[i].size);
        if (ret != IOTC_OK) {
            IOTC_LOGW("json get string %s error", NON_NULL_STR(tbl[i].name));
            return ret;
        }
    }
    return IOTC_OK;
}

char *UtilsJsonPrintByMalloc(const IotcJson *json)
{
    char *ret = IotcJsonPrint(json);
    if (ret == NULL) {
        return NULL;
    }

#if IOTC_CONF_JSON_FREE_EQUAL_MALLOC_FREE && !IOTC_CONF_MEM_DEBUG
    return ret;
#else
    char *copy = UtilsStrDup(ret);
    IotcJsonFreePrint(ret);
    return copy;
#endif
}

IotcJson *UtilsJsonCreateKeyIntValue(const char *key, int32_t value)
{
    IotcJson *obj = IotcJsonCreate();
    if (obj == NULL) {
        return NULL;
    }

    int32_t ret = IotcJsonAddNum2Obj(obj, key, value);
    if (ret != IOTC_OK) {
        IOTC_LOGW("add num to obj err %d", ret);
        IotcJsonDelete(obj);
        return NULL;
    }
    return obj;
}

int32_t UtilsGenKeyIntValueJsonStr(const char *key, int32_t value, char **out, uint32_t *outLen)
{
    CHECK_RETURN_LOGW((key != NULL) && (out != NULL) && (outLen != NULL), IOTC_ERR_PARAM_INVALID, "invalid param");

    IotcJson *obj = UtilsJsonCreateKeyIntValue(key, value);
    if (obj == NULL) {
        IOTC_LOGW("create json error");
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }

    *out = UtilsJsonPrintByMalloc(obj);
    IotcJsonDelete(obj);
    if (*out == NULL) {
        IOTC_LOGW("json print err");
        return IOTC_CORE_COMM_UTILS_ERR_JSON_MALLOC_PRINT;
    }
    *outLen = strlen(*out);

    return IOTC_OK;
}