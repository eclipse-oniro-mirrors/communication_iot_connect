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
#include "iotc_sle_server.h"
#include "securec.h"
#include "iotc_sle_def.h"
#include "iotc_mem.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "ohos_bt_gatt_server.h"
#include "iotc_sle_ext.h"

#include "kh_sle_ssap_server.h"
#include "kh_sle_connect.h"


#define SLE_ADV_HANDLE_DEFAULT 1
enum {
    UUID_STR_4_BYTES = 4,
    UUID_STR_8_BYTES = 8,
    UUID_STR_32_BYTES = 32,
};

static bool g_isBond = false;

static IotcAdptSleSsapCallback g_gattEventHandler = NULL;
static IotcAdptSleAnnounceSeekCallback g_sleAnnounceSeekEventHandler = NULL;
static IotcAdptSleConnectionCallback g_sleConnectionEventHandler = NULL;


static IotcAdptSleStatus OhosStatusToAdapterStatus(int32_t status)
{
    return (status == IOTC_ADPT_SLE_STATUS_SUCCESS) ? IOTC_ADPT_SLE_STATUS_SUCCESS : IOTC_ADPT_SLE_STATUS_FAIL;
}

static uint32_t AdapterPemissionToOhosPermission(uint32_t permission)
{
    uint32_t toPermission = 0;
    if (permission & IOTC_SLE_SSAP_PERMISSION_READ) {
        toPermission |= IOTC_SLE_SSAP_PERMISSION_READ;
    }
    if (permission & IOTC_SLE_SSAP_PERMISSION_READ_ENCRYPTED) {
        toPermission |= IOTC_SLE_SSAP_PERMISSION_READ_ENCRYPTED;
    }
    if (permission & IOTC_SLE_SSAP_PERMISSION_READ_ENCRYPTED_MITM) {
        toPermission |= IOTC_SLE_SSAP_PERMISSION_READ_ENCRYPTED_MITM;
    }
    if (permission & IOTC_SLE_SSAP_PERMISSION_WRITE) {
        toPermission |= IOTC_SLE_SSAP_PERMISSION_WRITE;
    }
    if (permission & IOTC_SLE_SSAP_PERMISSION_WRITE_ENCRYPTED) {
        toPermission |= IOTC_SLE_SSAP_PERMISSION_WRITE_ENCRYPTED;
    }
    if (permission & IOTC_SLE_SSAP_PERMISSION_WRITE_ENCRYPTED_MITM) {
        toPermission |= IOTC_SLE_SSAP_PERMISSION_WRITE_ENCRYPTED_MITM;
    }
    if (permission & IOTC_SLE_SSAP_PERMISSION_WRITE_SIGNED) {
        toPermission |= IOTC_SLE_SSAP_PERMISSION_WRITE_SIGNED;
    }
    if (permission & IOTC_SLE_SSAP_PERMISSION_WRITE_SIGNED_MITM) {
        toPermission |= IOTC_SLE_SSAP_PERMISSION_WRITE_SIGNED_MITM;
    }
    return toPermission;
}

static uint32_t AdapterPropertyToOhosProperty(uint32_t property)
{
    uint32_t toProperty = 0;
    if (property & IOTC_SLE_SSAP_CHARACTER_PROPERTY_BIT_BROADCAST) {
        toProperty |= IOTC_ADPT_SLE_CHAR_PROP_BROADCAST;
    }
    if (property & IOTC_SLE_SSAP_CHARACTER_PROPERTY_BIT_READ) {
        toProperty |= IOTC_ADPT_SLE_CHAR_PROP_READ;
    }
    if (property & IOTC_SLE_SSAP_CHARACTER_PROPERTY_BIT_WRITE_NO_RSP) {
        toProperty |= IOTC_ADPT_SLE_CHAR_PROP_WRITE_WITHOUT_RESP;
    }
    if (property & IOTC_SLE_SSAP_CHARACTER_PROPERTY_BIT_WRITE) {
        toProperty |= IOTC_ADPT_SLE_CHAR_PROP_WRITE;
    }
    if (property & IOTC_SLE_SSAP_CHARACTER_PROPERTY_BIT_NOTIFY) {
        toProperty |= IOTC_ADPT_SLE_CHAR_PROP_NOTIFY;
    }
    if (property & IOTC_SLE_SSAP_CHARACTER_PROPERTY_BIT_INDICATE) {
        toProperty |= IOTC_ADPT_SLE_CHAR_PROP_INDICATE;
    }
    if (property & IOTC_SLE_SSAP_CHARACTER_PROPERTY_BIT_SIGNED_WRITE) {
        toProperty |= IOTC_ADPT_SLE_CHAR_PROP_SIGNED_WRITE;
    }
    if (property & IOTC_SLE_SSAP_CHARACTER_PROPERTY_BIT_EXTENDED_PROPERTY) {
        toProperty |= IOTC_ADPT_SLE_CHAR_PROP_EXTENDED_PROPERTY;
    }
    return toProperty;
}


