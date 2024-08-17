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
#include "iotc_ble.h"
#include <ctype.h>
#include "securec.h"
#include "ohos_bt_def.h"
#include "ohos_bt_gap.h"
#include "ohos_bt_gatt.h"
#include "ohos_bt_gatt_server.h"
#include "iotc_mem.h"
#include "iotc_os.h"
#include "iotc_errcode.h"
#include "iotc_log.h"

#define IOTC_APP_UUID "0000fe36-0000-1000-8000-00805f9b34fb"
#define DESCRIPTOR_CONFIGURE_UUID "00002902-0000-1000-8000-00805f9b34fb"
#define STANDARD_128BIT_UUID "0000xxxx-0000-1000-8000-00805f9b34fb"

#define BLE_CB_WAIT_SLEEP_MS 10
#define BLE_CB_WAIT_TIMEOUT 1000
#define UUID_STR_BUF_MAX_LEN 40

#define UUID_INDEX_0 0
#define UUID_INDEX_8 8
#define UUID_INDEX_12 12
#define UUID_INDEX_16 16
#define UUID_INDEX_20 20

#define STANDARD_UUID_REPLACE_INDEX 4

#define INVALID_ADV_ID (-1)

#define RSP_DATA_START_INDEX 2

enum {
    UUID_STR_4_BYTES = 4,
    UUID_STR_8_BYTES = 8,
    UUID_STR_32_BYTES = 32,
};

typedef struct {
    bool has;
    int32_t status;
    int32_t serverId;
    char uuid[UUID_STR_BUF_MAX_LEN];
    uint8_t uuidLen;
} RegGattAppResult;

typedef struct {
    bool has;
    int32_t status;
    int32_t serverId;
    int32_t srvcHandle;
    char uuid[UUID_STR_BUF_MAX_LEN];
    uint8_t uuidLen;
} AddGattSvcResult;

typedef struct {
    bool has;
    int32_t status;
    int32_t serverId;
    int32_t srvcHandle;
    int32_t characteristicHandle;
    char uuid[UUID_STR_BUF_MAX_LEN];
    uint8_t uuidLen;
} AddGattCharResult;

typedef struct {
    bool has;
    int32_t status;
    int32_t serverId;
    int32_t srvcHandle;
    int32_t descriptorHandle;
    char uuid[UUID_STR_BUF_MAX_LEN];
    uint8_t uuidLen;
} AddGattDescResult;

typedef struct {
    bool has;
    int32_t clientId;
    int32_t status;
} AdvStartResult;

typedef struct {
    bool has;
    int32_t clientId;
    int32_t status;
} AdvStopResult;

static RegGattAppResult g_regGattAppResult;
static AddGattSvcResult g_addGattSvcResult;
static AddGattCharResult g_addGattCharResult;
static AddGattDescResult g_addGattDescResult;
static AdvStartResult g_advStartResult;
static AdvStopResult g_advStopResult;

static int32_t g_advId = INVALID_ADV_ID;

static IotcAdptBleGattCallback g_gattEventHandler = NULL;

static IotcAdptBleStatus OhosStatusToAdapterStatus(int32_t status)
{
    return (status == OHOS_BT_STATUS_SUCCESS) ? IOTC_ADPT_BLE_STATUS_SUCCESS : IOTC_ADPT_BLE_STATUS_FAIL;
}

static uint32_t AdapterPemissionToOhosPermission(uint32_t permission)
{
    uint32_t ohosPermission = 0;
    if (permission & IOTC_ADPT_BLE_CHAR_PERM_READ) {
        ohosPermission |= OHOS_GATT_PERMISSION_READ;
    }
    if (permission & IOTC_ADPT_BLE_CHAR_PERM_READ_ENCRYPTED) {
        ohosPermission |= OHOS_GATT_PERMISSION_READ_ENCRYPTED;
    }
    if (permission & IOTC_ADPT_BLE_CHAR_PERM_READ_ENCRYPTED_MITM) {
        ohosPermission |= OHOS_GATT_PERMISSION_READ_ENCRYPTED_MITM;
    }
    if (permission & IOTC_ADPT_BLE_CHAR_PERM_WRITE) {
        ohosPermission |= OHOS_GATT_PERMISSION_WRITE;
    }
    if (permission & IOTC_ADPT_BLE_CHAR_PERM_WRITE_ENCRYPTED) {
        ohosPermission |= OHOS_GATT_PERMISSION_WRITE_ENCRYPTED;
    }
    if (permission & IOTC_ADPT_BLE_CHAR_PERM_WRITE_ENCRYPTED_MITM) {
        ohosPermission |= OHOS_GATT_PERMISSION_WRITE_ENCRYPTED_MITM;
    }
    if (permission & IOTC_ADPT_BLE_CHAR_PERM_WRITE_SIGNED) {
        ohosPermission |= OHOS_GATT_PERMISSION_WRITE_SIGNED;
    }
    if (permission & IOTC_ADPT_BLE_CHAR_PERM_WRITE_SIGNED_MITM) {
        ohosPermission |= OHOS_GATT_PERMISSION_WRITE_SIGNED_MITM;
    }
    return ohosPermission;
}

