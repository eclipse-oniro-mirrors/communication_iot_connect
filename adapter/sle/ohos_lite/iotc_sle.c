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
#include "iotc_sle.h"
#include "securec.h"
#include "ohos_bt_def.h"
#include "ohos_bt_gatt.h"
#include "ohos_bt_gatt_server.h"
#include "iotc_mem.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "iotc_sle_tem.h"

#define SLE_ADV_HANDLE_DEFAULT 1
enum {
    UUID_STR_4_BYTES = 4,
    UUID_STR_8_BYTES = 8,
    UUID_STR_32_BYTES = 32,
};

static bool g_isBond = false;

static IotcAdptSleSsapCallback g_gattEventHandler = NULL;

static IotcAdptSleStatus OhosStatusToAdapterStatus(int32_t status)
{
    return (status == IOTC_ADPT_SLE_STATUS_SUCCESS) ? IOTC_ADPT_SLE_STATUS_SUCCESS : IOTC_ADPT_SLE_STATUS_FAIL;
}

static uint32_t AdapterPemissionToOhosPermission(uint32_t permission)
{
    uint32_t ohosPermission = 0;
    if (permission & IOTC_ADPT_SLE_CHAR_PERM_READ) {
        ohosPermission |= OHOS_GATT_PERMISSION_READ;
    }
    if (permission & IOTC_ADPT_SLE_CHAR_PERM_READ_ENCRYPTED) {
        ohosPermission |= OHOS_GATT_PERMISSION_READ_ENCRYPTED;
    }
    if (permission & IOTC_ADPT_SLE_CHAR_PERM_READ_ENCRYPTED_MITM) {
        ohosPermission |= OHOS_GATT_PERMISSION_READ_ENCRYPTED_MITM;
    }
    if (permission & IOTC_ADPT_SLE_CHAR_PERM_WRITE) {
        ohosPermission |= OHOS_GATT_PERMISSION_WRITE;
    }
    if (permission & IOTC_ADPT_SLE_CHAR_PERM_WRITE_ENCRYPTED) {
        ohosPermission |= OHOS_GATT_PERMISSION_WRITE_ENCRYPTED;
    }
    if (permission & IOTC_ADPT_SLE_CHAR_PERM_WRITE_ENCRYPTED_MITM) {
        ohosPermission |= OHOS_GATT_PERMISSION_WRITE_ENCRYPTED_MITM;
    }
    if (permission & IOTC_ADPT_SLE_CHAR_PERM_WRITE_SIGNED) {
        ohosPermission |= OHOS_GATT_PERMISSION_WRITE_SIGNED;
    }
    if (permission & IOTC_ADPT_SLE_CHAR_PERM_WRITE_SIGNED_MITM) {
        ohosPermission |= OHOS_GATT_PERMISSION_WRITE_SIGNED_MITM;
    }
    return ohosPermission;
}

static uint32_t AdapterPropertyToOhosProperty(uint32_t property)
{
    uint32_t ohosProperty = 0;

    if (property & IOTC_ADPT_SLE_CHAR_PROP_BROADCAST) {
        ohosProperty |= OHOS_GATT_CHARACTER_PROPERTY_BIT_BROADCAST;
    }
    if (property & IOTC_ADPT_SLE_CHAR_PROP_READ) {
        ohosProperty |= OHOS_GATT_CHARACTER_PROPERTY_BIT_READ;
    }
    if (property & IOTC_ADPT_SLE_CHAR_PROP_WRITE_WITHOUT_RESP) {
        ohosProperty |= OHOS_GATT_CHARACTER_PROPERTY_BIT_WRITE_NO_RSP;
    }
    if (property & IOTC_ADPT_SLE_CHAR_PROP_WRITE) {
        ohosProperty |= OHOS_GATT_CHARACTER_PROPERTY_BIT_WRITE;
    }
    if (property & IOTC_ADPT_SLE_CHAR_PROP_NOTIFY) {
        ohosProperty |= OHOS_GATT_CHARACTER_PROPERTY_BIT_NOTIFY;
    }
    if (property & IOTC_ADPT_SLE_CHAR_PROP_INDICATE) {
        ohosProperty |= OHOS_GATT_CHARACTER_PROPERTY_BIT_INDICATE;
    }
    if (property & IOTC_ADPT_SLE_CHAR_PROP_SIGNED_WRITE) {
        ohosProperty |= OHOS_GATT_CHARACTER_PROPERTY_BIT_SIGNED_WRITE;
    }
    if (property & IOTC_ADPT_SLE_CHAR_PROP_EXTENDED_PROPERTY) {
        ohosProperty |= OHOS_GATT_CHARACTER_PROPERTY_BIT_EXTENDED_PROPERTY;
    }
    return ohosProperty;
}

