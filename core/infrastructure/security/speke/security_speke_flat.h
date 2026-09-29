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
#ifndef SECURITY_SPEKE_FLAT_H
#define SECURITY_SPEKE_FLAT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* zero-copy view into completeBuff, valid while the buffer lives */
typedef struct {
    const char *start;   /* points into completeBuff */
    uint32_t len;        /* without quotes / without NUL */
} SpekeStrView;

/* 一条键值对的扫描位置（FindKey 内部遍历产物） */
typedef struct {
    uint32_t keyStart;   /* 键文本起始（引号后） */
    uint32_t keyClose;   /* 键文本结束（闭引号位置） */
    uint32_t valStart;   /* 值起始 */
} SpekeFlatEntryPos;

/* 括号配平扫描的游标与状态（Validate 的扫描子过程状态） */
typedef struct {
    uint32_t i;          /* 扫描游标 */
    int32_t depth;       /* 对象嵌套深度 */
    int32_t arrDepth;    /* 数组嵌套深度 */
    bool rootClosed;     /* 根对象是否已闭合 */
} SpekeFlatScanState;

/* security bound: per-value length upper bound (hex char count) */
/* epk 原始长度上限 512B（4096-bit prime 覆盖）；实机实测上限超出设计文档值 96，2026-09-20 证伪 */
#define SPEKE_FLAT_EPK_MAX_LEN        1024
#define SPEKE_FLAT_CHALLENGE_MAX_LEN  32    /* challenge: 16B = 32 hex */
#define SPEKE_FLAT_SALT_MAX_LEN       32    /* salt: <=16B = <=32 hex */
#define SPEKE_FLAT_KCF_MAX_LEN        64    /* kcfData(HMAC): 32B = 64 hex */
#define SPEKE_FLAT_SESSION_ID_MAX_LEN 32    /* sessionId: 16B = 32 hex */
#define SPEKE_FLAT_MESSAGE_MAX_LEN    5     /* message: msgType <= 0x8080 */
#define SPEKE_FLAT_OPCODE_MAX_LEN     5     /* operationCode */
#define SPEKE_FLAT_ERRCODE_MAX_LEN    5     /* errorCode */
#define SPEKE_FLAT_VERSION_MAX_LEN    16    /* "1.0.0" etc */
#define SPEKE_FLAT_MAX_DEPTH          4     /* root -> securityData -> payload -> version */

/* overall validity gate: brace balance, depth<=4, no trailing garbage; call at SpekeProcessPacket entry */
int32_t SpekeFlatValidate(const char *json, uint32_t len);

/* locate key's string value in the root object, return zero-copy view (without quotes) */
int32_t SpekeFlatGetStr(const char *json, uint32_t len, const char *key, SpekeStrView *out);

/* locate key's object value in the root object, return the brace-inclusive range;
 * the view starts with '{' so it can be fed back into FindKey/GetStr/GetNum/GetObj directly */
int32_t SpekeFlatGetObj(const char *json, uint32_t len, const char *key, SpekeStrView *out);

/* numeric fields (message/operationCode/errorCode): locate then parse with strtoll */
int32_t SpekeFlatGetNum(const char *json, uint32_t len, const char *key, int64_t *out);

#endif /* SECURITY_SPEKE_FLAT_H */