static uint32_t AdapterPropertyToOhosProperty(uint32_t property)
{
    uint32_t ohosProperty = 0;
    
    if (property & IOTC_ADPT_BLE_CHAR_PROP_BROADCAST) {
        ohosProperty |= OHOS_GATT_CHARACTER_PROPERTY_BIT_BROADCAST;
    }
    if (property & IOTC_ADPT_BLE_CHAR_PROP_READ) {
        ohosProperty |= OHOS_GATT_CHARACTER_PROPERTY_BIT_READ;
    }
    if (property & IOTC_ADPT_BLE_CHAR_PROP_WRITE_WITHOUT_RESP) {
        ohosProperty |= OHOS_GATT_CHARACTER_PROPERTY_BIT_WRITE_NO_RSP;
    }
    if (property & IOTC_ADPT_BLE_CHAR_PROP_WRITE) {
        ohosProperty |= OHOS_GATT_CHARACTER_PROPERTY_BIT_WRITE;
    }
    if (property & IOTC_ADPT_BLE_CHAR_PROP_NOTIFY) {
        ohosProperty |= OHOS_GATT_CHARACTER_PROPERTY_BIT_NOTIFY;
    }
    if (property & IOTC_ADPT_BLE_CHAR_PROP_INDICATE) {
        ohosProperty |= OHOS_GATT_CHARACTER_PROPERTY_BIT_INDICATE;
    }
    if (property & IOTC_ADPT_BLE_CHAR_PROP_SIGNED_WRITE) {
        ohosProperty |= OHOS_GATT_CHARACTER_PROPERTY_BIT_SIGNED_WRITE;
    }
    if (property & IOTC_ADPT_BLE_CHAR_PROP_EXTENDED_PROPERTY) {
        ohosProperty |= OHOS_GATT_CHARACTER_PROPERTY_BIT_EXTENDED_PROPERTY;
    }
    return ohosProperty;
}

static BleAdvType AdapterAdvTypeToOhosAdvType(IotcAdptBleAdvType type)
{
    if (type == IOTC_ADPT_BLE_ADV_TYPE_IND) {
        return OHOS_BLE_ADV_IND;
    } else if (type == IOTC_ADPT_BLE_ADV_TYPE_DIRECT_IND_HIGH) {
        return OHOS_BLE_ADV_DIRECT_IND_HIGH;
    } else if (type == IOTC_ADPT_BLE_ADV_TYPE_SCAN_IND) {
        return OHOS_BLE_ADV_SCAN_IND;
    } else if (type == IOTC_ADPT_BLE_ADV_TYPE_NONCONN_IND) {
        return OHOS_BLE_ADV_NONCONN_IND;
    } else if (type == IOTC_ADPT_BLE_ADV_TYPE_DIRECT_IND_LOW) {
        return OHOS_BLE_ADV_DIRECT_IND_LOW;
    }
    IOTC_LOGW("type:%d", type);
    return OHOS_BLE_ADV_IND;
}

static BleScanResultAddrType AdapterAddrTypeToOhosAddrType(IotcAdptBleAdvAddr type)
{
    if (type == IOTC_ADPT_BLE_ADV_ADDR_PUBLIC) {
        return OHOS_BLE_PUBLIC_DEVICE_ADDRESS;
    } else if (type == IOTC_ADPT_BLE_ADV_ADDR_RANDOM) {
        return OHOS_BLE_RANDOM_DEVICE_ADDRESS;
    } else if (type == IOTC_ADPT_BLE_ADV_ADDR_PUBLIC_ID) {
        return OHOS_BLE_PUBLIC_IDENTITY_ADDRESS;
    } else if (type == IOTC_ADPT_BLE_ADV_ADDR_RANDOM_ID) {
        return OHOS_BLE_RANDOM_STATIC_IDENTITY_ADDRESS;
    } else if (type == IOTC_ADPT_BLE_ADV_ADDR_UNKNOWN_TYPE) {
        return OHOS_BLE_NO_ADDRESS;
    }
    IOTC_LOGW("type:%d", type);
    return OHOS_BLE_PUBLIC_DEVICE_ADDRESS;
}

static void AdvStartCompleteCb(int32_t clientId, int32_t status)
{
    IOTC_LOGD("adv start complete cb:clientId:%d,status:%d", clientId, status);
    g_advStartResult.clientId = clientId;
    g_advStartResult.status = status;
    g_advStartResult.has = true;
    IotcAdptBleGattEventParam eventParam;
    eventParam.startAdv.status = OhosStatusToAdapterStatus(status);
    if (g_gattEventHandler != NULL &&
        g_gattEventHandler(IOTC_ADPT_BLE_GATT_EVENT_START_ADV_RESULT, &eventParam) != IOTC_OK) {
        IOTC_LOGE("gatt adv start event");
    }
}

static void AdvStopCompleteCb(int32_t clientId, int32_t status)
{
    IOTC_LOGD("adv stop complete cb:clientId:%d,status:%d", clientId, status);
    g_advStopResult.clientId = clientId;
    g_advStopResult.status = status;
    g_advStopResult.has = true;
    IotcAdptBleGattEventParam eventParam;
    eventParam.stopAdv.status = OhosStatusToAdapterStatus(status);
    if (g_gattEventHandler != NULL &&
        g_gattEventHandler(IOTC_ADPT_BLE_GATT_EVENT_STOP_ADV_RESULT, &eventParam) != IOTC_OK) {
        IOTC_LOGE("gap adv stop event");
    }
}

static void SecurityRespondCb(const BdAddr *bdAddr)
{
    IOTC_LOGD("security respond cb");
    int32_t ret = BleGattSecurityRsp(*bdAddr, true);
    if (ret != OHOS_BT_STATUS_SUCCESS) {
        IOTC_LOGE("gap security rsp ret=%d", ret);
    }
}

static BtGattCallbacks g_bleGattCb = {
    .advEnableCb = AdvStartCompleteCb,
    .advDisableCb = AdvStopCompleteCb,
    .securityRespondCb = SecurityRespondCb,
};


static void ConnectServerCb(int32_t connId, int32_t serverId, const BdAddr *bdAddr)
{
    IOTC_LOGD("connect server cb:connId:%d,serverId:%d", connId, serverId);
    if (bdAddr == NULL) {
        IOTC_LOGE("bdAddr null");
        return;
    }
    IotcAdptBleGattEventParam eventParam;
    eventParam.connSvc.connId = connId;
    eventParam.connSvc.serverId = serverId;
    if (memcpy_s(eventParam.connSvc.devAddr, IOTC_ADPT_BLE_ADDR_LEN, bdAddr, OHOS_BD_ADDR_LEN) != EOK) {
        IOTC_LOGE("memcpy_s");
        return;
    }
    if (g_gattEventHandler != NULL &&
        g_gattEventHandler(IOTC_ADPT_BLE_GATT_EVENT_CONNECT, &eventParam) != IOTC_OK) {
        IOTC_LOGE("doing gatt event");
    }
    g_advId = INVALID_ADV_ID; /* 蓝牙连接后系统会自动关闭蓝牙广播 */
}

