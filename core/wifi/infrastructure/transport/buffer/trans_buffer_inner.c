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
#include "trans_buffer_inner.h"
#include "utils_common.h"
#include "iotc_log.h"
#include "iotc_errcode.h"
#include "iotc_conf.h"
#include "utils_assert.h"

/* 全局使用两份buffer分别用来收发数据，sdk退出时释放 */
struct TransBufferContext {
    UtilsBufferCtx *sendBuf;
    UtilsBufferCtx *recvBuf;
    uint32_t sendRes;
    uint32_t sendMax;
    uint32_t recvRes;
    uint32_t recvMax;
} g_transBufCtx = {
    NULL,
    NULL,
    IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_SIZE,
    IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_MAX_SIZE,
    IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_SIZE,
    IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_MAX_SIZE
};

static inline struct TransBufferContext *GetBufferCtx(void)
{
    return &g_transBufCtx;
}

void TransSetSendBufferSize(uint32_t res, uint32_t max)
{
    CHECK_V_RETURN_LOGW(res != 0 && max != 0, "param invalid");
    GetBufferCtx()->sendRes = UTILS_MIN(IOTC_CONF_WIFI_BUFFER_MAX_SIZE, res);
    GetBufferCtx()->sendMax = UTILS_MIN(IOTC_CONF_WIFI_BUFFER_MAX_SIZE, UTILS_MAX(max, res));
    IOTC_LOGI("set send buf %u/%u", GetBufferCtx()->sendRes, GetBufferCtx()->sendMax);
}

void TransSetRecvBufferSize(uint32_t res, uint32_t max)
{
    CHECK_V_RETURN_LOGW(res != 0 && max != 0, "param invalid");
    GetBufferCtx()->recvRes = UTILS_MIN(IOTC_CONF_WIFI_BUFFER_MAX_SIZE, res);
    GetBufferCtx()->recvMax = UTILS_MIN(IOTC_CONF_WIFI_BUFFER_MAX_SIZE, UTILS_MAX(max, res));
    IOTC_LOGI("set recv buf %u/%u", GetBufferCtx()->recvRes, GetBufferCtx()->recvMax);
}

UtilsBufferCtx *TransCreateSendBuffer(void)
{
    return GetBufferCtx()->sendBuf;
}

UtilsBufferCtx *TransCreateRecvBuffer(void)
{
    return GetBufferCtx()->recvBuf;
}

uint32_t TransGetBufferSize(TransBufferType type)
{
    switch (type) {
        case TRANS_BUFFER_SEND_BUFFER_RES_SIZE:
            return GetBufferCtx()->sendRes;
        case TRANS_BUFFER_SEND_BUFFER_MAX_SIZE:
            return GetBufferCtx()->sendMax;
        case TRANS_BUFFER_RECV_BUFFER_RES_SIZE:
            return GetBufferCtx()->recvRes;
        case TRANS_BUFFER_RECV_BUFFER_MAX_SIZE:
            return GetBufferCtx()->recvMax;
        default:
            IOTC_LOGW("invalid type %d", type);
            break;
    }
    return 0;
}

/* 保留release接口用于对外屏蔽buffer内部的单例实现 */
void TransReleaseBuffer(UtilsBufferCtx *ctx)
{
    NOT_USED(ctx);
}

int32_t TransBufferInit(void)
{
    struct TransBufferContext *ctx = GetBufferCtx();
    if (ctx->sendBuf == NULL) {
        ctx->sendBuf = UtilsBufferCtxNew(ctx->sendRes, ctx->sendMax);
        if (ctx->sendBuf == NULL) {
            IOTC_LOGW("create send buffer error %u/%u", ctx->sendRes, ctx->sendMax);
            return IOTC_CORE_COMM_UTILS_ERR_BUFFER_CREATE;
        }
    }

    if (ctx->recvBuf == NULL) {
        ctx->recvBuf = UtilsBufferCtxNew(ctx->recvRes, ctx->recvMax);
        if (ctx->sendBuf == NULL) {
            IOTC_LOGW("create recv buffer error %u/%u", ctx->recvRes, ctx->recvMax);
            UtilsBufferCtxFree(ctx->sendBuf);
            ctx->sendBuf = NULL;
            return IOTC_CORE_COMM_UTILS_ERR_BUFFER_CREATE;
        }
    }

    return IOTC_OK;
}

void TransBufferDeinit(void)
{
    struct TransBufferContext *ctx = GetBufferCtx();
    if (ctx->sendBuf != NULL) {
        UtilsBufferCtxFree(ctx->sendBuf);
        ctx->sendBuf = NULL;
    }
    if (ctx->recvBuf != NULL) {
        UtilsBufferCtxFree(ctx->recvBuf);
        ctx->recvBuf = NULL;
    }
}