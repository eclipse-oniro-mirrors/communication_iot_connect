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
#include "utils_common.h"
#include <limits.h>
#include "iotc_os.h"
#include "securec.h"
#include "utils_assert.h"
#include "iotc_errcode.h"

#define MAX_STR_DUP_LEN (64 * 1024)
#define MAX_MALLOC_COPY_LEN (64 * 1024)

#define IP_UINT_INDEX_0 0
#define IP_UINT_INDEX_1 1
#define IP_UINT_INDEX_2 2
#define IP_UINT_INDEX_3 3

#define MAC_UINT_INDEX_0 0
#define MAC_UINT_INDEX_1 1
#define MAC_UINT_INDEX_2 2
#define MAC_UINT_INDEX_3 3
#define MAC_UINT_INDEX_4 4
#define MAC_UINT_INDEX_5 5

uint32_t UtilsDeltaTime(uint32_t timeNew, uint32_t timeOld)
{
    uint32_t deltaTime;

    if (timeNew >= timeOld) {
        deltaTime = timeNew - timeOld;
    } else {
        deltaTime = (UINT32_MAX - timeOld) + timeNew + 1; /* 处理时间翻转 */
    }

    return deltaTime;
}

uint16_t UtilsGetCurTaskIdShort(void)
{
#define TASK_ID_MOD_NUMBER 1000
    return (long)IotcTaskGetCurrentTaskId() % TASK_ID_MOD_NUMBER;
}

uint16_t UtilsGetTaskIdShort(void *taskId)
{
    return (long)taskId % TASK_ID_MOD_NUMBER;
}

static bool HexChar2Num(char hexChar, uint8_t *num)
{
    if (hexChar >= '0' && hexChar <= '9') {
        *num = hexChar - '0';
    } else if (hexChar >= 'a' && hexChar <= 'f') {
        /* 10表示16进制的a的值 */
        *num = hexChar - 'a' + 10;
    } else if (hexChar >= 'A' && hexChar <= 'F') {
        /* 10表示16进制的A的值 */
        *num = hexChar - 'A' + 10;
    } else {
        return false;
    }

    return true;
}

bool UtilsUnhexify(const char *inBuf, uint32_t inBufLen, uint8_t *outBuf, uint32_t outBufLen)
{
    if (inBuf == NULL || inBufLen == 0 || outBuf == NULL || outBufLen == 0) {
        return false;
    }

    /* 对2取余保证解码长度必须为偶数 */
    if ((inBufLen % 2) != 0) {
        return false;
    }

    uint32_t outLen = UNHEXIFY_LEN(inBufLen);
    if (outLen > outBufLen) {
        return false;
    }

    uint32_t outIndex = 0;
    uint8_t high;
    uint8_t low;
    while (outIndex < outLen) {
        if (!HexChar2Num(*(inBuf++), &high)) {
            return false;
        }

        if (!HexChar2Num(*(inBuf++), &low)) {
            return false;
        }
        /* 左移4位将单个16进制字符数的第一位移动到高位 */
        *outBuf++ = (high << 4) | low;
        outIndex++;
    }
    return true;
}

bool UtilsUnhexifyR(const char *inBuf, uint32_t inBufLen, uint8_t *outBuf, uint32_t outBufLen)
{
    if (inBuf == NULL || inBufLen == 0 || outBuf == NULL || outBufLen == 0) {
        return false;
    }

    /* 对2取余保证解码长度必须为偶数 */
    if ((inBufLen % 2) != 0) {
        return false;
    }

    uint32_t outLen = UNHEXIFY_LEN(inBufLen);
    if (outLen > outBufLen) {
        return false;
    }
    char *in = (char *)inBuf + inBufLen - 1;
    uint32_t outIndex = 0;
    uint8_t high;
    uint8_t low;
    while (outIndex < outLen) {
        if (!HexChar2Num(*(in--), &low)) {
            return false;
        }

        if (!HexChar2Num(*(in--), &high)) {
            return false;
        }
        /* 左移4位将单个16进制字符数的第一位移动到高位 */
        *outBuf++ = (high << 4) | low;
        outIndex++;
    }
    return true;
}

