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
#ifndef SLE_LINKLAYER_H
#define SLE_LINKLAYER_H

#include <stdbool.h>
#include "security_speke.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SLE_SVC_LEN      32

typedef enum {
    SLE_ENC_TYPE_UNENCRYPTED    = 0x00,
    SLE_ENC_TYPE_SPEKE          = 0x01,
    SLE_ENC_TYPE_STS            = 0x02,
    SLE_ENC_TYPE_SESSKEY        = 0x03,
    SLE_ENC_TYPE_NULL           = 0xFF,
} SleLinkLayerEncryptType;


typedef enum {
    SLE_CMD_DATA_MODE_TLV    = 0b00,     /* 当前不支持 */
    SLE_CMD_DATA_MODE_JSON   = 0b01,
} SleCmdDataFormat;

typedef enum {
    SLE_OPTYPE_PUT = 0x00,
    SLE_OPTYPE_GET = 0x01,
    SLE_OPTYPE_RPT = 0x02,
} SleOpType;

typedef struct {
    uint32_t connId;
    SleCmdDataFormat dataFormat;
    SleOpType opType;
    char service[SLE_SVC_LEN];
    const uint8_t *request;
    uint32_t requestLen;
} SleCmdParam;


#define SLE_ENC_SUPP_PLAIN      (1 << SLE_ENC_TYPE_UNENCRYPTED)
#define SLE_ENC_SUPP_SPEKE      (1 << SLE_ENC_TYPE_SPEKE)
#define SLE_ENC_SUPP_STS        (1 << SLE_ENC_TYPE_STS)
#define SLE_ENC_SUPP_SESSKEY    (1 << SLE_ENC_TYPE_SESSKEY)
#define SLE_ENC_SUPP_SPEKE_SESSKEY  (SLE_ENC_SUPP_SPEKE | SLE_ENC_SUPP_SESSKEY)
#define SLE_ENC_SUPP_ALL            (SLE_ENC_SUPP_PLAIN | SLE_ENC_SUPP_SPEKE | SLE_ENC_SUPP_STS | SLE_ENC_SUPP_SESSKEY)


typedef struct {
    uint8_t svcIdx;
    char service[SLE_SVC_LEN];
    uint32_t suppEncType;
    int32_t (*putSleFunc)(const SleCmdParam *param, uint8_t **out, uint32_t *outLen);
    int32_t (*getSleFunc)(const SleCmdParam *param, uint8_t **out, uint32_t *outLen);
} SleSvcInfo;


typedef struct {
    int32_t (*sleSessKeyEncrypt)(uint16_t connId, const uint8_t *data, uint32_t dataLen,
        uint8_t *outBuff, uint32_t outBuffLen);
    int32_t (*sleSessKeyDecrypt)(uint16_t connId, const uint8_t *data, uint32_t dataLen,
        uint8_t *outBuff, uint32_t outBuffLen);
    int32_t (*sleSessKeyCalHmac)(uint16_t connId, const uint8_t *data, uint32_t dataLen,
        uint8_t *calHmac, uint32_t calHmacLen);
    int32_t (*sleSessKeyCheckHmac)(uint16_t connId, const uint8_t *data, uint32_t dataLen,
        const uint8_t *hmacBuf, uint32_t hmacLen);
    bool (*sleSessKeyCheckExist)(uint16_t connId);
    int32_t (*sleSessDelNode)(uint16_t connId);
} SleLinkLayerSessKeyCallback;

int32_t SleLinkLayerRegisterSessKeyCb(SleLinkLayerSessKeyCallback cb);

/* 接收与上报相关 */
int32_t SleLinkLayerProcessData(uint32_t connId, const uint8_t *buff, uint32_t len);

int32_t SleLinkLayerReportSvcData(int32_t connId, const char *service, const uint8_t *data, uint32_t len,
    SleOpType opType);

int32_t SleLinkLayerReportSvcDataEnc(int32_t connId, const char *service, const uint8_t *data, uint32_t len,
    SleOpType opType);

/* 接收分包缓存相关 */
void SleLinkLayerClearAllCachePkg(void);

void SleLinkLayerServiceRelease(void);

/* 发送相关 */
typedef int32_t (*LinkLayerSendSleData)(uint32_t connId, const uint8_t *buff, uint32_t len);

int32_t SleLinkLayerRegisterDataSendCb(LinkLayerSendSleData cb);

int32_t SleLinkLayerServiceRegister(const SleSvcInfo *svcInfo, uint32_t svcNum);

/* speke 加密相关 */
typedef SpekeSession *(*SleLinkLayerGetSpekeSession)(uint32_t connid);

int32_t SleLinkLayerRegisterSpekeSessionGetCb(SleLinkLayerGetSpekeSession cb);

int32_t SleLinkLayerSetSendMaxPkg(uint8_t maxPkg);


#ifdef __cplusplus
}
#endif

#endif /* SLE_LINKLAYER_H */