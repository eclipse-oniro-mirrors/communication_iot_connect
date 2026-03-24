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
#ifndef SLE_SESSION_MNGR_H
#define SLE_SESSION_MNGR_H

#include <stdbool.h>
#include <stdint.h>
#include "utils_common.h"
#include "utils_assert.h"
#include "utils_list.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SESSION_ID_LEN  32
#define RAND_SN_LEN     8
#define SALT_LEN        (RAND_SN_LEN * 2)
#define SESSION_KEY_LEN 16

typedef struct {
    uint32_t maxRecvSeq;
    uint32_t recvBitMap;
    uint32_t nextSendSeq;
    uint8_t sessId[SESSION_ID_LEN];
    uint8_t salt[SALT_LEN];
    uint8_t key[SESSION_KEY_LEN];
} SleSessKeyInfo;

typedef struct {
    bool negoFinish;
    SleSessKeyInfo sessInfo;
} SleSessParam;

typedef struct {
    SleSessParam *sessParam;
    uint16_t connId;
    ListEntry node;
} SleSessionParmNode;

typedef struct {
    const uint8_t *sn1;
    uint32_t sn1Len;
    const uint8_t *sn2;
    uint32_t sn2Len;
    const uint8_t *password;
    uint32_t passwordLen;
    const uint8_t *sessId;
    uint32_t sessIdLen;
} SleSessionKeyGenParam;

void SleSessRecvSeqInit(uint16_t connId, uint32_t recvSeq);

bool SleSessionExistsByConnId(uint16_t connId);

bool SleSessRecvSeqCheck(uint16_t connId, uint32_t recvSeq);

void SleSessRecvSeqUpdate(uint32_t recvSeq);

void SleSessSendSeqUpdate(void);

uint32_t SleSessSendSeqGet(void);

int32_t SleSessionKeyGenerate(uint16_t connId, const SleSessionKeyGenParam *param);

int32_t SleSessKeyGen(uint16_t connId, const uint8_t *sn1, uint32_t sn1Len, const uint8_t *sn2, uint32_t sn2Len);

int32_t SleSessIdGen(void);

uint8_t *SleSessIdGet(void);

bool SleSessIsExist(void);

int32_t SleSessInit(void);

void SleSessReset(void);

#ifdef __cplusplus
}
#endif

#endif /* SLE_SESSION_MNGR_H */