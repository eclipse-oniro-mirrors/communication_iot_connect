/*
 * Copyright (c) 2026-2026 Huawei Device Co., Ltd.
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
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "security_speke_flat.h"
#include "security_speke_defs.h"
#include "iotc_errcode.h"
#include "securec.h"
#include "iotc_log.h"

/* skip whitespace, return new position */
static uint32_t SpekeFlatSkipWs(const char *json, uint32_t len, uint32_t i)
{
    while ((i < len) && isspace((unsigned char)json[i])) {
        i++;
    }
    return i;
}

/* skip string value (json[i]=='"'), return position after closing quote */
#define SPEKE_FLAT_STR_ESCAPE_STEP  2    /* 转义字符跳过步长：反斜杠+被转义字符 */
#define SPEKE_FLAT_STR_MIN_LEN      2    /* 字符串值最小长度：一对引号 */
#define SPEKE_FLAT_VAL_MAX_DEFAULT  256  /* 未登记字段的值长度默认上限 */
#define SPEKE_FLAT_NUM_BUF_LEN      32   /* 数值提取文本缓冲 */
#define SPEKE_FLAT_NUM_BASE         10   /* 协议数值为十进制 */

static uint32_t SpekeFlatSkipStr(const char *json, uint32_t len, uint32_t i)
{
    if ((i >= len) || (json[i] != '"')) {
        return len;
    }
    i++; /* skip opening quote */
    while (i < len) {
        if (json[i] == '\\') {
            i += SPEKE_FLAT_STR_ESCAPE_STEP;
            continue;
        }
        if (json[i] == '"') {
            return i + 1;
        }
        i++;
    }
    return len; /* unterminated */
}

/* skip a JSON value (string/object/array/number), return position after value */
/* skip a nested {...} or [...] block starting at i (the open char); return end+1 or len */
static uint32_t SpekeFlatSkipNested(const char *json, uint32_t len, uint32_t i, char open, char close)
{
    int32_t depth = 0;
    while (i < len) {
        if (json[i] == '"') {
            i = SpekeFlatSkipStr(json, len, i);
            continue;
        }
        if (json[i] == open) {
            depth++;
        } else if (json[i] == close) {
            depth--;
            if (depth == 0) {
                return i + 1;
            }
        }
        i++;
    }
    return len;
}

static uint32_t SpekeFlatSkipVal(const char *json, uint32_t len, uint32_t i)
{
    if (i >= len) {
        return len;
    }
    if (json[i] == '"') {
        return SpekeFlatSkipStr(json, len, i);
    }
    if (json[i] == '{') {
        return SpekeFlatSkipNested(json, len, i, '{', '}');
    }
    if (json[i] == '[') {
        return SpekeFlatSkipNested(json, len, i, '[', ']');
    }
    /* number / bool / null: read until delimiter */
    while ((i < len) && (json[i] != ',') && (json[i] != '}') && (json[i] != ']') &&
           !isspace((unsigned char)json[i])) {
        i++;
    }
    return i;
}

/* per-key length upper bound */
static uint32_t SpekeFlatGetMaxLen(const char *key)
{
    if (strcmp(key, SPEKE_SEC_DATA_EPK_JSON) == 0) {
        return SPEKE_FLAT_EPK_MAX_LEN;
    }
    if (strcmp(key, SPEKE_SEC_DATA_CHALLENGE_JSON) == 0) {
        return SPEKE_FLAT_CHALLENGE_MAX_LEN;
    }
    if (strcmp(key, SPEKE_SEC_DATA_SALT_JSON) == 0) {
        return SPEKE_FLAT_SALT_MAX_LEN;
    }
    if (strcmp(key, SPEKE_SEC_DATA_KCF_JSON) == 0) {
        return SPEKE_FLAT_KCF_MAX_LEN;
    }
    if (strcmp(key, SPEKE_SESSION_ID_JSON) == 0) {
        return SPEKE_FLAT_SESSION_ID_MAX_LEN;
    }
    if (strcmp(key, SPEKE_SEC_DATA_MESSAGE_JSON) == 0) {
        return SPEKE_FLAT_MESSAGE_MAX_LEN;
    }
    if (strcmp(key, SPEKE_SEC_DATA_OPCODE_JSON) == 0) {
        return SPEKE_FLAT_OPCODE_MAX_LEN;
    }
    if (strcmp(key, SPEKE_SEC_DATA_ERR_JSON) == 0) {
        return SPEKE_FLAT_ERRCODE_MAX_LEN;
    }
    if ((strcmp(key, SPEKE_SEC_DATA_CUR_VER_JSON) == 0) ||
        (strcmp(key, SPEKE_SEC_DATA_MIN_VER_JSON) == 0)) {
        return SPEKE_FLAT_VERSION_MAX_LEN;
    }
    return SPEKE_FLAT_VAL_MAX_DEFAULT; /* default */
}

/* brace balance + depth limit + buffer bounds + no trailing garbage */
/* 括号配平扫描体：从 i 起逐字符分类计数（对象深度/数组深度/root 闭合）。
   括号不配平/超深/root 未闭合/尾随垃圾 → PARSE（主函数拆分出的扫描子过程） */
