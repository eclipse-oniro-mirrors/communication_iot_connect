/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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
#include <stddef.h>
#include <string.h>
#include "security_speke_server.h"
#include "security_speke_defs.h"
#include "security_speke_common.h"
#include "security_speke_session.h"
#include "iotc_log.h"
#include "iotc_mem.h"
#include "security_random.h"
#include "security_speke_flat.h"
#include "securec.h"
#include "utils_assert.h"
#include "utils_common.h"
#include "iotc_errcode.h"

static int32_t SpekeServerBuildRspFallback(NegoContext *negoCtx, SpekeProcessParam param,
    uint8_t **msg, uint32_t *len);

static int32_t VerifyClientReqPayload(const char *payload, uint32_t payloadLen)
{
    /* flat extraction */
    int64_t opCode = 0;
    int32_t ret = SpekeFlatGetNum(payload, payloadLen, SPEKE_SEC_DATA_OPCODE_JSON, &opCode);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke server verify req get opcode flat err:%d", ret);
        return ret;
    }
    if (opCode != SPEKE_OPCODE) {
        IOTC_LOGE("Speke server verify opcode:%lld err", (long long)opCode);
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_OPCODE;
    }

    ret = SpekeCommonVerifyVersion(payload, payloadLen);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke server verify version err:%d", ret);
        return ret;
    }

    return IOTC_OK;
}

static int32_t CreateSpekeServerRspSecPayload(const NegoContext *negoCtx, IotcJson **rspPayload)
{
    IotcJson *payload = IotcJsonCreate();
    if (payload == NULL) {
        IOTC_LOGE("Speke server rsp create payload JSON err");
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }

    int32_t ret = SpekeCommonAddVerInfoToJson(payload);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke server rsp add ver info JSON err:%d", ret);
        IotcJsonDelete(payload);
        return ret;
    }
    ret = SpekeCommonAddDataToJson(payload, SPEKE_SEC_DATA_CHALLENGE_JSON, negoCtx->localChallenge, CHALLENGE_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke server rsp add challenge JSON err:%d", ret);
        IotcJsonDelete(payload);
        return ret;
    }
    ret = SpekeCommonAddDataToJson(payload, SPEKE_SEC_DATA_SALT_JSON, negoCtx->salt, negoCtx->saltLen);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke server rsp add salt JSON err:%d", ret);
        IotcJsonDelete(payload);
        return ret;
    }
    ret = SpekeCommonAddDataToJson(payload, SPEKE_SEC_DATA_EPK_JSON, negoCtx->pubKey, negoCtx->pubKeyLen);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke server rsp add epk JSON err:%d", ret);
        IotcJsonDelete(payload);
        return ret;
    }

    *rspPayload = payload;
    return IOTC_OK;
}

