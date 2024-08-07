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
#ifndef M2M_CLOUD_TOKEN_H
#define M2M_CLOUD_TOKEN_H
#include <stdint.h>
#include "m2m_cloud_ctx.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char access[CLOUD_ACCESS_TOKEN_STR_LEN + 1];
    char refresh[CLOUD_REFRESH_TOKEN_STR_LEN + 1];
    uint32_t timeout;
} CloudTokenInfo;

int32_t UpdateCloudTokenInfo(M2mCloudContext *ctx, const CloudTokenInfo *tokenInfo);

#ifdef __cplusplus
}
#endif

#endif /* M2M_CLOUD_TOKEN_H */