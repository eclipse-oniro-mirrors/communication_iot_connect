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
#include "security_random.h"
#include <stddef.h>
#include "comm_def.h"
#include "iotc_drbg.h"
#include "iotc_log.h"
#include "utils_mutex_local.h"
#include "utils_mutex_global.h"
#include "utils_assert.h"
#include "securec.h"
#include "iotc_errcode.h"

static IotcDrbgContext *g_randomCtx = NULL;
static UtilsMutexLocal g_mutexLocal;
static SecurityTrng g_trngCallback = NULL;
static const char *RANDOM_CUSTOM_STR = "iotc_random";

void SecurityRandomRegTrng(SecurityTrng trng)
{
    CHECK_V_RETURN_LOGW(trng != NULL, "param invalid");
    (void)UtilsGlobalMutexLock();
    g_trngCallback = trng;
    UtilsGlobalMutexUnlock();
}

int32_t SecurityRandomInit(void)
{
    if (g_randomCtx != NULL) {
        return IOTC_OK;
    }
    (void)memset_s(&g_mutexLocal, sizeof(UtilsMutexLocal), 0, sizeof(UtilsMutexLocal));
    (void)UtilsGlobalMutexLock();
    IotcTrngCallback trng = g_trngCallback;
    UtilsGlobalMutexUnlock();
    if (trng != NULL) {
        IOTC_LOGI("use trng for random seed");
    } else {
        IOTC_LOGI("trng not use");
    }
    g_randomCtx = IotcDrbgInit(RANDOM_CUSTOM_STR, trng);
    if (g_randomCtx == NULL) {
        IOTC_LOGW("drbg init error");
        return IOTC_ADAPTER_CRYPTO_ERR_DRBG_INIT;
    }

    int32_t ret = UtilsCreateMutexLocal(&g_mutexLocal);
    if (ret != IOTC_OK) {
        IOTC_LOGW("mutex local create error %d", ret);
        SecurityRandomDeinit();
        return IOTC_CORE_COMM_UTILS_ERR_LOCAL_MUTEX_CREATE;
    }

    return IOTC_OK;
}

void SecurityRandomDeinit(void)
{
    if (g_randomCtx != NULL) {
        IotcDrbgDeinit(g_randomCtx);
        g_randomCtx = NULL;
    }
    UtilsDestroyMutexLocal(&g_mutexLocal);
}

int32_t SecurityRandom(uint8_t *buf, uint32_t bufLen)
{
    if (buf == NULL || bufLen == 0) {
        IOTC_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    if (g_randomCtx == NULL) {
        IOTC_LOGW("random not init");
        return IOTC_ERR_NOT_INIT;
    }

    if (!UtilsMutexLocalLock(&g_mutexLocal)) {
        IOTC_LOGW("lock error");
        return IOTC_ERR_TIMEOUT;
    }

    int32_t ret = IotcDrbgRandom(g_randomCtx, buf, bufLen);
    if (ret != IOTC_OK) {
        UtilsMutexLocalUnlock(&g_mutexLocal);
        return ret;
    }
    UtilsMutexLocalUnlock(&g_mutexLocal);
    return IOTC_OK;
}

uint32_t SecurityRandomUint32(void)
{
    uint32_t rand = 0;
    int32_t ret = SecurityRandom((uint8_t *)&rand, sizeof(rand));
    if (ret != IOTC_OK) {
        IOTC_LOGW("get uint32 random error %d", ret);
    }

    return rand;
}