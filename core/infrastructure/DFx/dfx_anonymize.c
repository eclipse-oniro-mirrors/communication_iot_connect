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

#include "dfx_anonymize.h"
#include "securec.h"
#include "utils_assert.h"
#include "utils_common.h"
#include "iotc_errcode.h"

#define IS_VALID_ANONYMIZE_TYPE(type) (((type) >= ANONYMIZE_ID) && ((type) <= ANONYMIZE_NAME))

#define NAME_STR_LONG_LEN 6
#define NAME_STR_LONG_ANONYMIZE_START 2
#define NAME_STR_LONG_ANONYMIZE_UNIT 3

#define IP_UNIT_LEN 4
#define IP_UNIT_NUM 4
#define IP_UNIT_INDEX_0 0
#define IP_UNIT_INDEX_3 3

#define MAC_UNIT_LEN 3
#define MAC_UNIT_NUM 6
#define MAC_UNIT_INDEX_0 0
#define MAC_UNIT_INDEX_1 1
#define MAC_UNIT_INDEX_2 2
#define MAC_UNIT_INDEX_3 3
#define MAC_UNIT_INDEX_4 4
#define MAC_UNIT_INDEX_5 5

typedef struct {
    int32_t lowerLen;
    int32_t upperLen;
    int32_t startIndex;
    int32_t endIndex;
    const char *relplaceStr;
} AnonymizeRule;

typedef struct {
    AnonymizeType type;
    int32_t (*anonymizeStr)(char *srcAndDst, uint32_t bufLen);
} AnonymizeStrFunc;

static int32_t IdAnonymizeStr(char *srcAndDst, uint32_t bufLen);
static int32_t MacAnonymizeStr(char *srcAndDst, uint32_t bufLen);
static int32_t SnAnonymizeStr(char *srcAndDst, uint32_t bufLen);
static int32_t IpAnonymizeStr(char *srcAndDst, uint32_t bufLen);
static int32_t NameAnonymizeStr(char *srcAndDst, uint32_t bufLen);

static const AnonymizeRule ID_ANONYMIZE_RULE[] = {
    {.lowerLen = 1, .upperLen = 1, .startIndex = 0, .endIndex = 0, .relplaceStr = "*"},
    {.lowerLen = 2, .upperLen = 3, .startIndex = 1, .endIndex = -1, .relplaceStr = "*"},
    {.lowerLen = 4, .upperLen = 5, .startIndex = 2, .endIndex = -1, .relplaceStr = "*"},
    {.lowerLen = 6, .upperLen = 10, .startIndex = 4, .endIndex = -1, .relplaceStr = "*"},
    {.lowerLen = 11, .upperLen = 11, .startIndex = 3, .endIndex = 6, .relplaceStr = "*"},
    {.lowerLen = 12, .upperLen = 18, .startIndex = 6, .endIndex = -1, .relplaceStr = "*"},
    {.lowerLen = 19, .upperLen = INT32_MAX, .startIndex = 10, .endIndex = -1, .relplaceStr = "*"},
};

static const AnonymizeRule SN_ANONYMIZE_RULE[] = {
    {.lowerLen = 1, .upperLen = 5, .startIndex = 0, .endIndex = -1, .relplaceStr = "*"},
    {.lowerLen = 6, .upperLen = 12, .startIndex = 2, .endIndex = -3, .relplaceStr = "*"},
    {.lowerLen = 13, .upperLen = INT32_MAX, .startIndex = 3, .endIndex = -4, .relplaceStr = "*"},
};

static const AnonymizeRule NAME_ANONYMIZE_RULE[] = {
    {.lowerLen = 1, .upperLen = 1, .startIndex = 0, .endIndex = -1, .relplaceStr = "*"},
    {.lowerLen = 2, .upperLen = 3, .startIndex = 1, .endIndex = 1, .relplaceStr = "*"},
    {.lowerLen = 4, .upperLen = 5, .startIndex = 1, .endIndex = 2, .relplaceStr = "*"},
};

static const AnonymizeStrFunc ANONYMIZE_FUNC_TBL[] = {
    {
        .type = ANONYMIZE_ID,
        .anonymizeStr = IdAnonymizeStr,
    },
    {
        .type = ANONYMIZE_MAC,
        .anonymizeStr = MacAnonymizeStr,
    },
    {
        .type = ANONYMIZE_SN,
        .anonymizeStr = SnAnonymizeStr,
    },
    {
        .type = ANONYMIZE_IP,
        .anonymizeStr = IpAnonymizeStr,
    },
    {
        .type = ANONYMIZE_NAME,
        .anonymizeStr = NameAnonymizeStr,
    },
};