/* 向 buf 的 pos 处追加一段字节（容量参数为防御性校验，长度已预先精确计入 total） */
static int32_t RspMsgAppend(char *buf, uint32_t bufCap, uint32_t *pos, const char *str, uint32_t strLen)
{
    if (memcpy_s(buf + *pos, bufCap - *pos, str, strLen) != EOK) {
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    *pos += strLen;
    return IOTC_OK;
}

/* 向 buf 的 pos 处追加二进制数据的 hex 文本 */
static int32_t RspMsgAppendHex(char *buf, uint32_t bufCap, uint32_t *pos,
    const uint8_t *data, uint32_t dataLen)
{
    if (!UtilsHexify(data, dataLen, buf + *pos, bufCap - *pos)) {
        return IOTC_CORE_COMM_UTILS_ERR_HEXIFY;
    }
    *pos += HEXIFY_LEN(dataLen);
    return IOTC_OK;
}

/* cJSON print 会转义 '"'、'\\' 与 <0x20 控制字符。sessionId 取自对端
   报文的 flat 视图，含上述字符时手写序列化会产生非法 JSON——此时回退 cJSON 构造。 */
static bool SpekeSessionIdNeedEscape(const char *sessionId)
{
    for (const char *p = sessionId; *p != '\0'; p++) {
        if ((*p == '"') || (*p == '\\') || ((unsigned char)*p < 0x20)) {
            return true;
        }
    }
    return false;
}

/* SERVER_RSP 单缓冲手写序列化。基线路径需建 payload cJSON 树
   （节点 + epk 值 strdup ~1-1.3KB）、CreateSecDataObj 内 IotcDuplicateJson 整树复制
   （再 ~1-1.3KB）、print 再拷一份（~700-950B）及 hexify 临时缓冲；本函数按
   cJSON 紧凑格式（cJSON_PrintUnformatted）单缓冲直写，键序与 cJSON 插入序一致，
   正常输入下输出与基线逐字节相同，树与中转全部消除。
   prefixLen>0 时前部预留该字节数（置零，调用方写帧头），报文体直接
   序列化到偏移处，*len = prefixLen + 报文体长度。 */
/* RSP JSON 骨架字面量（文件作用域：total 核算与逐段写入共用同一份清单） */
#define SPEKE_HEX_CHARS_PER_BYTE 2   /* hex 文本 2 字符编码 1 字节 */
#define SPEKE_DEC_BASE 10            /* 十进制基数 */
#define SPEKE_DEC_MAX_DIGITS 10      /* uint32 十进制最大位数 */
#define SPEKE_RSP_VER_COUNT 2        /* 版本字段写入次数（当前/最低） */

static const char LIT_HEAD[] = "{\"sessionId\":\"";
static const char LIT_SEC_DATA[] = "\",\"securityData\":{\"message\":";
static const char LIT_PAYLOAD[] = ",\"payload\":{\"version\":{\"currentVersion\":\"";
static const char LIT_MIN_VER[] = "\",\"minVersion\":\"";
static const char LIT_VER_END[] = "\"},\"challenge\":\"";
static const char LIT_SALT[] = "\",\"salt\":\"";
static const char LIT_EPK[] = "\",\"epk\":\"";
static const char LIT_TAIL[] = "\"}}}";

/* 数值转十进制文本（无 snprintf 依赖，宏值变更亦正确），返回位数 */
static uint32_t SpekeRspNumToStr(uint32_t numVal, char *msgNum, uint32_t cap)
{
    char rev[SPEKE_DEC_MAX_DIGITS] = { 0 };
    uint32_t numLen = 0;
    do {
        rev[numLen] = (char)('0' + (numVal % SPEKE_DEC_BASE));
        numVal /= SPEKE_DEC_BASE;
        numLen++;
    } while ((numVal > 0) && (numLen < cap));
    for (uint32_t i = 0; i < numLen; i++) {
        msgNum[i] = rev[numLen - 1 - i];
    }
    return numLen;
}

/* 逐段写入 RSP 正文（段顺序 = 紧凑格式模板的字段顺序）。
   任一段失败立即返回错误码（buf 由调用方统一释放）；成功时 *pos 推进到正文末尾。 */
static int32_t SpekeRspWriteSegments(char *buf, uint32_t capAll, uint32_t *pos,
    const NegoContext *negoCtx, const char *sessionId)
{
    char msgNum[SPEKE_DEC_MAX_DIGITS + 1] = { 0 };
    uint32_t numLen = SpekeRspNumToStr((uint32_t)SPEKE_SEC_DATA_MSG_TYPE_SERVER_RSP, msgNum, sizeof(msgNum));
    uint32_t sidLen = (uint32_t)strlen(sessionId);
    uint32_t verLen = (uint32_t)strlen(SPEKE_VERSION);
    int32_t ret = RspMsgAppend(buf, capAll, pos, LIT_HEAD, sizeof(LIT_HEAD) - 1);      /* {"sessionId":" */
    CHECK_RETURN(ret == IOTC_OK, ret);
    ret = RspMsgAppend(buf, capAll, pos, sessionId, sidLen);  /* 会话 ID（已预检无转义） */
    CHECK_RETURN(ret == IOTC_OK, ret);
    ret = RspMsgAppend(buf, capAll, pos, LIT_SEC_DATA, sizeof(LIT_SEC_DATA) - 1);  /* ","securityData":{"message": */
    CHECK_RETURN(ret == IOTC_OK, ret);
    ret = RspMsgAppend(buf, capAll, pos, msgNum, numLen);  /* 消息类型十进制文本 */
    CHECK_RETURN(ret == IOTC_OK, ret);
    ret = RspMsgAppend(buf, capAll, pos, LIT_PAYLOAD, sizeof(LIT_PAYLOAD) - 1);  /* ,"payload":{"version":... */
    CHECK_RETURN(ret == IOTC_OK, ret);
    ret = RspMsgAppend(buf, capAll, pos, SPEKE_VERSION, verLen);  /* 当前版本 */
    CHECK_RETURN(ret == IOTC_OK, ret);
    ret = RspMsgAppend(buf, capAll, pos, LIT_MIN_VER, sizeof(LIT_MIN_VER) - 1);  /* ","minVersion":" */
    CHECK_RETURN(ret == IOTC_OK, ret);
    ret = RspMsgAppend(buf, capAll, pos, SPEKE_VERSION, verLen);  /* 最低版本（=当前版本） */
    CHECK_RETURN(ret == IOTC_OK, ret);
    ret = RspMsgAppend(buf, capAll, pos, LIT_VER_END, sizeof(LIT_VER_END) - 1);  /* "},"challenge":" */
    CHECK_RETURN(ret == IOTC_OK, ret);
    ret = RspMsgAppendHex(buf, capAll, pos, negoCtx->localChallenge, CHALLENGE_LEN); /* 本端 challenge（二进制→hex） */
    CHECK_RETURN(ret == IOTC_OK, ret);
    ret = RspMsgAppend(buf, capAll, pos, LIT_SALT, sizeof(LIT_SALT) - 1);  /* ","salt":" */
    CHECK_RETURN(ret == IOTC_OK, ret);
    ret = RspMsgAppendHex(buf, capAll, pos, negoCtx->salt, negoCtx->saltLen);  /* salt（二进制→hex） */
    CHECK_RETURN(ret == IOTC_OK, ret);
    ret = RspMsgAppend(buf, capAll, pos, LIT_EPK, sizeof(LIT_EPK) - 1);  /* ","epk":" */
    CHECK_RETURN(ret == IOTC_OK, ret);
    ret = RspMsgAppendHex(buf, capAll, pos, negoCtx->pubKey, negoCtx->pubKeyLen);  /* 本端公钥（二进制→hex） */
    CHECK_RETURN(ret == IOTC_OK, ret);
    return RspMsgAppend(buf, capAll, pos, LIT_TAIL, sizeof(LIT_TAIL) - 1);             /* "}}} */
}

static int32_t CreateSpekeServerRspMsg(const NegoContext *negoCtx, const char *sessionId,
    uint32_t prefixLen, uint8_t **msg, uint32_t *len)
{
    if ((negoCtx == NULL) || (negoCtx->pubKey == NULL) || (negoCtx->pubKeyLen == 0) ||
        (negoCtx->saltLen == 0) || (sessionId == NULL) || (msg == NULL) || (len == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }

    /* 紧凑格式（值均为 hex/数字/预检无转义字符串）：
       {"sessionId":"S","securityData":{"message":N,"payload":{"version":
       {"currentVersion":"V","minVersion":"V"},"challenge":"C","salt":"T","epk":"E"}}} */
    char msgNum[SPEKE_DEC_MAX_DIGITS + 1] = { 0 };
    uint32_t numLen = SpekeRspNumToStr((uint32_t)SPEKE_SEC_DATA_MSG_TYPE_SERVER_RSP, msgNum, sizeof(msgNum));
    uint32_t sidLen = (uint32_t)strlen(sessionId);
    uint32_t verLen = (uint32_t)strlen(SPEKE_VERSION);
    /* 报文总长核算：各段与 SpekeRspWriteSegments 的写入序列一一对应——
       新增/删除字段时两处必须同步修改；不一致会被下方 pos 兜底拦截（配网失败而非发坏报文） */
    uint32_t total = (uint32_t)(sizeof(LIT_HEAD) - 1 + sizeof(LIT_SEC_DATA) - 1 + sizeof(LIT_PAYLOAD) - 1 +
        sizeof(LIT_MIN_VER) - 1 + sizeof(LIT_VER_END) - 1 + sizeof(LIT_SALT) - 1 + sizeof(LIT_EPK) - 1 +
        sizeof(LIT_TAIL) - 1) + sidLen + numLen + (verLen * SPEKE_RSP_VER_COUNT) +
        HEXIFY_LEN(CHALLENGE_LEN) + HEXIFY_LEN(negoCtx->saltLen) + HEXIFY_LEN(negoCtx->pubKeyLen);

    uint32_t capAll = prefixLen + total + 1;   /* 前缀预留 + 报文体 + 结束符 */
    char *buf = (char *)IotcMalloc(capAll);
    if (buf == NULL) {
        IOTC_LOGE("Speke server rsp msg malloc:%u err", capAll);
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    if (prefixLen > 0) {
        (void)memset_s(buf, prefixLen, 0, prefixLen);   /* 前缀区置零，调用方写帧头 */
    }

    uint32_t pos = prefixLen;
    int32_t ret = SpekeRspWriteSegments(buf, capAll, &pos, negoCtx, sessionId);
    if ((ret == IOTC_OK) && (pos != prefixLen + total)) {   /* 长度核算不一致，防御性兜底 */
        IOTC_LOGE("Speke server rsp msg len mismatch:%u != %u", pos, prefixLen + total);
        ret = IOTC_ERR_PARAM_INVALID;
    }
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke server rsp msg append err:%d", ret);
        IotcFree(buf);
        return ret;
    }
    buf[prefixLen + total] = '\0';

    *msg = (uint8_t *)buf;
    *len = prefixLen + total;
    return IOTC_OK;
}

int32_t SpekeServerProcessReq(SpekeProcessParam param, uint8_t **msg, uint32_t *len)
{
    if ((msg == NULL) || (len == NULL) || (param.session == NULL) ||
        (param.sessionId == NULL) || (param.payload == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    SpekeSession *session = param.session;
    if (session->spekeType != SPEKE_TYPE_SERVER) {
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_TYPE;
    }

    int32_t ret = VerifyClientReqPayload(param.payload, param.payloadLen);
    if (ret != IOTC_OK) {
        return ret;
    }

    if (session->negoContext == NULL) {
        IOTC_LOGW("proc req err, nego ctx null");
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_NEGOCTX_NOT_INIT;
    }
    NegoContext *negoCtx = session->negoContext;

    /* pubKey 已在上一次 RSP 序列化后释放（SLE/WiFi 的 negoCtx 跨协商
       存活、以及重传场景），确定性重生成，值与初次生成逐字节相同；
       BLE 的 negoCtx 在 CFM 后已整体释放，重传 REQ 走不到 CFM 之后的路径 */
    if (negoCtx->pubKey == NULL) {
        ret = NegoContextRegenPubKey(negoCtx, session->pinCode, session->pinCodeLen);
        CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "Speke server regen pubKey err:%d", ret);
    }

    /* SERVER_RSP 常规路径单缓冲手写序列化，消 payload 树、
       CreateSecDataObj 的整树复制与 print 拷贝，输出与 cJSON 构造逐字节一致；
       sessionId 含需转义字符时回退 cJSON 构造，wire 与基线一致 */
    if (!SpekeSessionIdNeedEscape(param.sessionId)) {
        ret = CreateSpekeServerRspMsg(negoCtx, param.sessionId, param.prefixLen, msg, len);
        if (ret == IOTC_OK) {
            IotcFree(negoCtx->pubKey);
            negoCtx->pubKey = NULL;
            negoCtx->pubKeyLen = 0;
        }
        return ret;
    }

    /* sessionId 含需转义字符：回退 cJSON 构造（罕见场景），wire 与基线一致 */
    return SpekeServerBuildRspFallback(negoCtx, param, msg, len);
}

/* sessionId 含需转义字符时的回退构造：payload cJSON 树 + 信封 + 前缀化。
   序列化成功即释放 pubKey（报文已含 epk，此后无人再读——共享密钥计算只用
   remotePubKey/random/prime，HMAC 只绑 challenges）。 */
static int32_t SpekeServerBuildRspFallback(NegoContext *negoCtx, SpekeProcessParam param,
    uint8_t **msg, uint32_t *len)
{
    IotcJson *rspPayload = NULL;
    int32_t ret = CreateSpekeServerRspSecPayload(negoCtx, &rspPayload);
    if (ret != IOTC_OK) {
        return ret;
    }
    ret = SpekeCommonCreateNegoMsg(param.sessionId, SPEKE_SEC_DATA_MSG_TYPE_SERVER_RSP, rspPayload, msg, len);
    IotcJsonDelete(rspPayload);
    if (ret == IOTC_OK) {
        /* 前缀化：小报文一次拷贝 */
        ret = SpekeCommonWrapPrefix(msg, len, param.prefixLen);
    }
    if (ret == IOTC_OK) {
        IotcFree(negoCtx->pubKey);
        negoCtx->pubKey = NULL;
        negoCtx->pubKeyLen = 0;
    }
    return ret;
}

/* unhexify the remote pubKey view into binary form and derive the session key.
 * inPlace=true 时视图前半就地压缩（UtilsUnhexify 为前向 2:1 循环，
 * 写位置恒在读位置前方，就地安全；写入范围 [start, start+len/2) 严格在视图内部，
 * 不触碰闭合引号与后续 kcf 字段），消除贯穿 exp_mod 的 bin 堆分配（T10 峰 −256/384B）。 */
static int32_t SpekeServerGenSessionKey(NegoContext *negoCtx, const SpekeStrView *epkView, bool inPlace)
{
    if (epkView->len == 0 || (epkView->len % SPEKE_HEX_CHARS_PER_BYTE) != 0) {
        IOTC_LOGE("Speke server remote pubKey hex invalid, len:%u", epkView->len);
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }
    uint32_t binCap = epkView->len / SPEKE_HEX_CHARS_PER_BYTE;
    if (inPlace) {
        if (!UtilsUnhexify(epkView->start, epkView->len, (uint8_t *)epkView->start, binCap)) {
            IOTC_LOGE("Speke server remote pubKey in-place unhexify err");
            return IOTC_CORE_COMM_UTILS_ERR_UNHEXIFY;
        }
        return NegoContextGenSessionKey(negoCtx, (const uint8_t *)epkView->start, binCap);
    }
    /* 非 BLE（payload 宿主可写性未核实）：保留原 malloc 中转路径 */
    uint8_t *bin = (uint8_t *)IotcMalloc(binCap);
    if (bin == NULL) {
        IOTC_LOGE("Speke server remote pubKey malloc err:%u", binCap);
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    if (!UtilsUnhexify(epkView->start, epkView->len, bin, binCap)) {
        IOTC_LOGE("Speke server remote pubKey unhexify err");
        IotcFree(bin);
        return IOTC_CORE_COMM_UTILS_ERR_UNHEXIFY;
    }
    int32_t ret = NegoContextGenSessionKey(negoCtx, bin, binCap);
    IotcFree(bin);
    return ret;
}

#define CFM_FIELD_CHALLENGE 0
#define CFM_FIELD_EPK       1
#define CFM_FIELD_KCF       2
#define CFM_FIELD_NUM       3

static int32_t ParseClientCfmPayload(NegoContext *negoCtx, const char *payload, uint32_t payloadLen, bool inPlace)
{
    const char *fields[] = {SPEKE_SEC_DATA_CHALLENGE_JSON, SPEKE_SEC_DATA_EPK_JSON, SPEKE_SEC_DATA_KCF_JSON};
    SpekeStrView views[CFM_FIELD_NUM];
    int32_t ret = IOTC_OK;
    for (uint32_t i = 0; i < CFM_FIELD_NUM; i++) {
        ret = SpekeFlatGetStr(payload, payloadLen, fields[i], &views[i]);
        CHECK_RETURN(ret == IOTC_OK, ret);
    }

    uint8_t challengeBuf[UNHEXIFY_LEN(SPEKE_FLAT_CHALLENGE_MAX_LEN)];
    uint32_t cLen = UNHEXIFY_LEN(views[CFM_FIELD_CHALLENGE].len);
    if (!UtilsUnhexify(views[CFM_FIELD_CHALLENGE].start, views[CFM_FIELD_CHALLENGE].len,
        challengeBuf, sizeof(challengeBuf))) {
        IOTC_LOGE("Speke server unhexify challenge err");
        return IOTC_CORE_COMM_UTILS_ERR_UNHEXIFY;
    }
    ret = NegoContextSetRemoteChallenge(negoCtx, challengeBuf, cLen);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke server set remote challenge err:%d", ret);
        return ret;
    }

    uint8_t hmacBuf[HMAC_LEN];
    uint32_t hLen = UNHEXIFY_LEN(views[CFM_FIELD_KCF].len);
    if (!UtilsUnhexify(views[CFM_FIELD_KCF].start, views[CFM_FIELD_KCF].len, hmacBuf, sizeof(hmacBuf))) {
        IOTC_LOGE("Speke server unhexify hmac err");
        return IOTC_CORE_COMM_UTILS_ERR_UNHEXIFY;
    }
    /* challenge/epk/kcf 三次 flat 扫描已全部完成，缓冲上不再有扫描——
       此时就地 unhexify 写入的二进制不会再破坏后续解析 */
    ret = SpekeServerGenSessionKey(negoCtx, &views[CFM_FIELD_EPK], inPlace);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke server gen session key err:%d", ret);
        return ret;
    }

    ret = NegoContextVerifyHmac(negoCtx, hmacBuf, hLen);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke server verify hmac err:%d", ret);
        return ret;
    }
    return IOTC_OK;
}
static int32_t CreateSpekeServerCfmSecPayload(const NegoContext *negoCtx, IotcJson **cfmPayload)
{
    IotcJson *payload = IotcJsonCreate();
    if (payload == NULL) {
        IOTC_LOGE("Speke server create cfm payload JSON err");
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }

    uint8_t hmac[HMAC_LEN] = { 0 };
    int32_t ret = NegoContextGenHmac(negoCtx, hmac, HMAC_LEN);
    if (ret != IOTC_OK) {
        IotcJsonDelete(payload);
        return ret;
    }
    ret = SpekeCommonAddDataToJson(payload, SPEKE_SEC_DATA_KCF_JSON, hmac, HMAC_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Speke server add hmac to cfm payload err:%d", ret);
        IotcJsonDelete(payload);
        return ret;
    }

    *cfmPayload = payload;
    return IOTC_OK;
}

int32_t SpekeServerProcessCfm(SpekeProcessParam param, uint8_t **msg, uint32_t *len)
{
    if ((msg == NULL) || (len == NULL) || (param.session == NULL) ||
        (param.sessionId == NULL) || (param.payload == NULL)) {
        return IOTC_ERR_PARAM_INVALID;
    }
    SpekeSession *session = param.session;
    if (session->spekeType != SPEKE_TYPE_SERVER) {
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_TYPE;
    }
    if (session->negoContext == NULL) {
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_NEGOCTX_NOT_INIT;
    }

    /* pubKey 已在 RSP 序列化后释放（见 SpekeServerProcessReq），
       此处无需再释放；CFM 路径只读 remotePubKey/random/prime，
       GenNegoHmac 只绑 challenges 不绑公钥，重复 CFM 亦不读 pubKey */

    int32_t ret = ParseClientCfmPayload(session->negoContext, param.payload, param.payloadLen,
        param.payloadWritable);
    if (ret != IOTC_OK) {
        return ret;
    }

    IotcJson *cfmPayload = NULL;
    ret = CreateSpekeServerCfmSecPayload(session->negoContext, &cfmPayload);
    if (ret != IOTC_OK) {
        return ret;
    }

    ret = SpekeCommonCreateNegoMsg(param.sessionId, SPEKE_SEC_DATA_MSG_TYPE_SERVER_CFM, cfmPayload, msg, len);
    if (ret != IOTC_OK) {
        IotcJsonDelete(cfmPayload);
        return ret;
    }
    IotcJsonDelete(cfmPayload);

    return NegoContextGenDataEncKey(session->negoContext, session->dataEncKey, sizeof(session->dataEncKey));
}