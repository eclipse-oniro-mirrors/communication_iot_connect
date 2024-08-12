
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
#ifndef LOCAL_CONTROL_COAP_STACK_H
#define LOCAL_CONTROL_COAP_STACK_H
#include <stdint.h>
#include "lan_search_ctx.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t LanSearchCoapStackStart(LanSearchContext *ctx);

void LanSearchCoapStackStop(LanSearchContext *ctx);

void LanSearchCoapStackDestroy(LanSearchContext *ctx);

#ifdef __cplusplus
}
#endif
#endif /* LOCAL_CONTROL_COAP_STACK_H */