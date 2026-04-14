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
#include "lan_search_peer_mngr.h"
#include "utils_assert.h"
#include "iotc_errcode.h"
#include "securec.h"
#include "sched_timer.h"
#include "utils_common.h"
#include "iotc_os.h"
#include "dfx_anonymize.h"
#include "iotc_svc_dev.h"
#include "utils_bit_map.h"
#include "product_adapter.h"

#define LAN_SEARCH_NEW_PEER_SPEKE_TIMEOUT       UTILS_SEC_TO_MS(30)
#define LAN_SEARCH_EXPIRE_TIMER_CHECK_PERIOD    UTILS_SEC_TO_MS(30)

static void LanSearchPeerInfoFree(LanSearchPeer *peer)
{
    if (peer->sessInfo.speke != NULL) {
        SpekeFreeSession(peer->sessInfo.speke);
        peer->sessInfo.speke = NULL;
    }
    IotcFree(peer);
}

static void HashMapPeerFreeFunc(void *value)
{
    CHECK_V_RETURN_LOGW(value != NULL, "param invalid");
    LanSearchPeerInfoFree(value);
}

static HashMapTravCode PeerMapExpireCheckTraversal(const void *value, va_list argp)
{
    const LanSearchPeer *peer = (const LanSearchPeer *)value;
    LanSearchContext *ctx = va_arg(argp, LanSearchContext *);
    uint32_t curTime = va_arg(argp, uint32_t);
    CHECK_RETURN_LOGW(ctx != NULL && peer != NULL, HASH_MAP_TRAVE_BREAK, "param invalid");

    if (UtilsDeltaTime(curTime, peer->timeInfo.createTime) < peer->timeInfo.expireTime) {
        return HASH_MAP_TRAVE_CONTINUE;
    }

    DFX_ANONYMIZE_IP_ADDR(anonyIp, peer->peerInfo.addr);
    IOTC_LOGI("remove expire peer %s", anonyIp);

    int32_t ret = UtilsHashMapRemove(ctx->peerManager.peerMap, (const uint8_t *)&peer->peerInfo.addr,
        sizeof(peer->peerInfo.addr));
    if (ret != IOTC_OK) {
        IOTC_LOGW("remove peer error %d/%s", ret, anonyIp);
    }
    return HASH_MAP_TRAVE_CONTINUE;
}

static void PeerExpireCheckTimer(int32_t id, void *userData)
{
    (void)id;
    CHECK_V_RETURN_LOGW(userData != NULL, "param invalid");
    LanSearchContext *ctx = (LanSearchContext *)userData;

    uint32_t curTime = IotcGetSysTimeMs();
    (void)UtilsHashMapIterate(ctx->peerManager.peerMap, PeerMapExpireCheckTraversal, ctx, curTime);
    ctx->peerManager.curPeerNum = UtilsHashMapLength(ctx->peerManager.peerMap);
}