int32_t IotcSsapsAddProperty(
    uint8_t serviceId,
    uint16_t serviceHandle,
    IotcAdptSleSsapsPropertyInfo *property,
    uint16_t *handle
)
{
    if (property == NULL || handle == NULL) {
        IOTC_LOGE("IotcSsapsAddProperty Invalid input parameters");
        return IOTC_ERROR;
    }
    SsapsPropertyParam  param = {0};
    SleUuid uuid = {0};
    uuid.len = property->uuid.len;
    if (memcpy_s(uuid.uuid, sizeof(uuid.uuid), property->uuid.uuid, property->uuid.len) != EOK) {
        IOTC_LOGE("IotcSsapsAddProperty memcpy_s failed: UUID data copy error");
        return IOTC_ERROR;
    }
    param.uuid = uuid;
    param.opIndication = property->operateIndication;
    param.permissions = property->permissions;
    param.value = property->value;
    param.valueLen = property->valueLen;

    int32_t ret = AddProperty(serviceId, serviceHandle, &param, handle);
    if (ret != IOTC_OK) {
        IOTC_LOGE("IotcAddSsapServer ret=%d", ret);
        return ret;
    }
    return IOTC_OK;
}

uint8_t IotcSsapsAddDescriptor(
    uint8_t serverId,
    uint16_t serviceHandle,
    uint16_t propHandle,
    const IotcAdptSleSsapsDescInfo *descParam,
    uint16_t *descHandle
)
{
    if (descParam == NULL || descHandle == NULL) {
        IOTC_LOGE("IotcSsapsAddDescriptor Invalid input parameters");
        return IOTC_ERROR;
    }
    SsapsDescriptorParam  param = {0};
    SleUuid uuid = {0};
    uuid.len = descParam->uuid.len;
    if (memcpy_s(uuid.uuid, sizeof(uuid.uuid), descParam->uuid.uuid, descParam->uuid.len) != EOK) {
        IOTC_LOGE("IotcSsapsAddProperty memcpy_s failed: UUID data copy error");
        return IOTC_ERROR;
    }
    param.uuid = uuid;
    param.operateInd = descParam->operateInd;
    param.permissions = descParam->permissions;
    param.value = descParam->value;
    param.valueLen = descParam->valueLen;
    param.type = descParam->type;

    int32_t ret = AddDescriptor(serverId, serviceHandle, propHandle, &param, descHandle);
    if (ret != IOTC_OK) {
        IOTC_LOGE("AddDescriptor ret=%d", ret);
        return ret;
    }
    return IOTC_OK;
}


