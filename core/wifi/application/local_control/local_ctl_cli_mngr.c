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
#include "local_ctl_cli_mngr.h"
#include "iotc_errcode.h"
#include "utils_assert.h"
#include "sched_timer.h"
#include "utils_common.h"
#include "iotc_os.h"
#include "dfx_anonymize.h"
#include "securec.h"
#include "security_random.h"
#include "utils_bit_map.h"
#include "iotc_svc_dev.h"

#define CLIENT_EXPIRE_TIMER_CHECK_PERIOD    UTILS_HOUR_TO_MS(1)

static void LocalCtlClientFree(LocalControlClient *cli)
{
    if (cli->appInfo.puuid != NULL) {
        DFX_ANONYMIZE_ID_STR(anonyPuuid, cli->appInfo.puuid);
        IOTC_LOGD("client free %s", anonyPuuid);
        IotcFree(cli->appInfo.puuid);
        cli->appInfo.puuid = NULL;
    }

    (void)memset_s(&cli->sessInfo, sizeof(cli->sessInfo), 0, sizeof(cli->sessInfo));
    IotcFree(cli);
}

static void ClientHashMapFreeValue(void *value)
{
    CHECK_V_RETURN_LOGW(value != NULL, "param invalid");
    LocalCtlClientFree(value);
}

static HashMapTravCode ClientMapTraversalCheckExpire(const void *value, va_list argp)
{
    CHECK_RETURN_LOGW(value != NULL, HASH_MAP_TRAVE_BREAK, "param invalid");

    const LocalControlClient *client = (const LocalControlClient *)value;
    LocalControlContext *ctx = va_arg(argp, LocalControlContext *);
    uint32_t curTime = va_arg(argp, uint32_t);
    if (ctx == NULL) {
        IOTC_LOGW("invalid ctx");
        return HASH_MAP_TRAVE_BREAK;
    }

    if (UtilsDeltaTime(curTime, client->timeInfo.createTime) < client->timeInfo.expireTime) {
        return HASH_MAP_TRAVE_CONTINUE;
    }

    DFX_ANONYMIZE_IP_ADDR(anonyIp, client->appInfo.addr);
    DFX_ANONYMIZE_ID_STR(anonyPuuid, client->appInfo.puuid);
    IOTC_LOGI("local client expire %s/%s/%u/%u", anonyIp, anonyPuuid,
        client->timeInfo.createTime, client->timeInfo.expireTime);
    UtilsHashMapRemove(ctx->clientManager.clientMap, (const uint8_t *)client->sessInfo.sessId,
        strlen(client->sessInfo.sessId));
    return HASH_MAP_TRAVE_CONTINUE;
}

static void ClientExpireTimerCallback(int32_t id, void *userData)
{
    NOT_USED(id);
    NOT_USED(userData);

    LocalControlContext *ctx = GetLocalCtlCtx();
    CHECK_V_RETURN_LOGE(ctx != NULL, "local ctl ctx null");
    uint32_t curTime = IotcGetSysTimeMs();

    int32_t ret = UtilsHashMapIterate(ctx->clientManager.clientMap, ClientMapTraversalCheckExpire, ctx, curTime);
    if (ret != IOTC_OK) {
        IOTC_LOGW("map trave error %d", ret);
    }
    ctx->clientManager.curClientNum = UtilsHashMapLength(ctx->clientManager.clientMap);
}

