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
#ifndef UTILS_JSON_H
#define UTILS_JSON_H

#include <stdint.h>
#include <stddef.h>
#include "iotc_json.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *name;
    const char *value;
} UtilsJsonStrItem;

typedef struct {
    const char *name;
    char *buf;
    uint32_t size;
} UtilsJsonStrBufItem;

IotcJson *UtilsJsonCreateErrcode(int32_t errcode);

int32_t UtilsJsonGetNum(const IotcJson *json, const char *name, int32_t *num);

int32_t UtilsJsonGetUint(const IotcJson *json, const char *name, uint32_t *num);

int32_t UtilsJsonGetString(const IotcJson *json, const char *key, char *buf, uint32_t len);

int32_t UtilsGenErrcodeJsonStr(int32_t errcode, char **out, uint32_t *outLen);

int32_t UtilsJsonAddHexify(IotcJson *json, const char *name, const uint8_t *input, uint32_t inputLen);

int32_t UtilsJsonAddStrTable(IotcJson *json, const UtilsJsonStrItem *tbl, uint32_t size);

int32_t UtilsJsonParseStrTable(IotcJson *json, UtilsJsonStrBufItem *tbl, uint32_t size);

/* json使用IotcMalloc分配内存打印，返回的指针必须使用IotcFree释放 */
char *UtilsJsonPrintByMalloc(const IotcJson *json);

IotcJson *UtilsJsonCreateKeyIntValue(const char *key, int32_t value);

int32_t UtilsGenKeyIntValueJsonStr(const char *key, int32_t value, char **out, uint32_t *outLen);

#ifdef __cplusplus
}
#endif

#endif /* UTILS_JSON_H */
