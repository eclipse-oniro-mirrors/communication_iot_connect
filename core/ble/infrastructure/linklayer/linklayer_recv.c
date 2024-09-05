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
#include <stdbool.h>
#include "linklayer_recv.h"
#include "utils_assert.h"
#include "utils_list.h"
#include "utils_common.h"
#include "iotc_mem.h"
#include "iotc_os.h"
#include "securec.h"
#include "iotc_errcode.h"

#define PKG_MAX_NUM     5
#define PKG_TIMEOUT (10 * 1000)

typedef struct TagSubPkgList {
    uint8_t index;          /* 分包序列号 */
    uint8_t *buff;          /* 分包数据内容 */
    uint32_t buffLen;       /* 分包数据内容长度 */
    ListEntry node;
} SubPkg;

typedef struct {
    uint8_t curSize;        /* 当前已缓存分包数 */
    uint32_t curDataLen;    /* 当前已缓存数据长度 */
    ListEntry subPkgs;      /* SubPkg 的 head */
} SubPkgList;

typedef struct {
    uint8_t token;          /* 用于唯一标识该报文 */
    uint8_t pkgNum;         /* 分包总数量 */
    uint64_t savedTime;     /* 首次保存时间 */
    SubPkgList subPkgList;  /* 分包链表 */
    ListEntry node;
} CachePkgData;

typedef struct {
    uint8_t size;           /* 当前已缓存报文数 */
    ListEntry pkgs;         /* CachePkgData 的 head */
} PkgList;

static PkgList g_pkgList = { 0, LIST_DECLARE_INIT(&g_pkgList.pkgs) };

static void CachePkgDataFree(CachePkgData *pkgData)
{
    CHECK_V_RETURN_LOGE(pkgData != NULL, "pkgData double free");

    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &pkgData->subPkgList.subPkgs) {
        SubPkg *subPkg = CONTAINER_OF(item, SubPkg, node);
        if (subPkg->buff != NULL) {
            IotcFree(subPkg->buff);
        }
        LIST_REMOVE(&subPkg->node);
        IotcFree(subPkg);
    }

    IotcFree(pkgData);
}

void LinkLayerClearAllCachePkg(void)
{
    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &g_pkgList.pkgs) {
        CachePkgData *pkgData = CONTAINER_OF(item, CachePkgData, node);
        LIST_REMOVE(&pkgData->node);
        CachePkgDataFree(pkgData);
        g_pkgList.size--;
    }
}

static void ClearTimeoutPkg(void)
{
    uint64_t curTime = IotcGetSysTimeMs();

    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &g_pkgList.pkgs) {
        CachePkgData *pkgData = CONTAINER_OF(item, CachePkgData, node);
        if (UtilsDeltaTime(curTime, pkgData->savedTime) < PKG_TIMEOUT) {
            continue;
        }
        LIST_REMOVE(&pkgData->node);
        CachePkgDataFree(pkgData);
        g_pkgList.size--;
    }
}

static CachePkgData* FindPkgByToken(uint8_t token)
{
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &g_pkgList.pkgs) {
        CachePkgData *pkgData = CONTAINER_OF(item, CachePkgData, node);
        if (pkgData->token == token) {
            return pkgData;
        }
    }
    return NULL;
}

static int32_t InsertSubPkg(SubPkgList *subPkgList, uint8_t pkgIdx, const uint8_t *data, uint32_t dataLen)
{
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &subPkgList->subPkgs) {
        SubPkg *subPkg = CONTAINER_OF(item, SubPkg, node);
        if (subPkg->index == pkgIdx) {
            return IOTC_OK;
        } else if (subPkg->index > pkgIdx) {
            break;
        }
    }

    SubPkg *newSubPkg = (SubPkg *)IotcCalloc(1, sizeof(SubPkg));
    CHECK_RETURN(newSubPkg != NULL, IOTC_ADAPTER_MEM_ERR_CALLOC);

    newSubPkg->buff = (uint8_t *)IotcCalloc(dataLen, sizeof(uint8_t));
    if (newSubPkg->buff == NULL) {
        IotcFree(newSubPkg);
        return IOTC_ADAPTER_MEM_ERR_CALLOC;
    }
    if (memcpy_s(newSubPkg->buff, dataLen, data, dataLen) != EOK) {
        IotcFree(newSubPkg->buff);
        IotcFree(newSubPkg);
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    newSubPkg->buffLen = dataLen;
    newSubPkg->index = pkgIdx;

    subPkgList->curSize++;
    subPkgList->curDataLen += dataLen;
    /* 按idx顺序插入 */
    LIST_INSERT_BEFORE(&newSubPkg->node, item);
    return IOTC_OK;
}

