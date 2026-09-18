/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

#define PKG_MAX_NUM     1
#define PKG_TIMEOUT (10 * 1000)
/* per-frag data max, matches linklayer_send.c (SINGLE_PKG_MAX_SIZE - PKG_HEAD_LEN) */
#define RECV_SINGLE_PKG_DATA_MAX_SIZE   153
/* frag count limit, matches PKG_MAX_NUM in linklayer_send.c; <= 16 fits recvMask */
#define PKG_MAX_NUM_LIMIT   16

/* single pre-allocated buffer, frag written at pkgIdx offset; SubPkg list removed */
typedef struct {
    uint64_t savedTime;
    uint8_t *mergeBuff;        /* pre-alloc pkgNum*RECV_SINGLE_PKG_DATA_MAX_SIZE, +1 terminator */
    uint32_t mergeBuffCap;     /* = pkgNum * RECV_SINGLE_PKG_DATA_MAX_SIZE */
    uint32_t curDataLen;       /* total data bytes written (tail frag may be shorter) */
    uint16_t recvMask;         /* arrival bitmask for dedup: bit i set = frag i received */
    uint8_t token;
    uint8_t pkgNum;
    uint8_t curSize;           /* frags received for completion check */
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
    if (pkgData->mergeBuff != NULL) {
        IotcFree(pkgData->mergeBuff);   /* single buffer, one free */
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

int32_t LinkLayerRecvPkgInsert(uint8_t token, uint8_t pkgNum, uint8_t pkgIdx, const uint8_t *data, uint32_t dataLen)
{
    CHECK_RETURN((data != NULL) && (dataLen > 0), IOTC_ERR_PARAM_INVALID);
    CHECK_RETURN_LOGE(pkgNum > 0 && pkgNum <= PKG_MAX_NUM_LIMIT, IOTC_CORE_BLE_LL_ERR_PKGNUM,
        "pkgNum:%u err", pkgNum);
    CHECK_RETURN_LOGE(pkgIdx < pkgNum, IOTC_CORE_BLE_LL_ERR_PKGNUM, "pkgIdx:%u >= pkgNum:%u", pkgIdx, pkgNum);
    CHECK_RETURN_LOGE(dataLen <= RECV_SINGLE_PKG_DATA_MAX_SIZE, IOTC_CORE_BLE_LL_ERR_PKGLEN,
        "sub pkg dataLen:%u > %u", dataLen, RECV_SINGLE_PKG_DATA_MAX_SIZE);
    /* only the tail frag may be short; middle frags must fill a full slot (offset layout) */
    CHECK_RETURN_LOGE(dataLen == RECV_SINGLE_PKG_DATA_MAX_SIZE || pkgIdx + 1 == pkgNum,
        IOTC_CORE_BLE_LL_ERR_PKGLEN, "non-tail frag pkgIdx:%u dataLen:%u", pkgIdx, dataLen);

    ClearTimeoutPkg();

    CachePkgData *pkgData = FindPkgByToken(token);
    if (pkgData == NULL) {
        CHECK_RETURN(g_pkgList.size < PKG_MAX_NUM, IOTC_CORE_BLE_LL_ERR_POOL_FULL);
        /* first frag arrived, pre-alloc one buffer of pkgNum slots */
        uint32_t cap = (uint32_t)pkgNum * RECV_SINGLE_PKG_DATA_MAX_SIZE;
        pkgData = (CachePkgData *)IotcCalloc(1, sizeof(CachePkgData));
        CHECK_RETURN_LOGE(pkgData != NULL, IOTC_ADAPTER_MEM_ERR_CALLOC, "calloc pkgData err");
        pkgData->mergeBuff = (uint8_t *)IotcCalloc(cap + 1, sizeof(uint8_t));  /* +1 terminator */
        if (pkgData->mergeBuff == NULL) {
            IotcFree(pkgData);
            return IOTC_ADAPTER_MEM_ERR_CALLOC;
        }
        pkgData->mergeBuffCap = cap;
        pkgData->token = token;
        pkgData->pkgNum = pkgNum;
        pkgData->savedTime = IotcGetSysTimeMs();
        LIST_INIT(&pkgData->node);
        g_pkgList.size++;
        LIST_INSERT_BEFORE(&pkgData->node, &g_pkgList.pkgs);
    }

    /* dedup: same semantics as old InsertSubPkg (index==pkgIdx returns OK) */
    if ((pkgData->recvMask & (1U << pkgIdx)) != 0) {
        return IOTC_OK;
    }

    /* write directly at pkgIdx offset, out-of-order arrival supported */
    uint32_t offset = (uint32_t)pkgIdx * RECV_SINGLE_PKG_DATA_MAX_SIZE;
    CHECK_RETURN_LOGE(offset + dataLen <= pkgData->mergeBuffCap, IOTC_CORE_BLE_LL_ERR_PKGLEN,
        "pkgIdx:%u offset+%u > cap:%u", pkgIdx, dataLen, pkgData->mergeBuffCap);
    if (memcpy_s(pkgData->mergeBuff + offset, pkgData->mergeBuffCap - offset, data, dataLen) != EOK) {
        IOTC_LOGE("ll merge pkg memcpy err");
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    pkgData->recvMask |= (uint16_t)(1U << pkgIdx);
    pkgData->curSize++;
    pkgData->curDataLen += dataLen;
    return IOTC_OK;
}

int32_t LinkLayerRecvCompleteCheck(uint8_t token, bool *isComplete)
{
    CHECK_RETURN(isComplete != NULL, IOTC_ERR_PARAM_INVALID);

    CachePkgData* pkgData = FindPkgByToken(token);
    CHECK_RETURN_LOGE(pkgData != NULL, IOTC_CORE_BLE_LL_ERR_TOKEN,
        "ll check pkg with token:%u not found", token);

    if (pkgData->curSize == pkgData->pkgNum) {
        *isComplete = true;
        return IOTC_OK;
    } else if (pkgData->curSize < pkgData->pkgNum) {
        *isComplete = false;
        return IOTC_OK;
    } else {
        IOTC_LOGE("ll sub pkg cnts:%u over pkgNum:%u", pkgData->curSize, pkgData->pkgNum);
        return IOTC_CORE_BLE_LL_ERR_PKGNUM;
    }
}

int32_t LinkLayerRecvMergePkgs(uint8_t token, uint8_t **outData, uint32_t *outDataLen)
{
    CHECK_RETURN((outData != NULL) && (outDataLen != NULL), IOTC_ERR_PARAM_INVALID);

    bool isComplete = false;
    int32_t ret = LinkLayerRecvCompleteCheck(token, &isComplete);
    CHECK_RETURN((ret == IOTC_OK) && isComplete, ret);

    CachePkgData *pkgData = FindPkgByToken(token);
    CHECK_RETURN_LOGE(pkgData != NULL, IOTC_CORE_BLE_LL_ERR_TOKEN, "token:%u not found", token);
    CHECK_RETURN_LOGE(pkgData->curDataLen != 0, IOTC_CORE_BLE_LL_ERR_PKGLEN, "ll merge pkg with 0 len err");

    /* mergeBuff IS the merged result: no extra alloc+memcpy, ownership moves to caller */
    *outData = pkgData->mergeBuff;
    *outDataLen = pkgData->curDataLen;
    pkgData->mergeBuff = NULL;   /* so CachePkgDataFree will not free it */

    LIST_REMOVE(&pkgData->node);
    CachePkgDataFree(pkgData);   /* frees the struct itself only */
    g_pkgList.size--;
    return IOTC_OK;
}