bool UtilsHexify(const uint8_t *inBuf, uint32_t inBufLen, char *outBuf, uint32_t outBufLen)
{
    if (inBuf == NULL || inBufLen == 0 || outBuf == NULL || outBufLen == 0) {
        return false;
    }

    if (inBufLen > UNHEXIFY_LEN(outBufLen)) {
        return false;
    }

    uint8_t high;
    uint8_t low;
    while (inBufLen > 0) {
        /* 对16取模得到高位 */
        high = *inBuf / 16;
        /* 对16取余得到低位 */
        low = *inBuf % 16;
        /* 数字10表示16进制字符a与其代表10进制数字的值 */
        if (high < 10) {
            *outBuf++ = '0' + high;
        } else {
            /* 数字10表示16进制字符a与其代表10进制数字的值 */
            *outBuf++ = 'a' + high - 10;
        }
        /* 数字10表示16进制字符a与其代表10进制数字的值 */
        if (low < 10) {
            *outBuf++ = '0' + low;
        } else {
            /* 数字10表示16进制字符a与其代表10进制数字的值 */
            *outBuf++ = 'a' + low - 10;
        }

        ++inBuf;
        inBufLen--;
    }

    return true;
}

char *UtilsStrDup(const char *str)
{
    if (str == NULL) {
        return NULL;
    }
    uint32_t size = strlen(str) + 1;
    if (size > MAX_STR_DUP_LEN) {
        return NULL;
    }

    return (char *)UtilsMallocCopy((const uint8_t *)str, size);
}

char *UtilsStrDupWithLen(const char *str, uint32_t len)
{
    if (str == NULL || len > MAX_STR_DUP_LEN) {
        return NULL;
    }
    char *ret = (char *)UtilsMallocCopy((uint8_t *)str, len + 1);
    if (ret == NULL) {
        return NULL;
    }
    ret[len] = '\0';
    uint32_t curLen = strlen(ret);
    if (curLen != len) {
        IOTC_LOGW("str dup len not match %u/%u", curLen, len);
        IotcFree(ret);
        return NULL;
    }
    return ret;
}

uint8_t *UtilsMallocCopy(const uint8_t *data, uint32_t len)
{
    if (data == NULL || len == 0 || len > MAX_MALLOC_COPY_LEN) {
        return NULL;
    }

    uint8_t *ret = IotcCalloc(len, sizeof(uint8_t));
    if (ret == NULL) {
        return NULL;
    }
    if (memcpy_s(ret, len, data, len) != EOK) {
        IotcFree(ret);
        return NULL;
    }
    return ret;
}

bool UtilsIsEmptyStr(const char *str)
{
    return str == NULL || str[0] == '\0';
}

bool UtilsIsValidStr(const char *str, uint32_t minLen, uint32_t maxLen)
{
    if (str == NULL) {
        return false;
    }
    uint32_t len = strlen(str);
    return len >= minLen && len <= maxLen;
}

bool UtilsIsValidStrArrayCheck(const UtilsStrCheckItem *array, uint32_t num)
{
    if (array == NULL || num == 0) {
        return false;
    }
    for (uint32_t i = 0; i < num; ++i) {
        if (!UtilsIsValidStr(array[i].str, array[i].minLen, array[i].maxLen)) {
            IOTC_LOGW("invalid str %u,%s,%u,%u", i, NON_NULL_STR(array[i].str), array[i].minLen, array[i].maxLen);
            return false;
        }
    }
    return true;
}