static int32_t SpekeFlatScanBalance(const SpekeStrView *pkt, SpekeFlatScanState *st)
{
    const char *json = pkt->start;
    uint32_t len = pkt->len;
    uint32_t i = st->i;
    while (i < len) {
        if (st->rootClosed) {
            if (!isspace((unsigned char)json[i])) {
                return IOTC_ADAPTER_JSON_ERR_PARSE;
            }
            i++;
            continue;
        }
        if (json[i] == '"') {
            i = SpekeFlatSkipStr(json, len, i);
            continue;
        }
        if (json[i] == '{') {
            st->depth++;
            if (st->depth > SPEKE_FLAT_MAX_DEPTH) {
                return IOTC_ADAPTER_JSON_ERR_PARSE;
            }
        } else if (json[i] == '}') {
            st->depth--;
            if (st->depth < 0) {
                return IOTC_ADAPTER_JSON_ERR_PARSE;
            }
            if (st->depth == 0) {
                st->rootClosed = true;
            }
        } else if (json[i] == '[') {
            st->arrDepth++;
        } else if (json[i] == ']') {
            st->arrDepth--;
            if (st->arrDepth < 0) {
                return IOTC_ADAPTER_JSON_ERR_PARSE;
            }
        }
        i++;
    }
    st->i = i;
    return IOTC_OK;
}

