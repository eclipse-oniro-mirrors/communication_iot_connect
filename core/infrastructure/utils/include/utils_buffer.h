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
#ifndef UTILS_BUFFER_H
#define UTILS_BUFFER_H

#include <stdint.h>
#include <stddef.h>
#include "comm_def.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef CommBuffer UtilsBuffer;

/** 最大预分配内存64k */
#define UTILS_BUFFER_MAX_ALLOC_SIZE (64 * 1024)
/** 最大申请内存为2M+64K */
#define UTILS_BUFFER_MAX_SIZE (2 * 1024 * 1024 + UTILS_BUFFER_MAX_ALLOC_SIZE)

#define UTILS_BUFFER_MAX_WAIT_TIME_MS (60 * 1000)

#define UTILS_EACH_EXPEND_CNT_INCREASE 5

typedef struct UtilsBufferCtx UtilsBufferCtx;

UtilsBufferCtx *UtilsBufferCtxNew(uint32_t preAlloc, uint32_t max);

void UtilsBufferCtxFree(UtilsBufferCtx *ctx);

UtilsBuffer *UtilsGetBuffer(UtilsBufferCtx *ctx);

void UtilsReleaseBuffer(UtilsBufferCtx *ctx, UtilsBuffer **buf);

int32_t UtilsExpendBuffer(UtilsBufferCtx *ctx, UtilsBuffer *buf, uint32_t max);

int32_t UtilsBufferAlloc(UtilsBuffer *buf, uint32_t size);

void UtilsBufferFree(UtilsBuffer *buf);

#ifdef __cplusplus
}
#endif

#endif /* UTILS_BUFFER_H */
