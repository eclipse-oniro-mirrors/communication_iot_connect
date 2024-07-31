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
#ifndef SCHEDULE_EXECUTOR_H
#define SCHEDULE_EXECUTOR_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t (*SchedExecutorWaitCallback)(void *inData, void **outData);

typedef void (*SchedExecutorCallback)(void *userData);

int32_t SchedAsyncExecutor(SchedExecutorCallback cb, void *userData);

/* 不能由SDK内部调用 */
int32_t SchedAsyncExecutorWait(SchedExecutorWaitCallback cb, void *inData, void **outData,
    int32_t *errcode, uint32_t timeout);

int32_t SchedAsyncExecutorInit(void);

void SchedAsyncExecutorDeinit(void);

#ifdef __cplusplus
}
#endif

#endif /* SCHEDULE_EXECUTOR_H */