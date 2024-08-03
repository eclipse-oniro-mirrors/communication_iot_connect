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
#ifndef BLE_SESSION_MNGR_H
#define BLE_SESSION_MNGR_H

#include <stdbool.h>
#include <stdint.h>

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
} BleSessKeyInfo;

typedef struct {
    bool negoFinish;
    BleSessKeyInfo sessInfo;
} BleSessParam;

void BleSessRecvSeqInit(uint32_t recvSeq);

bool BleSessRecvSeqCheck(uint32_t recvSeq);

void BleSessRecvSeqUpdate(uint32_t recvSeq);

void BleSessSendSeqUpdate(void);

uint32_t BleSessSendSeqGet(void);

int32_t BleSessKeyGen(const uint8_t *sn1, uint32_t sn1Len, const uint8_t *sn2, uint32_t sn2Len);

int32_t BleSessIdGen(void);

uint8_t *BleSessIdGet(void);

bool BleSessIsExist(void);

int32_t BleSessInit(void);

void BleSessReset(void);

#ifdef __cplusplus
}
#endif

#endif /* BLE_SESSION_MNGR_H */