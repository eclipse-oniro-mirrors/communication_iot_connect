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
#include "adapter_json.h"
#include "iotc_errcode.h"
#include "cJSON.h"
#include "securec.h"

/* 创建json */
AdapterJson *AdapterCreateJson(void)
{
    return cJSON_CreateObject();
}

AdapterJson *AdapterDuplicateJson(const AdapterJson *json, bool recurse)
{
    if (json == NULL) {
        return NULL;
    }
    return cJSON_Duplicate(json, recurse);
}

AdapterJson *AdapterJsonCreateArray(void)
{
    return cJSON_CreateArray();
}

AdapterJson *AdapterJsonCreateStr(const char *val)
{
    return cJSON_CreateString(val);
}

AdapterJson *AdapterJsonCreateNum(int64_t val)
{
    return cJSON_CreateNumber((double)val);
}

AdapterJson *AdapterJsonCreateFloat(double val)
{
    return cJSON_CreateNumber(val);
}

AdapterJson *AdapterJsonCreateBool(bool val)
{
    return cJSON_CreateBool(val);
}

AdapterJson *AdapterJsonParse(const char *str)
{
    if (str == NULL) {
        return NULL;
    }
    return cJSON_Parse(str);
}

AdapterJson *AdapterJsonParseWithLen(const char *str, uint32_t len)
{
    if ((str == NULL) || (len == 0)) {
        return NULL;
    }
    return cJSON_ParseWithLength(str, len);
}

void AdapterJsonDelete(AdapterJson *json)
{
    if (json == NULL) {
        return;
    }
    cJSON_Delete(json);
}

char *AdapterJsonPrint(const AdapterJson *json)
{
    if (json == NULL) {
        return NULL;
    }
    return cJSON_PrintUnformatted(json);
}

void AdapterJsonFreePrint(char *print)
{
    if (print == NULL) {
        return;
    }
    cJSON_free(print);
}

int32_t AdapterJsonPrint2Buffer(AdapterJson *json, char *buffer, uint32_t *len)
{
    if ((json == NULL) || (buffer == NULL) || (len == NULL) || (*len == 0)) {
        return IOTC_ERR_PARAM_INVALID;
    }

    (void)memset_s(buffer, *len, 0, *len);
    if (!cJSON_PrintPreallocated(json, buffer, *len, false)) {
        return IOTC_ADAPTER_JSON_ERR_PRINT;
    }
    *len = strlen(buffer);
    return IOTC_OK;
}

AdapterJson *AdapterJsonGetObj(const AdapterJson *json, const char *name)
{
    if ((json == NULL) || (name == NULL)) {
        return NULL;
    }
    return cJSON_GetObjectItem(json, name);
}

AdapterJson *AdapterJsonGetArrayItem(const AdapterJson *json, uint32_t index)
{
    if (json == NULL) {
        return NULL;
    }
    return cJSON_GetArrayItem(json, index);
}