static void DisconnectServerCb(int32_t connId, int32_t serverId, const BdAddr *bdAddr)
{
    IOTC_LOGD("disconnect server cb:connId:%d,serverId:%d", connId, serverId);
    if (bdAddr == NULL) {
        IOTC_LOGE("bdAddr null");
        return;
    }
    IotcAdptBleGattEventParam eventParam;
    eventParam.disconnSvc.connId = connId;
    eventParam.disconnSvc.serverId = serverId;
    eventParam.disconnSvc.reason = IOTC_ADPT_BLE_GATT_UNKNOWN_REASON;
    if (memcpy_s(eventParam.disconnSvc.devAddr, IOTC_ADPT_BLE_ADDR_LEN, bdAddr, OHOS_BD_ADDR_LEN) != EOK) {
        IOTC_LOGE("memcpy_s");
        return;
    }
    if (g_gattEventHandler != NULL &&
        g_gattEventHandler(IOTC_ADPT_BLE_GATT_EVENT_DISCONNECT, &eventParam) != IOTC_OK) {
        IOTC_LOGE("doing gatt event");
    }
}

static void ServiceStartCb(int32_t status, int32_t serverId, int32_t svcHandle)
{
    IOTC_LOGD("service start cb:status:%d,serverId:%d,svcHandle=%d", status, serverId, svcHandle);
    IotcAdptBleGattEventParam eventParam;
    eventParam.startSvc.status = OhosStatusToAdapterStatus(status);
    eventParam.startSvc.serverId = serverId;
    eventParam.startSvc.svcHandle = svcHandle;
    if (g_gattEventHandler != NULL &&
        g_gattEventHandler(IOTC_ADPT_BLE_GATT_EVENT_START_SVC_RESULT, &eventParam) != IOTC_OK) {
        IOTC_LOGE("doing gatt event");
    }
}

static void ServiceStopCb(int32_t status, int32_t serverId, int32_t svcHandle)
{
    IOTC_LOGD("service stop cb:status:%d,serverId:%d,svcHandle=%d", status, serverId, svcHandle);
    IotcAdptBleGattEventParam eventParam;
    eventParam.stopSvc.status = OhosStatusToAdapterStatus(status);
    eventParam.stopSvc.serverId = serverId;
    eventParam.stopSvc.svcHandle = (uint32_t)svcHandle;
    if (g_gattEventHandler != NULL &&
        g_gattEventHandler(IOTC_ADPT_BLE_GATT_EVENT_STOP_SVC_RESULT, &eventParam) != IOTC_OK) {
        IOTC_LOGE("doing gatt event");
    }
}

static void IndicationSendCb(int32_t connId, int32_t status)
{
    IOTC_LOGD("indication send cb:status:%d,connId:%d", status, connId);
    IotcAdptBleGattEventParam eventParam;
    eventParam.indicateConf.status = OhosStatusToAdapterStatus(status);
    eventParam.indicateConf.connId = connId;
    if (g_gattEventHandler != NULL &&
        g_gattEventHandler(IOTC_ADPT_BLE_GATT_EVENT_INDICATE_CONF, &eventParam) != IOTC_OK) {
        IOTC_LOGE("doing gatt event");
    }
}

static void MtuChangeCb(int32_t connId, int32_t mtu)
{
    IOTC_LOGD("mtu event:connId:%d,mtu:%d", connId, mtu);
    IotcAdptBleGattEventParam eventParam;
    eventParam.setMtu.status = IOTC_ADPT_BLE_STATUS_SUCCESS;
    eventParam.setMtu.connId = connId;
    eventParam.setMtu.mtu = mtu;
    if (g_gattEventHandler != NULL &&
        g_gattEventHandler(IOTC_ADPT_BLE_GATT_EVENT_SET_MTU_RESULT, &eventParam) != IOTC_OK) {
        IOTC_LOGE("doing gatt event");
    }
}

void RegisterServerCb(int32_t status, int32_t serverId, BtUuid *appUuid)
{
    IOTC_LOGD("register server cb:status=%d, serverId=%d", status, serverId);
    g_regGattAppResult.status = status;
    g_regGattAppResult.serverId = serverId;
    if (memcpy_s(g_regGattAppResult.uuid, sizeof(g_regGattAppResult.uuid),
        appUuid->uuid, appUuid->uuidLen) != EOK) {
        IOTC_LOGE("memcpy_s err len1=%d, len2=%d", sizeof(g_regGattAppResult.uuid), appUuid->uuidLen);
        return;
    }
    g_regGattAppResult.uuidLen = appUuid->uuidLen;
    g_regGattAppResult.has = true;
}

void ServiceAddCb(int32_t status, int32_t serverId, BtUuid *uuid, int32_t srvcHandle)
{
    IOTC_LOGD("service add cb:status=%d, serverId=%d, srvcHandle=%d", status, serverId, srvcHandle);
    g_addGattSvcResult.status = status;
    g_addGattSvcResult.serverId = serverId;
    g_addGattSvcResult.srvcHandle = srvcHandle;
    if (memcpy_s(g_addGattSvcResult.uuid, sizeof(g_addGattSvcResult.uuid),
        uuid->uuid, uuid->uuidLen) != EOK) {
        IOTC_LOGE("memcpy_s err len1=%d, len2=%d", sizeof(g_addGattSvcResult.uuid), uuid->uuidLen);
        return;
    }
    g_addGattSvcResult.uuidLen = uuid->uuidLen;
    g_addGattSvcResult.has = true;
}

