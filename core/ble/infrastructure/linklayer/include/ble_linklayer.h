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
#ifndef BLE_LINKLAYER_H
#define BLE_LINKLAYER_H

#include <stdbool.h>
#include "security_speke.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BT_SVC_LEN      32

/* 发送相关 */
typedef int32_t (*LinkLayerSendBtData)(const uint8_t *buff, uint32_t len);

int32_t LinkLayerRegisterBtDataSendCb(LinkLayerSendBtData cb);

int32_t LinkLayerSetSendMaxPkg(uint8_t maxPkg);

/* 加密相关 */
/* 若拓展至 0x31 以上, 需同步考虑 BtSvcInfo 中 bitmap 的数据类型扩容 */
typedef enum {
    ENC_TYPE_UNENCRYPTED    = 0x00,
    ENC_TYPE_SPEKE          = 0x01,
    ENC_TYPE_STS            = 0x02,
    ENC_TYPE_SESSKEY        = 0x03,
    ENC_TYPE_NULL           = 0xFF,
} LinkLayerEncryptType;

/* speke 加密相关 */
typedef SpekeSession *(*LinkLayerGetSpekeSession)(void);

int32_t LinkLayerRegisterSpekeSessionGetCb(LinkLayerGetSpekeSession cb);

/* session key 加密相关 */
typedef struct {
    int32_t (*sessKeyEncrypt)(const uint8_t *data, uint32_t dataLen, uint8_t *outBuff, uint32_t outBuffLen);
    int32_t (*sessKeyDecrypt)(const uint8_t *data, uint32_t dataLen, uint8_t *outBuff, uint32_t outBuffLen);
    int32_t (*sessKeyCalHmac)(const uint8_t *data, uint32_t dataLen, uint8_t *calHmac, uint32_t calHmacLen);
    int32_t (*sessKeyCheckHmac)(const uint8_t *data, uint32_t dataLen, const uint8_t *hmacBuf, uint32_t hmacLen);
    bool (*sessKeyCheckExist)(void);
} LinkLayerSessKeyCallback;

int32_t LinkLayerRegisterSessKeyCb(LinkLayerSessKeyCallback cb);

/* 接收与上报相关 */
int32_t LinkLayerProcessBtData(const uint8_t *buff, uint32_t len);

int32_t LinkLayerReportSvcData(const char *service, const uint8_t *data, uint32_t len);

int32_t LinkLayerReportSvcDataEnc(const char *service, const uint8_t *data, uint32_t len);

/* 接收分包缓存相关 */
void LinkLayerClearAllCachePkg(void);

/* 服务相关 */
typedef enum {
    BT_CMD_DATA_MODE_TLV    = 0b00,     /* 当前不支持 */
    BT_CMD_DATA_MODE_JSON   = 0b01,
} BtCmdDataFormat;

typedef enum {
    BT_OPTYPE_PUT = 0x00,
    BT_OPTYPE_GET = 0x01,
    BT_OPTYPE_RPT = 0x02,
} BtOpType;

typedef struct {
    BtCmdDataFormat dataFormat;
    BtOpType opType;
    char service[BT_SVC_LEN];
    const uint8_t *request;
    uint32_t requestLen;
} BtCmdParam;

#define ENC_SUPP_PLAIN      (1 << ENC_TYPE_UNENCRYPTED)
#if defined(IOTC_CONNECT_SPEKE_NOT_SUPPORT) && IOTC_CONNECT_SPEKE_NOT_SUPPORT
#define ENC_SUPP_SPEKE          ENC_SUPP_PLAIN
#define ENC_SUPP_STS            ENC_SUPP_PLAIN
#define ENC_SUPP_SESSKEY        (1 << ENC_TYPE_SESSKEY)
#define ENC_SUPP_SPEKE_SESSKEY  (ENC_SUPP_PLAIN | ENC_SUPP_SESSKEY)
#else
#define ENC_SUPP_SPEKE      (1 << ENC_TYPE_SPEKE)
#define ENC_SUPP_STS        (1 << ENC_TYPE_STS)
#define ENC_SUPP_SESSKEY    (1 << ENC_TYPE_SESSKEY)
#define ENC_SUPP_SPEKE_SESSKEY  (ENC_SUPP_SPEKE | ENC_SUPP_SESSKEY)
#endif
#define ENC_SUPP_ALL            (ENC_SUPP_PLAIN | ENC_SUPP_SPEKE | ENC_SUPP_STS | ENC_SUPP_SESSKEY)

typedef struct {
    uint8_t svcIdx;
    char service[BT_SVC_LEN];
    uint32_t suppEncType;
    int32_t (*putFunc)(const BtCmdParam *param, uint8_t **out, uint32_t *outLen);
    int32_t (*getFunc)(const BtCmdParam *param, uint8_t **out, uint32_t *outLen);
} BtSvcInfo;

int32_t LinkLayerServiceRegister(const BtSvcInfo *svcInfo, uint32_t svcNum);

void LinkLayerServiceRelease(void);

#ifdef __cplusplus
}
#endif

#endif /* BLE_LINKLAYER_H */