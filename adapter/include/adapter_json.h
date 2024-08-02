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
#ifndef ADAPTER_KV_H
#define ADAPTER_KV_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void AdapterJson;

/* 创建json */
AdapterJson *AdapterCreateJson(void);

AdapterJson *AdapterDuplicateJson(const AdapterJson *json, bool recurse);

AdapterJson *AdapterJsonCreateArray(void);

AdapterJson *AdapterJsonCreateStr(const char *val);

AdapterJson *AdapterJsonCreateNum(int64_t val);

AdapterJson *AdapterJsonCreateFloat(double val);

AdapterJson *AdapterJsonCreateBool(bool val);

AdapterJson *AdapterJsonParse(const char *str);

AdapterJson *AdapterJsonParseWithLen(const char *str, uint32_t len);

/* 释放json */
void AdapterJsonDelete(AdapterJson *json);

/* json打印，返回的指针必须通过AdapterJsonFreePrint释放 */
char *AdapterJsonPrint(const AdapterJson *json);

void AdapterJsonFreePrint(char *print);

int32_t AdapterJsonPrint2Buffer(AdapterJson *json, char *buffer, uint32_t *len);

/* json获取成员 */
AdapterJson *AdapterJsonGetObj(const AdapterJson *json, const char *name);

AdapterJson *AdapterJsonGetArrayItem(const AdapterJson *json, uint32_t index);

/* json组合 */
int32_t AdapterJsonAddItem2Array(AdapterJson *array, AdapterJson *item);

int32_t AdapterJsonAddItem2Obj(AdapterJson *json, const char *name, AdapterJson *item);

int32_t AdapterJsonAddNum2Obj(AdapterJson *json, const char *name, int64_t number);

int32_t AdapterJsonAddFloat2Obj(AdapterJson *json, const char *name, double number);

int32_t AdapterJsonAddBool2Obj(AdapterJson *json, const char *name, bool val);

int32_t AdapterJsonAddStr2Obj(AdapterJson *json, const char *name, const char *string);

/* json获取值 */
const char *AdapterJsonGetStr(const AdapterJson *json);

int32_t AdapterJsonGetNum(const AdapterJson *json, int64_t *val);

int32_t AdapterJsonGetFloat(const AdapterJson *json, double *val);

int32_t AdapterJsonGetBool(const AdapterJson *json, bool *val);

/* json判断 */
bool AdapterJsonIsStr(const AdapterJson *json);

bool AdapterJsonIsNum(const AdapterJson *json);

bool AdapterJsonIsFloat(const AdapterJson *json);

bool AdapterJsonIsBool(const AdapterJson *json);

bool AdapterJsonIsArray(const AdapterJson *json);

bool AdapterJsonHasObj(const AdapterJson *json, const char *name);

/* json数组 */
int32_t AdapterJsonGetArraySize(const AdapterJson *json, uint32_t *size);

/* 删除 */
void AdapterDeleteItemFromJson(AdapterJson *json, const char *name);

void AdapterDeleteItemFromJsonArray(AdapterJson *json, uint32_t index);

#ifdef __cplusplus
}
#endif

#endif /* ADAPTER_KV_H */