void CharacteristicAddCb(int32_t status, int32_t serverId, BtUuid *uuid,
    int32_t srvcHandle, int32_t characteristicHandle)
{
    IOTC_LOGD("characteristic add cb:status=%d, serverId=%d, srvcHandle=%d, characteristicHandle=%d",
        status, serverId, srvcHandle, characteristicHandle);
    g_addGattCharResult.status = status;
    g_addGattCharResult.serverId = serverId;
    g_addGattCharResult.srvcHandle = srvcHandle;
    g_addGattCharResult.characteristicHandle = characteristicHandle;
    if (memcpy_s(g_addGattCharResult.uuid, sizeof(g_addGattCharResult.uuid),
        uuid->uuid, uuid->uuidLen) != EOK) {
        IOTC_LOGE("memcpy_s err len1=%d, len2=%d", sizeof(g_addGattCharResult.uuid), uuid->uuidLen);
        return;
    }
    g_addGattCharResult.uuidLen = uuid->uuidLen;
    g_addGattCharResult.has = true;
}

void DescriptorAddCb(int32_t status, int32_t serverId, BtUuid *uuid, int32_t srvcHandle, int32_t descriptorHandle)
{
    IOTC_LOGD("descriptor add cb:status=%d, serverId=%d, srvcHandle=%d, descriptorHandle=%d",
        status, serverId, srvcHandle, descriptorHandle);
    g_addGattDescResult.status = status;
    g_addGattDescResult.serverId = serverId;
    g_addGattDescResult.srvcHandle = srvcHandle;
    g_addGattDescResult.descriptorHandle = descriptorHandle;
    if (memcpy_s(g_addGattDescResult.uuid, sizeof(g_addGattDescResult.uuid),
        uuid->uuid, uuid->uuidLen) != EOK) {
        IOTC_LOGE("memcpy_s err len1=%d, len2=%d", sizeof(g_addGattDescResult.uuid), uuid->uuidLen);
        return;
    }
    g_addGattDescResult.uuidLen = uuid->uuidLen;
    g_addGattDescResult.has = true;
}

static void RequestReadCb(BtReqReadCbPara readCbPara)
{
    IOTC_LOGD("request read cb:connId=%d, transId=%d, attrHandle=%d, offset=%d, isLong=%d",
        readCbPara.connId, readCbPara.transId, readCbPara.attrHandle, readCbPara.offset, readCbPara.isLong);
    IotcAdptBleGattEventParam eventParam;
    eventParam.reqRead.connId =readCbPara.connId;
    eventParam.reqRead.attrHandle = readCbPara.attrHandle;
    eventParam.reqRead.transId = readCbPara.transId;
    if (g_gattEventHandler != NULL &&
        g_gattEventHandler(IOTC_ADPT_BLE_GATT_EVENT_REQ_READ, &eventParam) != IOTC_OK) {
        IOTC_LOGE("doing gatt event");
    }
}

static void RequestWriteCb(BtReqWriteCbPara writeCbPara)
{
    IOTC_LOGD("request write cb:connId=%d, transId=%d, attrHandle=%d, offset=%d, length=%d, needRsp=%d, isPrep=%d",
        writeCbPara.connId, writeCbPara.transId, writeCbPara.attrHandle, writeCbPara.offset,
        writeCbPara.length, writeCbPara.needRsp, writeCbPara.isPrep);
    IotcAdptBleGattEventParam eventParam;
    eventParam.reqWrite.connId =writeCbPara.connId;
    eventParam.reqWrite.attrHandle = writeCbPara.attrHandle;
    eventParam.reqWrite.transId = writeCbPara.transId;
    eventParam.reqWrite.value = IotcMalloc(writeCbPara.length);
    if (eventParam.reqWrite.value == NULL) {
        IOTC_LOGE("malloc err len=%d", writeCbPara.length);
        return;
    }
    if (memcpy_s(eventParam.reqWrite.value, writeCbPara.length, writeCbPara.value, writeCbPara.length) != EOK) {
        IOTC_LOGE("memcpy_s err len=%d", writeCbPara.length);
        IotcFree(eventParam.reqWrite.value);
        eventParam.reqWrite.value = NULL;
        return;
    }
    eventParam.reqWrite.valueLen = writeCbPara.length;
    if (g_gattEventHandler != NULL &&
        g_gattEventHandler(IOTC_ADPT_BLE_GATT_EVENT_REQ_WRITE, &eventParam) != IOTC_OK) {
        IOTC_LOGE("doing gatt event");
        IotcFree(eventParam.reqWrite.value);
        eventParam.reqWrite.value = NULL;
        return;
    }
}

static BtGattServerCallbacks g_bleGattsCb = {
    .registerServerCb = RegisterServerCb,
    .serviceAddCb = ServiceAddCb,
    .characteristicAddCb = CharacteristicAddCb,
    .descriptorAddCb = DescriptorAddCb,
    .connectServerCb = ConnectServerCb,
    .disconnectServerCb = DisconnectServerCb,
    .serviceStartCb = ServiceStartCb,
    .serviceStopCb = ServiceStopCb,
    .indicationSentCb = IndicationSendCb,
    .mtuChangeCb = MtuChangeCb,
    .requestReadCb = RequestReadCb,
    .requestWriteCb = RequestWriteCb,
};

static int32_t WaitStartAdvResult(int32_t advId)
{
    uint32_t waitMs = 0;
    while (waitMs < BLE_CB_WAIT_TIMEOUT) {
        if (g_advStartResult.has) {
            if (g_advStartResult.clientId == advId) {
                return (g_advStartResult.status == OHOS_BT_STATUS_SUCCESS) ? IOTC_OK : IOTC_ERROR;
            }
        }
        IotcSleepMs(BLE_CB_WAIT_SLEEP_MS);
        waitMs += BLE_CB_WAIT_SLEEP_MS;
    }
    return IOTC_ERROR;
}

