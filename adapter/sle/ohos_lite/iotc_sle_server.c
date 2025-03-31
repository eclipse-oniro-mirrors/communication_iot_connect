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

uint8_t IotcSsapsAddProperty(uint8_t serviceId, uint16_t serviceHandle, IotcAdptSleSsapsPropertyInfo *property, uint16_t *handle)
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
        IOTC_LOGE("IotcAddSsapServer ret=%d", ret);
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
    uuid->len = appUuid->len;
     if (memcpy_s(uuid->uuid, sizeof(uuid->uuid), appUuid->uuid, appUuid->len) != EOK) {
        IOTC_LOGE("IotcAddSsapServer memcpy_s failed: UUID data copy error");
        return IOTC_ERROR;
    }
    int32_t ret = AddSsapServer(uuid,serverId);
    if (ret != IOTC_OK) {
        IOTC_LOGE("IotcAddSsapServer ret=%d", ret);
        return ret;
    }
    free(uuid);
    return IOTC_OK;
}

uint8_t IotcSleSsapsStartService(uint8_t serviceId, uint16_t serviceHandle)
{
    IOTC_LOGD("SLE  IotcSleStartSsapsService:serverId:%d,handle=%d",serviceId,serviceHandle);
    int32_t ret = StartService(serviceId, serviceHandle);
    if (ret != IOTC_OK) {
        IOTC_LOGE("SsapsStartService ret=%d", ret);
        return ret;
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
    uuid.len = &param->uuid.len;
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

uint8_t IotcSleSendSsapsResponse(uint8_t serverId, uint16_t connectId, const IotcAdptSleResponseParam *param)
{
    if ((param == NULL)) {
       IOTC_LOGE("IotcSleSendSsapsResponse invalid param");
        return IOTC_ERROR;
    }
    SsapsSendRspParam resParam = {0};
    resParam.requestId = param->requestId;
    resParam.status  = param->status;
    resParam.valueLen = param->valueLen;
    resParam.value = param->value;
    uint8_t ret = SendResponse(serverId, connectId, param);
    if(ret != IOTC_OK) {
        IOTC_LOGE("IotcSleSendSsapsResponse ret=%d", ret);
        return ret;
    }
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

static void WriteRequestCb(int32_t errCode, uint8_t serverId, uint16_t connectId, const SsapsReqWriteCbPara *writeCbPara)
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
       IOTC_LOGD("SLE  IotcSsapsAddService:serverId:%d,handle=%d",serviceId,handle);
    if ((serviceUuid == NULL)  || serviceUuid->len == 0 || serviceUuid->len > UUID_LEN || (handle == NULL)) {
        IOTC_LOGE("IotcSsapsAddService invalid param");
        return IOTC_ERROR;
    }
    SleUuid *uuid = (SleUuid *)malloc(sizeof(SleUuid));
    uuid->len = serviceUuid->len;
    // 复制 addr 字段
    if (memcpy_s(uuid->uuid, uuid->len, serviceUuid->uuid, serviceUuid->len) != EOK) {
        IOTC_LOGE(" IotcSsapsAddService memcpy_s  uuid fail");
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

static void connectStateCB(uint16_t connectId, const SleDeviceAddress *addr, SleConnectState connState, SlePairState pairState, SleDisConnectReason discReason)
{
    IOTC_LOGD("SLE  connectStateCB:conn_id:%d,connState:%d", connectId, connState);
}

static void connectParamUpdateCB(uint16_t connectId, SleErrorCode errCode, const SleConnectionParamUpdateEvt *connParam)
{
    IOTC_LOGD("SLE  connectParamUpdateCB:conn_id:%d,code:%d", connectId, errCode);
}

static SleConnectCallbacks g_SleConnectionCb = {
    .OnSleConnStateChangeCb = connectStateCB,
    .OnSleConnParamUpdatedCb = connectParamUpdateCB
};

int32_t IotcSleRegisterConnectionCallbacks(const IotcAdptSleConnectionCallback callback)
{

    g_sleConnectionEventHandler = callback;
    int32_t ret = RegisterConnectCallbacks(&g_SleConnectionCb);
    if (ret != IOTC_ADPT_SLE_STATUS_SUCCESS) {
        IOTC_LOGE("register connection callback ret=%d", ret);
        return IOTC_ERROR;
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