int32_t SpekeFlatValidate(const char *json, uint32_t len)
{
    if ((json == NULL) || (len == 0)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    SpekeStrView pkt = { json, len };
    uint32_t i = SpekeFlatSkipWs(json, len, 0);
    if ((i >= len) || (json[i] != '{')) {
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }

    SpekeFlatScanState st = { i, 0, 0, false };
    int32_t ret = SpekeFlatScanBalance(&pkt, &st);
    if (ret != IOTC_OK) {
        return ret;
    }
    if ((st.depth != 0) || (st.arrDepth != 0) || !st.rootClosed) {
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    return IOTC_OK;
}

/* skip the current value and the trailing comma; return the next position,
 * len+1 on error */
static uint32_t SpekeFlatSkipEntryVal(const char *json, uint32_t len, uint32_t valStart)
{
    uint32_t i = SpekeFlatSkipVal(json, len, valStart);
    if (i > len) {
        return len + 1;
    }
    i = SpekeFlatSkipWs(json, len, i);
    if ((i < len) && (json[i] == ',')) {
        i++;
    }
    return i;
}

/* find key at depth 1 in object, return value range [vStart, vEnd) */
/* parse one quoted key string at i; return position after the closing quote,
 * key content range via keyStart/keyClose. Returns len+1 on error. */
static uint32_t SpekeFlatScanKey(const char *json, uint32_t len, uint32_t i,
    uint32_t *keyStart, uint32_t *keyClose)
{
    if ((i >= len) || (json[i] != '"')) {
        return len + 1;
    }
    *keyStart = i + 1;
    i = SpekeFlatSkipStr(json, len, i);
    if (i > len) {
        return len + 1;
    }
    *keyClose = i - 1; /* position of closing quote */
    return i;
}

/* skip ':' and surrounding whitespace; return position after it, len+1 on error */
static uint32_t SpekeFlatScanColon(const char *json, uint32_t len, uint32_t i)
{
    i = SpekeFlatSkipWs(json, len, i);
    if ((i >= len) || (json[i] != ':')) {
        return len + 1;
    }
    return SpekeFlatSkipWs(json, len, i + 1);
}

/* 判断 depth-1 的键是否命中：命中则回填值区间并返回 true；未命中/值非法返回 false
   （值非法场景由主循环后续 SkipEntryVal 兜底拒绝，行为与原实现一致） */
static bool SpekeFlatKeyHit(const SpekeStrView *pkt, const char *key,
    const SpekeFlatEntryPos *pos, SpekeStrView *out)
{
    const char *json = pkt->start;
    uint32_t kLen = pos->keyClose - pos->keyStart;
    uint32_t targetLen = (uint32_t)strlen(key);
    if ((kLen != targetLen) || (memcmp(json + pos->keyStart, key, targetLen) != 0)) {
        return false;
    }
    uint32_t valEnd = SpekeFlatSkipVal(json, pkt->len, pos->valStart);
    if (valEnd > pkt->len) {
        return false;
    }
    out->start = json + pos->valStart;
    out->len = valEnd - pos->valStart;
    return true;
}

static int32_t SpekeFlatFindKey(const char *json, uint32_t len, const char *key,
    uint32_t *vStart, uint32_t *vEnd)
{
    SpekeStrView pkt = { json, len };
    uint32_t i = SpekeFlatSkipWs(json, len, 0);
    if ((i >= len) || (json[i] != '{')) {
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    i++; /* skip { */

    int32_t depth = 1;
    while ((i < len) && (depth > 0)) {
        i = SpekeFlatSkipWs(json, len, i);
        if (i >= len) {
            return IOTC_ADAPTER_JSON_ERR_PARSE;
        }
        if (json[i] == '}') {
            depth--;
            i++;
            continue;
        }
        SpekeFlatEntryPos pos;
        i = SpekeFlatScanKey(json, len, i, &pos.keyStart, &pos.keyClose);
        if (i > len) {
            return IOTC_ADAPTER_JSON_ERR_PARSE;
        }
        i = SpekeFlatScanColon(json, len, i);
        if (i > len) {
            return IOTC_ADAPTER_JSON_ERR_PARSE;
        }
        pos.valStart = i;

        /* check if key matches at depth 1 */
        if (depth == 1) {
            SpekeStrView hit;
            if (SpekeFlatKeyHit(&pkt, key, &pos, &hit)) {
                *vStart = hit.start - json;
                *vEnd = hit.start - json + hit.len;
                return IOTC_OK;
            }
        }
        i = SpekeFlatSkipEntryVal(json, len, pos.valStart);
        if (i > len) {
            return IOTC_ADAPTER_JSON_ERR_PARSE;
        }
    }
    return IOTC_ADAPTER_JSON_ERR_PARSE; /* key not found */
}

/* length upper bound + string value view (without quotes) */
int32_t SpekeFlatGetStr(const char *json, uint32_t len, const char *key, SpekeStrView *out)
{
    if ((json == NULL) || (len == 0) || (key == NULL) || (out == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    uint32_t vStart = 0;
    uint32_t vEnd = 0;
    int32_t ret = SpekeFlatFindKey(json, len, key, &vStart, &vEnd);
    if (ret != IOTC_OK) {
        return ret;
    }
    if ((vStart >= len) || (json[vStart] != '"')) {
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    /* vEnd is after closing quote; content is [vStart+1, vEnd-1) */
    if ((vEnd < SPEKE_FLAT_STR_MIN_LEN) || (json[vEnd - 1] != '"')) {
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    uint32_t contentLen = (vEnd - 1) - (vStart + 1);
    uint32_t maxLen = SpekeFlatGetMaxLen(key);
    if (contentLen > maxLen) {
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    out->start = json + vStart + 1;
    out->len = contentLen;
    return IOTC_OK;
}

/* object value content range (between { and }, exclusive) */
int32_t SpekeFlatGetObj(const char *json, uint32_t len, const char *key, SpekeStrView *out)
{
    if ((json == NULL) || (len == 0) || (key == NULL) || (out == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    uint32_t vStart = 0;
    uint32_t vEnd = 0;
    int32_t ret = SpekeFlatFindKey(json, len, key, &vStart, &vEnd);
    if (ret != IOTC_OK) {
        return ret;
    }
    if ((vStart >= len) || (json[vStart] != '{')) {
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    if (vEnd < SPEKE_FLAT_STR_MIN_LEN) {
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    /* closing brace sanity: vEnd-1 must land on the object's '}' */
    if (json[vEnd - 1] != '}') {
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    out->start = json + vStart;
    out->len = vEnd - vStart;
    return IOTC_OK;
}

/* numeric field: locate then parse with strtoll */
/* 十进制文本 → int64（仅接受纯数字，拒绝符号/空白/溢出；协议数值均非负） */
static int32_t SpekeFlatStrToNum(const char *numBuf, int64_t *out)
{
    int64_t val = 0;
    for (const char *p = numBuf; *p != '\0'; p++) {
        if ((*p < '0') || (*p > '9')) {
            return IOTC_ADAPTER_JSON_ERR_PARSE;
        }
        if (val > ((int64_t)INT64_MAX - (*p - '0')) / SPEKE_FLAT_NUM_BASE) {
            return IOTC_ADAPTER_JSON_ERR_PARSE;
        }
        val = val * SPEKE_FLAT_NUM_BASE + (*p - '0');
    }
    if (numBuf[0] == '\0') {
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    *out = val;
    return IOTC_OK;
}

int32_t SpekeFlatGetNum(const char *json, uint32_t len, const char *key, int64_t *out)
{
    if ((json == NULL) || (len == 0) || (key == NULL) || (out == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    uint32_t vStart = 0;
    uint32_t vEnd = 0;
    int32_t ret = SpekeFlatFindKey(json, len, key, &vStart, &vEnd);
    if (ret != IOTC_OK) {
        return ret;
    }
    if ((vStart >= len) || (vEnd > len) || (vEnd <= vStart)) {
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    uint32_t numLen = vEnd - vStart;
    uint32_t maxLen = SpekeFlatGetMaxLen(key);
    if (numLen > maxLen) {
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    char numBuf[SPEKE_FLAT_NUM_BUF_LEN];
    if (numLen >= sizeof(numBuf)) {
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    if (memcpy_s(numBuf, sizeof(numBuf), json + vStart, numLen) != EOK) {
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    numBuf[numLen] = '\0';
    int64_t val = 0;
    ret = SpekeFlatStrToNum(numBuf, &val);
    if (ret != IOTC_OK) {
        return ret;
    }
    *out = val;
    return IOTC_OK;
}