static int32_t StartAdvEx(int32_t *advId, const StartAdvRawData rawData, BleAdvParams advParam)
{
    (void)memset_s(&g_advStartResult, sizeof(g_advStartResult), 0, sizeof(g_advStartResult));
    int32_t ret = BleStartAdvEx(advId, rawData, advParam);
    if (ret != OHOS_BT_STATUS_SUCCESS) {
        IOTC_LOGE("start adv ret=%d", ret);
        return IOTC_ERROR;
    }
    ret = WaitStartAdvResult(*advId);
    if (ret != IOTC_OK) {
        IOTC_LOGE("wait start adv err ret=%d,advId=%d", ret, *advId);
        return ret;
    }

    return IOTC_OK;
}

static int32_t WaitStopAdvResult(int32_t advId)
{
    uint32_t waitMs = 0;
    while (waitMs < BLE_CB_WAIT_TIMEOUT) {
        if (g_advStopResult.has) {
            if (g_advStopResult.clientId == advId) {
                return (g_advStopResult.status == OHOS_BT_STATUS_SUCCESS) ? IOTC_OK : IOTC_ERROR;
            }
        }
        IotcSleepMs(BLE_CB_WAIT_SLEEP_MS);
        waitMs += BLE_CB_WAIT_SLEEP_MS;
    }
    return IOTC_ERROR;
}

static int32_t StopAdv(int32_t advId)
{
    (void)memset_s(&g_advStopResult, sizeof(g_advStopResult), 0, sizeof(g_advStopResult));
    int32_t ret = BleStopAdv(advId);
    if (ret != OHOS_BT_STATUS_SUCCESS) {
        IOTC_LOGE("stop adv ret=%d", ret);
        return IOTC_ERROR;
    }
    ret = WaitStopAdvResult(advId);
    if (ret != IOTC_OK) {
        IOTC_LOGE("wait stop adv err err ret=%d,advId=%d", ret, advId);
        return ret;
    }
    return IOTC_OK;
}

static void ConvertLower(char *str)
{
    char *p = str;
    while (*p != '\0') {
        if (isupper(*p)) {
            *p = tolower(*p);
        }
        p++;
    }
}

static char *ConvertUuid(const char *uuid)
{
    static char out[UUID_STR_BUF_MAX_LEN];
    int32_t uuidLen = strlen(uuid);
    if (uuidLen == UUID_STR_4_BYTES) {
        if (strcpy_s(out, sizeof(out), STANDARD_128BIT_UUID) != EOK) {
            IOTC_LOGW("strcpy_s");
            return NULL;
        }
        if (memcpy_s(&out[STANDARD_UUID_REPLACE_INDEX], sizeof(out) - STANDARD_UUID_REPLACE_INDEX,
            uuid, UUID_STR_4_BYTES) != EOK) {
            IOTC_LOGW("memcpy_s");
            return NULL;
        }
    } else if (uuidLen == UUID_STR_32_BYTES) {
        if (sprintf_s(out, sizeof(out), "%.8s-%.4s-%.4s-%.4s-%s", &uuid[UUID_INDEX_0], &uuid[UUID_INDEX_8],
            &uuid[UUID_INDEX_12], &uuid[UUID_INDEX_16], &uuid[UUID_INDEX_20]) <= 0) {
            IOTC_LOGW("sprintf_s");
            return NULL;
        }
    } else {
        IOTC_LOGW("unsupport uuidLen=%d", uuidLen);
        return NULL;
    }
    ConvertLower(out);
    IOTC_LOGD("out uuid=%s", out);
    return out;
}

static int32_t RegGattApp(void)
{
    BtUuid appUuid;
    appUuid.uuid = IOTC_APP_UUID;
    appUuid.uuidLen = strlen(IOTC_APP_UUID);
    (void)memset_s(&g_regGattAppResult, sizeof(g_regGattAppResult), 0, sizeof(g_regGattAppResult));
    int32_t ret = BleGattsRegister(appUuid);
    if (ret != OHOS_BT_STATUS_SUCCESS) {
        IOTC_LOGE("reg svc err ret=%d", ret);
        return IOTC_ERROR;
    }

    return IOTC_OK;
}

static int32_t WaitRegGattAppResult(void)
{
    uint32_t waitMs = 0;
    while (waitMs < BLE_CB_WAIT_TIMEOUT) {
        if (g_regGattAppResult.has) {
            if (strcmp(IOTC_APP_UUID, g_regGattAppResult.uuid) == 0) {
                return (g_regGattAppResult.status == OHOS_BT_STATUS_SUCCESS) ? IOTC_OK : IOTC_ERROR;
            }
        }
        IotcSleepMs(BLE_CB_WAIT_SLEEP_MS);
        waitMs += BLE_CB_WAIT_SLEEP_MS;
    }
    return IOTC_ERROR;
}

static int32_t AddGattService(IotcAdptBleGattService *svc)
{
    BtUuid srvcUuid;
    srvcUuid.uuid = ConvertUuid(svc->uuid);
    if (srvcUuid.uuid == NULL) {
        IOTC_LOGE("convert uuid err");
        return IOTC_ERROR;
    }
    srvcUuid.uuidLen = strlen(srvcUuid.uuid);
    (void)memset_s(&g_addGattSvcResult, sizeof(g_addGattSvcResult), 0, sizeof(g_addGattSvcResult));
    int32_t ret = BleGattsAddService(g_regGattAppResult.serverId, srvcUuid, true, svc->charNum);
    if (ret != OHOS_BT_STATUS_SUCCESS) {
        IOTC_LOGE("add svc err ret = %d", ret);
        return IOTC_ERROR;
    }

    return IOTC_OK;
}

