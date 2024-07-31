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

#define GET_CTX() (&g_transBufCtx)

void TransSetSendBufferSize(uint32_t res, uint32_t max)
{
    CHECK_V_RETURN_LOGW(res != 0 && max != 0, "param invalid");
    GET_CTX()->sendRes = UTILS_MIN(IOTC_CONF_WIFI_BUFFER_MAX_SIZE, res);
    GET_CTX()->sendMax = UTILS_MIN(IOTC_CONF_WIFI_BUFFER_MAX_SIZE, UTILS_MAX(max, res));
    IOTC_LOGI("set send buf %u/%u", GET_CTX()->sendRes, GET_CTX()->sendMax);
}

void TransSetRecvBufferSize(uint32_t res, uint32_t max)
{
    CHECK_V_RETURN_LOGW(res != 0 && max != 0, "param invalid");
    GET_CTX()->recvRes = UTILS_MIN(IOTC_CONF_WIFI_BUFFER_MAX_SIZE, res);
    GET_CTX()->recvMax = UTILS_MIN(IOTC_CONF_WIFI_BUFFER_MAX_SIZE, UTILS_MAX(max, res));
    IOTC_LOGI("set recv buf %u/%u", GET_CTX()->recvRes, GET_CTX()->recvMax);
}

uint32_t TransGetSendBufferResSize(void)
{
    return GET_CTX()->sendRes;
}

uint32_t TransGetSendBufferMaxSize(void)
{
    return GET_CTX()->sendMax;
}

uint32_t TransGetRecvBufferResSize(void)
{
    return GET_CTX()->recvRes;
}

uint32_t TransGetRecvBufferMaxSize(void)
{
    return GET_CTX()->recvMax;
}

UtilsBufferCtx *TransCreateSendBuffer(void)
{
    return GET_CTX()->sendBuf;
}

UtilsBufferCtx *TransCreateRecvBuffer(void)
{
    return GET_CTX()->recvBuf;
}

/* 保留release接口用于屏蔽buffer内部实现 */
void TransReleaseBuffer(UtilsBufferCtx *ctx)
{
    NOT_USED(ctx);
}

int32_t TransBufferInit(void)
{
    if (GET_CTX()->sendBuf == NULL) {
        GET_CTX()->sendBuf = UtilsBufferCtxNew(GET_CTX()->sendRes, GET_CTX()->sendMax);
        if (GET_CTX()->sendBuf == NULL) {
            IOTC_LOGW("create send buffer error %u/%u", GET_CTX()->sendRes, GET_CTX()->sendMax);
            return IOTC_CORE_COMM_UTILS_ERR_BUFFER_CREATE;
        }
    }

    if (GET_CTX()->recvBuf == NULL) {
        GET_CTX()->recvBuf = UtilsBufferCtxNew(GET_CTX()->recvRes, GET_CTX()->recvMax);
        if (GET_CTX()->sendBuf == NULL) {
            IOTC_LOGW("create recv buffer error %u/%u", GET_CTX()->recvRes, GET_CTX()->recvMax);
            return IOTC_CORE_COMM_UTILS_ERR_BUFFER_CREATE;
        }
    }

    return IOTC_OK;
}

void TransBufferDeinit(void)
{
    if (GET_CTX()->sendBuf != NULL) {
        UtilsBufferCtxFree(GET_CTX()->sendBuf);
        GET_CTX()->sendBuf = NULL;
    }
    if (GET_CTX()->recvBuf != NULL) {
        UtilsBufferCtxFree(GET_CTX()->recvBuf);
        GET_CTX()->recvBuf = NULL;
    }
}