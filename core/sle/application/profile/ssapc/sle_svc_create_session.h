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
#ifndef SLE_SVC_CREATE_SESSION_H
#define SLE_SVC_CREATE_SESSION_H

#include <stdint.h>
#include "sle_linklayer.h"
#include "utils_list.h"
#include "sle_session_mngr.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t sn1[RAND_SN_LEN];
    const uint8_t *password; // 生态设备authcode
    uint16_t connId;
    ListEntry node;
} SleInitialSaltSn1Part;

/**
 * @brief Create Server-side Session
 *
 * @param connId sle conn ID
 * @param out Output buffer
 * @param outLen Output buffer length
 *
 * @return 0: success, others: fail
 */
int32_t CreateSvcSessionIssue(uint16_t connId, uint8_t **out, uint32_t *outLen);

/**
 * @brief Get Server-side Session
 *
 * @param param Command parameter
 * @param out Output buffer
 * @param outLen Output buffer length
 *
 * @return 0: success, others: fail
 */
int32_t GetSleSvcCreateSession(const SleCmdParam *param, uint8_t **out, uint32_t *outLen);

#ifdef __cplusplus
}
#endif

#endif /* SLE_SVC_CREATE_SESSION_H */