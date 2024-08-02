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
#include "security_key.h"
#include <stddef.h>
#include "iotc_log.h"
#include "utils_mutex_global.h"
#include "adapter_aes.h"
#include "adapter_kdf.h"
#include "securec.h"
#include "utils_assert.h"
#include "iotc_errcode.h"

static SecurityGetPskCallback g_pskCb = NULL;
static SecurityGetAcKey g_acCb = NULL;
static SecurityGetUdid g_udidCb = NULL;

void SecurityRegUdidCallback(SecurityGetUdid udidCb)
{
    if (udidCb != NULL) {
        GLOBAL_LOCK_PROTECT(g_udidCb = udidCb);
        IOTC_LOGN("uuid callback reg");
    }
    return;
}

void SecurityRegPskCallback(SecurityGetPskCallback pskCb)
{
    if (pskCb != NULL) {
        GLOBAL_LOCK_PROTECT(g_pskCb = pskCb);
        IOTC_LOGN("psk callback reg");
    }
    return;
}

void SecurityRegAcKeyCallback(SecurityGetAcKey acCb)
{
    if (acCb != NULL) {
        GLOBAL_LOCK_PROTECT(g_acCb = acCb);
        IOTC_LOGN("psk callback reg");
    }
    return;
}

int32_t SecurityGetPsk(uint8_t *buf, uint32_t len)
{
    if (buf == NULL || len < SECURITY_PSK_LEN) {
        IOTC_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    UtilsGlobalMutexLock();
    SecurityGetPskCallback pskCallback = g_pskCb;
    UtilsGlobalMutexUnlock();

    CHECK_RETURN_LOGW(pskCallback != NULL, IOTC_CORE_COMM_SEC_ERR_NO_PSK_CALLBACK, "no psk cb");

    int32_t ret = pskCallback(buf, len);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get psk error %d", ret);
        return IOTC_CORE_COMM_SEC_ERR_GET_PSK;
    }

    return IOTC_OK;
}

int32_t SecurityGenHkdfLocalKey(const uint8_t *salt, uint32_t saltLen, uint8_t out[SECURITY_HKDF_LOCAL_KEY_LEN])
{
    if (salt == NULL || saltLen == 0 || out == NULL) {
        IOTC_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    UtilsGlobalMutexLock();
    SecurityGetAcKey acKeyCallback = g_acCb;
    SecurityGetUdid udidCallback = g_udidCb;
    UtilsGlobalMutexUnlock();

    CHECK_RETURN_LOGW(udidCallback != NULL, IOTC_CORE_COMM_SEC_ERR_NO_UDID, "no udid cb");
    CHECK_RETURN_LOGW(acKeyCallback != NULL, IOTC_CORE_COMM_SEC_ERR_NO_AC_KEY, "no ac key cb");

    uint8_t udid[SECURITY_UDID_LEN] = {0};
    int32_t ret = udidCallback(udid, sizeof(udid));
    if (ret != 0) {
        IOTC_LOGW("get udid error %d", ret);
        return IOTC_CORE_COMM_SEC_ERR_GET_UDID;
    }

    uint8_t acKey[IOTC_AC_KEY_LEN] = {0};
    ret = acKeyCallback(acKey, sizeof(acKey));
    if (ret != 0) {
        IOTC_LOGW("get ac key error %d", ret);
        return IOTC_CORE_COMM_SEC_ERR_GET_AC_KEY;
    }

    AdapterHkdfParam hkdf = {
        .md = ADAPTER_MD_SHA256,
        .salt = salt,
        .saltLen = saltLen,
        .info = udid,
        .infoLen = sizeof(udid),
        .material = acKey,
        .materialLen = sizeof(acKey),
    };
    ret = AdapterHkdf(&hkdf, out, SECURITY_HKDF_LOCAL_KEY_LEN);
    (void)memset_s(acKey, sizeof(acKey), 0, sizeof(acKey));
    return ret;
}