int32_t LanSearchPeerManagerInit(LanSearchContext *ctx)
{
    CHECK_RETURN_LOGW(ctx != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    int32_t ret;
    do {
        if (ctx->peerManager.peerMap == NULL) {
            ctx->peerManager.peerMap = UtilsHashMapCreate(ctx->config.maxPeerNum, "PEER_MAP", HashMapPeerFreeFunc);
        }

        if (ctx->peerManager.peerMap == NULL) {
            IOTC_LOGW("create map error %u", ctx->config.maxPeerNum);
            ret = IOTC_CORE_COMM_UTILS_ERR_HASH_MAP_CREATE;
            break;
        }

        ctx->peerManager.curPeerNum = UtilsHashMapLength(ctx->peerManager.peerMap);
        IOTC_LOGD("peer manager hash map init %u/%u", ctx->peerManager.curPeerNum, ctx->config.maxPeerNum);

        if (ctx->peerManager.expireCheckTimer < 0) {
            ctx->peerManager.expireCheckTimer = SchedTimerAdd(EVENT_SOURCE_TIMER_TYPE_REPEAT, PeerExpireCheckTimer,
                LAN_SEARCH_EXPIRE_TIMER_CHECK_PERIOD, ctx);
        }

        if (ctx->peerManager.expireCheckTimer < 0) {
            IOTC_LOGW("create timer error %d", ctx->peerManager.expireCheckTimer);
            ret = ctx->peerManager.expireCheckTimer;
            break;
        }

        return IOTC_OK;
    } while (0);
    LanSearchPeerManagerDeinit(ctx);
    return ret;
}

void LanSearchPeerManagerDeinit(LanSearchContext *ctx)
{
    CHECK_V_RETURN_LOGW(ctx != NULL, "param invalid");
    if (ctx->peerManager.expireCheckTimer >= 0) {
        SchedTimerRemove(ctx->peerManager.expireCheckTimer);
    }

    if (ctx->peerManager.peerMap != NULL) {
        UtilsHashMapDestroy(&ctx->peerManager.peerMap);
    }

    (void)memset_s(&ctx->peerManager, sizeof(ctx->peerManager), 0, sizeof(ctx->peerManager));
    ctx->peerManager.expireCheckTimer = EVENT_SOURCE_TIMER_STATUS_INVALID;
}

int32_t LanSearchGetPeer(LanSearchContext *ctx, uint32_t ipAddr, LanSearchPeer **peer)
{
    CHECK_RETURN_LOGW(ctx != NULL && peer != NULL && ctx->peerManager.peerMap != NULL,
        IOTC_ERR_PARAM_INVALID, "param invalid");

    *peer = NULL;
    if (ctx->peerManager.curPeerNum == 0) {
        return IOTC_OK;
    }

    *peer = (LanSearchPeer *)UtilsHashMapGetValue(ctx->peerManager.peerMap,
        (const uint8_t *)&ipAddr, sizeof(uint32_t));
    return IOTC_OK;
}

static int32_t LanSearchSpekeGetPinCodeCallback(SpekeSession *session, void *user, uint8_t *pinCode, uint32_t *len)
{
    NOT_USED(user);
    NOT_USED(session);
    CHECK_RETURN(len != NULL && *len >= IOTC_PINCODE_LEN, IOTC_ERR_PARAM_INVALID);

    *len = IOTC_PINCODE_LEN;
    int32_t ret = ProductProfGetPincode(pinCode, *len);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get pin err %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static int32_t LanSearchSpekeNotifySpekeFinishedCallback(SpekeSession *session, void *user, int32_t errorCode)
{
    NOT_USED(session);
    CHECK_RETURN_LOGW(user != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    LanSearchPeer *peer = (LanSearchPeer *)user;

    DFX_ANONYMIZE_IP_ADDR(anonyIp, peer->peerInfo.addr);
    if (errorCode != IOTC_OK) {
        IOTC_LOGW("peer speke finish error %d/%s", errorCode, anonyIp);
        return IOTC_OK;
    }
    UTILS_BIT_SET(peer->bitMap, LAN_SEARCH_PEER_BIT_SPEKE_FINISHED);
    LanSearchContext *ctx = GetLanSearchCtx();
    CHECK_RETURN_LOGE(ctx != NULL, IOTC_ERR_CONTEXT_NULL, "lan search ctx null");

    peer->timeInfo.expireTime = ctx->config.peerExpireTime;
    IOTC_LOGI("peer speke finish ok %s/%u", anonyIp, peer->timeInfo.expireTime);
    return IOTC_OK;
}

static LanSearchPeer *LanSearchPeerNew(uint32_t addr)
{
    LanSearchPeer *newPeer = (LanSearchPeer *)IotcMalloc(sizeof(LanSearchPeer));
    if (newPeer == NULL) {
        IOTC_LOGW("malloc error");
        return NULL;
    }
    (void)memset_s(newPeer, sizeof(LanSearchPeer), 0, sizeof(LanSearchPeer));
    newPeer->timeInfo.createTime = IotcGetSysTimeMs();
    newPeer->timeInfo.expireTime = LAN_SEARCH_NEW_PEER_SPEKE_TIMEOUT;
    newPeer->peerInfo.addr = addr;
    SpekeCallback spekeCbs = {
        .getPinCode = LanSearchSpekeGetPinCodeCallback,
        .notifySpekeFinished = LanSearchSpekeNotifySpekeFinishedCallback,
    };
    newPeer->sessInfo.speke = SpekeInitSession(SPEKE_TYPE_SERVER, &spekeCbs, newPeer);
    if (newPeer->sessInfo.speke == NULL) {
        IOTC_LOGW("create speke error");
        IotcFree(newPeer);
        return NULL;
    }
    return newPeer;
}

static void LanSearchDestroyPeer(LanSearchContext *ctx, uint32_t ipAddr)
{
    CHECK_V_RETURN_LOGW(ctx != NULL && ctx->peerManager.peerMap != NULL, "param invalid");

    if (UtilsHashMapGetValue(ctx->peerManager.peerMap, (const uint8_t *)&ipAddr, sizeof(uint32_t)) == NULL) {
        return;
    }

    DFX_ANONYMIZE_IP_ADDR(anonyIp, ipAddr);
    int32_t ret = UtilsHashMapRemove(ctx->peerManager.peerMap, (const uint8_t *)&ipAddr, sizeof(uint32_t));
    if (ret != IOTC_OK) {
        IOTC_LOGW("remove peer error %d/%s", ret, anonyIp);
        return;
    }
    ctx->peerManager.curPeerNum = UtilsHashMapLength(ctx->peerManager.peerMap);
    IOTC_LOGI("peer remove %u/%s", ctx->peerManager.curPeerNum, anonyIp);
}

int32_t LanSearchCreatePeer(LanSearchContext *ctx, uint32_t ipAddr, LanSearchPeer **peer)
{
    CHECK_RETURN_LOGW(ctx != NULL && peer != NULL && ctx->peerManager.peerMap != NULL,
        IOTC_ERR_PARAM_INVALID, "param invalid");

    /* 如果同ip已存在peer信息，则移除 */
    LanSearchDestroyPeer(ctx, ipAddr);
    if (ctx->peerManager.curPeerNum >= ctx->config.maxPeerNum) {
        IOTC_LOGW("peer over max num %u/%u", ctx->peerManager.curPeerNum, ctx->config.maxPeerNum);
        return IOTC_CORE_WIFI_LAN_SEARCH_ERR_PEER_OVER_MAX_NUM;
    }

    LanSearchPeer *newPeer = LanSearchPeerNew(ipAddr);
    if (newPeer == NULL) {
        return IOTC_CORE_WIFI_LAN_SEARCH_ERR_PEER_CREATE;
    }

    int32_t ret = UtilsHashMapInsert(ctx->peerManager.peerMap, (const uint8_t *)&ipAddr, sizeof(uint32_t), newPeer);
    if (ret != IOTC_OK) {
        IOTC_LOGW("insert map error %d", ret);
        LanSearchPeerInfoFree(newPeer);
        return ret;
    }

    *peer = newPeer;
    return IOTC_OK;
}