static int32_t AnonymizeStrWithRule(char *srcAndDst, uint32_t bufLen, const AnonymizeRule *rule, uint32_t ruleSize)
{
    int32_t len = strlen(srcAndDst);

    for (uint32_t i = 0; i < ruleSize; i++) {
        if (len < rule[i].lowerLen || len > rule[i].upperLen) {
            continue;
        }
        int32_t startIndex = (rule[i].startIndex >= 0) ? rule[i].startIndex : (len + rule[i].startIndex);
        int32_t endIndex = (rule[i].endIndex >= 0) ? rule[i].endIndex : (len + rule[i].endIndex);
        return UtilsReplaceStrWithRange(srcAndDst, bufLen, startIndex, endIndex, rule[i].relplaceStr);
    }

    return IOTC_ERR_PARAM_INVALID;
}

static int32_t IdAnonymizeStr(char *srcAndDst, uint32_t bufLen)
{
    return AnonymizeStrWithRule(srcAndDst, bufLen,
        ID_ANONYMIZE_RULE, sizeof(ID_ANONYMIZE_RULE)/ sizeof(AnonymizeRule));
}

static int32_t MacAnonymizeStr(char *srcAndDst, uint32_t bufLen)
{
    char macUnit[MAC_UNIT_NUM][MAC_UNIT_LEN] = {0};
    char *context = NULL;
    char *ch = strtok_s(srcAndDst, ":", &context);
    uint32_t index = 0;
    while (ch != NULL) {
        CHECK_RETURN_LOGW(strcpy_s(macUnit[index++], MAC_UNIT_LEN, ch) == EOK,
            IOTC_ERR_SECUREC_STRCPY, "len=%d", MAC_UNIT_LEN);
        ch = strtok_s(NULL, ":", &context);
    }
    CHECK_RETURN_LOGW(index == MAC_UNIT_NUM, IOTC_ERR_PARAM_INVALID, "index=%u", index);
    CHECK_RETURN_LOGW(sprintf_s(srcAndDst, bufLen, "%s:%s:%s:**:**:%s",
        macUnit[MAC_UNIT_INDEX_0], macUnit[MAC_UNIT_INDEX_1],
        macUnit[MAC_UNIT_INDEX_2], macUnit[MAC_UNIT_INDEX_5]) > 0,
        IOTC_ERR_SECUREC_SPRINTF, "bufLen=%u", bufLen);
    return IOTC_OK;
}

static int32_t SnAnonymizeStr(char *srcAndDst, uint32_t bufLen)
{
    return AnonymizeStrWithRule(srcAndDst, bufLen,
        SN_ANONYMIZE_RULE, sizeof(SN_ANONYMIZE_RULE)/ sizeof(AnonymizeRule));
}

static int32_t IpAnonymizeStr(char *srcAndDst, uint32_t bufLen)
{
    char ipUnit[IP_UNIT_NUM][IP_UNIT_LEN] = {0};
    char *context = NULL;
    char *ch = strtok_s(srcAndDst, ".", &context);
    uint32_t index = 0;
    while (ch != NULL) {
        CHECK_RETURN_LOGW(strcpy_s(ipUnit[index++], IP_UNIT_LEN, ch) == EOK,
            IOTC_ERR_SECUREC_STRCPY, "len=%d", IP_UNIT_LEN);
            ch = strtok_s(NULL, ".", &context);
    }
    CHECK_RETURN_LOGW(index == IP_UNIT_NUM, IOTC_ERR_PARAM_INVALID, "index=%u", index);
    CHECK_RETURN_LOGW(sprintf_s(srcAndDst, bufLen, "%s.*.*.%s", ipUnit[IP_UNIT_INDEX_0], ipUnit[IP_UNIT_INDEX_3]) > 0,
        IOTC_ERR_SECUREC_SPRINTF, "bufLen=%d", bufLen);
    return IOTC_OK;
}