static int32_t WaitAddGattServiceResult(IotcAdptBleGattService *svc)
{
    char *uuid = ConvertUuid(svc->uuid);
    if (uuid == NULL) {
        IOTC_LOGE("convert uuid err");
        return IOTC_ERROR;
    }
    uint32_t waitMs = 0;
    while (waitMs < BLE_CB_WAIT_TIMEOUT) {
        if (g_addGattSvcResult.has) {
            if (strcmp(uuid, g_addGattSvcResult.uuid) == 0) {
                svc->svcHandle = g_addGattSvcResult.srvcHandle;
                svc->serverId = g_addGattSvcResult.serverId;
                return (g_addGattSvcResult.status == OHOS_BT_STATUS_SUCCESS) ? IOTC_OK : IOTC_ERROR;
            }
        }
        IotcSleepMs(BLE_CB_WAIT_SLEEP_MS);
        waitMs += BLE_CB_WAIT_SLEEP_MS;
    }
    return IOTC_ERROR;
}

static int32_t AddGattChar(IotcAdptBleGattService *svc, IotcAdptBleGattsChar *charcter)
{
    BtUuid characUuid;
    characUuid.uuid =  ConvertUuid(charcter->uuid);
    if (characUuid.uuid == NULL) {
        IOTC_LOGE("convert uuid err");
        return IOTC_ERROR;
    }
    characUuid.uuidLen = strlen(characUuid.uuid);
    (void)memset_s(&g_addGattCharResult, sizeof(g_addGattCharResult), 0, sizeof(g_addGattCharResult));
    int32_t ret = BleGattsAddCharacteristic(svc->serverId, svc->svcHandle, characUuid,
        AdapterPropertyToOhosProperty(charcter->property),
        AdapterPemissionToOhosPermission(charcter->permission) |
        OHOS_GATT_PERMISSION_READ | OHOS_GATT_PERMISSION_WRITE);
    if (ret != OHOS_BT_STATUS_SUCCESS) {
        IOTC_LOGE("add char err ret = %d", ret);
        return IOTC_ERROR;
    }

    return IOTC_OK;
}

static int32_t WaitAddGattCharResult(IotcAdptBleGattService *svc, IotcAdptBleGattsChar *charcter)
{
    char *uuid = ConvertUuid(charcter->uuid);
    if (uuid == NULL) {
        IOTC_LOGE("convert uuid err");
        return IOTC_ERROR;
    }
    uint32_t waitMs = 0;
    while (waitMs < BLE_CB_WAIT_TIMEOUT) {
        if (g_addGattCharResult.has) {
            if (strcmp(uuid, g_addGattCharResult.uuid) == 0) {
                charcter->charHandle = g_addGattCharResult.characteristicHandle;
                return (g_addGattCharResult.status == OHOS_BT_STATUS_SUCCESS) ? IOTC_OK : IOTC_ERROR;
            }
        }
        IotcSleepMs(BLE_CB_WAIT_SLEEP_MS);
        waitMs += BLE_CB_WAIT_SLEEP_MS;
    }
    return IOTC_ERROR;
}

static int32_t AddGattDesc(IotcAdptBleGattService *svc, IotcAdptBleGattCharDesc *desc)
{
    BtUuid descUuid;
    descUuid.uuid = ConvertUuid(desc->uuid);
    if (descUuid.uuid == NULL) {
        IOTC_LOGE("convert uuid err");
        return IOTC_ERROR;
    }
    descUuid.uuidLen = strlen(descUuid.uuid);
    (void)memset_s(&g_addGattDescResult, sizeof(g_addGattDescResult), 0, sizeof(g_addGattDescResult));
    int32_t ret = BleGattsAddDescriptor(svc->serverId, svc->svcHandle, descUuid,
        AdapterPemissionToOhosPermission(desc->permission));
    if (ret != OHOS_BT_STATUS_SUCCESS) {
        IOTC_LOGE("add desc err ret = %d", ret);
        return IOTC_ERROR;
    }

    return IOTC_OK;
}

static int32_t WaitAddGattDescResult(IotcAdptBleGattService *svc, IotcAdptBleGattCharDesc *desc)
{
    char *uuid = ConvertUuid(desc->uuid);
    if (uuid == NULL) {
        IOTC_LOGE("convert uuid err");
        return IOTC_ERROR;
    }
    uint32_t waitMs = 0;
    while (waitMs < BLE_CB_WAIT_TIMEOUT) {
        if (g_addGattDescResult.has) {
            if (strcmp(uuid, g_addGattDescResult.uuid) == 0) {
                desc->descHandle = g_addGattDescResult.descriptorHandle;
                return (g_addGattDescResult.status == OHOS_BT_STATUS_SUCCESS) ? IOTC_OK : IOTC_ERROR;
            }
        }
        IotcSleepMs(BLE_CB_WAIT_SLEEP_MS);
        waitMs += BLE_CB_WAIT_SLEEP_MS;
    }
    return IOTC_ERROR;
}

static int32_t AddGattConfigDesc(IotcAdptBleGattService *svc)
{
    BtUuid descUuid;
    descUuid.uuid = DESCRIPTOR_CONFIGURE_UUID;
    descUuid.uuidLen = strlen(DESCRIPTOR_CONFIGURE_UUID);
    (void)memset_s(&g_addGattDescResult, sizeof(g_addGattDescResult), 0, sizeof(g_addGattDescResult));
    int32_t ret = BleGattsAddDescriptor(svc->serverId, svc->svcHandle, descUuid,
        OHOS_GATT_PERMISSION_READ | OHOS_GATT_PERMISSION_WRITE);
    if (ret != OHOS_BT_STATUS_SUCCESS) {
        IOTC_LOGE("add desc err ret = %d", ret);
        return IOTC_ERROR;
    }

    return IOTC_OK;
}

static int32_t WaitAddGattConfigDescResult(IotcAdptBleGattService *svc)
{
    uint32_t waitMs = 0;
    while (waitMs < BLE_CB_WAIT_TIMEOUT) {
        if (g_addGattDescResult.has) {
            if (strcmp(DESCRIPTOR_CONFIGURE_UUID, g_addGattDescResult.uuid) == 0) {
                return (g_addGattDescResult.status == OHOS_BT_STATUS_SUCCESS) ? IOTC_OK : IOTC_ERROR;
            }
        }
        IotcSleepMs(BLE_CB_WAIT_SLEEP_MS);
        waitMs += BLE_CB_WAIT_SLEEP_MS;
    }
    return IOTC_ERROR;
}