int32_t UtilsReplaceStrWithRange(char *srcAndDst, uint32_t bufLen,
    int32_t startIndex, int32_t endIndex, const char *replaceStr)
{
    CHECK_RETURN_LOGW(srcAndDst != NULL && bufLen != 0 && replaceStr != NULL &&
        startIndex >= 0 && endIndex >= 0 && startIndex <= endIndex,
        IOTC_ERR_PARAM_INVALID, "param invalid");
    int32_t len = strlen(srcAndDst);
    CHECK_RETURN_LOGW(endIndex < len, IOTC_ERR_PARAM_INVALID, "param invalid");
    char tmpStr[bufLen];
    CHECK_RETURN_LOGW(strcpy_s(tmpStr, bufLen, srcAndDst) == EOK, IOTC_ERR_SECUREC_STRCPY, "bufLen=%d", bufLen);
    int32_t relplaceStrLen = strlen(replaceStr);
    int32_t endStrSize = len - endIndex + 1;
    char endStr[endStrSize];
    if (endIndex < len - 1) {
        CHECK_RETURN_LOGW(strcpy_s(endStr, endStrSize, &tmpStr[endIndex + 1]) == EOK,
            IOTC_ERR_SECUREC_STRCPY, "len=%d", endStrSize);
    }
    CHECK_RETURN_LOGW(strcpy_s(&tmpStr[startIndex], bufLen - startIndex, replaceStr) == EOK,
        IOTC_ERR_SECUREC_STRCPY, "len=%d", bufLen - startIndex);
    if (endIndex < len - 1) {
        CHECK_RETURN_LOGW(strcpy_s(&tmpStr[startIndex + relplaceStrLen],
            bufLen - startIndex - relplaceStrLen, endStr) == EOK, IOTC_ERR_SECUREC_STRCPY,
            "len=%d", bufLen - startIndex - relplaceStrLen);
    }
    CHECK_RETURN_LOGW(strcpy_s(srcAndDst, bufLen, tmpStr) == EOK, IOTC_ERR_SECUREC_STRCPY, "bufLen=%d", bufLen);
    return IOTC_OK;
}

int32_t UtilsGetIpV4Str(uint32_t ip, char *ipStr, uint32_t ipStrSize)
{
    CHECK_RETURN_LOGW(ipStr != NULL && ipStrSize != 0,
        IOTC_ERR_PARAM_INVALID, "param invalid");
    uint8_t *ipUint = (uint8_t *)&ip;
    CHECK_RETURN_LOGW(sprintf_s(ipStr, ipStrSize, "%u.%u.%u.%u",
        ipUint[IP_UINT_INDEX_3], ipUint[IP_UINT_INDEX_2], ipUint[IP_UINT_INDEX_1], ipUint[IP_UINT_INDEX_0]) > 0,
        IOTC_ERR_SECUREC_SPRINTF, "sprintf_s err");
    return IOTC_OK;
}

int32_t UtilsGetMacStr(const uint8_t *mac, uint32_t macLen, char *macStr, uint32_t macStrSize)
{
    CHECK_RETURN_LOGW(mac != NULL && macLen == MAC_BUF_LEN && macStr != NULL && macStrSize != 0,
        IOTC_ERR_PARAM_INVALID, "param invalid");
    CHECK_RETURN_LOGW(sprintf_s(macStr, macStrSize, "%02x:%02x:%02x:%02x:%02x:%02x",
        mac[MAC_UINT_INDEX_5], mac[MAC_UINT_INDEX_4], mac[MAC_UINT_INDEX_3],
        mac[MAC_UINT_INDEX_2], mac[MAC_UINT_INDEX_1], mac[MAC_UINT_INDEX_0]) > 0,
        IOTC_ERR_SECUREC_SPRINTF, "sprintf_s err");
    return IOTC_OK;
}

void UtilsReplaceCharacters(char *src, char m, char t)
{
    CHECK_V_RETURN(src != NULL);

    char *p = src;
    while (*p != '\0') {
        if (*p == m) {
            *p = t;
        }
        p++;
    }
}