uint8_t IotcInitSleSsapsService(void)
{
    int32_t ret = InitSleSsapsService();
    if (ret != IOTC_OK) {
        IOTC_LOGE("InitSleSsapsService ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

uint8_t IotcAddSsapServer(const IotcSleUuidAddr *appUuid, uint8_t *serverId)
{
    if (appUuid == NULL || appUuid->len == 0 || appUuid->len > UUID_LEN) {
        IOTC_LOGE("IotcAddSsapServer Invalid input parameters");
        return IOTC_ERROR;
    }

    SleUuid *uuid = (SleUuid *)malloc(sizeof(SleUuid));
    if (uuid == NULL) {
        IOTC_LOGE("IotcAddSsapServer malloc failed");
        return IOTC_ERROR;
    }
    uuid->len = appUuid->len;
    if (memcpy_s(uuid->uuid, sizeof(uuid->uuid), appUuid->uuid, appUuid->len) != EOK) {
        IOTC_LOGE("IotcAddSsapServer memcpy_s failed: UUID data copy error");
        free(uuid);
        return IOTC_ERROR;
    }
    int32_t ret = AddSsapServer(uuid, serverId);
    if (ret != IOTC_OK) {
        IOTC_LOGE("IotcAddSsapServer ret=%d", ret);
        return ret;
    }
    free(uuid);
    return IOTC_OK;
}

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

static int32_t AdapterSvcToOhosSvc(IotcAdptSleSsapService *in, SleAttr *to)
{
    to->attrType = OHOS_SLE_ATTRIB_TYPE_SERVICE;
    to->uuidType = GetOhosUuidType(in->uuid);
    if (!UtilsUnhexifyR(in->uuid, strlen(in->uuid), to->uuid, sizeof(to->uuid))) {
        IOTC_LOGE("str to hex");
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

static uint8_t GetSvcAttrNum(IotcAdptSleSsapService *svc)
{
    uint8_t attrCnt = 1;
    attrCnt += svc->charNum;
    for (uint8_t i = 0; i < svc->charNum; i++) {
        attrCnt += svc->character[i].descNum;
    }
    return attrCnt;
}

static int32_t AdapterCharToOhosChar(IotcAdptSleSsapsChar *in, SleAttr *to)
{
    IOTC_LOGI("AdapterCharToOhosChar start");
    if (in->uuid == NULL) {
        IOTC_LOGE("uuid is null");
        return IOTC_ERROR;
    }
    to->attrType = OHOS_SLE_ATTRIB_TYPE_CHAR;
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

static int32_t AdapterDescToOhosDesc(IotcAdptSleSsapCharDesc *in, SleAttr *to)
{
    if (in->uuid == NULL) {
        IOTC_LOGE("uuid is null");
        return IOTC_ERROR;
    }
    to->attrType = OHOS_SLE_ATTRIB_TYPE_CHAR_USER_DESCR;
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

static int32_t AdapterServiceCopyToOhosGattAttr(IotcAdptSleSsapService *svc, SleAttr *attrList, uint8_t attrNum)
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

int32_t IotcSleSsapsStartService(uint8_t serviceId, uint16_t serviceHandle)
{
    IOTC_LOGD("SLE  IotcSleStartSsapsService:serverId:%d,handle=%d", serviceId, serviceHandle);
    int32_t ret = StartService(serviceId, serviceHandle);
    if (ret != IOTC_OK) {
        IOTC_LOGE("SsapsStartService ret=%d", ret);
        return ret;
    }
    return IOTC_OK;
}

int32_t IotcSleSsapsStartServiceExt(IotcAdptSleSsapService *svc, uint8_t svcNum)
{
    if ((svc == NULL) || (svcNum == 0)) {
        IOTC_LOGE("IotcSleSsapsStartService invalid param");
        return IOTC_ERROR;
    }

    IOTC_LOGI("IotcSleSsapsStartService svcNum:%u", svcNum);
    for (uint8_t i = 0; i < svcNum; i++) {
        uint8_t attrNum = GetSvcAttrNum(svc + i);
        IOTC_LOGI(" attrNum:%u", attrNum);
        SleAttr attrList[attrNum];
        (void)memset_s(&attrList, attrNum * sizeof(SleAttr), 0, attrNum * sizeof(SleAttr));
        if (AdapterServiceCopyToOhosGattAttr(svc + i, attrList, attrNum) != IOTC_OK) {
            IOTC_LOGE("copy gatt service");
            return IOTC_ERROR;
        }
        SleService srvcInfo = {0};
        srvcInfo.attrNum = attrNum;
        srvcInfo.attrList = attrList;
        //print uuid
        IOTC_LOGI("IotcBleStartGattsService uuid:%s  svcHandle:%d", svc[i].uuid, &svc[i].svcHandle);

        int32_t ret = IotcSleStartServiceEx(&svc[i].svcHandle, &srvcInfo);
        if (ret != IOTC_OK) {
            IOTC_LOGE("sle start service ret=%d", ret);
            return ret;
        }
        RefreshHandle(svc + i);
    }

    return IOTC_OK;
}

uint8_t IotcSleSendSsapsIndicate(uint8_t serverId, uint16_t connectId, const IotcAdptSleSendIndicateParam *param)
{
    if ((param == NULL) || (param->value == NULL) || (param->valueLen == 0)) {
        IOTC_LOGE("IotcSleSendSsapsIndicate invalid param");
        return IOTC_ERROR;
    }

    SsapsNotifyParam indParam = {0};
    indParam.handle = param->handle;
    indParam.type = param->type;
    indParam.valueLen = param->valueLen;
    indParam.value = param->value;
    int32_t ret = NotifyIndicate(serverId, connectId, &indParam);
    if (ret != IOTC_OK) {
        IOTC_LOGE("IotcSleSendSsapsIndicate ret=%d", ret);
        return ret;
    }
    return IOTC_OK;
}

uint8_t IotcSleSendSsapsIndicateByUuid(
    uint8_t serverId,
    uint16_t connectId,
    const IotcAdptSleSendIndicateByUuidParam *param
)
{
    if ((param == NULL) || (param->value == NULL) || (param->valueLen == 0)) {
        IOTC_LOGE("IotcSleSendSsapsIndicate invalid param");
        return IOTC_ERROR;
    }

    SsapsNotyfyByUuidParam indParam = {0};
    indParam.startHandle = param->startHandle;
    indParam.endHandle = param->endHandle;
    indParam.type = param->type;
    indParam.valueLen = param->valueLen;
    indParam.value = param->value;
    SleUuid uuid = {0};
    uuid.len = param->uuid.len;
    if (memcpy_s(uuid.uuid, sizeof(uuid.uuid), param->uuid.uuid, param->uuid.len) != EOK) {
        IOTC_LOGE("IotcSsapsAddProperty memcpy_s failed: UUID data copy error");
        return IOTC_ERROR;
    }
    indParam.uuid = uuid;
    int32_t ret = NotifyIndicateByUuid(serverId, connectId, &indParam);
    if (ret != IOTC_OK) {
        IOTC_LOGE("IotcSleSendSsapsIndicateByUuid ret=%d", ret);
        return ret;
    }
    return IOTC_OK;
}

int32_t IotcSleSendSsapsResponse(uint8_t serverId, uint16_t connectId, const IotcAdptSleResponseParam *param)
{
    (void)param;
    return IOTC_OK;
}

static void ReadRequestCb(int32_t errCode, uint8_t serverId, uint16_t connectId, const SsapsReqReadCbParam *readCbPara)
{
    IOTC_LOGD("SLE  ReadRequestCb:errCode:%d,serverId:%d,conn_id=%d", errCode, serverId, connectId);
    if (readCbPara == NULL) {
        IOTC_LOGE("SLE  ReadRequestCb readCbPara null");
        return;
    }
    IotcAdptSleSsapEventParam eventParam;
    eventParam.reqRead.serverId = serverId;
    eventParam.reqRead.connectId = connectId;
    eventParam.reqRead.requestId = readCbPara->requestId;
    eventParam.reqRead.handle = readCbPara->handle;
    eventParam.reqRead.type = readCbPara->type;
    eventParam.reqRead.needRsp = readCbPara->needRsp;
    eventParam.reqRead.needAuthorize = readCbPara ->needAuthorize;
    if (g_gattEventHandler != NULL &&
        g_gattEventHandler(IOTC_ADPT_SLE_SSAP_EVENT_REQ_READ, &eventParam) != IOTC_OK) {
        IOTC_LOGE("SLE WriteRequestCb handle error");
    }
}

static void WriteRequestCb(
    int32_t errCode,
    uint8_t serverId,
    uint16_t connectId,
    const SsapsReqWriteCbPara *writeCbPara)
{
    IOTC_LOGD("SLE  WriteRequestCb:errCode:%d,serverId:%d,conn_id=%d", errCode, serverId, connectId);
    if (writeCbPara == NULL) {
        IOTC_LOGE("SLE  ReadRequestCb writeCbPara null");
        return;
    }
    IotcAdptSleSsapEventParam eventParam;
    eventParam.reqWrite.serverId = serverId;
    eventParam.reqWrite.connectId = connectId;
    eventParam.reqWrite.requestId = writeCbPara->requestId;
    eventParam.reqWrite.handle = writeCbPara->handle;
    eventParam.reqWrite.type = writeCbPara->type;
    eventParam.reqWrite.needRsp = writeCbPara->needRsp;
    eventParam.reqWrite.needAuthorize = writeCbPara ->needAuthorize;
    eventParam.reqWrite.valueLen = writeCbPara ->valueLen;
    eventParam.reqWrite.value = writeCbPara ->value;
    if (g_gattEventHandler != NULL &&
      g_gattEventHandler(IOTC_ADPT_SLE_SSAP_EVENT_REQ_WRITE, &eventParam) != IOTC_OK) {
        IOTC_LOGE("SLE WriteRequestCb handle error");
    }
}

static void MtuChangeCb(int32_t errCode, uint8_t serverId, uint16_t connectId, const SsapMtuInfo *mtuInfo)
{
    IOTC_LOGD("SLE  MtuChangeCb:errCode:%d,serverId:%d,conn_id=%d", errCode, serverId, connectId);
    if (mtuInfo == NULL) {
        IOTC_LOGE("SLE  MtuChangeCb mtuInfo null");
        return;
    }
    IotcAdptSleSsapEventParam eventParam;
    eventParam.setMtu.serverId = serverId;
    eventParam.setMtu.connectId = connectId;
    eventParam.setMtu.version = mtuInfo->version;
    eventParam.setMtu.mtuSize = mtuInfo->mtuSize;
    if (g_gattEventHandler != NULL &&
        g_gattEventHandler(IOTC_ADPT_SLE_SSAP_EVENT_SET_MTU_RESULT, &eventParam) != IOTC_OK) {
        IOTC_LOGE("SLE MtuChangeCb handle error");
    }
}

static SleSsapsCallbacks g_sleSsapsCb = {
    .OnRequestReadCb = ReadRequestCb,
    .OnRequestWriteCb = WriteRequestCb,
    .OnChangeMtuCb = MtuChangeCb,
};

uint8_t IotcSleSsapsRegisterServer(const IotcAdptSleSsapCallback callback)
{
    g_gattEventHandler = callback;
    int32_t ret = RegisterSsapServerCallbacks(&g_sleSsapsCb);
    if (ret != IOTC_OK) {
        IOTC_LOGE("IotcSleSsapsRegisterServer ret =%d", ret);
        return ret;
    }
    return IOTC_OK;
}

uint8_t IotcSleSsapsUnregisterServer(const IotcAdptSleSsapCallback callback)
{
    g_gattEventHandler = callback;
    int32_t ret = UnregisterSsapServerCallbacks(&g_sleSsapsCb);
    if (ret != IOTC_OK) {
        IOTC_LOGE("IotcSleSsapsUnregisterServer ret =%d", ret);
        return ret;
    }
    return IOTC_OK;
}

uint8_t IotcSsapsAddService(uint8_t serviceId, IotcSleUuidAddr *serviceUuid, bool isPrimary, uint16_t *handle)
{
    IOTC_LOGD("SLE  IotcSsapsAddService:serverId:%d,handle=%d", serviceId, handle);
    if ((serviceUuid == NULL)  || serviceUuid->len == 0 || serviceUuid->len > UUID_LEN || (handle == NULL)) {
        IOTC_LOGE("IotcSsapsAddService invalid param");
        return IOTC_ERROR;
    }
    SleUuid *uuid = (SleUuid *)malloc(sizeof(SleUuid));
    if (uuid == NULL) {
        IOTC_LOGE("IotcSsapsAddService malloc failed");
        return IOTC_ERROR;
    }
    uuid->len = serviceUuid->len;
    // 复制 addr 字段
    if (memcpy_s(uuid->uuid, uuid->len, serviceUuid->uuid, serviceUuid->len) != EOK) {
        IOTC_LOGE(" IotcSsapsAddService memcpy_s  uuid fail");
        free(uuid);
        return IOTC_ERROR;
    }

    int32_t ret = AddService(serviceId, uuid, isPrimary, handle);
    if (ret != IOTC_OK) {
        free(uuid);
        IOTC_LOGE("IotcSsapsAddService ret=%d", ret);
        return ret;
    }
    free(uuid);
    return IOTC_OK;
}

uint8_t IotcSsapsDeleteAllServices(uint8_t serviceId)
{
    int32_t ret =  RemoveAllServices(serviceId);
    if (ret != IOTC_OK) {
        IOTC_LOGE("IotcSsapsDeleteAllServices ret=%d", ret);
        return ret;
    }
    return IOTC_OK;
}

uint8_t IotcSsapsRemoveSsapServer(uint8_t serverId)
{
    int32_t ret =  RemoveSsapServer(serverId);
    if (ret != IOTC_OK) {
        IOTC_LOGE("IotcSsapsRemoveSsapServer ret=%d", ret);
        return ret;
    }
    return IOTC_OK;
}

uint8_t IotcAddSsapSetServerMtuInfo(uint8_t serverId, const IotcAdptSleMtuInfo *mtuInfo)
{
    SsapMtuInfo  info = {0};
    info.mtuSize = mtuInfo->mtuSize;
    info.version = mtuInfo->version;
    int32_t ret =  SetServerMtuInfo(serverId, &info);
    if (ret != IOTC_OK) {
        IOTC_LOGE("IotcAddSsapSetServerMtuInfo ret=%d", ret);
        return ret;
    }
    return IOTC_OK;
}

static void SleConnectStateChangedCallback(uint16_t connectId, const SleDeviceAddress *addr,
    SleConnectState connState, SlePairState pairState, SleDisConnectReason discReason)
{
    if (addr == NULL) {
        IOTC_LOGE("invalid param");
        return;
    }

    IOTC_LOGI("SLE ConnectStateChanged cb:conn_id:%d,conn_state:%d,pair_state:%d,disc_reason:%d",
        connectId, connState, pairState, discReason);
    IotcAdptSleConnectionEventParam param = {0};
    (void)memset_s(&param, sizeof(param), 0, sizeof(IotcAdptSleConnectionEventParam));
    param.sleConnectStateChanged.conn_id = connectId;
    param.sleConnectStateChanged.addr.type = addr->addrType;
    (void)memcpy_s(param.sleConnectStateChanged.addr.addr, IOTC_ADPT_SLE_ADDR_LEN, addr->addr, sizeof(addr->addr));
    
    param.sleConnectStateChanged.conn_state = (IotcAdptSleAcbState)connState;
    param.sleConnectStateChanged.pair_state = (IotcAdptSlePairState)pairState;
    param.sleConnectStateChanged.disc_reason = (IotcAdptSleDiscReason)discReason;
    if (g_sleConnectionEventHandler != NULL) {
        int32_t ret = g_sleConnectionEventHandler(IOTC_ADPT_SLE_CONNECT_STATE_CHANGED_EVENT, &param);
        IOTC_LOGD("SLE ConnectStateChanged cb:ret:%d", ret);
    }
}

static void SleConnectParamUpdateCB(uint16_t connectId, SleErrorCode errCode,
    const SleConnectionParamUpdateEvt *connParam)
{
    IOTC_LOGI("SLE SleConnectParamUpdateCB cb:errCode: %d", errCode);
}

static SleConnectCallbacks g_SleConnectionCb = {
    .OnSleConnStateChangeCb = SleConnectStateChangedCallback,
    .OnSleConnParamUpdatedCb = SleConnectParamUpdateCB
};

int32_t IotcSleRegisterConnectionCallbacks(const IotcAdptSleConnectionCallback callback)
{
    g_sleConnectionEventHandler = callback;
    int32_t ret = InitSleConnectService();
    if (ret != IOTC_OK) {
        IOTC_LOGE("init connection  ret=%d", ret);
        return ret;
    }
    ret = RegisterConnectCallbacks(&g_SleConnectionCb);
    if (ret != IOTC_OK) {
        IOTC_LOGE("register connection callback ret=%d", ret);
        return ret;
    }
    return IOTC_OK;
}

uint8_t IotcDeinitSleSsapsService(void)
{
    int32_t ret =  DeinitSleSsapsService();
    if (ret != IOTC_OK) {
        IOTC_LOGE("deinit sle host service ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}