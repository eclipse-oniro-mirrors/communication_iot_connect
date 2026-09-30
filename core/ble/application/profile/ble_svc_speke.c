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
#include <string.h>
#include "ble_svc_speke.h"
#include "ble_speke_session.h"
#include "utils_common.h"
#include "utils_assert.h"
#include "security_speke.h"
#include "utils_json.h"
#include "iotc_errcode.h"
#include "linklayer_process.h"
#include "linklayer_service.h"
#include "iotc_mem.h"
#include "securec.h"

/* 帧前缀 = PKG_HEAD_LEN(7, SendRspData 写包头) + svc 帧头 */
static uint32_t SpekeFramedPrefixLen(const BtCmdParam *param)
{
    return PKG_HEAD_LEN + SVC_TYPE_LEN + SVC_LEN_LEN + (uint32_t)strlen(param->service) + SVC_PAYLOAD_LEN_LEN;
}

/* 错误响应帧化——小报文，一次拷贝进帧可接受 */
static int32_t SpekeFramedErrcodeJson(const BtCmdParam *param, int32_t errCode, uint8_t **out, uint32_t *outLen)
{
    uint8_t *errJson = NULL;
    uint32_t errLen = 0;
    int32_t ret = UtilsGenErrcodeJsonStr(errCode, (char **)&errJson, &errLen);
    if ((ret != IOTC_OK) || (errJson == NULL) || (errLen == 0)) {
        if (errJson != NULL) {
            IotcFree(errJson);
        }
        return (ret != IOTC_OK) ? ret : IOTC_CORE_COMM_UTILS_ERR_JSON_MALLOC_PRINT;
    }

    uint32_t prefixLen = SpekeFramedPrefixLen(param);
    uint8_t *buf = (uint8_t *)IotcCalloc(prefixLen + errLen + 1, sizeof(uint8_t));
    if (buf == NULL) {
        IotcFree(errJson);
        return IOTC_ADAPTER_MEM_ERR_CALLOC;
    }
    uint32_t headerLen = 0;
    ret = LinkLayerWriteSvcHeader(buf + PKG_HEAD_LEN, prefixLen - PKG_HEAD_LEN, param, errLen, &headerLen);
    if (ret != IOTC_OK) {
        IotcFree(buf);
        IotcFree(errJson);
        return ret;
    }
    if (memcpy_s(buf + prefixLen, errLen, errJson, errLen) != EOK) {
        IotcFree(buf);
        IotcFree(errJson);
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    IotcFree(errJson);

    *out = buf;
    *outLen = prefixLen + errLen - PKG_HEAD_LEN;   /* svc 帧长度 */
    return IOTC_OK;
}

int32_t PutBleSvcSpeke(const BtCmdParam *param, uint8_t **out, uint32_t *outLen)
{
    CHECK_RETURN_LOGW((param != NULL) && (param->request != NULL) && (param->requestLen != 0) &&
        (out != NULL) && (outLen != NULL), IOTC_ERR_PARAM_INVALID, "invalid param");

    *out = NULL;
    *outLen = 0;
    int32_t ret = CreateBleSpekeSess();
    if (ret != IOTC_OK) {
        IOTC_LOGE("create ble speke session err ret=%d", ret);
        return ret;
    }

    /* completeBuff(mergeBuff) 为本端可写堆缓冲，允许 epk 视图就地 unhexify */
    SpekePktView pkt = { (const char *)param->request, param->requestLen, true };
    ret = SpekeProcessPacketView(GetBleSpekeSess(), &pkt, out, outLen);
    if (ret != IOTC_OK) {
        IOTC_LOGW("process speke packet %d", ret);
        if (*out != NULL) {
            IotcFree(*out);
            *out = NULL;
            *outLen = 0;
        }
        int32_t spekeErr = GetBleSpekeErrCode();
        return UtilsGenErrcodeJsonStr(spekeErr != IOTC_OK ? spekeErr : ret, (char **)out, outLen);
    }

    return IOTC_OK;
}

/* SPEKE 服务帧化回调——响应直接在帧内构造：
   SERVER_RSP（唯一大报文）经 SpekeProcessPacketPrefix 的 prefixLen 直写帧内偏移，
   零中转拷贝，消 EncodeCmdData 的整帧二次分配（RSP 构造链单帧收尾）；
   其余小报文（CFM/INFORM/错误码）由安全层统一前缀化后在此写 svc 帧头 */
int32_t PutBleSvcSpekeFramed(const BtCmdParam *param, uint8_t **out, uint32_t *outLen)
{
    CHECK_RETURN_LOGW((param != NULL) && (param->request != NULL) && (param->requestLen != 0) &&
        (out != NULL) && (outLen != NULL), IOTC_ERR_PARAM_INVALID, "invalid param");

    *out = NULL;
    *outLen = 0;
    int32_t ret = CreateBleSpekeSess();
    if (ret != IOTC_OK) {
        IOTC_LOGE("create ble speke session err ret=%d", ret);
        return ret;
    }

    uint8_t *msg = NULL;
    uint32_t msgLen = 0;
    /* mergeBuff 可写，epk 就地 unhexify */
    SpekePktView pkt = { (const char *)param->request, param->requestLen, true };
    ret = SpekeProcessPacketPrefix(GetBleSpekeSess(), &pkt, SpekeFramedPrefixLen(param), &msg, &msgLen);
    if (ret != IOTC_OK) {
        IOTC_LOGW("process speke packet %d", ret);
        if (msg != NULL) {
            IotcFree(msg);
        }
        int32_t spekeErr = GetBleSpekeErrCode();
        return SpekeFramedErrcodeJson(param, spekeErr != IOTC_OK ? spekeErr : ret, out, outLen);
    }
    if (msg == NULL) {
        /* 无回复数据，链路层按基线走异常路径 */
        return IOTC_OK;
    }

    /* msg 布局: [0, PKG_HEAD_LEN) 包头预留（SendRspData 写） | [PKG_HEAD_LEN, prefixLen) svc 帧头
       | [prefixLen, msgLen) 报文体 */
    uint32_t prefixLen = SpekeFramedPrefixLen(param);
    uint32_t headerLen = 0;
    ret = LinkLayerWriteSvcHeader(msg + PKG_HEAD_LEN, prefixLen - PKG_HEAD_LEN, param,
        msgLen - prefixLen, &headerLen);
    if (ret != IOTC_OK) {
        IotcFree(msg);
        return ret;
    }

    *out = msg;
    *outLen = msgLen - PKG_HEAD_LEN;   /* svc 帧长度，与 EncodeCmdData 输出约定一致 */
    return IOTC_OK;
}