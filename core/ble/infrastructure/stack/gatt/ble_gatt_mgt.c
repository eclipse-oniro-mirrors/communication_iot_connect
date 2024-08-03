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
#include "ble_gatt_mgt.h"
#include "securec.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "adapter_mem.h"
#include "utils_assert.h"
#include "utils_list.h"
#include "utils_mutex_global.h"
#include "ble_sched_event.h"
#include "utils_common.h"
#include "ble_gatt_event.h"

static BleGattMgtApp g_bleGattApp = {
    .peerDevInfo = NULL,
    .connNum = 0,
    .svcNum = 0,
    .svc = NULL,
    .startedSvcNum = 0,
};

typedef struct {
    const IotcBleGattProfileSvc *svc;
    ListEntry list;
} BleGattSvcList;

static uint32_t g_gattSvcListNum = 0;
static ListEntry g_gattSvcListHead = LIST_DECLARE_INIT(&g_gattSvcListHead);
static bool g_isBlePair = true;

static ListEntry *GetBleGattSvcListHead(void)
{
    return &g_gattSvcListHead;
}

static uint32_t GetBleGattSvcListNum(void)
{
    return g_gattSvcListNum;
}

void BleSetPair(bool isBlePair)
{
    g_isBlePair = isBlePair;
}

bool BleIsPair(void)
{
    return g_isBlePair;
}

static void PrintBleGattCharDescList(AdapterBleGattCharDesc *desc, uint8_t num)
{
    (void)desc;
    (void)num;
    for (uint8_t i = 0; i < num; i++) {
        IOTC_LOGI("desc[%u|%u]:descHandle:%u,uuid:%s,permission:%u",
            num, i, desc[i].descHandle, desc[i].uuid, desc[i].permission);
    }
}

static void PrintBleGattsCharList(AdapterBleGattsChar *character, uint8_t num)
{
    (void)character;
    (void)num;
    for (uint8_t i = 0; i < num; i++) {
        IOTC_LOGI("char[%u|%u]:charHandle:%u,uuid:%s,permission:%u,property:%u,descNum:%u",
            num, i, character[i].charHandle, character[i].uuid,
            character[i].permission, character[i].property, character[i].descNum);
        PrintBleGattCharDescList(character[i].desc, character[i].descNum);
    }
}

void PrintBleGattServiceList(AdapterBleGattService *svc, uint8_t num)
{
    (void)svc;
    (void)num;
    for (uint8_t i = 0; i < num; i++) {
        IOTC_LOGI("svc[%u|%u]:svcHandle:%u,uuid:%s,charNum:%u",
            num, i, svc[i].svcHandle, svc[i].uuid, svc[i].charNum);
        PrintBleGattsCharList(svc[i].character, svc[i].charNum);
    }
}

