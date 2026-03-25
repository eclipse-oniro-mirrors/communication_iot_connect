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
#include "securec.h"
#include "ohos_bt_def.h"
#include "ohos_bt_gatt.h"
#include "ohos_bt_gatt_server.h"
#include "iotc_mem.h"
#include "iotc_errcode.h"
#include "iotc_log.h"

enum {
    UUID_STR_4_BYTES = 4,
    UUID_STR_8_BYTES = 8,
    UUID_STR_32_BYTES = 32,
};

static bool g_isBond = false;

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

static uint32_t GetOhosUuidType(const char *uuid)
{
    if (uuid == NULL) {
        return OHOS_UUID_TYPE_NULL;
    }
    uint32_t len = strlen(uuid);
    if (len == UUID_STR_4_BYTES) {
        return OHOS_UUID_TYPE_16_BIT;
    } else if (len == UUID_STR_8_BYTES) {
        return OHOS_UUID_TYPE_32_BIT;
    } else if (len == UUID_STR_32_BYTES) {
        return OHOS_UUID_TYPE_128_BIT;
    }
    return OHOS_UUID_TYPE_NULL;
}

static uint32_t GetSvcAttrNum(IotcAdptBleGattService *svc)
{
    uint32_t attrCnt = 1;
    attrCnt += svc->charNum;
    for (uint32_t i = 0; i < svc->charNum; i++) {
        attrCnt += svc->character[i].descNum;
    }
    return attrCnt;
}

