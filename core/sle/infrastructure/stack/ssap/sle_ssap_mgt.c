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
#include "sle_ssap_mgt.h"
#include "securec.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "iotc_mem.h"
#include "utils_assert.h"
#include "utils_list.h"
#include "utils_mutex_global.h"
#include "sle_sched_event.h"
#include "utils_common.h"
#include "sle_ssap_event.h"
#include "sle_disc_event.h"
#include "sle_conn_event.h"
#include "sle_ssapc_event.h"
#include "iotc_sle_client.h"
#include "iotc_sle_ext.h"

static SleSsapMgtApp g_SleSsapApp = {
    .peerDevInfo = NULL,
    .connNum = 0,
    .svcNum = 0,
    .svc = NULL,
    .startedSvcNum = 0,
};

typedef struct {
    const IotcSleSsapProfileSvc *svc;
    ListEntry list;
} SleSsapSvcList;

static uint32_t g_ssapSvcListNum = 0;
static ListEntry g_ssapSvcListHead = LIST_DECLARE_INIT(&g_ssapSvcListHead);
static bool g_isSlePair = true;

static ListEntry *GetSleSsapSvcListHead(void)
{
    return &g_ssapSvcListHead;
}

static uint32_t GetSleSsapSvcListNum(void)
{
    return g_ssapSvcListNum;
}

void SleSetPair(bool isSlePair)
{
    g_isSlePair = isSlePair;
}

bool SleIsPair(void)
{
    return g_isSlePair;
}

static void PrintSleSsapCharDescList(IotcAdptSleSsapCharDesc *desc, uint8_t num)
{
    (void)desc;
    (void)num;
    for (uint8_t i = 0; i < num; i++) {
        IOTC_LOGI("desc[%u|%u]:descHandle:%u,uuid:%s,permission:%u",
            num, i, desc[i].descHandle, desc[i].uuid, desc[i].permission);
    }
}

static void PrintSleSsapsCharList(IotcAdptSleSsapsChar *character, uint8_t num)
{
    (void)character;
    (void)num;
    for (uint8_t i = 0; i < num; i++) {
        IOTC_LOGI("char[%u|%u]:charHandle:%u,uuid:%s,permission:%u,property:%u,descNum:%u",
            num, i, character[i].charHandle, character[i].uuid,
            character[i].permission, character[i].property, character[i].descNum);
        PrintSleSsapCharDescList(character[i].desc, character[i].descNum);
    }
}

void PrintSleSsapServiceList(IotcAdptSleSsapService *svc, uint8_t num)
{
    (void)svc;
    (void)num;
    for (uint8_t i = 0; i < num; i++) {
        IOTC_LOGI("svc[%u|%u]:svcHandle:%u,uuid:%s,charNum:%u",
            num, i, svc[i].svcHandle, svc[i].uuid, svc[i].charNum);
        PrintSleSsapsCharList(svc[i].character, svc[i].charNum);
    }
}

