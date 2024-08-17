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
#ifndef IOTC_KV_H
#define IOTC_KV_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void IotcJson;

/* 创建json */
IotcJson *IotcJsonCreate(void);

IotcJson *IotcDuplicateJson(const IotcJson *json, bool recurse);

IotcJson *IotcJsonCreateArray(void);

IotcJson *IotcJsonCreateStr(const char *val);

IotcJson *IotcJsonCreateNum(int64_t val);

IotcJson *IotcJsonCreateFloat(double val);

IotcJson *IotcJsonCreateBool(bool val);

IotcJson *IotcJsonParse(const char *str);

IotcJson *IotcJsonParseWithLen(const char *str, uint32_t len);

/* 释放json */
void IotcJsonDelete(IotcJson *json);

/* json打印，返回的指针必须通过IotcJsonFreePrint释放 */
char *IotcJsonPrint(const IotcJson *json);

void IotcJsonFreePrint(char *print);

int32_t IotcJsonPrint2Buffer(IotcJson *json, char *buffer, uint32_t *len);

/* json获取成员 */
IotcJson *IotcJsonGetObj(const IotcJson *json, const char *name);

IotcJson *IotcJsonGetArrayItem(const IotcJson *json, uint32_t index);

/* json组合 */
int32_t IotcJsonAddItem2Array(IotcJson *array, IotcJson *item);

int32_t IotcJsonAddItem2Obj(IotcJson *json, const char *name, IotcJson *item);

int32_t IotcJsonAddNum2Obj(IotcJson *json, const char *name, int64_t number);

int32_t IotcJsonAddFloat2Obj(IotcJson *json, const char *name, double number);

int32_t IotcJsonAddBool2Obj(IotcJson *json, const char *name, bool val);

int32_t IotcJsonAddStr2Obj(IotcJson *json, const char *name, const char *string);

/* json获取值 */
const char *IotcJsonGetStr(const IotcJson *json);

int32_t IotcJsonGetNum(const IotcJson *json, int64_t *val);

int32_t IotcJsonGetFloat(const IotcJson *json, double *val);

int32_t IotcJsonGetBool(const IotcJson *json, bool *val);

/* json判断 */
bool IotcJsonIsStr(const IotcJson *json);

bool IotcJsonIsNum(const IotcJson *json);

bool IotcJsonIsFloat(const IotcJson *json);

bool IotcJsonIsBool(const IotcJson *json);

bool IotcJsonIsArray(const IotcJson *json);

bool IotcJsonHasObj(const IotcJson *json, const char *name);

/* json数组 */
int32_t IotcJsonGetArraySize(const IotcJson *json, uint32_t *size);

/* 删除 */
void IotcJsonDeleteItem(IotcJson *json, const char *name);

void IotcJsonArrayDeleteItem(IotcJson *json, uint32_t index);

#ifdef __cplusplus
}
#endif

#endif /* IOTC_KV_H */
