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
IotcJson *IotcJsonCreate(void)
{
    return cJSON_CreateObject();
}

IotcJson *IotcDuplicateJson(const IotcJson *json, bool recurse)
{
    if (json == NULL) {
        return NULL;
    }
    return cJSON_Duplicate(json, recurse);
}

IotcJson *IotcJsonCreateArray(void)
{
    return cJSON_CreateArray();
}

IotcJson *IotcJsonCreateStr(const char *val)
{
    return cJSON_CreateString(val);
}

IotcJson *IotcJsonCreateNum(int64_t val)
{
    return cJSON_CreateNumber((double)val);
}

IotcJson *IotcJsonCreateFloat(double val)
{
    return cJSON_CreateNumber(val);
}

IotcJson *IotcJsonCreateBool(bool val)
{
    return cJSON_CreateBool(val);
}

IotcJson *IotcJsonParse(const char *str)
{
    if (str == NULL) {
        return NULL;
    }
    return cJSON_Parse(str);
}

IotcJson *IotcJsonParseWithLen(const char *str, uint32_t len)
{
    if ((str == NULL) || (len == 0)) {
        return NULL;
    }
    return cJSON_ParseWithLength(str, len);
}

void IotcJsonDelete(IotcJson *json)
{
    if (json == NULL) {
        return;
    }
    cJSON_Delete(json);
}

char *IotcJsonPrint(const IotcJson *json)
{
    if (json == NULL) {
        return NULL;
    }
    return cJSON_PrintUnformatted(json);
}

void IotcJsonFreePrint(char *print)
{
    if (print == NULL) {
        return;
    }
    cJSON_free(print);
}

int32_t IotcJsonPrint2Buffer(IotcJson *json, char *buffer, uint32_t *len)
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

IotcJson *IotcJsonGetObj(const IotcJson *json, const char *name)
{
    if ((json == NULL) || (name == NULL)) {
        return NULL;
    }
    return cJSON_GetObjectItem(json, name);
}

IotcJson *IotcJsonGetArrayItem(const IotcJson *json, uint32_t index)
{
    if (json == NULL) {
        return NULL;
    }
    return cJSON_GetArrayItem(json, index);
}

int32_t IotcJsonAddItem2Array(IotcJson *array, IotcJson *item)
{
    if ((array == NULL) || (item == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    if (!cJSON_AddItemToArray(array, item)) {
        return IOTC_ADAPTER_JSON_ERR_ADD;
    }
    return IOTC_OK;
}

int32_t IotcJsonAddItem2Obj(IotcJson *json, const char *name, IotcJson *item)
{
    if ((json == NULL) || (name == NULL) || (item == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    if (!cJSON_AddItemToObject(json, name, item)) {
        return IOTC_ADAPTER_JSON_ERR_ADD;
    }
    return IOTC_OK;
}

int32_t IotcJsonAddNum2Obj(IotcJson *json, const char *name, int64_t number)
{
    return IotcJsonAddFloat2Obj(json, name, (double)number);
}

int32_t IotcJsonAddFloat2Obj(IotcJson *json, const char *name, double number)
{
    if ((json == NULL) || (name == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    if (cJSON_AddNumberToObject(json, name, number) == NULL) {
        return IOTC_ADAPTER_JSON_ERR_ADD;
    }
    return IOTC_OK;
}

int32_t IotcJsonAddBool2Obj(IotcJson *json, const char *name, bool val)
{
    if ((json == NULL) || (name == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    if (cJSON_AddBoolToObject(json, name, val) == NULL) {
        return IOTC_ADAPTER_JSON_ERR_ADD;
    }
    return IOTC_OK;
}

int32_t IotcJsonAddStr2Obj(IotcJson *json, const char *name, const char *string)
{
    if ((json == NULL) || (name == NULL) || (string == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    if (cJSON_AddStringToObject(json, name, string) == NULL) {
        return IOTC_ADAPTER_JSON_ERR_ADD;
    }
    return IOTC_OK;
}

const char *IotcJsonGetStr(const IotcJson *json)
{
    if (json == NULL) {
        return NULL;
    }
    return cJSON_GetStringValue(json);
}

int32_t IotcJsonGetNum(const IotcJson *json, int64_t *val)
{
    if ((json == NULL) || (val == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    if (!IotcJsonIsNum(json)) {
        return IOTC_ADAPTER_JSON_ERR_TYPE;
    }

    double value = cJSON_GetNumberValue(json);
    *val = (int64_t)value;
    return IOTC_OK;
}

int32_t IotcJsonGetFloat(const IotcJson *json, double *val)
{
    if ((json == NULL) || (val == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    if (!IotcJsonIsNum(json)) {
        return IOTC_ADAPTER_JSON_ERR_TYPE;
    }

    double value = cJSON_GetNumberValue(json);
    *val = value;
    return IOTC_OK;
}

int32_t IotcJsonGetBool(const IotcJson *json, bool *val)
{
    if ((json == NULL) || (val == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    if (!IotcJsonIsBool(json)) {
        return IOTC_ADAPTER_JSON_ERR_TYPE;
    }
    *val = cJSON_IsTrue(json);
    return IOTC_OK;
}

bool IotcJsonIsStr(const IotcJson *json)
{
    if (json == NULL) {
        return false;
    }
    return cJSON_IsString(json);
}

bool IotcJsonIsNum(const IotcJson *json)
{
    if (json == NULL) {
        return false;
    }
    return cJSON_IsNumber(json);
}

bool IotcJsonIsFloat(const IotcJson *json)
{
    if (json == NULL) {
        return false;
    }
    /* cJSON 未提供相应接口 */
    return cJSON_IsNumber(json);
}

bool IotcJsonIsBool(const IotcJson *json)
{
    if (json == NULL) {
        return false;
    }
    return cJSON_IsBool(json);
}

bool IotcJsonIsArray(const IotcJson *json)
{
    if (json == NULL) {
        return false;
    }
    return cJSON_IsArray(json);
}

bool IotcJsonHasObj(const IotcJson *json, const char *name)
{
    if (json == NULL || name == NULL) {
        return false;
    }
    return cJSON_HasObjectItem(json, name);
}

int32_t IotcJsonGetArraySize(const IotcJson *json, uint32_t *size)
{
    if ((json == NULL) || (size == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    if (!IotcJsonIsArray(json)) {
        return IOTC_ADAPTER_JSON_ERR_TYPE;
    }
    int32_t arrSize = cJSON_GetArraySize(json);
    if (arrSize < 0) {
        return IOTC_ADAPTER_JSON_ERR_SIZE_OVERFLOW;
    }
    *size = (uint32_t)arrSize;
    return IOTC_OK;
}

void IotcJsonDeleteItem(IotcJson *json, const char *name)
{
    if ((json == NULL) || (name == NULL)) {
        return;
    }
    cJSON_DeleteItemFromObject(json, name);
}

void IotcJsonArrayDeleteItem(IotcJson *json, uint32_t index)
{
    if (json == NULL) {
        return;
    }
    cJSON_DeleteItemFromArray(json, index);
}