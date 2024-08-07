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
#ifndef CLOUD_FSM_BACKOFF_H
#define CLOUD_FSM_BACKOFF_H
#include <stdint.h>
#include <stdbool.h>
#include "m2m_cloud_ctx.h"

#ifdef __cplusplus
extern "C" {
#endif

void M2mCloudBackoffInit(M2mCloudContext *ctx);

bool IsM2mCloudBackoffTime(M2mCloudContext *ctx);

void M2mCloudBackoffUpdate(M2mCloudContext *ctx);

#ifdef __cplusplus
}
#endif

#endif /* CLOUD_FSM_BACKOFF_H */