static int32_t InsertPkg(uint8_t token, uint8_t pkgNum, CachePkgData **cachePkgData)
{
    CHECK_RETURN(g_pkgList.size < PKG_MAX_NUM, IOTC_CORE_BLE_LL_ERR_POOL_FULL);

    CachePkgData *pkgData = (CachePkgData *)IotcCalloc(1, sizeof(CachePkgData));
    CHECK_RETURN(pkgData != NULL, IOTC_ADAPTER_MEM_ERR_CALLOC);

    pkgData->savedTime = IotcGetSysTimeMs();
    pkgData->token = token;
    pkgData->pkgNum = pkgNum;

    LIST_INIT(&pkgData->subPkgList.subPkgs);
    LIST_INIT(&pkgData->node);

    g_pkgList.size++;
    LIST_INSERT_BEFORE(&pkgData->node, &g_pkgList.pkgs);
    *cachePkgData = pkgData;
    return IOTC_OK;
}

int32_t LinkLayerRecvPkgInsert(uint8_t token, uint8_t pkgNum, uint8_t pkgIdx, const uint8_t *data, uint32_t dataLen)
{
    CHECK_RETURN((data != NULL) && (dataLen > 0), IOTC_ERR_PARAM_INVALID);

    ClearTimeoutPkg();

    int32_t ret = IOTC_OK;
    CachePkgData* pkgData = FindPkgByToken(token);
    if (pkgData == NULL) {
        ret = InsertPkg(token, pkgNum, &pkgData);
        CHECK_RETURN_LOGE((ret == IOTC_OK) && (pkgData != NULL), ret, "ll create pkg err:%d", ret);
    }

    ret = InsertSubPkg(&pkgData->subPkgList, pkgIdx, data, dataLen);
    if (ret == IOTC_OK) {
        return IOTC_OK;
    }

    IOTC_LOGE("ll insert sub pkg err:%d", ret);
    if (pkgData->subPkgList.curSize == 0) {
        LIST_REMOVE(&pkgData->node);
        CachePkgDataFree(pkgData);
        g_pkgList.size--;
    }
    return ret;
}

int32_t LinkLayerRecvCompleteCheck(uint8_t token, bool *isComplete)
{
    CHECK_RETURN(isComplete != NULL, IOTC_ERR_PARAM_INVALID);

    CachePkgData* pkgData = FindPkgByToken(token);
    CHECK_RETURN_LOGE(pkgData != NULL, IOTC_CORE_BLE_LL_ERR_TOKEN,
        "ll check pkg with token:%u not found", token);

    if (pkgData->subPkgList.curSize == pkgData->pkgNum) {
        *isComplete = true;
        return IOTC_OK;
    } else if (pkgData->subPkgList.curSize < pkgData->pkgNum) {
        *isComplete = false;
        return IOTC_OK;
    } else {
        IOTC_LOGE("ll sub pkg cnts:%u over pkgNum:%u", pkgData->subPkgList.curSize, pkgData->pkgNum);
        return IOTC_CORE_BLE_LL_ERR_PKGNUM;
    }
}

int32_t LinkLayerRecvMergePkgs(uint8_t token, uint8_t **outData, uint32_t *outDataLen)
{
    CHECK_RETURN((outData != NULL) && (outDataLen != NULL), IOTC_ERR_PARAM_INVALID);

    bool isComplete = false;
    int32_t ret = LinkLayerRecvCompleteCheck(token, &isComplete);
    CHECK_RETURN((ret == IOTC_OK) && isComplete, ret);

    CachePkgData* pkgData = FindPkgByToken(token);
    CHECK_RETURN_LOGE(pkgData != NULL, IOTC_CORE_BLE_LL_ERR_TOKEN,
        "ll merge pkg with token:%u not found", token);

    uint32_t dataLen = pkgData->subPkgList.curDataLen;
    CHECK_RETURN_LOGE(dataLen != 0, IOTC_CORE_BLE_LL_ERR_PKGLEN, "ll merge pkg with 0 len err");

    /* 合包补充结束符, 防止报文不带结束符 */
    uint8_t *data = (uint8_t *)IotcCalloc(dataLen + 1, sizeof(uint8_t));
    CHECK_RETURN(data != NULL, IOTC_ADAPTER_MEM_ERR_CALLOC);

    uint8_t *curPtr = data;
    uint32_t curLen = 0;
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &pkgData->subPkgList.subPkgs) {
        SubPkg *subPkg = CONTAINER_OF(item, SubPkg, node);
        if (subPkg->buff == NULL) {
            IOTC_LOGE("ll merge sub pkg null err");
            IotcFree(data);
            LIST_REMOVE(&pkgData->node);
            CachePkgDataFree(pkgData);
            g_pkgList.size--;
            return IOTC_CORE_BLE_LL_ERR_SUBPKG_NULL;
        }
        if (memcpy_s(curPtr, dataLen - curLen, subPkg->buff, subPkg->buffLen) != EOK) {
            IotcFree(data);
            return IOTC_ERR_SECUREC_MEMCPY;
        }
        curPtr += subPkg->buffLen;
        curLen += subPkg->buffLen;
    }

    *outData = data;
    *outDataLen = dataLen;
    LIST_REMOVE(&pkgData->node);
    CachePkgDataFree(pkgData);
    g_pkgList.size--;
    return IOTC_OK;
}