static int32_t NameAnonymizeStr(char *srcAndDst, uint32_t bufLen)
{
    int32_t len = strlen(srcAndDst);
    if (len >= NAME_STR_LONG_LEN) {
        return UtilsReplaceStrWithRange(srcAndDst, bufLen, NAME_STR_LONG_ANONYMIZE_START,
            NAME_STR_LONG_ANONYMIZE_START + len / NAME_STR_LONG_ANONYMIZE_UNIT, "*");
    }
    return AnonymizeStrWithRule(srcAndDst, bufLen,
        NAME_ANONYMIZE_RULE, sizeof(NAME_ANONYMIZE_RULE)/ sizeof(AnonymizeRule));
}

int32_t DfxAnonymizeStrWithBuffer(const char *src, AnonymizeType type, char *dstBuf, uint32_t bufLen)
{
    CHECK_RETURN_LOGW(src != NULL && dstBuf != NULL && bufLen != 0 && IS_VALID_ANONYMIZE_TYPE(type),
        IOTC_ERR_PARAM_INVALID, "param invalid");
    if (src != dstBuf) {
        CHECK_RETURN_LOGW(strcpy_s(dstBuf, bufLen, src) == EOK, IOTC_ERR_SECUREC_STRCPY, "bufLen=%u", bufLen);
    }
    uint32_t anonymizeFuncTblSize = sizeof(ANONYMIZE_FUNC_TBL) / sizeof(AnonymizeStrFunc);
    for (uint32_t i = 0; i < anonymizeFuncTblSize; i++) {
        if (ANONYMIZE_FUNC_TBL[i].type == type) {
            return ANONYMIZE_FUNC_TBL[i].anonymizeStr(dstBuf, bufLen);
        }
    }
    IOTC_LOGW("no find handle %d", type);
    return IOTC_ERR_PARAM_INVALID;
}

int32_t DfxAnonymizeStr(const char *src, AnonymizeType type, char **dst)
{
    CHECK_RETURN_LOGW(src != NULL && IS_VALID_ANONYMIZE_TYPE(type) && dst != NULL,
        IOTC_ERR_PARAM_INVALID, "param invalid");
    uint32_t dstBufLen = strlen(src) + 1;
    char *dstBuf = IotcCalloc(1, dstBufLen);
    CHECK_RETURN_LOGW(dstBuf != NULL, IOTC_ADAPTER_MEM_ERR_MALLOC, "malloc failed");
    int32_t ret = DfxAnonymizeStrWithBuffer(src, type, dstBuf, dstBufLen);
    if (ret != IOTC_OK) {
        IOTC_LOGW("anonymize str err ret=%d", ret);
        IotcFree(dstBuf);
        return ret;
    }

    *dst = dstBuf;
    return IOTC_OK;
}

int32_t DfxAnonymizeIpAddrWithBuffer(uint32_t ip, char *dstBuf, uint32_t bufLen)
{
    char ipStr[IPV4_STR_MAX_LEN] = {0};
    int32_t ret = UtilsGetIpV4Str(ip, ipStr, IPV4_STR_MAX_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get ip str err ret=%d", ret);
        return ret;
    }
    return DfxAnonymizeStrWithBuffer(ipStr, ANONYMIZE_IP, dstBuf, bufLen);
}

int32_t DfxAnonymizeIpAddr(uint32_t ip, char **dst)
{
    char ipStr[IPV4_STR_MAX_LEN] = {0};
    int32_t ret = UtilsGetIpV4Str(ip, ipStr, IPV4_STR_MAX_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get ip str err ret=%d", ret);
        return ret;
    }
    return DfxAnonymizeStr(ipStr, ANONYMIZE_IP, dst);
}

int32_t DfxAnonymizeMacAddrWithBuffer(const uint8_t *mac, uint32_t macLen, char *dstBuf, uint32_t bufLen)
{
    char macStr[MAC_STR_MAX_LEN] = {0};
    int32_t ret = UtilsGetMacStr(mac, macLen, macStr, MAC_STR_MAX_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get mac str err ret=%d", ret);
        return ret;
    }
    return DfxAnonymizeStrWithBuffer(macStr, ANONYMIZE_MAC, dstBuf, bufLen);
}

int32_t DfxAnonymizeMacAddr(const uint8_t *mac, uint32_t macLen, char **dst)
{
    char macStr[MAC_STR_MAX_LEN] = {0};
    int32_t ret = UtilsGetMacStr(mac, macLen, macStr, MAC_STR_MAX_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get mac str err ret=%d", ret);
        return ret;
    }
    return DfxAnonymizeStr(macStr, ANONYMIZE_MAC, dst);
}