static BleScanResultAddrType AdapterAddrTypeToOhosAddrType(IotcAdptSleAdvAddr type)
{
    if (type == IOTC_ADPT_SLE_ADV_ADDR_PUBLIC) {
        return OHOS_BLE_PUBLIC_DEVICE_ADDRESS;
    } else if (type == IOTC_ADPT_SLE_ADV_ADDR_RANDOM) {
        return OHOS_BLE_RANDOM_DEVICE_ADDRESS;
    } else if (type == IOTC_ADPT_SLE_ADV_ADDR_PUBLIC_ID) {
        return OHOS_BLE_PUBLIC_IDENTITY_ADDRESS;
    } else if (type == IOTC_ADPT_SLE_ADV_ADDR_RANDOM_ID) {
        return OHOS_BLE_RANDOM_STATIC_IDENTITY_ADDRESS;
    } else if (type == IOTC_ADPT_SLE_ADV_ADDR_UNKNOWN_TYPE) {
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


static int32_t SetGapSecurityParam(bool isBond)
{
    int32_t mode = isBond ? OHOS_BLE_AUTH_REQ_SC_BOND : OHOS_BLE_AUTH_NO_BOND;
    (void)mode;
    return IOTC_OK;
}

static void AdvStartCompleteCb(int32_t clientId, int32_t status)
{
    (void)clientId;
    IOTC_LOGD("adv start complete cb:clientId:%d,status:%d", clientId, status);
    IotcAdptSleSsapEventParam eventParam;
    eventParam.startAdv.status = OhosStatusToAdapterStatus(status);
    if (g_gattEventHandler != NULL &&
        g_gattEventHandler(IOTC_ADPT_SLE_SSAP_EVENT_START_ADV_RESULT, &eventParam) != IOTC_OK) {
        IOTC_LOGE("gatt adv start event");
    }
}

static void AdvStopCompleteCb(int32_t clientId, int32_t status)
{
    (void)clientId;
    IOTC_LOGD("adv stop complete cb:clientId:%d,status:%d", clientId, status);
    IotcAdptSleSsapEventParam eventParam;
    eventParam.stopAdv.status = OhosStatusToAdapterStatus(status);
    if (g_gattEventHandler != NULL &&
        g_gattEventHandler(IOTC_ADPT_SLE_SSAP_EVENT_STOP_ADV_RESULT, &eventParam) != IOTC_OK) {
        IOTC_LOGE("gap adv stop event");
    }
}

static void SecurityRespondCb(const BdAddr *bdAddr)
{
    IOTC_LOGD("security respond cb");
    int32_t ret = SleSsapSecurityRsp(*bdAddr, true);
    if (ret != OHOS_BT_STATUS_SUCCESS) {
        IOTC_LOGE("gap security rsp ret=%d", ret);
    }
}


static void AddServiceCb(uint8_t server_id, IotcAdptUuidAddr *uuid, uint16_t handle, uint32_t status)
{
    IOTC_LOGD("SLE AddServiceCb:status:%d,serverId:%d,Handle=%d", status, server_id, handle);
}

static void AddPropertyCb(uint8_t server_id, IotcAdptUuidAddr *uuid, uint16_t service_handle,
    uint16_t handle, uint32_t status)
{
    IOTC_LOGD("SLE AddPropertyCb:status:%d,serverId:%d,Handle=%d", status, server_id, handle);
}

static void AddDescriptorCb(uint8_t server_id, IotcAdptUuidAddr *uuid, uint16_t service_handle,
    uint16_t property_handle, uint32_t status)
{
    IOTC_LOGD("SLE AddDescriptorCb:status:%d,serverId:%d,Handle=%d", status, server_id, service_handle);
}

static void ServiceStartCb(uint8_t server_id, uint16_t handle, uint32_t status)
{
    IOTC_LOGD("SLE service start cb:status:%d,serverId:%d,Handle=%d", status, server_id, handle);
    IotcAdptSleSsapEventParam eventParam;
    eventParam.startSvc.status = OhosStatusToAdapterStatus(status);
    eventParam.startSvc.serverId = server_id;
    eventParam.startSvc.svcHandle = handle;
    if (g_gattEventHandler != NULL &&
        g_gattEventHandler(IOTC_ADPT_SLE_SSAP_EVENT_START_SVC_RESULT, &eventParam) != IOTC_OK) {
        IOTC_LOGE("doing gatt event");
    }
}

static void DeleteAllServiceCb(uint8_t server_id, uint32_t status)
{
    IOTC_LOGD("SLE DeleteAllServiceCb:status:%d,serverId:%d", status, server_id);
}

static void ReadRequestCb(uint8_t server_id, uint16_t conn_id, IotcAdptSleReqRead *read_cb_para,
    uint32_t status)
{
    IOTC_LOGD("SLE  ReadRequestCb:status:%d,serverId:%d,conn_id=%d", status, server_id, conn_id);
}

static void WriteRequestCb(uint8_t server_id, uint16_t conn_id, IotcAdptSleReqWrite *write_cb_para,
    uint32_t status)
{
    IOTC_LOGD("SLE  WriteRequestCb:status:%d,serverId:%d,conn_id=%d", status, server_id, conn_id);
}

static void MtuChangeCb(uint8_t server_id, uint16_t conn_id, IotcAdptSleExchangeInfo *info, uint32_t status)
{
    IOTC_LOGD("SLE MtuChangeCb:status:%d,serverId:%d,conn_id=%d", status, server_id, conn_id);
}

static SsapsCallbacks g_sleSsapsCb = {
    .add_service_cb = AddServiceCb,
    .add_property_cb = AddPropertyCb,
    .add_descriptor_cb = AddDescriptorCb,
    .start_service_cb = ServiceStartCb,
    .delete_all_service_cb = DeleteAllServiceCb,
    .read_request_cb = ReadRequestCb,
    .write_request_cb = WriteRequestCb,
    .mtu_changed_cb = MtuChangeCb,
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

static int32_t AdapterSvcToOhosSvc(IotcAdptSleSsapService *in, BleGattAttr *to)
{
    to->attrType = OHOS_BLE_ATTRIB_TYPE_SERVICE;
    to->uuidType = GetOhosUuidType(in->uuid);
    if (!UtilsUnhexifyR(in->uuid, strlen(in->uuid), to->uuid, sizeof(to->uuid))) {
        IOTC_LOGE("str to hex");
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

static int32_t AdapterCharToOhosChar(IotcAdptSleSsapsChar *in, BleGattAttr *to)
{
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

static int32_t AdapterDescToOhosDesc(IotcAdptSleSsapCharDesc *in, BleGattAttr *to)
{
    if (in->uuid == NULL) {
        IOTC_LOGE("uuid is null");
        return IOTC_ERROR;
    }
    to->attrType = OHOS_BLE_ATTRIB_TYPE_CHAR_USER_DESCR;
    to->permission = AdapterPemissionToOhosPermission(in->permission);
    if (!UtilsUnhexifyR(in->uuid, strlen(in->uuid),
        to->uuid, sizeof(to->uuid))) {
        IOTC_LOGE("str to hex");
        return IOTC_ERROR;
    }
    to->func.read = in->readFunc;
    to->func.write = in->writeFunc;
    return IOTC_OK;
}

static int32_t AdapterServiceCopyToOhosSsapAttr(IotcAdptSleSsapService *svc, BleGattAttr *attrList, uint8_t attrNum)
{
    if (svc->uuid == NULL) {
        IOTC_LOGE("uuid is null");
        return IOTC_ERROR;
    }
    uint8_t attrCnt = 0;
    if (AdapterSvcToOhosSvc(svc, &attrList[attrCnt]) != IOTC_OK) {
        IOTC_LOGE("svc copy err");
        return IOTC_ERROR;
    }
    attrCnt++;
    for (uint8_t i = 0; i < svc->charNum; i++) {
        if (AdapterCharToOhosChar(&svc->character[i], &attrList[attrCnt]) != IOTC_OK) {
            IOTC_LOGE("char copy err");
            return IOTC_ERROR;
        }
        svc->character[i].charHandle = attrCnt;
        attrCnt++;
        for (uint8_t j = 0; j < svc->character[i].descNum; j++) {
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

static void RefreshHandle(IotcAdptSleSsapService *svc)
{
    for (uint8_t i = 0; i < svc->charNum; i++) {
        svc->character[i].charHandle += svc->svcHandle;
        for (uint8_t j = 0; j < svc->character[i].descNum; j++) {
            svc->character[i].desc[j].descHandle += svc->svcHandle;
        }
    }
}

int32_t IotcSleInitStack(void)
{
    int32_t ret = EnableSle();
    if (ret != IOTC_ADPT_SLE_STATUS_SUCCESS) {
        IOTC_LOGE("init stack ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t IotcSleSetConnectParam(const IotcAdptSleConnectParam *param)
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

int32_t IotcSleRegisterSsapCb(const IotcAdptSleSsapCallback callback)
{
    if (callback == NULL) {
        IOTC_LOGE("invalid param");
        return IOTC_ERROR;
    }
    g_gattEventHandler = callback;
    IOTC_LOGD("gatts reg start");
    // 注册回调
    int32_t ret =  SsapsRegisterCallbacks(&g_sleSsapsCb);
    if (ret != IOTC_ADPT_SLE_STATUS_SUCCESS) {
        IOTC_LOGE("register gatt callback ret=%d", ret);
        return IOTC_ERROR;
    }
    IOTC_LOGI("gatts reg success");
    return IOTC_OK;
}

int32_t IotcSleSetSleName(const uint8_t *name, uint8_t len)
{
    if (name == NULL) {
        IOTC_LOGE("invalid param");
        return IOTC_ERROR;
    }
    int32_t ret = SleSetLocalName(&name, len);
    if (ret != IOTC_ADPT_SLE_STATUS_SUCCESS) {
        IOTC_LOGE("set name ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

static uint8_t g_announceId = SLE_ADV_HANDLE_DEFAULT;

int32_t IotcSleStartAdv(const IotcAdptSleAdvParam *advParam, const IotcAdptSleAdvData *advData)
{
    if ((advParam == NULL) || (advData == NULL)) {
        IOTC_LOGE("invalid param");
        return IOTC_ERROR;
    }
    SleAnnounceData ohosAdvData = {0};
    ohosAdvData.announceData = (uint8_t *)advData->announceData;
    ohosAdvData.announceDataLen = advData->announceDataLen;
    ohosAdvData.seekRspData = (uint8_t *)advData->seekRspData;
    ohosAdvData.seekRspDataLen = advData->seekRspDataLen;
    int32_t retData = SleSetAnnounceData(g_announceId, &ohosAdvData);
    if (retData != ERRCODE_SLE_SUCCESS) {
        IOTC_LOGE("start adv set data=%d", retData);
    }
    SleAnnounceParam ohosAdvParam = {0};
    ohosAdvParam.announce_interval_min = advParam->announceIntervalMin;
    int32_t paramRet = SleSetAnnounceParam(g_announceId, &ohosAdvParam);
    if (paramRet != ERRCODE_SLE_SUCCESS) {
        IOTC_LOGE("start adv set param=%d", paramRet);
    }
    // 发起广播
    int32_t ret = SleStartAnnounce(g_announceId);
    if (ret != ERRCODE_SLE_SUCCESS) {
        IOTC_LOGE("start adv ret=%d", ret);
        return;
    }
    IOTC_LOGD("start adv success");
}

int32_t IotcSleStopAdv()
{
    /* 由于当前设备仅有一个广播，暂时不涉及多路广播 */
    int32_t ret = SleStopAnnounce(g_announceId);
    if (ret != ERRCODE_SLE_SUCCESS) {
        IOTC_LOGE("stop adv ret=%d", ret);
        return ;
    }
    IOTC_LOGD("stop adv success");
}

int32_t IotcSleStartSsapsService(IotcAdptSleSsapService *svc, uint8_t svcNum)
{
    if ((svc == NULL) || (svcNum == 0)) {
        IOTC_LOGE("invalid param");
        return IOTC_ERROR;
    }
    for (uint8_t i = 0; i < svcNum; i++) {
        int32_t ret = SsapsStartService(&svc[i].serverId, &svc[i].svcHandle);
        if (ret != IOTC_ADPT_SLE_STATUS_SUCCESS) {
            IOTC_LOGE("gatt start service ret=%d", ret);
            return IOTC_ERROR;
        }
        RefreshHandle(svc + i);
    }
    return IOTC_OK;
}


int32_t IotcSleSendSsapsIndicate(const IotcAdptSleSendIndicateParam *param)
{
    if ((param == NULL) || (param->value == NULL) || (param->valueLen == 0)) {
        IOTC_LOGE("invalid param");
        return IOTC_ERROR;
    }

    SsapsNtfInd indParam = {0};
    (void)memset_s(&indParam, sizeof(indParam), 0, sizeof(indParam));
    indParam.handle = param->handle;
    indParam.type = param->type;
    indParam.value_len = param->valueLen;
    indParam.value = (char *)param->value;
    int32_t ret = SsapsNotifyIndicate(param->serverId, param->connId, &indParam);
    if (ret != IOTC_ADPT_SLE_STATUS_SUCCESS) {
        IOTC_LOGE("sle send indicate ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t IotcSleSendSsapsResponse(const IotcAdptSleResponseParam *param)
{
    (void)param;

    return IOTC_OK;
}


int32_t IotcSleDisconnectSsap(const uint8_t *bdAddr, uint32_t addrLen)
{
    if ((bdAddr == NULL) || (addrLen > OHOS_BD_ADDR_LEN)) {
        IOTC_LOGE("invalid param");
        return IOTC_ERROR;
    }
     IotcAdptSleAddr iotcAddr = {0};
    if (memcpy_s(iotcAddr.addr, OHOS_BD_ADDR_LEN, bdAddr, addrLen) != EOK) {
        IOTC_LOGE("memcpy");
        return IOTC_ERROR;
    }

    int32_t ret = SlePairRemoteDevice(iotcAddr);
    if (ret != OHOS_BT_STATUS_SUCCESS) {
        IOTC_LOGE("gatt disconnect ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t IotcSleDeInitStack(void)
{
    bool ret = DisableSle();
    if (!ret) {
        IOTC_LOGE("disable bt stack ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}