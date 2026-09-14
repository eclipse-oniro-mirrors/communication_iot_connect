/*
 * Copyright (c) 2026-2026 Huawei Device Co., Ltd.
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

/*
 * RAM-only replacement for the KV-persisted config layer (config_info.c /
 * config_authinfo.c / config_revoke*.c / security_store.c), used when
 * iot_connect_kv_support = false (board without a filesystem).
 *
 * Semantics: identical to a freshly-factory-reset device at every boot.
 * Binding/auth info written during the current power cycle is kept in RAM so
 * in-session flows (BLE auth setup, revoke, rebind checks) behave normally;
 * nothing survives a reboot.
 */
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "securec.h"
#include "utils_assert.h"
#include "iotc_errcode.h"
#include "config_info.h"
#include "config_authinfo.h"
#include "config_revoke_flag.h"
#include "config_revoke.h"
#include "security_store.h"

/* session-only state (cleared by BSS on boot) */
static DevAuthInfo g_stubAuthInfo = { 0 };
static bool g_stubRevokeFlag = false;

/* ---- ConfigInfo: no external callers besides config_*.c; keep RAM copy out ---- */

int32_t ConfigInfoInit(void)
{
    return IOTC_OK;
}

void ConfigInfoDeinit(void)
{
}

int32_t ConfigInfoGet(ConfigInfoKey key, uint8_t *buf, uint32_t *len)
{
    (void)key;
    CHECK_RETURN_LOGW(buf != NULL && len != NULL && *len != 0, IOTC_ERR_PARAM_INVALID, "param invalid");
    (void)memset_s(buf, *len, 0, *len);
    *len = 0;
    return IOTC_OK;
}

int32_t ConfigInfoSet(ConfigInfoKey key, const uint8_t *buf, uint32_t len)
{
    (void)key;
    CHECK_RETURN_LOGW(buf != NULL && len != 0, IOTC_ERR_PARAM_INVALID, "param invalid");
    return IOTC_OK;
}

int32_t ConfigInfoClear(ConfigInfoKey key)
{
    (void)key;
    return IOTC_OK;
}

int32_t ConfigInfoSave(void)
{
    return IOTC_OK;
}

/* ---- Auth info: RAM-backed so binding works within the current session ---- */

int32_t ConfigSaveAuthInfo(const DevAuthInfo *info)
{
    CHECK_RETURN_LOGW(info != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    int32_t ret = memcpy_s(&g_stubAuthInfo, sizeof(DevAuthInfo), info, sizeof(DevAuthInfo));
    CHECK_RETURN(ret == EOK, IOTC_ERR_SECUREC_MEMCPY);
    return IOTC_OK;
}

int32_t ConfigGetAuthInfo(DevAuthInfo *info)
{
    CHECK_RETURN_LOGW(info != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    int32_t ret = memcpy_s(info, sizeof(DevAuthInfo), &g_stubAuthInfo, sizeof(DevAuthInfo));
    CHECK_RETURN(ret == EOK, IOTC_ERR_SECUREC_MEMCPY);
    return IOTC_OK;
}

int32_t ConfigClearAuthInfo(void)
{
    (void)memset_s(&g_stubAuthInfo, sizeof(DevAuthInfo), 0, sizeof(DevAuthInfo));
    return IOTC_OK;
}

bool IsDeviceBinded(void)
{
    return g_stubAuthInfo.devId[0] != '\0';
}

/* ---- Revoke flag: RAM-backed ---- */

bool IsRevokeFlagExist(void)
{
    return g_stubRevokeFlag;
}

int32_t SetRevokeFlag(void)
{
    g_stubRevokeFlag = true;
    return IOTC_OK;
}

int32_t ClearRevokeFlag(void)
{
    g_stubRevokeFlag = false;
    return IOTC_OK;
}

int32_t ConfigRevokeInit(void)
{
    return IOTC_OK;
}

void ConfigRevokeDeinit(void)
{
}

/* ---- SecurityStore: no persistent backend, behave as always-empty store ---- */

int32_t SecurityStoreInit(const char *path, SecurityStoreCallback *cb)
{
    (void)path;
    (void)cb;
    return IOTC_OK;
}

void SecurityStoreDeinit(void)
{
}

int32_t SecurityStoreRegStoreList(SecurityStoreItem *list, uint32_t num)
{
    (void)list;
    (void)num;
    return IOTC_OK;
}

void SecurityStoreUnregStoreList(SecurityStoreItem *list)
{
    (void)list;
}

int32_t SecurityStoreSet(int32_t key, const uint8_t *value, uint32_t len)
{
    (void)key;
    (void)value;
    (void)len;
    return IOTC_OK;
}

int32_t SecurityStoreGet(int32_t key, uint8_t *value, uint32_t *len)
{
    (void)key;
    CHECK_RETURN_LOGW(value != NULL && len != NULL && *len != 0, IOTC_ERR_PARAM_INVALID, "param invalid");
    (void)memset_s(value, *len, 0, *len);
    *len = 0;
    return IOTC_OK;
}

int32_t SecurityStoreGetLen(int32_t key, uint32_t *len)
{
    (void)key;
    CHECK_RETURN_LOGW(len != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    *len = 0;
    return IOTC_OK;
}

int32_t SecurityStorePull(int32_t key)
{
    (void)key;
    return IOTC_OK;
}

int32_t SecurityStoreCommit(int32_t key)
{
    (void)key;
    return IOTC_OK;
}

int32_t SecurityStoreDel(int32_t key)
{
    (void)key;
    return IOTC_OK;
}

int32_t SecurityStorePullList(SecurityStoreItem *list)
{
    (void)list;
    return IOTC_OK;
}

int32_t SecurityStoreCommitList(SecurityStoreItem *list)
{
    (void)list;
    return IOTC_OK;
}

int32_t SecurityStoreDelList(SecurityStoreItem *list)
{
    (void)list;
    return IOTC_OK;
}

int32_t SecurityStorePullAll(void)
{
    return IOTC_OK;
}

int32_t SecurityStoreCommitAll(void)
{
    return IOTC_OK;
}

int32_t SecurityStoreDelAll(void)
{
    return IOTC_OK;
}
