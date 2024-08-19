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
#ifndef BLE_GATT_MGT_H
#define BLE_GATT_MGT_H

#include <stdint.h>
#include "iotc_ble.h"
#include "iotc_ble_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLE_DEV_NAME "IotcBT"
#define BLE_DEFAULT_MAX_CONN_NUM 1
#define BLE_MTU_SIZE 500

typedef struct {
    uint32_t connId;
    uint32_t serverId;
    uint8_t peerAddr[IOTC_ADPT_BLE_ADDR_LEN];
} BlePeerDevInfo;

typedef struct {
    BlePeerDevInfo *peerDevInfo;
    uint8_t connNum;
    uint8_t svcNum;
    IotcAdptBleGattService *svc;
    uint8_t startedSvcNum;
} BleGattMgtApp;

typedef struct {
    const char *svcUuid;
    const char *charUuid;
    uint32_t valueLen;
    uint8_t *value;
} BleIndicateParam;

int32_t BleAddGattSvc(const IotcBleGattProfileSvc *svc);
int32_t BleGattMgtInit(void);
void BleGattMgtDestroy(void);
BleGattMgtApp *GetBleGattMgtApp(void);
bool BleIsPair(void);
void BleSetPair(bool isBlePair);
int32_t BleSendIndicateDataInner(const char *svcUuid, const char *charUuid, const uint8_t *value, uint32_t valueLen);
void PrintBleGattServiceList(IotcAdptBleGattService *svc, uint8_t num);
int32_t IotcBleSendIndicateData(const char *svcUuid, const char *charUuid,
    const uint8_t *value, uint32_t valueLen);
int32_t BleScheduleEventInit(void);
void BleGattDisconnectAll(void);
int32_t SetBleConnectParam(void);
int32_t BleGattReqRead(int32_t connId, int32_t attrHandle, int32_t transId);
int32_t BleGattReqWrite(int32_t connId, int32_t attrHandle, int32_t transId, uint8_t *value, int32_t valueLen);

#ifdef __cplusplus
}
#endif

#endif /* BLE_GATT_MGT_H */