static int32_t AddGattDescs(IotcAdptBleGattService *svc, IotcAdptBleGattsChar *character)
{
    int32_t ret = 0;
    if ((character->property & IOTC_ADPT_BLE_CHAR_PROP_NOTIFY) ||
        (character->property & IOTC_ADPT_BLE_CHAR_PROP_INDICATE)) {
        ret = AddGattConfigDesc(svc);
        if (ret != IOTC_OK) {
            IOTC_LOGE("add gatt desc err ret=%d", ret);
            return ret;
        }
        ret = WaitAddGattConfigDescResult(svc);
        if (ret != IOTC_OK) {
            IOTC_LOGE("wait add gatt desc err ret=%d", ret);
            return ret;
        }
    }
    for (uint8_t i = 0; i < character->descNum; i++) {
        ret = AddGattDesc(svc, &character->desc[i]);
        if (ret != IOTC_OK) {
            IOTC_LOGE("add gatt desc err ret=%d", ret);
            return ret;
        }
        ret = WaitAddGattDescResult(svc, &character->desc[i]);
        if (ret != IOTC_OK) {
            IOTC_LOGE("wait add gatt desc err ret=%d", ret);
            return ret;
        }
    }
    return IOTC_OK;
}

static int32_t AddGattChars(IotcAdptBleGattService *svc)
{
    int32_t ret = 0;
    for (uint8_t i = 0; i < svc->charNum; i++) {
        ret = AddGattChar(svc, &svc->character[i]);
        if (ret != IOTC_OK) {
            IOTC_LOGE("add gatt char err ret=%d", ret);
            return ret;
        }
        ret = WaitAddGattCharResult(svc, &svc->character[i]);
        if (ret != IOTC_OK) {
            IOTC_LOGE("wait add gatt char err ret=%d", ret);
            return ret;
        }
        ret = AddGattDescs(svc, &svc->character[i]);
        if (ret != IOTC_OK) {
            IOTC_LOGE("add gatt desc err ret=%d", ret);
            return ret;
        }
    }
    return IOTC_OK;
}

static int32_t StartRegGattApp(void)
{
    int32_t ret = RegGattApp();
    if (ret != IOTC_OK) {
        IOTC_LOGE("reg gatt app err ret=%d", ret);
        return ret;
    }
    ret = WaitRegGattAppResult();
    if (ret != IOTC_OK) {
        IOTC_LOGE("wait reg gatt app err ret=%d", ret);
        return ret;
    }
    return IOTC_OK;
}

