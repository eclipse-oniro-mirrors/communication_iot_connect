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
#ifndef SERVICE_SOFTAP_RETRANS_H
#define SERVICE_SOFTAP_RETRANS_H

#include "svc_softap_ctx.h"
#include "trans_sess.h"
#include "coap_endpoint_retrans.h"

#ifdef __cplusplus
extern "C" {
#endif

bool SoftapCoapRetransCheckFunc(const CoapRetransParam *param, const CoapData *raw, void *userData, uint32_t *next);

#ifdef __cplusplus
}
#endif

#endif /* SERVICE_SOFTAP_RETRANS_H */