static int32_t SetGapSecurityParam(bool isBond)
{
    if (BleSetSecurityIoCap(OHOS_BLE_IO_CAP_NONE) != IOTC_OK) {
        IOTC_LOGE("set security io cap");
        return IOTC_ERROR;
    }
    BleAuthReqMode mode = isBond ? OHOS_BLE_AUTH_REQ_SC_BOND : OHOS_BLE_AUTH_NO_BOND;
    if (BleSetSecurityAuthReq(mode) != IOTC_OK) {
        IOTC_LOGE("set security auth req");
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

static void AdvStartCompleteCb(int32_t clientId, int32_t status)
{
    (void)clientId;
    IOTC_LOGD("adv start complete cb:clientId:%d,status:%d", clientId, status);
    IotcAdptBleGattEventParam eventParam;
    eventParam.startAdv.status = OhosStatusToAdapterStatus(status);
    if (g_gattEventHandler != NULL &&
        g_gattEventHandler(IOTC_ADPT_BLE_GATT_EVENT_START_ADV_RESULT, &eventParam) != IOTC_OK) {
        IOTC_LOGE("gatt adv start event");
    }
}

static void AdvStopCompleteCb(int32_t clientId, int32_t status)
{
    (void)clientId;
    IOTC_LOGD("adv stop complete cb:clientId:%d,status:%d", clientId, status);
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
    BleSecAct secAct = g_isBond ? OHOS_BLE_SEC_ENCRYPT : OHOS_BLE_SEC_NONE;
    int32_t ret = BleGattsSetEncryption(*bdAddr, secAct);
    if (ret != OHOS_BT_STATUS_SUCCESS) {
        IOTC_LOGE("set encryption ret=%d", ret);
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
    (void)(appUuid);
    
    IOTC_LOGD("register server cb:status=%d, serverId=%d", status, serverId);
    g_regGattAppResult.status = status;
    g_regGattAppResult.serverId = serverId;
    g_regGattAppResult.has = true;
}

static BtGattServerCallbacks g_bleGattsCb = {
    .connectServerCb = ConnectServerCb,
    .disconnectServerCb = DisconnectServerCb,
    .serviceStartCb = ServiceStartCb,
    .serviceStopCb = ServiceStopCb,
    .indicationSentCb = IndicationSendCb,
    .mtuChangeCb = MtuChangeCb,
};

static bool HexChar2Num(char hexChar, uint8_t *num)
{
    if (hexChar >= '0' && hexChar <= '9') {
        *num = hexChar - '0';
    } else if (hexChar >= 'a' && hexChar <= 'f') {
        /* 10表示16进制的a的值 */
        *num = hexChar - 'a' + 10;
    } else if (hexChar >= 'A' && hexChar <= 'F') {
        /* 10表示16进制的A的值 */
        *num = hexChar - 'A' + 10;
    } else {
        return false;
    }

    return true;
}

static bool UtilsUnhexifyR(const char *inBuf, uint32_t inBufLen, uint8_t *outBuf, uint32_t outBufLen)
{
    if (inBuf == NULL || inBufLen == 0 || outBuf == NULL || outBufLen == 0) {
        return false;
    }

    /* 对2取余保证解码长度必须为偶数 */
    if ((inBufLen % 2) != 0) {
        return false;
    }

    /* unhex后长度除以2 */
    uint32_t outLen = inBufLen / 2;
    if (outLen > outBufLen) {
        return false;
    }
    char *in = (char *)inBuf + inBufLen - 1;
    uint32_t outIndex = 0;
    uint8_t high;
    uint8_t low;
    while (outIndex < outLen) {
        if (!HexChar2Num(*(in--), &low)) {
            return false;
        }

        if (!HexChar2Num(*(in--), &high)) {
            return false;
        }
        /* 左移4位将单个16进制字符数的第一位移动到高位 */
        *outBuf++ = (high << 4) | low;
        outIndex++;
    }
    return true;
}

static int32_t AdapterSvcToOhosSvc(IotcAdptBleGattService *in, BleGattAttr *to)
{
    to->attrType = OHOS_BLE_ATTRIB_TYPE_SERVICE;
    to->uuidType = GetOhosUuidType(in->uuid);
    if (!UtilsUnhexifyR(in->uuid, strlen(in->uuid), to->uuid, sizeof(to->uuid))) {
        IOTC_LOGE("str to hex");
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

static int32_t AdapterCharToOhosChar(IotcAdptBleGattsChar *in, BleGattAttr *to)
{
    IOTC_LOGI("AdapterCharToOhosChar start");
    if (in->uuid == NULL) {
        IOTC_LOGE("uuid is null");
        return IOTC_ERROR;
    }
    to->attrType = OHOS_BLE_ATTRIB_TYPE_CHAR;
    to->permission = AdapterPemissionToOhosPermission(in->permission);
    to->uuidType = GetOhosUuidType(in->uuid);
    if (!UtilsUnhexifyR(in->uuid, strlen(in->uuid), to->uuid, sizeof(to->uuid))) {
        IOTC_LOGE("str to hex");
        return IOTC_ERROR;
    }
    to->properties = AdapterPropertyToOhosProperty(in->property);
    to->func.read = in->readFunc;
    to->func.write = in->writeFunc;
    to->func.indicate = in->indicateFunc;
    return IOTC_OK;
}

static int32_t AdapterDescToOhosDesc(IotcAdptBleGattCharDesc *in, BleGattAttr *to)
{
    if (in->uuid == NULL) {
        IOTC_LOGE("uuid is null");
        return IOTC_ERROR;
    }
    to->attrType = OHOS_BLE_ATTRIB_TYPE_CHAR_USER_DESCR;
    to->permission = AdapterPemissionToOhosPermission(in->permission);
    if (!UtilsUnhexifyR(in->uuid, strlen(in->uuid), to->uuid, sizeof(to->uuid))) {
        IOTC_LOGE("str to hex");
        return IOTC_ERROR;
    }
    to->func.read = in->readFunc;
    to->func.write = in->writeFunc;
    return IOTC_OK;
}

static int32_t AdapterServiceCopyToOhosGattAttr(IotcAdptBleGattService *svc, BleGattAttr *attrList, uint32_t attrNum)
{
    (void)(attrNum);

    if (svc->uuid == NULL) {
        IOTC_LOGE("uuid is null");
        return IOTC_ERROR;
    }
    uint32_t attrCnt = 0;
    if (AdapterSvcToOhosSvc(svc, &attrList[attrCnt]) != IOTC_OK) {
        IOTC_LOGE("svc copy err");
        return IOTC_ERROR;
    }
    attrCnt++;
    for (uint32_t i = 0; i < svc->charNum; i++) {
        if (AdapterCharToOhosChar(&svc->character[i], &attrList[attrCnt]) != IOTC_OK) {
            IOTC_LOGE("char copy err");
            return IOTC_ERROR;
        }
        svc->character[i].charHandle = attrCnt;
        attrCnt++;
        for (uint32_t j = 0; j < svc->character[i].descNum; j++) {
            if (AdapterDescToOhosDesc(&svc->character[i].desc[j], &attrList[attrCnt])) {
                IOTC_LOGE("desc copy err");
                return IOTC_ERROR;
            }
            svc->character[i].desc[j].descHandle = attrCnt;
            attrCnt++;
        }
    }
    return IOTC_OK;
}

static void RefreshHandle(IotcAdptBleGattService *svc)
{
    for (uint32_t i = 0; i < svc->charNum; i++) {
        svc->character[i].charHandle += svc->svcHandle;
        for (uint32_t j = 0; j < svc->character[i].descNum; j++) {
            svc->character[i].desc[j].descHandle += svc->svcHandle;
        }
    }
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
    if (param == NULL) {
        IOTC_LOGE("invalid param");
        return IOTC_ERROR;
    }

    g_isBond = param->isBond;
    if (SetGapSecurityParam(param->isBond) != IOTC_OK) {
        IOTC_LOGE("set gap security param");
        return IOTC_ERROR;
    }
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
    if (name == NULL) {
        IOTC_LOGE("invalid param");
        return IOTC_ERROR;
    }
    IOTC_LOGI("IotcBleSetBleName name:%s", name);
    int32_t ret = SetDeviceName(name, strlen(name));
    if (ret != OHOS_BT_STATUS_SUCCESS) {
        IOTC_LOGE("set name ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t IotcBleStartAdv(const IotcAdptBleAdvParam *advParam, const IotcAdptBleAdvData *advData)
{
    if ((advParam == NULL) || (advData == NULL)) {
        IOTC_LOGE("invalid param");
        return IOTC_ERROR;
    }

    StartAdvRawData ohosAdvData = {0};
    ohosAdvData.advData = (uint8_t *)advData->advData;
    ohosAdvData.advDataLen = advData->advDataLen;
    ohosAdvData.rspData = (uint8_t *)advData->rspData;
    ohosAdvData.rspDataLen = advData->rspDataLen;
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
    int32_t advId = 0;
    int32_t ret = BleStartAdvEx(&advId, ohosAdvData, ohosAdvParam);
    if (ret != OHOS_BT_STATUS_SUCCESS) {
        IOTC_LOGE("start adv ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t IotcBleStopAdv(void)
{
    /* 由于当前设备仅有一个广播，暂时不涉及多路广播 */
    int32_t ret = BleStopAdv(0);
    if (ret != OHOS_BT_STATUS_SUCCESS) {
        IOTC_LOGE("stop adv ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t IotcBleStartGattsService(IotcAdptBleGattService *svc, uint32_t svcNum)
{
    if ((svc == NULL) || (svcNum == 0)) {
        IOTC_LOGE("invalid param");
        return IOTC_ERROR;
    }
    IOTC_LOGI("IotcBleStartGattsService svcNum:%u", svcNum);
    for (uint8_t i = 0; i < svcNum; i++) {
        uint8_t attrNum = GetSvcAttrNum(svc + i);
        IOTC_LOGI(" attrNum:%u", attrNum);
        BleGattAttr attrList[attrNum];
        (void)memset_s(&attrList, attrNum * sizeof(BleGattAttr), 0, attrNum * sizeof(BleGattAttr));
        if (AdapterServiceCopyToOhosGattAttr(svc + i, attrList, attrNum) != IOTC_OK) {
            IOTC_LOGE("copy gatt service");
            return IOTC_ERROR;
        }
        BleGattService srvcInfo = {0};
        srvcInfo.attrNum = attrNum;
        srvcInfo.attrList = attrList;
        //print uuid
        IOTC_LOGI("IotcBleStartGattsService i:%u, uuid:%s  svcHandle:%d", i, svc[i].uuid, &svc[i].svcHandle);
        int32_t ret = BleGattsStartServiceEx(&svc[i].svcHandle, &srvcInfo);
        if (ret != OHOS_BT_STATUS_SUCCESS) {
            IOTC_LOGE("gatt start service ret=%d", ret);
            return IOTC_ERROR;
        }
        RefreshHandle(svc + i);
    }
    return IOTC_OK;
}

int32_t IotcBleStopGattsService(int32_t serverId, uint32_t svcHandle)
{
    (void)serverId;
    int32_t ret = BleGattsStopServiceEx(svcHandle);
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
    (void)param;

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
    if ((bdAddr == NULL) || (addrLen > OHOS_BD_ADDR_LEN) || (addrLen == 0)) {
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