int32_t BleAddGattSvc(const IotcBleGattProfileSvc *svc)
{
    CHECK_RETURN_LOGW(svc != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");
    BleGattSvcList *newNode = (BleGattSvcList *)AdapterMalloc(sizeof(BleGattSvcList));
    if (newNode == NULL) {
        IOTC_LOGW("malloc error");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(newNode, sizeof(BleGattSvcList), 0, sizeof(BleGattSvcList));
    newNode->svc = svc;

    (void)UtilsGlobalMutexLock();
    LIST_INSERT_BEFORE(&newNode->list, GetBleGattSvcListHead());
    g_gattSvcListNum++;
    UtilsGlobalMutexUnlock();
    return IOTC_OK;
}

static void DestroyBleGattSvcList(void)
{
    (void)UtilsGlobalMutexLock();
    ListEntry *item;
    ListEntry *next;
    LIST_FOR_EACH_ITEM_SAFE(item, next, GetBleGattSvcListHead()) {
        BleGattSvcList *node = CONTAINER_OF(item, BleGattSvcList, list);
        LIST_REMOVE(&node->list);
        AdapterFree(node);
    }
    g_gattSvcListNum = 0;
    UtilsGlobalMutexUnlock();
}

static uint32_t ProfilePermissionToAdapterPermission(uint32_t permission)
{
    uint32_t toPermission = 0;
    if (permission & IOTC_BLE_GATT_PERMISSION_READ) {
        toPermission |= ADAPTER_BLE_CHAR_PERM_READ;
    }
    if (permission & IOTC_BLE_GATT_PERMISSION_READ_ENCRYPTED) {
        toPermission |= ADAPTER_BLE_CHAR_PERM_READ_ENCRYPTED;
    }
    if (permission & IOTC_BLE_GATT_PERMISSION_READ_ENCRYPTED_MITM) {
        toPermission |= ADAPTER_BLE_CHAR_PERM_READ_ENCRYPTED_MITM;
    }
    if (permission & IOTC_BLE_GATT_PERMISSION_WRITE) {
        toPermission |= ADAPTER_BLE_CHAR_PERM_WRITE;
    }
    if (permission & IOTC_BLE_GATT_PERMISSION_WRITE_ENCRYPTED) {
        toPermission |= ADAPTER_BLE_CHAR_PERM_WRITE_ENCRYPTED;
    }
    if (permission & IOTC_BLE_GATT_PERMISSION_WRITE_ENCRYPTED_MITM) {
        toPermission |= ADAPTER_BLE_CHAR_PERM_WRITE_ENCRYPTED_MITM;
    }
    if (permission & IOTC_BLE_GATT_PERMISSION_WRITE_SIGNED) {
        toPermission |= ADAPTER_BLE_CHAR_PERM_WRITE_SIGNED;
    }
    if (permission & IOTC_BLE_GATT_PERMISSION_WRITE_SIGNED_MITM) {
        toPermission |= ADAPTER_BLE_CHAR_PERM_WRITE_SIGNED_MITM;
    }
    return toPermission;
}

static uint32_t ProfilePropertyToAdapterProperty(uint32_t property)
{
    uint32_t toProperty = 0;
    if (property & IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_BROADCAST) {
        toProperty |= ADAPTER_BLE_CHAR_PROP_BROADCAST;
    }
    if (property & IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_READ) {
        toProperty |= ADAPTER_BLE_CHAR_PROP_READ;
    }
    if (property & IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_WRITE_NO_RSP) {
        toProperty |= ADAPTER_BLE_CHAR_PROP_WRITE_WITHOUT_RESP;
    }
    if (property & IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_WRITE) {
        toProperty |= ADAPTER_BLE_CHAR_PROP_WRITE;
    }
    if (property & IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_NOTIFY) {
        toProperty |= ADAPTER_BLE_CHAR_PROP_NOTIFY;
    }
    if (property & IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_INDICATE) {
        toProperty |= ADAPTER_BLE_CHAR_PROP_INDICATE;
    }
    if (property & IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_SIGNED_WRITE) {
        toProperty |= ADAPTER_BLE_CHAR_PROP_SIGNED_WRITE;
    }
    if (property & IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_EXTENDED_PROPERTY) {
        toProperty |= ADAPTER_BLE_CHAR_PROP_EXTENDED_PROPERTY;
    }
    return toProperty;
}

static int32_t ProfileCharCopyToAdapterChar(const IotcBleGattProfileChar *in, AdapterBleGattsChar *to)
{
    CHECK_RETURN_LOGW(in->uuid != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");
    to->uuid = in->uuid;
    to->permission = ProfilePermissionToAdapterPermission(in->permission);
    to->property = ProfilePropertyToAdapterProperty(in->property);
    to->readFunc = in->readFunc;
    to->writeFunc = in->writeFunc;
    to->indicateFunc = in->indicateFunc;
    to->descNum = in->descNum;
    if (in->descNum == 0) {
        return IOTC_OK;
    }
    uint32_t descMallocSize = in->descNum * sizeof(AdapterBleGattCharDesc);
    AdapterBleGattCharDesc *desc = (AdapterBleGattCharDesc *)AdapterMalloc(descMallocSize);
    if (desc == NULL) {
        IOTC_LOGE("malloc");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(desc, descMallocSize, 0, descMallocSize);
    for (uint8_t i = 0; i < in->descNum; i++) {
        desc[i].uuid = in->desc[i].uuid;
        desc[i].permission = ProfilePermissionToAdapterPermission(in->desc[i].permission);
        desc[i].readFunc = in->desc[i].readFunc;
        desc[i].writeFunc = in->desc[i].writeFunc;
    }
    to->desc = desc;
    return IOTC_OK;
}

static int32_t ProfileSvcCopyToAdapterSvc(const IotcBleGattProfileSvc *in, AdapterBleGattService *to)
{
    CHECK_RETURN_LOGW((in->uuid != NULL) && (in->charNum != 0) && (in->character != NULL),
        IOTC_ERR_PARAM_INVALID, "invalid param");
    uint32_t charMallocSize = in->charNum * sizeof(AdapterBleGattsChar);
    AdapterBleGattsChar *character = (AdapterBleGattsChar *)AdapterMalloc(charMallocSize);
    if (character == NULL) {
        IOTC_LOGE("malloc");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(character, charMallocSize, 0, charMallocSize);
    for (uint8_t i = 0; i < in->charNum; i++) {
        int32_t ret = ProfileCharCopyToAdapterChar(&in->character[i], &character[i]);
        if (ret != IOTC_OK) {
            IOTC_LOGE("copy ret=%d", ret);
            AdapterFree(character);
            return ret;
        }
    }

    to->uuid = in->uuid;
    to->character = character;
    to->charNum = in->charNum;
    return IOTC_OK;
}

static void GattServiceDestroy(AdapterBleGattService **svcAddr, uint8_t svcNum)
{
    if ((svcAddr == NULL) || (*svcAddr == NULL) || (svcNum == 0)) {
        return;
    }
    AdapterBleGattService *svc = *svcAddr;
    for (uint8_t i = 0; i < svcNum; i++) {
        if (svc[i].character == NULL) {
            continue;
        }
        for (uint8_t j = 0; j < svc[i].charNum; j++) {
            if (svc[i].character[j].desc == NULL) {
                continue;
            }
            AdapterFree(svc[i].character[j].desc);
            svc[i].character[j].desc = NULL;
            svc[i].character[j].descNum = 0;
        }
        AdapterFree(svc[i].character);
        svc[i].character = NULL;
        svc[i].charNum = 0;
    }
    AdapterFree(svc);
    *svcAddr = NULL;
}

static int32_t BleGattProfileSvcInit(void)
{
    if (GetBleGattSvcListNum() == 0) {
        IOTC_LOGE("gatt svc no init");
        return IOTC_ERR_NOT_INIT;
    }

    uint32_t svcNum = GetBleGattSvcListNum();
    AdapterBleGattService *svc = (AdapterBleGattService *)AdapterCalloc(svcNum, sizeof(AdapterBleGattService));
    if (svc == NULL) {
        IOTC_LOGE("calloc error %u", svcNum);
        return IOTC_ADAPTER_MEM_ERR_CALLOC;
    }

    (void)UtilsGlobalMutexLock();

    uint32_t index = 0;
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, GetBleGattSvcListHead()) {
        BleGattSvcList *node = CONTAINER_OF(item, BleGattSvcList, list);
        int32_t ret = ProfileSvcCopyToAdapterSvc(node->svc, &svc[index]);
        if (ret != IOTC_OK) {
            IOTC_LOGW("self copy ret=%d", ret);
            GattServiceDestroy(&svc, svcNum);
            UtilsGlobalMutexUnlock();
            return ret;
        }
        index++;
    }

    UtilsGlobalMutexUnlock();
    g_bleGattApp.svc = svc;
    g_bleGattApp.svcNum = svcNum;
    return IOTC_OK;
}

static int32_t BleGattPeerDevInfoInit(void)
{
    uint32_t mallocSize = BLE_DEFAULT_MAX_CONN_NUM * sizeof(BlePeerDevInfo);
    BlePeerDevInfo *peerDevInfo = (BlePeerDevInfo *)AdapterMalloc(mallocSize);
    if (peerDevInfo == NULL) {
        IOTC_LOGE("malloc");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(peerDevInfo, mallocSize, 0, mallocSize);
    GetBleGattMgtApp()->peerDevInfo = peerDevInfo;
    return IOTC_OK;
}

int32_t BleGattMgtInit(void)
{
    int32_t ret = BleGattProfileSvcInit();
    if (ret != IOTC_OK) {
        IOTC_LOGE("gatt profile init err ret=%d", ret);
        return ret;
    }
    PrintBleGattServiceList(g_bleGattApp.svc, g_bleGattApp.svcNum);
    ret = BleGattEventInit();
    if (ret != IOTC_OK) {
        IOTC_LOGE("gatt event init err ret=%d", ret);
        return ret;
    }
    ret = BleGattPeerDevInfoInit();
    if (ret != IOTC_OK) {
        IOTC_LOGE("gatt peer dev info init err ret=%d", ret);
        return ret;
    }
    ret = AdapterBleStartGattsService(GetBleGattMgtApp()->svc, GetBleGattMgtApp()->svcNum);
    if (ret != IOTC_OK) {
        IOTC_LOGE("add gatt svc ret=%d", ret);
        return ret;
    }
    return IOTC_OK;
}

void BleGattMgtDestroy(void)
{
    GattServiceDestroy(&g_bleGattApp.svc, g_bleGattApp.svcNum);
    g_bleGattApp.svcNum = 0;
    if (GetBleGattMgtApp()->peerDevInfo != NULL) {
        AdapterFree(GetBleGattMgtApp()->peerDevInfo);
        GetBleGattMgtApp()->peerDevInfo = NULL;
    }
    DestroyBleGattSvcList();
}

BleGattMgtApp *GetBleGattMgtApp(void)
{
    return &g_bleGattApp;
}

static int32_t GetSeviceHandle(const char *svcUuid, int32_t *handle)
{
    for (uint8_t i = 0; i < GetBleGattMgtApp()->svcNum; i++) {
        if (strcmp(svcUuid, GetBleGattMgtApp()->svc[i].uuid) != 0) {
            continue;
        }
        *handle = GetBleGattMgtApp()->svc[i].svcHandle;
        return IOTC_OK;
    }
    IOTC_LOGE("no find svc");
    return IOTC_CORE_BLE_INVALID_SVC_UUID;
}

static int32_t GetCharHandle(const char *svcUuid, const char *charUuid, int32_t *handle)
{
    for (uint8_t i = 0; i < GetBleGattMgtApp()->svcNum; i++) {
        if (strcmp(svcUuid, GetBleGattMgtApp()->svc[i].uuid) != 0) {
            continue;
        }
        for (uint8_t j = 0; j < GetBleGattMgtApp()->svc[i].charNum; j++) {
            if (strcmp(charUuid, GetBleGattMgtApp()->svc[i].character[j].uuid) != 0) {
                continue;
            }
            *handle = GetBleGattMgtApp()->svc[i].character[j].charHandle;
            return IOTC_OK;
        }
    }
    IOTC_LOGE("no find char");
    return IOTC_CORE_BLE_INVALID_CHAR_UUID;
}

static int32_t GetCharProperty(const char *svcUuid, const char *charUuid, uint32_t *property)
{
    for (uint8_t i = 0; i < GetBleGattMgtApp()->svcNum; i++) {
        if (strcmp(svcUuid, GetBleGattMgtApp()->svc[i].uuid) != 0) {
            continue;
        }
        for (uint8_t j = 0; j < GetBleGattMgtApp()->svc[i].charNum; j++) {
            if (strcmp(charUuid, GetBleGattMgtApp()->svc[i].character[j].uuid) != 0) {
                continue;
            }
            *property = GetBleGattMgtApp()->svc[i].character[j].property;
            return IOTC_OK;
        }
    }
    IOTC_LOGE("no find char");
    return IOTC_CORE_BLE_INVALID_CHAR_UUID;
}

static bool IsAttrHandleInCharacterTbl(int32_t attrHandle, AdapterBleGattsChar *character, uint8_t charNum)
{
    for (uint8_t i = 0; i < charNum; i++) {
        if (character[i].charHandle == attrHandle) {
            return true;
        }
        for (uint8_t j = 0; j < character[i].descNum; j++) {
            if (character[i].desc[j].descHandle == attrHandle) {
                return true;
            }
        }
    }
    return false;
}

static int32_t GetAttrHandleServerId(int32_t attrHandle, int32_t *serverId)
{
    for (uint8_t i = 0; i < GetBleGattMgtApp()->svcNum; i++) {
        if (IsAttrHandleInCharacterTbl(attrHandle,
            GetBleGattMgtApp()->svc[i].character, GetBleGattMgtApp()->svc[i].charNum)) {
            *serverId =  GetBleGattMgtApp()->svc[i].serverId;
            return IOTC_OK;
        }
    }
    return IOTC_ERROR;
}

static AdapterBleGattReadFunc FindReadFuncFromCharacterTbl(int32_t attrHandle,
    AdapterBleGattsChar *character, uint8_t charNum)
{
    for (uint8_t i = 0; i < charNum; i++) {
        if (character[i].charHandle == attrHandle) {
            return character[i].readFunc;
        }
        for (uint8_t j = 0; j < character[i].descNum; j++) {
            if (character[i].desc[j].descHandle == attrHandle) {
                return character[i].desc[j].readFunc;
            }
        }
    }
    return NULL;
}

static AdapterBleGattReadFunc FindAttrHandleReadFunc(int32_t attrHandle)
{
    AdapterBleGattReadFunc res = NULL;
    for (uint8_t i = 0; i < GetBleGattMgtApp()->svcNum; i++) {
        res = FindReadFuncFromCharacterTbl(attrHandle,
            GetBleGattMgtApp()->svc[i].character, GetBleGattMgtApp()->svc[i].charNum);
        if (res != NULL) {
            return res;
        }
    }
    return NULL;
}

static AdapterBleGattWriteFunc FindWriteFuncFromCharacterTbl(int32_t attrHandle,
    AdapterBleGattsChar *character, uint8_t charNum)
{
    for (uint8_t i = 0; i < charNum; i++) {
        if (character[i].charHandle == attrHandle) {
            return character[i].writeFunc;
        }
        for (uint8_t j = 0; j < character[i].descNum; j++) {
            if (character[i].desc[j].descHandle == attrHandle) {
                return character[i].desc[j].writeFunc;
            }
        }
    }
    return NULL;
}

static AdapterBleGattWriteFunc FindAttrHandleWriteFunc(int32_t attrHandle)
{
    AdapterBleGattWriteFunc res = NULL;
    for (uint8_t i = 0; i < GetBleGattMgtApp()->svcNum; i++) {
        res = FindWriteFuncFromCharacterTbl(attrHandle,
            GetBleGattMgtApp()->svc[i].character, GetBleGattMgtApp()->svc[i].charNum);
        if (res != NULL) {
            return res;
        }
    }
    return NULL;
}

int32_t BleSendIndicateDataInner(const char *svcUuid, const char *charUuid, const uint8_t *value, uint32_t valueLen)
{
    CHECK_RETURN_LOGW((svcUuid != NULL) && (charUuid != NULL) &&  (value != NULL) && (valueLen != 0),
        IOTC_ERR_PARAM_INVALID, "invalid param");
    if ((GetBleGattMgtApp()->connNum == 0) || (GetBleGattMgtApp()->peerDevInfo == NULL)) {
        IOTC_LOGE("no connect");
        return IOTC_CORE_BLE_NO_CONNECT;
    }
    AdapterBleSendIndicateParam param;
    (void)memset_s(&param, sizeof(param), 0, sizeof(param));
    uint32_t property = 0;
    int32_t ret = GetSeviceHandle(svcUuid, &param.svcHandle);
    if (ret != IOTC_OK) {
        IOTC_LOGE("get svc handle err ret=%d", ret);
        return ret;
    }
    ret = GetCharHandle(svcUuid, charUuid, &param.charHandle);
    if (ret != IOTC_OK) {
        IOTC_LOGE("get char handle err ret=%d", ret);
        return ret;
    }
    ret = GetCharProperty(svcUuid, charUuid, &property);
    if (ret != IOTC_OK) {
        IOTC_LOGE("get char property err ret=%d", ret);
        return ret;
    }
    ret = GetAttrHandleServerId(param.charHandle, &param.serverId);
    if (ret != IOTC_OK) {
        IOTC_LOGE("get server id err ret=%d", ret);
        return ret;
    }
    param.connId = GetBleGattMgtApp()->peerDevInfo[0].connId;
    param.needConfirm = ((property & IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_INDICATE) != 0) ? true : false;
    param.valueLen = valueLen;
    param.value = (uint8_t *)value;
    ret = AdapterBleSendGattsIndicate(&param);
    IOTC_LOGN("send indicate msg ret=%d svcHandle=%d,charHandle=%d,valueLen=%u",
        ret, param.svcHandle, param.charHandle, param.valueLen);
    return ret;
}

void BleGattDisconnectAll(void)
{
    BlePeerDevInfo *peerDevInfoList = GetBleGattMgtApp()->peerDevInfo;
    for (uint8_t i = 0; i < GetBleGattMgtApp()->connNum; i++) {
        int32_t ret = AdapterBleDissconnectGatt(peerDevInfoList[i].peerAddr,
            ADAPTER_BLE_ADDR_LEN);
        if (ret != IOTC_OK) {
            IOTC_LOGW("disconn with peer[%u] err", i);
        }
    }
}

int32_t SetBleConnectParam(void)
{
    AdapterBleConnectParam param = {0};
    param.isDiscover = true;
    param.isConnect = true;
    param.isBond = BleIsPair();
    return AdapterBleSetConnectParam(&param);
}

int32_t BleGattReqRead(int32_t connId, int32_t attrHandle, int32_t transId)
{
    if ((GetBleGattMgtApp()->connNum == 0) || (GetBleGattMgtApp()->peerDevInfo == NULL)) {
        IOTC_LOGE("no connect");
        return IOTC_CORE_BLE_NO_CONNECT;
    }
    AdapterBleResponseParam param;
    (void)memset_s(&param, sizeof(param), 0, sizeof(param));
    param.connectId = connId;
    param.transId = transId;
    int32_t ret = GetAttrHandleServerId(attrHandle, &param.serverId);
    if (ret != IOTC_OK) {
        IOTC_LOGE("get server id err ret=%d", ret);
        return ret;
    }
    param.value = AdapterCalloc(1, ADAPTER_BLE_GATT_READ_BUF_SIZE);
    if (param.value == NULL) {
        (void)AdapterBleSendGattsResponse(&param);
        return IOTC_ADAPTER_MEM_ERR_CALLOC;
    }
    AdapterBleGattReadFunc func = FindAttrHandleReadFunc(attrHandle);
    if (func == NULL) {
        IOTC_LOGE("no find read func");
        AdapterFree(param.value);
        param.value = NULL;
        (void)AdapterBleSendGattsResponse(&param);
        return IOTC_ERROR;
    }
    param.valueLen = ADAPTER_BLE_GATT_READ_BUF_SIZE;
    ret = func(param.value, (uint32_t *)&param.valueLen);
    if ((ret != IOTC_OK) || (param.valueLen == 0)) {
        IOTC_LOGE("read err ret=%d, len=%d", ret, param.valueLen);
        AdapterFree(param.value);
        param.value = NULL;
        param.valueLen = 0;
        (void)AdapterBleSendGattsResponse(&param);
        return IOTC_ERROR;
    }
    ret = AdapterBleSendGattsResponse(&param);
    if (ret != IOTC_OK) {
        IOTC_LOGE("response err ret=%d", ret);
        AdapterFree(param.value);
        param.value = NULL;
        return IOTC_ERROR;
    }
    AdapterFree(param.value);
    param.value = NULL;
    return IOTC_OK;
}

int32_t BleGattReqWrite(int32_t connId, int32_t attrHandle, int32_t transId, uint8_t *value, int32_t valueLen)
{
    if ((GetBleGattMgtApp()->connNum == 0) || (GetBleGattMgtApp()->peerDevInfo == NULL)) {
        IOTC_LOGE("no connect");
        return IOTC_CORE_BLE_NO_CONNECT;
    }

    AdapterBleResponseParam param;
    int32_t ret = GetAttrHandleServerId(attrHandle, &param.serverId);
    if (ret != IOTC_OK) {
        IOTC_LOGE("get server id err ret=%d", ret);
        return ret;
    }
    param.connectId = connId;
    param.transId = transId;
    param.value = NULL;
    param.valueLen = 0;
    (void)AdapterBleSendGattsResponse(&param);

    AdapterBleGattWriteFunc func = FindAttrHandleWriteFunc(attrHandle);
    if (func == NULL) {
        IOTC_LOGE("no find write func");
        return IOTC_ERROR;
    }
    ret = func(value, valueLen);
    if (ret != IOTC_OK) {
        IOTC_LOGE("write err ret=%d", ret);
        return IOTC_ERROR;
    }
    return IOTC_OK;
}