int32_t AdapterJsonAddItem2Array(AdapterJson *array, AdapterJson *item)
{
    if ((array == NULL) || (item == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    if (!cJSON_AddItemToArray(array, item)) {
        return IOTC_ADAPTER_JSON_ERR_ADD;
    }
    return IOTC_OK;
}

int32_t AdapterJsonAddItem2Obj(AdapterJson *json, const char *name, AdapterJson *item)
{
    if ((json == NULL) || (name == NULL) || (item == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    if (!cJSON_AddItemToObject(json, name, item)) {
        return IOTC_ADAPTER_JSON_ERR_ADD;
    }
    return IOTC_OK;
}

int32_t AdapterJsonAddNum2Obj(AdapterJson *json, const char *name, int64_t number)
{
    return AdapterJsonAddFloat2Obj(json, name, (double)number);
}

int32_t AdapterJsonAddFloat2Obj(AdapterJson *json, const char *name, double number)
{
    if ((json == NULL) || (name == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    if (cJSON_AddNumberToObject(json, name, number) == NULL) {
        return IOTC_ADAPTER_JSON_ERR_ADD;
    }
    return IOTC_OK;
}

int32_t AdapterJsonAddBool2Obj(AdapterJson *json, const char *name, bool val)
{
    if ((json == NULL) || (name == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    if (cJSON_AddBoolToObject(json, name, val) == NULL) {
        return IOTC_ADAPTER_JSON_ERR_ADD;
    }
    return IOTC_OK;
}

int32_t AdapterJsonAddStr2Obj(AdapterJson *json, const char *name, const char *string)
{
    if ((json == NULL) || (name == NULL) || (string == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    if (cJSON_AddStringToObject(json, name, string) == NULL) {
        return IOTC_ADAPTER_JSON_ERR_ADD;
    }
    return IOTC_OK;
}

const char *AdapterJsonGetStr(const AdapterJson *json)
{
    if (json == NULL) {
        return NULL;
    }
    return cJSON_GetStringValue(json);
}

int32_t AdapterJsonGetNum(const AdapterJson *json, int64_t *val)
{
    if ((json == NULL) || (val == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    if (!AdapterJsonIsNum(json)) {
        return IOTC_ADAPTER_JSON_ERR_TYPE;
    }

    double value = cJSON_GetNumberValue(json);
    *val = (int64_t)value;
    return IOTC_OK;
}

int32_t AdapterJsonGetFloat(const AdapterJson *json, double *val)
{
    if ((json == NULL) || (val == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    if (!AdapterJsonIsNum(json)) {
        return IOTC_ADAPTER_JSON_ERR_TYPE;
    }

    double value = cJSON_GetNumberValue(json);
    *val = value;
    return IOTC_OK;
}

int32_t AdapterJsonGetBool(const AdapterJson *json, bool *val)
{
    if ((json == NULL) || (val == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    if (!AdapterJsonIsBool(json)) {
        return IOTC_ADAPTER_JSON_ERR_TYPE;
    }
    *val = cJSON_IsTrue(json);
    return IOTC_OK;
}

bool AdapterJsonIsStr(const AdapterJson *json)
{
    if (json == NULL) {
        return false;
    }
    return cJSON_IsString(json);
}

bool AdapterJsonIsNum(const AdapterJson *json)
{
    if (json == NULL) {
        return false;
    }
    return cJSON_IsNumber(json);
}

bool AdapterJsonIsFloat(const AdapterJson *json)
{
    if (json == NULL) {
        return false;
    }
    /* cJSON 未提供相应接口 */
    return cJSON_IsNumber(json);
}

bool AdapterJsonIsBool(const AdapterJson *json)
{
    if (json == NULL) {
        return false;
    }
    return cJSON_IsBool(json);
}

bool AdapterJsonIsArray(const AdapterJson *json)
{
    if (json == NULL) {
        return false;
    }
    return cJSON_IsArray(json);
}

bool AdapterJsonHasObj(const AdapterJson *json, const char *name)
{
    if (json == NULL || name == NULL) {
        return false;
    }
    return cJSON_HasObjectItem(json, name);
}

int32_t AdapterJsonGetArraySize(const AdapterJson *json, uint32_t *size)
{
    if ((json == NULL) || (size == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    if (!AdapterJsonIsArray(json)) {
        return IOTC_ADAPTER_JSON_ERR_TYPE;
    }
    int32_t arrSize = cJSON_GetArraySize(json);
    if (arrSize < 0) {
        return IOTC_ADAPTER_JSON_ERR_SIZE_OVERFLOW;
    }
    *size = (uint32_t)arrSize;
    return IOTC_OK;
}

void AdapterDeleteItemFromJson(AdapterJson *json, const char *name)
{
    if ((json == NULL) || (name == NULL)) {
        return;
    }
    cJSON_DeleteItemFromObject(json, name);
}

void AdapterDeleteItemFromJsonArray(AdapterJson *json, uint32_t index)
{
    if (json == NULL) {
        return;
    }
    cJSON_DeleteItemFromArray(json, index);
}