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
#ifndef UTILS_COMMON_H
#define UTILS_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "adapter_mem.h"

#ifdef __cplusplus
extern "C" {
#endif


#define NOT_USED(param) ((void)(param))
#define UTILS_MS_PER_SECOND 1000
#define UTILS_SEC_PER_MIN 60
#define UTILS_MIN_PER_HOUR 60
#define UTILS_SEC_TO_MS(_second)  ((_second) * UTILS_MS_PER_SECOND)
#define UTILS_MIN_TO_MS(_minute)  UTILS_SEC_TO_MS((_minute) * UTILS_SEC_PER_MIN)
#define UTILS_HOUR_TO_MS(_hour)  UTILS_MIN_TO_MS((_hour) * UTILS_MIN_PER_HOUR)
#define UTILS_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define UTILS_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define NON_NULL_STR(s) (((s) != NULL) ? (s) : "NULL")
#define UTILS_MIN_STR_LEN 1
#define BIT_PER_BYTE 8
#define TO_STR(x) (#x)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#define EXEC_CALLBACK_CHECK_NULL(cb, ...) do { \
    if ((cb) != NULL) { \
        (cb(__VA_ARGS__)); \
    } \
} while (0)


#define CHECK_TYPE_SIZE(_type, _size) \
    typedef char CHECK_##_type##_SIZE_IS_##_size[(sizeof(_type) == (_size)) ? 1 : -1]

#define UTILS_FREE_2_NULL(ptr) do { \
    if ((ptr) != NULL) { \
        AdapterFree((void *)(ptr)); \
        (ptr) = NULL; \
    } \
} while (0)

typedef struct {
    const char *str;
    uint32_t minLen;
    uint32_t maxLen;
} UtilsStrCheckItem;

/* 单个字节编码后长度 */
#define ONE_BYTE_HEXIFY_LEN 2

/* 十六进制解码后长度，长度变为二分之一 */
#define UNHEXIFY_LEN(len) ((len) / ONE_BYTE_HEXIFY_LEN)

/* 十六进编解码后长度，长度变为2倍 */
#define HEXIFY_LEN(len) ((len) * ONE_BYTE_HEXIFY_LEN)

#define IPV4_STR_MAX_LEN sizeof("xxx.xxx.xxx.xxx")

#define MAC_STR_MAX_LEN sizeof("xx:xx:xx:xx:xx:xx")

#define MAC_BUF_LEN 6

uint32_t UtilsDeltaTime(uint32_t timeNew, uint32_t timeOld);

uint16_t UtilsGetCurTaskIdShort(void);

uint16_t UtilsGetTaskIdShort(void *taskId);

bool UtilsUnhexify(const char *inBuf, uint32_t inBufLen, uint8_t *outBuf, uint32_t outBufLen);

bool UtilsUnhexifyR(const char *inBuf, uint32_t inBufLen, uint8_t *outBuf, uint32_t outBufLen);

bool UtilsHexify(const uint8_t *inBuf, uint32_t inBufLen, char *outBuf, uint32_t outBufLen);

char *UtilsStrDup(const char *str);

char *UtilsStrDupWithLen(const char *str, uint32_t len);

uint8_t *UtilsMallocCopy(const uint8_t *data, uint32_t len);

bool UtilsIsEmptyStr(const char *str);

bool UtilsIsValidStr(const char *str, uint32_t minLen, uint32_t maxLen);

bool UtilsIsValidStrArrayCheck(const UtilsStrCheckItem *array, uint32_t num);

int32_t UtilsReplaceStrWithRange(char *srcAndDst, uint32_t bufLen,
    int32_t startIndex, int32_t endIndex, const char *replaceStr);

int32_t UtilsGetIpV4Str(uint32_t ip, char *ipStr, uint32_t ipStrSize);

int32_t UtilsGetMacStr(const uint8_t *mac, uint32_t macLen, char *macStr, uint32_t macStrSize);

#ifdef __cplusplus
}
#endif

#endif /* UTILS_COMMON_H */