int32_t LocalCtlClientManagerInit(LocalControlContext *ctx)
{
    CHECK_RETURN_LOGW(ctx != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    int32_t ret;
    do {
        if (ctx->clientManager.clientMap == NULL) {
            ctx->clientManager.clientMap = UtilsHashMapCreate(ctx->config.maxClientNum,
                "LOCAL_CLIENT_MAP", ClientHashMapFreeValue);
        }
        if (ctx->clientManager.clientMap == NULL) {
            ret = IOTC_CORE_COMM_UTILS_ERR_HASH_MAP_CREATE;
            IOTC_LOGW("create hash map error %u", ctx->config.maxClientNum);
            break;
        }

        ctx->clientManager.curClientNum = UtilsHashMapLength(ctx->clientManager.clientMap);

        if (ctx->clientManager.expireCheckTimer < 0) {
            ctx->clientManager.expireCheckTimer = SchedTimerAdd(EVENT_SOURCE_TIMER_TYPE_REPEAT,
                ClientExpireTimerCallback, CLIENT_EXPIRE_TIMER_CHECK_PERIOD, NULL);
        }

        if (ctx->clientManager.expireCheckTimer < 0) {
            ret = ctx->clientManager.expireCheckTimer;
            IOTC_LOGW("create timer error %d", ret);
            break;
        }
        return IOTC_OK;
    } while (0);

    LocalCtlClientManagerDestroy(ctx);
    return ret;
}

void LocalCtlClientManagerDestroy(LocalControlContext *ctx)
{
    CHECK_V_RETURN_LOGW(ctx != NULL, "param invalid");

    if (ctx->clientManager.clientMap != NULL) {
        UtilsHashMapDestroy(&ctx->clientManager.clientMap);
    }
    if (ctx->clientManager.expireCheckTimer >= 0) {
        SchedTimerRemove(ctx->clientManager.expireCheckTimer);
        ctx->clientManager.expireCheckTimer = EVENT_SOURCE_INVALID_TIMER_FD;
    }
    ctx->clientManager.curClientNum = 0;
}

static HashMapTravCode ClientMapTraversalFindPuuid(const void *value, va_list argp)
{
    CHECK_RETURN_LOGW(value != NULL, HASH_MAP_TRAVE_BREAK, "param invalid");

    LocalControlClient *client = (LocalControlClient *)value;
    if (client->appInfo.puuid == NULL || client->appInfo.puuidLen == 0) {
        IOTC_LOGW("invalid client in map");
        return HASH_MAP_TRAVE_CONTINUE;
    }

    const char *puuid = va_arg(argp, const char *);
    uint32_t puuidLen = va_arg(argp, uint32_t);
    LocalControlClient **clientFind = va_arg(argp, LocalControlClient **);
    if (puuid == NULL || puuidLen == 0 || clientFind == NULL) {
        IOTC_LOGW("param invalid");
        return HASH_MAP_TRAVE_BREAK;
    }

    if (puuidLen != client->appInfo.puuidLen || strncmp(puuid, client->appInfo.puuid, puuidLen) != 0) {
        return HASH_MAP_TRAVE_CONTINUE;
    }

    *clientFind = client;
    return HASH_MAP_TRAVE_BREAK;
}

int32_t GetLocalControlClient(LocalControlContext *ctx, ClientIdType type, const char *id, uint32_t len,
    LocalControlClient **client)
{
    CHECK_RETURN_LOGW(ctx != NULL && id != NULL && len != 0 && client != NULL,
        IOTC_ERR_PARAM_INVALID, "param invalid");

    *client = NULL;
    if (ctx->clientManager.clientMap == NULL) {
        IOTC_LOGW("hash map not init");
        return IOTC_CORE_WIFI_LOCAL_CTL_ERR_INVALID_CLIENT_MAP;
    }

    if (ctx->clientManager.curClientNum == 0) {
        return IOTC_OK;
    }

    if (type == CLIENT_SESS_ID) {
        *client = UtilsHashMapGetValue(ctx->clientManager.clientMap, (const uint8_t *)id, len);
    } else {
        (void)UtilsHashMapIterate(ctx->clientManager.clientMap, ClientMapTraversalFindPuuid,
            id, len, client);
    }
    return IOTC_OK;
}

static bool LocalControlSessKeyCreate(LocalControlClient *client, const LocalClientBuildParam *param)
{
    SessKeyContext *sessCtx = SecuritySessKeyInit();
    if (sessCtx == NULL) {
        IOTC_LOGW("sess key init error");
        return false;
    }

    do {
        int32_t ret = SecuritySessKeyAddSn(sessCtx, param->sn1, SESS_KEY_SN1);
        if (ret != IOTC_OK) {
            IOTC_LOGW("add sn1 error %d", ret);
            break;
        }

        ret = SecuritySessKeyAddSn(sessCtx, param->sn2, SESS_KEY_SN2);
        if (ret != IOTC_OK) {
            IOTC_LOGW("add sn2 error %d", ret);
            break;
        }

        DevAuthInfo authInfo = {0};
        bool isExist = false;
        ret = DevSvcProxyGetAuthInfo(&isExist, &authInfo);
        if (ret != IOTC_OK || !isExist) {
            IOTC_LOGW("get auth info error %d", ret);
            break;
        }

        ret = SecuritySessKeyGenTransKey(sessCtx, authInfo.authCode,
            sizeof(authInfo.authCode), client->sessInfo.transKey);
        (void)memset_s(&authInfo, sizeof(DevAuthInfo), 0, sizeof(DevAuthInfo));
        if (ret != IOTC_OK) {
            IOTC_LOGW("gem trans key error %d", ret);
            break;
        }

        ret = SecuritySessKeyGenAuthKey(sessCtx, client->sessInfo.authKey);
        if (ret != IOTC_OK) {
            IOTC_LOGW("gen auth key error %d", ret);
            break;
        }

        SecuritySessDestroy(sessCtx);
        return true;
    } while (0);

    (void)memset_s(client->sessInfo.transKey, SESS_TRANS_KEY_LEN, 0, SESS_TRANS_KEY_LEN);
    (void)memset_s(client->sessInfo.authKey, SESS_AUTH_KEY_LEN, 0, SESS_AUTH_KEY_LEN);
    SecuritySessDestroy(sessCtx);
    return false;
}

static LocalControlClient *LocalControlClientNew(LocalControlContext *ctx, const LocalClientBuildParam *param)
{
    LocalControlClient *newCli = (LocalControlClient *)IotcMalloc(sizeof(LocalControlClient));
    if (newCli == NULL) {
        IOTC_LOGW("malloc error");
        return NULL;
    }
    (void)memset_s(newCli, sizeof(LocalControlClient), 0, sizeof(LocalControlClient));

    newCli->timeInfo.createTime = IotcGetSysTimeMs();
    newCli->timeInfo.expireTime = ctx->config.clientExpireTime;
    newCli->sessInfo.recvSeq = SecurityRandomUint32() % UINT16_MAX;
    uint8_t sessId[LOCAL_CONTROL_SESS_ID_LEN] = {0};
    (void)SecurityRandom(sessId, sizeof(sessId));
    if (!UtilsHexify(sessId, sizeof(sessId), newCli->sessInfo.sessId, LOCAL_CONTROL_SESS_ID_STR_LEN)) {
        IOTC_LOGW("gen sess id error");
        IotcFree(newCli);
        return NULL;
    }
    newCli->appInfo.addr = param->addr;
    newCli->appInfo.puuidLen = param->puuidLen;
    newCli->appInfo.puuid = UtilsStrDupWithLen(param->puuid, param->puuidLen);
    if (newCli->appInfo.puuid == NULL) {
        IOTC_LOGW("str dup error");
        IotcFree(newCli);
        return NULL;
    }

    if (!LocalControlSessKeyCreate(newCli, param)) {
        IotcFree(newCli);
        return NULL;
    }

    return newCli;
}

static HashMapTravCode GetEarliestClientSessId(const void *value, va_list argp)
{
    CHECK_RETURN_LOGW(value != NULL, HASH_MAP_TRAVE_BREAK, "param invalid");

    const LocalControlClient *client = (const LocalControlClient *)value;
    LocalControlContext *ctx = va_arg(argp, LocalControlContext *);
    const LocalControlClient **earliestClient = va_arg(argp, const LocalControlClient **);
    uint32_t *maxLifeTime = va_arg(argp, uint32_t *);
    uint32_t curTime = va_arg(argp, uint32_t);
    if (ctx == NULL || maxLifeTime == NULL || earliestClient == NULL) {
        IOTC_LOGW("invalid arg");
        return HASH_MAP_TRAVE_BREAK;
    }

    uint32_t curCliLifeTime = UtilsDeltaTime(curTime, client->timeInfo.createTime);
    if (curCliLifeTime < *maxLifeTime) {
        return HASH_MAP_TRAVE_CONTINUE;
    }

    *maxLifeTime = curCliLifeTime;
    *earliestClient = client;
    return HASH_MAP_TRAVE_CONTINUE;
}

static int32_t LocalCtlClientFullProcess(LocalControlClient *client, LocalControlContext *ctx)
{
    if (ctx->clientManager.curClientNum < ctx->config.maxClientNum) {
        return IOTC_OK;
    }

    const LocalControlClient *earliestClient = NULL;
    uint32_t maxLifeTime = 0;
    uint32_t curTime = IotcGetSysTimeMs();
    (void)UtilsHashMapIterate(ctx->clientManager.clientMap, GetEarliestClientSessId,
        ctx, &earliestClient, &maxLifeTime, curTime);
    if (earliestClient == NULL) {
        IOTC_LOGW("get earliest client error");
        return IOTC_CORE_WIFI_LOCAL_CTL_ERR_GET_EARLIEST_CLIENT;
    }

    DFX_ANONYMIZE_IP_ADDR(anonyIp, earliestClient->appInfo.addr);
    DFX_ANONYMIZE_ID_STR(anonyPuuid, earliestClient->appInfo.puuid);
    IOTC_LOGI("client remove %s/%s/%u", anonyIp, anonyPuuid, client->timeInfo.createTime);

    int32_t ret = UtilsHashMapRemove(ctx->clientManager.clientMap, (const uint8_t *)earliestClient->sessInfo.sessId,
        LOCAL_CONTROL_SESS_ID_STR_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGW("remove earliest client error %d", ret);
        return ret;
    }
    ctx->clientManager.curClientNum = UtilsHashMapLength(ctx->clientManager.clientMap);
    return IOTC_OK;
}

static int32_t LocalControlClientInsert(LocalControlClient *client, LocalControlContext *ctx)
{
    int32_t ret = LocalCtlClientFullProcess(client, ctx);
    if (ret != IOTC_OK) {
        IOTC_LOGW("client full process error %d", ret);
        return ret;
    }

    ret = UtilsHashMapInsert(ctx->clientManager.clientMap, (const uint8_t *)client->sessInfo.sessId,
        LOCAL_CONTROL_SESS_ID_STR_LEN, client);
    if (ret != IOTC_OK) {
        IOTC_LOGW("insert client error %d", ret);
        return ret;
    }
    ctx->clientManager.curClientNum = UtilsHashMapLength(ctx->clientManager.clientMap);
    return IOTC_OK;
}

static void RemoveTargetPuuidClient(LocalControlContext *ctx, const char *puuid, uint32_t puuidLen)
{
    LocalControlClient *client = NULL;
    (void)UtilsHashMapIterate(ctx->clientManager.clientMap, ClientMapTraversalFindPuuid,
        puuid, puuidLen, &client);
    if (client == NULL) {
        return;
    }
    UtilsHashMapRemove(ctx->clientManager.clientMap, (const uint8_t *)client->sessInfo.sessId,
        LOCAL_CONTROL_SESS_ID_STR_LEN);
}

int32_t CreateLocalControlClient(LocalControlContext *ctx, const LocalClientBuildParam *param,
    LocalControlClient **client)
{
    CHECK_RETURN_LOGW(ctx != NULL && param != NULL && param->sn1 != NULL && param->sn2 != NULL &&
        param->sn1Len == SESS_SN_LEN && param->sn2Len == SESS_SN_LEN &&
        param->puuid != NULL && param->puuidLen != 0 &&
        param->puuidLen <= PUUID_MAX_STR_LEN && client != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    if (ctx->clientManager.clientMap == NULL) {
        IOTC_LOGW("hash map not init");
        return IOTC_CORE_WIFI_LOCAL_CTL_ERR_INVALID_CLIENT_MAP;
    }

    RemoveTargetPuuidClient(ctx, param->puuid, param->puuidLen);
    LocalControlClient *newClient = LocalControlClientNew(ctx, param);
    if (newClient == NULL) {
        IOTC_LOGW("create client error");
        return IOTC_CORE_WIFI_LOCAL_CTL_ERR_CREATE_CLIENT;
    }

    int32_t ret = LocalControlClientInsert(newClient, ctx);
    if (ret != IOTC_OK) {
        LocalCtlClientFree(newClient);
        return ret;
    }

    DFX_ANONYMIZE_IP_ADDR(anonyIp, param->addr);
    DFX_ANONYMIZE_ID_STR(anonyPuuid, newClient->appInfo.puuid);
    IOTC_LOGI("add new local client %s/%s", anonyIp, anonyPuuid);
    *client = newClient;
    return IOTC_OK;
}

void ClearAllLocalControlClient(LocalControlContext *ctx)
{
    CHECK_V_RETURN_LOGW(ctx != NULL, "param invalid");
    if (ctx->clientManager.clientMap == NULL) {
        IOTC_LOGW("hash map not init");
        return;
    }

    UtilsHashMapDelete(ctx->clientManager.clientMap);
    ctx->clientManager.curClientNum = 0;
    IOTC_LOGI("clear all local client");
}

static HashMapTravCode ClientMapTraversalActiveUpdateAddr(const void *value, va_list argp)
{
    CHECK_RETURN_LOGW(value != NULL, HASH_MAP_TRAVE_BREAK, "param invalid");
    LocalControlClient *client = (LocalControlClient *)value;

    const char *sessId = va_arg(argp, const char *);
    uint32_t addr = va_arg(argp, uint32_t);
    if (sessId == NULL) {
        IOTC_LOGW("invalid sessid");
        return HASH_MAP_TRAVE_BREAK;
    }

    if (strncmp(sessId, client->sessInfo.sessId, LOCAL_CONTROL_SESS_ID_STR_LEN) == 0) {
        client->appInfo.addr = addr;
        UTILS_BIT_SET(client->bitMap, LOCAL_CTL_CLI_BIT_SESS_ACTIVE);
    } else if (client->appInfo.addr == addr) {
        UTILS_BIT_RESET(client->bitMap, LOCAL_CTL_CLI_BIT_SESS_ACTIVE);
    } else {
        return HASH_MAP_TRAVE_CONTINUE;
    }

    DFX_ANONYMIZE_IP_ADDR(anonyIp, addr);
    DFX_ANONYMIZE_ID_STR(anonyPuuid, client->appInfo.puuid);
    if (UTILS_IS_BIT_SET(client->bitMap, LOCAL_CTL_CLI_BIT_SESS_ACTIVE)) {
        IOTC_LOGI("local sess active %s/%s", anonyIp, anonyPuuid);
    } else {
        IOTC_LOGI("local sess deactive %s/%s", anonyIp, anonyPuuid);
    }
    return HASH_MAP_TRAVE_CONTINUE;
}

void ClientSessActiveUpdateAddr(LocalControlContext *ctx, const char *sessId, uint32_t sessIdLen, uint32_t addr)
{
    CHECK_V_RETURN_LOGW(ctx != NULL && sessId != NULL && sessIdLen == LOCAL_CONTROL_SESS_ID_STR_LEN && addr != 0,
        "param invalid");

    if (ctx->clientManager.clientMap == NULL || ctx->clientManager.curClientNum == 0) {
        IOTC_LOGW("client map invalid");
        return;
    }

    (void)UtilsHashMapIterate(ctx->clientManager.clientMap, ClientMapTraversalActiveUpdateAddr, sessId, addr);
}