static int32_t StartGattService(IotcAdptBleGattService *svc)
{
    int32_t ret = AddGattService(svc);
    if (ret != IOTC_OK) {
        IOTC_LOGE("add gatt service err ret=%d", ret);
        return ret;
    }
    ret = WaitAddGattServiceResult(svc);
    if (ret != IOTC_OK) {
        IOTC_LOGE("wait add gatt service err ret=%d", ret);
        return ret;
    }
    ret = AddGattChars(svc);
    if (ret != IOTC_OK) {
        IOTC_LOGE("add gatt chars err ret=%d", ret);
        return ret;
    }
    ret = BleGattsStartService(svc->serverId, svc->svcHandle);
    if (ret != OHOS_BT_STATUS_SUCCESS) {
        IOTC_LOGE("start gatt svc err ret = %d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t IotcBleInitStack(void)
{
    int32_t ret = InitBtStack();
    if (ret != OHOS_BT_STATUS_SUCCESS) {
        IOTC_LOGE("init stack ret=%d", ret);
        return IOTC_ERROR;
    }
    ret = EnableBtStack();
    if (ret != OHOS_BT_STATUS_SUCCESS) {
        IOTC_LOGE("enable stack ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t IotcBleSetConnectParam(const IotcAdptBleConnectParam *param)
{
    (void)param;
    return IOTC_OK;
}

int32_t IotcBleRegisterGattCb(const IotcAdptBleGattCallback callback)
{
    if (callback == NULL) {
        IOTC_LOGE("invalid param");
        return IOTC_ERROR;
    }
    g_gattEventHandler = callback;
    IOTC_LOGD("gatts reg start");
    int32_t ret = BleGattsRegisterCallbacks(&g_bleGattsCb);
    if (ret != OHOS_BT_STATUS_SUCCESS) {
        IOTC_LOGE("register gatt callback ret=%d", ret);
        return IOTC_ERROR;
    }
    IOTC_LOGI("gatts reg success");
    IOTC_LOGD("gatt reg start");
    ret = BleGattRegisterCallbacks(&g_bleGattCb);
    if (ret != OHOS_BT_STATUS_SUCCESS) {
        IOTC_LOGE("register gap callback ret=%d", ret);
        return IOTC_ERROR;
    }
    IOTC_LOGI("gatt reg success");
    return IOTC_OK;
}

int32_t IotcBleSetBleName(const char *name)
{
    (void)name;
    return IOTC_OK;
}

int32_t IotcBleStartAdv(const IotcAdptBleAdvParam *advParam, const IotcAdptBleAdvData *advData)
{
    if ((advParam == NULL) || (advData == NULL)) {
        IOTC_LOGE("invalid param");
        return IOTC_ERROR;
    }
    if (!SetLocalName((unsigned char *)&advData->rspData[RSP_DATA_START_INDEX],
        (unsigned char)advData->rspDataLen - RSP_DATA_START_INDEX)) {
        IOTC_LOGE("set local name err");
        return IOTC_ERROR;
    }
    StartAdvRawData ohosAdvData = {0};
    ohosAdvData.advData = (uint8_t *)advData->advData;
    ohosAdvData.advDataLen = advData->advDataLen;
    ohosAdvData.rspData = NULL;
    ohosAdvData.rspDataLen = 0;
    BleAdvParams ohosAdvParam = {0};
    ohosAdvParam.minInterval = advParam->advMinInt;
    ohosAdvParam.maxInterval = advParam->advMaxInt;
    ohosAdvParam.advFilterPolicy = OHOS_BLE_ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
    ohosAdvParam.channelMap = advParam->channelMap;
    ohosAdvParam.advType = AdapterAdvTypeToOhosAdvType(advParam->advType);
    ohosAdvParam.ownAddrType = AdapterAddrTypeToOhosAddrType(advParam->ownerAddrType);
    if (advParam->directAddr != NULL) {
        if (memcpy_s(ohosAdvParam.peerAddr.addr, sizeof(ohosAdvParam.peerAddr.addr),
            advParam->directAddr, IOTC_ADPT_BLE_ADDR_LEN) != EOK) {
            IOTC_LOGE("set peer addr");
            return IOTC_ERROR;
        }
        ohosAdvParam.peerAddrType = AdapterAddrTypeToOhosAddrType(advParam->directAddrType);
    }
    /* 由于当前设备仅有一个广播，暂时不涉及多路广播 */
    if (g_advId != INVALID_ADV_ID) {
        (void)StopAdv(g_advId);
        g_advId = INVALID_ADV_ID;
    }
    int32_t ret = StartAdvEx(&g_advId, ohosAdvData, ohosAdvParam);
    if (ret != IOTC_OK) {
        IOTC_LOGE("start adv ret=%d", ret);
        return IOTC_ERROR;
    }
    IOTC_LOGD("start adv success advId=%d", g_advId);
    return IOTC_OK;
}

int32_t IotcBleStopAdv(void)
{
    /* 由于当前设备仅有一个广播，暂时不涉及多路广播 */
    if (g_advId == INVALID_ADV_ID) {
        IOTC_LOGD("no start adv");
        return IOTC_OK;
    }
    int32_t ret = StopAdv(g_advId);
    g_advId = INVALID_ADV_ID;
    if (ret != IOTC_OK) {
        IOTC_LOGE("stop adv ret=%d", ret);
        return IOTC_ERROR;
    }
    IOTC_LOGD("stop adv success advId=%d", g_advId);

    return IOTC_OK;
}

int32_t IotcBleStartGattsService(IotcAdptBleGattService *svc, uint8_t svcNum)
{
    if ((svc == NULL) || (svcNum == 0)) {
        IOTC_LOGE("invalid param");
        return IOTC_ERROR;
    }
    int32_t ret = 0;
    ret = StartRegGattApp();
    if (ret != IOTC_OK) {
        IOTC_LOGE("start reg gatt app err ret=%d", ret);
        return ret;
    }
    for (uint8_t i = 0; i < svcNum; i++) {
        ret = StartGattService(&svc[i]);
        if (ret != IOTC_OK) {
            IOTC_LOGE("start gatt service err ret=%d", ret);
            return ret;
        }
    }
    return IOTC_OK;
}

int32_t IotcBleStopGattsService(int32_t serverId, uint32_t svcHandle)
{
    int32_t ret = BleGattsStopService(serverId, svcHandle);
    if (ret != OHOS_BT_STATUS_SUCCESS) {
        IOTC_LOGE("gatt stop service ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t IotcBleSendGattsIndicate(const IotcAdptBleSendIndicateParam *param)
{
    if ((param == NULL) || (param->value == NULL) || (param->valueLen == 0)) {
        IOTC_LOGE("invalid param");
        return IOTC_ERROR;
    }

    GattsSendIndParam indParam;
    (void)memset_s(&indParam, sizeof(indParam), 0, sizeof(indParam));
    indParam.connectId = param->connId;
    indParam.attrHandle = param->charHandle;
    indParam.confirm = param->needConfirm;
    indParam.valueLen = param->valueLen;
    indParam.value = (char *)param->value;
    int32_t ret = BleGattsSendIndication(param->serverId, &indParam);
    if (ret != OHOS_BT_STATUS_SUCCESS) {
        IOTC_LOGE("send indicate ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t IotcBleSendGattsResponse(const IotcAdptBleResponseParam *param)
{
    if (param == NULL) {
        IOTC_LOGE("invalid param");
        return IOTC_ERROR;
    }
    GattsSendRspParam rspParam;
    rspParam.status = OHOS_GATT_SUCCESS;
    rspParam.attrHandle = param->transId;
    rspParam.connectId = param->connectId;
    rspParam.value = (char *)param->value;
    rspParam.valueLen = param->valueLen;
    int32_t ret = BleGattsSendResponse(param->serverId, &rspParam);
    if (ret != OHOS_BT_STATUS_SUCCESS) {
        IOTC_LOGE("send rsp ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t IotcBleGetBleMac(uint8_t *mac, uint32_t len)
{
    if (mac == NULL) {
        IOTC_LOGE("mac null");
        return IOTC_ERROR;
    }
    int32_t ret = ReadBtMacAddr(mac, len);
    if (ret != OHOS_BT_STATUS_SUCCESS) {
        IOTC_LOGE("read mac ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t IotcBleDisconnectGatt(const uint8_t *bdAddr, uint32_t addrLen)
{
    if ((bdAddr == NULL) || (addrLen > OHOS_BD_ADDR_LEN)) {
        IOTC_LOGE("invalid param");
        return IOTC_ERROR;
    }
    BdAddr addr = {0};
    if (memcpy_s(addr.addr, OHOS_BD_ADDR_LEN, bdAddr, addrLen) != EOK) {
        IOTC_LOGE("memcpy");
        return IOTC_ERROR;
    }
    int32_t ret = BleGattsDisconnect(0, addr, 0);
    if (ret != OHOS_BT_STATUS_SUCCESS) {
        IOTC_LOGE("gatt disconnect ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t IotcBleDeInitStack(void)
{
    int32_t ret = DisableBtStack();
    if (ret != OHOS_BT_STATUS_SUCCESS) {
        IOTC_LOGE("disable bt stack ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}