int32_t SleAddSsapSvc(const IotcSleSsapProfileSvc *svc)
{
    CHECK_RETURN_LOGW(svc != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");
    SleSsapSvcList *newNode = (SleSsapSvcList *)IotcMalloc(sizeof(SleSsapSvcList));
    if (newNode == NULL) {
        IOTC_LOGW("malloc error");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(newNode, sizeof(SleSsapSvcList), 0, sizeof(SleSsapSvcList));
    newNode->svc = svc;

    (void)UtilsGlobalMutexLock();
    LIST_INSERT_BEFORE(&newNode->list, GetSleSsapSvcListHead());
    g_ssapSvcListNum++;
    UtilsGlobalMutexUnlock();
    return IOTC_OK;
}

static void DestroySleSsapSvcList(void)
{
    (void)UtilsGlobalMutexLock();
    ListEntry *item;
    ListEntry *next;
    LIST_FOR_EACH_ITEM_SAFE(item, next, GetSleSsapSvcListHead()) {
        SleSsapSvcList *node = CONTAINER_OF(item, SleSsapSvcList, list);
        LIST_REMOVE(&node->list);
        IotcFree(node);
    }
    g_ssapSvcListNum = 0;
    UtilsGlobalMutexUnlock();
}

static uint32_t ProfilePermissionToAdapterPermission(uint32_t permission)
{
    uint32_t toPermission = 0;
    if (permission & IOTC_SLE_SSAP_PERMISSION_READ) {
        toPermission |= IOTC_ADPT_SLE_CHAR_PERM_READ;
    }
    if (permission & IOTC_SLE_SSAP_PERMISSION_READ_ENCRYPTED) {
        toPermission |= IOTC_ADPT_SLE_CHAR_PERM_READ_ENCRYPTED;
    }
    if (permission & IOTC_SLE_SSAP_PERMISSION_READ_ENCRYPTED_MITM) {
        toPermission |= IOTC_ADPT_SLE_CHAR_PERM_READ_ENCRYPTED_MITM;
    }
    if (permission & IOTC_SLE_SSAP_PERMISSION_WRITE) {
        toPermission |= IOTC_ADPT_SLE_CHAR_PERM_WRITE;
    }
    if (permission & IOTC_SLE_SSAP_PERMISSION_WRITE_ENCRYPTED) {
        toPermission |= IOTC_ADPT_SLE_CHAR_PERM_WRITE_ENCRYPTED;
    }
    if (permission & IOTC_SLE_SSAP_PERMISSION_WRITE_ENCRYPTED_MITM) {
        toPermission |= IOTC_ADPT_SLE_CHAR_PERM_WRITE_ENCRYPTED_MITM;
    }
    if (permission & IOTC_SLE_SSAP_PERMISSION_WRITE_SIGNED) {
        toPermission |= IOTC_ADPT_SLE_CHAR_PERM_WRITE_SIGNED;
    }
    if (permission & IOTC_SLE_SSAP_PERMISSION_WRITE_SIGNED_MITM) {
        toPermission |= IOTC_ADPT_SLE_CHAR_PERM_WRITE_SIGNED_MITM;
    }
    return toPermission;
}

static uint32_t ProfilePropertyToAdapterProperty(uint32_t property)
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

static int32_t ProfileCharCopyToAdapterChar(const IotcSleSsapProfileChar *in, IotcAdptSleSsapsChar *to)
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
    uint32_t descMallocSize = in->descNum * sizeof(IotcAdptSleSsapCharDesc);
    IotcAdptSleSsapCharDesc *desc = (IotcAdptSleSsapCharDesc *)IotcMalloc(descMallocSize);
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

static int32_t ProfileSvcCopyToAdapterSvc(const IotcSleSsapProfileSvc *in, IotcAdptSleSsapService *to)
{
    CHECK_RETURN_LOGW((in->uuid != NULL) && (in->charNum != 0) && (in->character != NULL),
        IOTC_ERR_PARAM_INVALID, "invalid param");
    uint32_t charMallocSize = in->charNum * sizeof(IotcAdptSleSsapsChar);
    IotcAdptSleSsapsChar *character = (IotcAdptSleSsapsChar *)IotcMalloc(charMallocSize);
    if (character == NULL) {
        IOTC_LOGE("malloc");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(character, charMallocSize, 0, charMallocSize);
    for (uint8_t i = 0; i < in->charNum; i++) {
        int32_t ret = ProfileCharCopyToAdapterChar(&in->character[i], &character[i]);
        if (ret != IOTC_OK) {
            IOTC_LOGE("copy ret=%d", ret);
            IotcFree(character);
            return ret;
        }
    }

    to->uuid = in->uuid;
    to->character = character;
    to->charNum = in->charNum;
    return IOTC_OK;
}

static void SsapServiceDestroy(IotcAdptSleSsapService **svcAddr, uint8_t svcNum)
{
    if ((svcAddr == NULL) || (*svcAddr == NULL) || (svcNum == 0)) {
        return;
    }
    IotcAdptSleSsapService *svc = *svcAddr;
    for (uint8_t i = 0; i < svcNum; i++) {
        if (svc[i].character == NULL) {
            continue;
        }
        for (uint8_t j = 0; j < svc[i].charNum; j++) {
            if (svc[i].character[j].desc == NULL) {
                continue;
            }
            IotcFree(svc[i].character[j].desc);
            svc[i].character[j].desc = NULL;
            svc[i].character[j].descNum = 0;
        }
        IotcFree(svc[i].character);
        svc[i].character = NULL;
        svc[i].charNum = 0;
    }
    IotcFree(svc);
    *svcAddr = NULL;
}

static int32_t SleSsapProfileSvcInit(void)
{
    if (GetSleSsapSvcListNum() == 0) {
        IOTC_LOGE("ssap svc no init");
        return IOTC_ERR_NOT_INIT;
    }

    uint32_t svcNum = GetSleSsapSvcListNum();
    IotcAdptSleSsapService *svc = (IotcAdptSleSsapService *)IotcCalloc(svcNum, sizeof(IotcAdptSleSsapService));
    if (svc == NULL) {
        IOTC_LOGE("calloc error %u", svcNum);
        return IOTC_ADAPTER_MEM_ERR_CALLOC;
    }

    (void)UtilsGlobalMutexLock();

    uint32_t index = 0;
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, GetSleSsapSvcListHead()) {
        SleSsapSvcList *node = CONTAINER_OF(item, SleSsapSvcList, list);
        int32_t ret = ProfileSvcCopyToAdapterSvc(node->svc, &svc[index]);
        if (ret != IOTC_OK) {
            IOTC_LOGW("self copy ret=%d", ret);
            SsapServiceDestroy(&svc, svcNum);
            UtilsGlobalMutexUnlock();
            return ret;
        }
        index++;
    }

    UtilsGlobalMutexUnlock();
    g_SleSsapApp.svc = svc;
    g_SleSsapApp.svcNum = svcNum;
    return IOTC_OK;
}

static int32_t SleSsapPeerDevInfoInit(void)
{
    uint32_t mallocSize = SLE_DEFAULT_MAX_CONN_NUM * sizeof(SlePeerDevInfo);
    SlePeerDevInfo *peerDevInfo = (SlePeerDevInfo *)IotcMalloc(mallocSize);
    if (peerDevInfo == NULL) {
        IOTC_LOGE("malloc");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(peerDevInfo, mallocSize, 0, mallocSize);
    GetSleSsapMgtApp()->peerDevInfo = peerDevInfo;
    return IOTC_OK;
}

int32_t SleSsapMgtInit(void)
{
    int32_t ret = SleSsapProfileSvcInit();
    if (ret != IOTC_OK) {
        IOTC_LOGE("ssap profile init err ret=%d", ret);
        return ret;
    }
    PrintSleSsapServiceList(g_SleSsapApp.svc, g_SleSsapApp.svcNum);
    IOTC_LOGI(" ---> uuid:%s", g_SleSsapApp.svc[0].uuid);
    ret = IotcInitSleSsapsService();

    ret = IotcSleSsapsRegisterServer(NULL);
    IOTC_LOGI(" ---> init service :%d", ret);
    ret = SleSsapEventInit();
    if (ret != IOTC_OK) {
        IOTC_LOGE("ssap event init err ret=%d", ret);
        return ret;
    }
    ret = SleDiscEventInit();
    if (ret != IOTC_OK) {
        IOTC_LOGE("sle disc init err ret=%d", ret);
        return ret;
    }
    ret = SleConnectionEventInit();
    if (ret != IOTC_OK) {
        IOTC_LOGE("sle conn init err ret=%d", ret);
        return ret;
    }
       ret = SleSsapClinetEventInit();
    if (ret != IOTC_OK) {
        IOTC_LOGE("sle ssap client init err ret=%d", ret);
        return ret;
    }
    ret = SleSsapPeerDevInfoInit();
    if (ret != IOTC_OK) {
        IOTC_LOGE("ssap peer dev info init err ret=%d", ret);
    }
    IOTC_LOGI(" ---> start service:%s", g_SleSsapApp.svc[0].uuid);
    ret = IotcSleStartServiceEx(GetSleSsapMgtApp()->svc, GetSleSsapMgtApp()->svcNum);
    if (ret != IOTC_OK) {
        IOTC_LOGE("add sle svc ret=%d", ret);
        return ret;
    }
     IOTC_LOGI(" ---> start service success");
    return IOTC_OK;
}

void SleSsapMgtDestroy(void)
{
    SsapServiceDestroy(&g_SleSsapApp.svc, g_SleSsapApp.svcNum);
    g_SleSsapApp.svcNum = 0;
    if (GetSleSsapMgtApp()->peerDevInfo != NULL) {
        IotcFree(GetSleSsapMgtApp()->peerDevInfo);
        GetSleSsapMgtApp()->peerDevInfo = NULL;
    }
    DestroySleSsapSvcList();
}

SleSsapMgtApp *GetSleSsapMgtApp(void)
{
    return &g_SleSsapApp;
}

static int32_t GetSeviceHandle(const char *svcUuid, int32_t *handle)
{
    for (uint8_t i = 0; i < GetSleSsapMgtApp()->svcNum; i++) {
        if (strcmp(svcUuid, GetSleSsapMgtApp()->svc[i].uuid) != 0) {
            continue;
        }
        *handle = GetSleSsapMgtApp()->svc[i].svcHandle;
        return IOTC_OK;
    }
    IOTC_LOGE("no find svc");
    return IOTC_CORE_SLE_INVALID_SVC_UUID;
}

static bool IsAttrHandleInCharacterTbl(int32_t attrHandle, IotcAdptSleSsapsChar *character, uint8_t charNum)
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
    for (uint8_t i = 0; i < GetSleSsapMgtApp()->svcNum; i++) {
        if (IsAttrHandleInCharacterTbl(attrHandle,
            GetSleSsapMgtApp()->svc[i].character, GetSleSsapMgtApp()->svc[i].charNum)) {
            *serverId =  GetSleSsapMgtApp()->svc[i].serverId;
            return IOTC_OK;
        }
    }
    return IOTC_ERROR;
}

static IotcAdptSleSsapReadFunc FindReadFuncFromCharacterTbl(int32_t attrHandle,
    IotcAdptSleSsapsChar *character, uint8_t charNum)
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

static IotcAdptSleSsapReadFunc FindAttrHandleReadFunc(int32_t attrHandle)
{
    IotcAdptSleSsapReadFunc res = NULL;
    for (uint8_t i = 0; i < GetSleSsapMgtApp()->svcNum; i++) {
        res = FindReadFuncFromCharacterTbl(attrHandle,
            GetSleSsapMgtApp()->svc[i].character, GetSleSsapMgtApp()->svc[i].charNum);
        if (res != NULL) {
            return res;
        }
    }
    return NULL;
}

static IotcAdptSleSsapWriteFunc FindWriteFuncFromCharacterTbl(int32_t attrHandle,
    IotcAdptSleSsapsChar *character, uint8_t charNum)
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

static IotcAdptSleSsapWriteFunc FindAttrHandleWriteFunc(int32_t attrHandle)
{
    IotcAdptSleSsapWriteFunc res = NULL;
    for (uint8_t i = 0; i < GetSleSsapMgtApp()->svcNum; i++) {
        res = FindWriteFuncFromCharacterTbl(attrHandle,
            GetSleSsapMgtApp()->svc[i].character, GetSleSsapMgtApp()->svc[i].charNum);
        if (res != NULL) {
            return res;
        }
    }
    return NULL;
}

static bool IsAttrHandleInCharacterTbl(int32_t attrHandle, IotcAdptSleSsapsChar *character, uint8_t charNum)
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
    for (uint8_t i = 0; i < GetSleSsapMgtApp()->svcNum; i++) {
        if (IsAttrHandleInCharacterTbl(attrHandle,
            GetSleSsapMgtApp()->svc[i].character, GetSleSsapMgtApp()->svc[i].charNum)) {
            *serverId =  GetSleSsapMgtApp()->svc[i].serverId;
            return IOTC_OK;
        }
    }
    return IOTC_ERROR;
}

static IotcAdptSleSsapReadFunc FindReadFuncFromCharacterTbl(int32_t attrHandle,
    IotcAdptSleSsapsChar *character, uint8_t charNum)
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

static IotcAdptSleSsapReadFunc FindAttrHandleReadFunc(int32_t attrHandle)
{
    IotcAdptSleSsapReadFunc res = NULL;
    for (uint8_t i = 0; i < GetSleSsapMgtApp()->svcNum; i++) {
        res = FindReadFuncFromCharacterTbl(attrHandle,
            GetSleSsapMgtApp()->svc[i].character, GetSleSsapMgtApp()->svc[i].charNum);
        if (res != NULL) {
            return res;
        }
    }
    return NULL;
}

static IotcAdptSleSsapWriteFunc FindWriteFuncFromCharacterTbl(int32_t attrHandle,
    IotcAdptSleSsapsChar *character, uint8_t charNum)
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

static IotcAdptSleSsapWriteFunc FindAttrHandleWriteFunc(int32_t attrHandle)
{
    IotcAdptSleSsapWriteFunc res = NULL;
    for (uint8_t i = 0; i < GetSleSsapMgtApp()->svcNum; i++) {
        res = FindWriteFuncFromCharacterTbl(attrHandle,
            GetSleSsapMgtApp()->svc[i].character, GetSleSsapMgtApp()->svc[i].charNum);
        if (res != NULL) {
            return res;
        }
    }
    return NULL;
}

int32_t SleSendIndicateDataInner(const char *svcUuid, const char *charUuid, const uint8_t *value, uint32_t valueLen)
{
    CHECK_RETURN_LOGW((svcUuid != NULL) && (charUuid != NULL) &&  (value != NULL) && (valueLen != 0),
        IOTC_ERR_PARAM_INVALID, "invalid param");
    if ((GetSleSsapMgtApp()->connNum == 0) || (GetSleSsapMgtApp()->peerDevInfo == NULL)) {
        IOTC_LOGE("no connect");
        return IOTC_CORE_SLE_NO_CONNECT;
    }
    IotcAdptSleSendIndicateParam param;
    (void)memset_s(&param, sizeof(param), 0, sizeof(param));
    int32_t ret = GetSeviceHandle(svcUuid, (int32_t*)(&param.handle));
    if (ret != IOTC_OK) {
        IOTC_LOGE("get svc handle err ret=%d", ret);
        return ret;
    }

    return ret;
}

void SleSsapDisconnectAll(void)
{
    SlePeerDevInfo *peerDevInfoList = GetSleSsapMgtApp()->peerDevInfo;
    for (uint8_t i = 0; i < GetSleSsapMgtApp()->connNum; i++) {
        int32_t ret = IotcSleDisconnectSsap(peerDevInfoList[i].peerAddr,
            IOTC_ADPT_SLE_ADDR_LEN);
        if (ret != IOTC_OK) {
            IOTC_LOGW("disconn with peer[%u] err", i);
        }
    }
}

int32_t SetSleConnectParam(void)
{
    IotcAdptSleConnectParam param = {0};
    param.isDiscover = true;
    param.isConnect = true;
    param.isBond = SleIsPair();
    return IotcSleSetConnectParam(&param);
}

int32_t SleSsapReqRead(uint8_t serverId,uint16_t connId, uint16_t attrHandle, int16_t requestId)
{
    if ((GetSleSsapMgtApp()->connNum == 0) || (GetSleSsapMgtApp()->peerDevInfo == NULL)) {
        IOTC_LOGE("no connect");
        return IOTC_CORE_SLE_NO_CONNECT;
    }
    IotcAdptSleResponseParam param;
    uint8_t ret = 0;
    (void)memset_s(&param, sizeof(param), 0, sizeof(param));
    param.requestId = requestId;
    param.value = IotcCalloc(1, IOTC_ADPT_SLE_SSAP_READ_BUF_SIZE);
    if (param.value == NULL) {
        uint8_t ret = IotcSleSendSsapsResponse(serverId,connId,&param);
        return IOTC_ADAPTER_MEM_ERR_CALLOC;
    }
    IotcAdptSleSsapReadFunc func = FindAttrHandleReadFunc(attrHandle);
    if (func == NULL) {
        IOTC_LOGE("no find read func");
        IotcFree(param.value);
        param.value = NULL;
        ret = IotcSleSendSsapsResponse(serverId,connId,&param);
        return IOTC_ERROR;
    }
    param.valueLen = IOTC_ADPT_SLE_SSAP_READ_BUF_SIZE;
    ret = func(param.value, (uint32_t *)&param.valueLen);
    if ((ret != IOTC_OK) || (param.valueLen == 0)) {
        IOTC_LOGE("read err ret=%d, len=%d", ret, param.valueLen);
        IotcFree(param.value);
        param.value = NULL;
        param.valueLen = 0;
        ret = IotcSleSendSsapsResponse(serverId,connId,&param);
        return IOTC_ERROR;
    }
    ret = IotcSleSendSsapsResponse(serverId,connId,&param);
    if (ret != IOTC_OK) {
        IOTC_LOGE("response err ret=%d", ret);
        IotcFree(param.value);
        param.value = NULL;
        return IOTC_ERROR;
    }
    IotcFree(param.value);
    param.value = NULL;
    return IOTC_OK;
}


int32_t SleSsapReqWrite(const SleSsapWriteParam *param)
{
    if (param == NULL) {
        IOTC_LOGE("invalid param");
        return IOTC_ERROR;
    }

    if ((GetSleSsapMgtApp()->connNum == 0) || (GetSleSsapMgtApp()->peerDevInfo == NULL)) {
        IOTC_LOGE("no connect");
        return IOTC_CORE_SLE_NO_CONNECT;
    }

    IotcAdptSleResponseParam respParam;
    respParam.requestId = param->requestId;
    respParam.status = param->type;
    respParam.value = param->value;
    respParam.valueLen = param->valueLen;
    uint8_t ret = IotcSleSendSsapsResponse(param->serverId, param->connectId, &respParam);
    if (ret != IOTC_OK) {
        IOTC_LOGE("write err ret=%d", ret);
        return IOTC_ERROR;
    }
    IotcAdptSleSsapWriteFunc func = FindAttrHandleWriteFunc(handle);
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