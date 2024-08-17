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
#include "utils_buffer.h"
#include "adapter_mem.h"
#include "adapter_os.h"
#include "utils_common.h"
#include "utils_assert.h"
#include "securec.h"
#include "iotc_errcode.h"

struct UtilsBufferCtx {
    uint8_t exCnt;
    uint32_t allocSize;
    uint32_t maxSize;
    IotcSemId *sem;
    UtilsBuffer buffer;
};

UtilsBufferCtx *UtilsBufferCtxNew(uint32_t preAlloc, uint32_t max)
{
    if (preAlloc > UTILS_BUFFER_MAX_ALLOC_SIZE || max > UTILS_BUFFER_MAX_SIZE) {
        IOTC_LOGW("param invalid");
        return NULL;
    }
    UtilsBufferCtx *ctx = IotcMalloc(sizeof(UtilsBufferCtx));
    if (ctx == NULL) {
        IOTC_LOGW("malloc error");
        return NULL;
    }
    (void)memset_s(ctx, sizeof(UtilsBufferCtx), 0, sizeof(UtilsBufferCtx));

    do {
        ctx->allocSize = preAlloc;
        ctx->maxSize = max;
        ctx->buffer.buffer = IotcCalloc(sizeof(uint8_t), preAlloc);
        if (ctx->buffer.buffer == NULL) {
            IOTC_LOGW("calloc error %u", preAlloc);
            break;
        }
        ctx->buffer.size = preAlloc;
        /* 1表示初始资源数量 */
        ctx->sem = IotcSemCreate(1);
        if (ctx->sem == NULL) {
            IOTC_LOGW("create sem error");
            break;
        }
        return ctx;
    } while (0);
    UtilsBufferCtxFree(ctx);
    return NULL;
}

void UtilsBufferCtxFree(UtilsBufferCtx *ctx)
{
    CHECK_V_RETURN(ctx != NULL);
    UTILS_FREE_2_NULL(ctx->buffer.buffer);
    if (ctx->sem != NULL) {
        IotcSemDestroy(ctx->sem);
        ctx->sem = NULL;
    }
    IotcFree(ctx);
}

static int32_t BufferRealloc(UtilsBuffer *buf, int32_t size)
{
    if (buf->size == size && buf->buffer != NULL) {
        return IOTC_OK;
    }
    buf->size = size;
    if (buf->buffer != NULL) {
        IotcFree(buf->buffer);
    }
    buf->buffer = IotcCalloc(sizeof(uint8_t), size);
    if (buf->buffer == NULL) {
        IOTC_LOGW("calloc error %u", size);
        buf->size = 0;
        return IOTC_ADAPTER_MEM_ERR_CALLOC;
    }
    return IOTC_OK;
}

UtilsBuffer *UtilsGetBuffer(UtilsBufferCtx *ctx)
{
    CHECK_RETURN_LOGW(ctx != NULL, NULL, "invalid param");
    int32_t ret = IotcSemWait(ctx->sem, UTILS_BUFFER_MAX_WAIT_TIME_MS);
    if (ret != IOTC_OK) {
        IOTC_LOGW("wait sem error %d", ret);
        return NULL;
    }
    if (ctx->buffer.size == 0 || ctx->buffer.buffer == NULL) {
        ret = BufferRealloc(&ctx->buffer, ctx->allocSize);
        if (ret != IOTC_OK) {
            IOTC_LOGW("buf realloc error %d", ret);
            (void)IotcSemPost(ctx->sem);
            return NULL;
        }
    }
    (void)memset_s(ctx->buffer.buffer, ctx->buffer.size, 0, ctx->buffer.size);
    ctx->buffer.len = 0;
    return &ctx->buffer;
}

void UtilsReleaseBuffer(UtilsBufferCtx *ctx, UtilsBuffer **buf)
{
    CHECK_V_RETURN_LOGW(ctx != NULL && buf != NULL && *buf != NULL && *buf == &ctx->buffer, "invalid param");

    *buf = NULL;
    int32_t ret;
    /* 使用长度大于预分配长度则刷新扩展长度使用次数 */
    if (ctx->buffer.len > ctx->allocSize) {
        ctx->exCnt = UTILS_EACH_EXPEND_CNT_INCREASE;
    } else if (ctx->exCnt > 0) {
        --ctx->exCnt;
        /* 连续多次不使用长buffer后恢复预分配大小 */
        if (ctx->exCnt == 0) {
            ret = BufferRealloc(&ctx->buffer, ctx->allocSize);
            if (ret != IOTC_OK) {
                IOTC_LOGW("realloc buffer error %d", ret);
            }
        }
    }

    ret = IotcSemPost(ctx->sem);
    if (ret != IOTC_OK) {
        IOTC_LOGW("post sem error %d", ret);
        return;
    }
    return;
}

int32_t UtilsExpendBuffer(UtilsBufferCtx *ctx, UtilsBuffer *buf, uint32_t max)
{
    CHECK_RETURN_LOGW(ctx != NULL && buf != NULL && buf->len < buf->size,
        IOTC_ERR_PARAM_INVALID, "invalid param");
    if (max <= buf->size) {
        return IOTC_OK;
    }

    if (max > ctx->maxSize) {
        return IOTC_CORE_COMM_UTILS_ERR_BUFFER_EXPEND_OVERSIZE;
    }
    /* 暂存数据 */
    UtilsBuffer temp = ctx->buffer;
    (void)memset_s(&ctx->buffer, sizeof(UtilsBuffer), 0, sizeof(UtilsBuffer));
    int32_t ret = BufferRealloc(&ctx->buffer, max);
    if (ret != IOTC_OK) {
        ctx->buffer = temp;
        return ret;
    }

    /* 拷贝并释放原始数据 */
    if (temp.buffer != NULL) {
        int32_t ret = memcpy_s(ctx->buffer.buffer, ctx->buffer.size, temp.buffer, temp.len);
        ctx->buffer.len = temp.len;
        UTILS_FREE_2_NULL(temp.buffer);
        if (ret != EOK) {
            IOTC_LOGW("memcpy error %u/%u", ctx->buffer.size, temp.len);
            return IOTC_ERR_SECUREC_MEMCPY;
        }
    }
    ctx->exCnt = UTILS_EACH_EXPEND_CNT_INCREASE;
    return IOTC_OK;
}

int32_t UtilsBufferAlloc(UtilsBuffer *buf, uint32_t size)
{
    if (buf == NULL || buf->buffer != NULL || size == 0 || size > UTILS_BUFFER_MAX_SIZE) {
        IOTC_LOGW("invalid buffer alloc %u", size);
        return IOTC_ERR_PARAM_INVALID;
    }

    buf->buffer = (uint8_t *)IotcCalloc(size, sizeof(uint8_t));
    if (buf->buffer == NULL) {
        IOTC_LOGW("buffer alloc %u error", size);
        return IOTC_ADAPTER_MEM_ERR_CALLOC;
    }

    buf->size = size;
    buf->len = 0;
    return IOTC_OK;
}

void UtilsBufferFree(UtilsBuffer *buf)
{
    if (buf == NULL || buf->buffer == NULL) {
        return;
    }
    IotcFree(buf->buffer);
    (void)memset_s(buf, sizeof(UtilsBuffer), 0, sizeof(UtilsBuffer));
}