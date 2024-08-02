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
#include "svc_softap_retrans.h"
#include "utils_assert.h"
#include "coap_codec_def.h"
#include "utils_common.h"
#include "svc_softap_sess.h"

#define SOFTAP_MAX_RETRANS_TIMES 4
#define SOFTAP_RETRANS_LAST_INTERVAL 100
#define SOFTAP_RETRANS_NORMAL_INTERVAL 1000

bool SoftapCoapRetransCheckFunc(const CoapRetransParam *param, const CoapData *raw, void *userData, uint32_t *next)
{
    CHECK_RETURN_LOGW(param != NULL && raw != NULL && userData != NULL && next != NULL, false, "param invalid");

    SoftapSess *sess = (SoftapSess *)userData;
    if (param->cnt >= SOFTAP_MAX_RETRANS_TIMES) {
        return false;
    }

    SoftapPeerSess *peer = SoftapGetPeerSess(&param->addr, sess);
    if (peer != NULL && param->msgId == peer->lastMsgId) {
        *next = SOFTAP_RETRANS_LAST_INTERVAL;
    } else {
        *next = SOFTAP_RETRANS_NORMAL_INTERVAL;
    }

    return true;
}