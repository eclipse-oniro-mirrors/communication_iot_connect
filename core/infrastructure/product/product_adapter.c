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

#include "product_adapter.h"
#include "utils_mutex_global.h"
#include "securec.h"
#include "iotc_errcode.h"
#include "utils_assert.h"

typedef int32_t (*GetRootCaCertCallback)(const char **ca[], uint32_t *num);
typedef int32_t (*RecvNetCfgInfoCallback)(const char *netInfo, uint32_t len);
typedef int32_t (*RecvCustomSecDataCallback)(const uint8_t *data, uint32_t len);
typedef int32_t (*ProfPutCharStateCallback)(const IotcCharState state[], uint32_t num);
typedef int32_t (*GetSurfacePowerCallback)(int8_t *power);
typedef int32_t (*ProfGetCharStateCallback)(const IotcCharState state[], char *out[], uint32_t len[], uint32_t num);
typedef int32_t (*ProfReportAllCallback)(void);
typedef int32_t (*ProfGetPincodeCallback)(uint8_t *buf, uint32_t bufLen);
typedef int32_t (*ProfGetAcKeyCallback)(uint8_t *buf, uint32_t bufLen);
typedef void (*ProfFreeCallback)(void *ptr);
typedef int32_t (*DevRebootCallback)(int32_t res);
typedef int32_t (*DevTrngCallback)(uint8_t *buf, uint32_t len);

static ProductHooks g_hooks = {0};

static int32_t RegisterAllHooks(const ProductHooks *hooks)
{
    (void)UtilsGlobalMutexLock();
    int32_t ret = memcpy_s(&g_hooks, sizeof(ProductHooks), hooks, sizeof(ProductHooks));
    UtilsGlobalMutexUnlock();
    if (ret != EOK) {
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    return IOTC_OK;
}

#define HOOK_CHECK_ASSIGN(_src, _dst, _member) \
    do { \
        if ((_src)->_member != NULL) { \
            (_dst)->_member = (_src)->_member; \
        }; \
    } while (0)

int32_t ProductRegisterHooks(const ProductHooks *hooks, ProdHookRegPolicy policy)
{
    CHECK_RETURN_LOGW(hooks != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    if (policy == PROD_HOOK_REG_POLICY_COVER_ALL) {
        IOTC_LOGI("prod hook cover all");
        return RegisterAllHooks(hooks);
    }

    (void)UtilsGlobalMutexLock();
    HOOK_CHECK_ASSIGN(hooks, &g_hooks, onGetRootCaCert);
    HOOK_CHECK_ASSIGN(hooks, &g_hooks, onRecvNetCfgInfo);
    HOOK_CHECK_ASSIGN(hooks, &g_hooks, onRecvCustomSecData);
    HOOK_CHECK_ASSIGN(hooks, &g_hooks, onGetSurfacePower);
    HOOK_CHECK_ASSIGN(hooks, &g_hooks, onProfPutCharState);
    HOOK_CHECK_ASSIGN(hooks, &g_hooks, onProfGetCharState);
    HOOK_CHECK_ASSIGN(hooks, &g_hooks, onProfReportAll);
    HOOK_CHECK_ASSIGN(hooks, &g_hooks, onProfGetPincode);
    HOOK_CHECK_ASSIGN(hooks, &g_hooks, onProfGetAcKey);
    HOOK_CHECK_ASSIGN(hooks, &g_hooks, onProfFree);
    HOOK_CHECK_ASSIGN(hooks, &g_hooks, onDevReboot);
    HOOK_CHECK_ASSIGN(hooks, &g_hooks, onDevTrng);
    UtilsGlobalMutexUnlock();

    return IOTC_OK;
}

#define GET_PRODUCT_CALLBACK_RETURN(_cb, _hooks, _member) \
    do { \
        (void)UtilsGlobalMutexLock(); \
        (_cb) = (_hooks)._member; \
        UtilsGlobalMutexUnlock(); \
        if ((_cb) == NULL) { \
            IOTC_LOGI("callback is null"); \
            return IOTC_ERR_CALLBACK_NULL; \
        } \
    } while (0)

int32_t ProductGetRootCaCert(const char **ca[], uint32_t *num)
{
    CHECK_RETURN_LOGW(ca != NULL && num != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    GetRootCaCertCallback cb = NULL;
    GET_PRODUCT_CALLBACK_RETURN(cb, g_hooks, onGetRootCaCert);

    int32_t ret = cb(ca, num);
    if (ret != IOTC_OK || *num == 0 || *ca == NULL) {
        IOTC_LOGW("get custom ca error %d/%u", ret, *num);
        return IOTC_CORE_WIFI_DATA_ERR_GET_CUSTOM_CA;
    }
    return IOTC_OK;
}

int32_t ProductRecvNetCfgInfo(const char *netInfo, uint32_t len)
{
    CHECK_RETURN_LOGW(netInfo != NULL && len != 0, IOTC_ERR_PARAM_INVALID, "param invalid");

    RecvNetCfgInfoCallback cb = NULL;
    GET_PRODUCT_CALLBACK_RETURN(cb, g_hooks, onRecvNetCfgInfo);

    int32_t ret = cb(netInfo, len);
    if (ret != IOTC_OK) {
        IOTC_LOGW("recv netcfg info process error %d", ret);
    }
    return ret;
}

int32_t ProductRecvCustomSecData(const uint8_t *data, uint32_t len)
{
    CHECK_RETURN_LOGW(data != NULL && len != 0, IOTC_ERR_PARAM_INVALID, "param invalid");

    RecvCustomSecDataCallback cb = NULL;
    GET_PRODUCT_CALLBACK_RETURN(cb, g_hooks, onRecvCustomSecData);

    int32_t ret = cb(data, len);
    if (ret != IOTC_OK) {
        IOTC_LOGW("recv custom sec data process error %d", ret);
    }
    return ret;
}

int32_t ProductGetSurfacePower(int8_t *power)
{
    CHECK_RETURN_LOGW(power != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    GetSurfacePowerCallback cb = NULL;
    GET_PRODUCT_CALLBACK_RETURN(cb, g_hooks, onGetSurfacePower);

    int32_t ret = cb(power);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get surface power error %d", ret);
    }
    return ret;
}

int32_t ProductProfPutCharState(const IotcCharState state[], uint32_t num)
{
    CHECK_RETURN_LOGW(state != NULL && num != 0, IOTC_ERR_PARAM_INVALID, "param invalid");

    ProfPutCharStateCallback cb = NULL;
    GET_PRODUCT_CALLBACK_RETURN(cb, g_hooks, onProfPutCharState);

    int32_t ret = cb(state, num);
    if (ret != IOTC_OK) {
        IOTC_LOGW("put char error %d", ret);
    }
    return ret;
}

int32_t ProductProfGetCharState(const IotcCharState state[], char *out[], uint32_t len[], uint32_t num)
{
    CHECK_RETURN_LOGW(state != NULL && out != NULL && len != NULL && num != 0,
        IOTC_ERR_PARAM_INVALID, "param invalid");

    ProfGetCharStateCallback cb = NULL;
    GET_PRODUCT_CALLBACK_RETURN(cb, g_hooks, onProfGetCharState);

    int32_t ret = cb(state, out, len, num);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get char error %d", ret);
    }
    return ret;
}

int32_t ProductProfReportAll(void)
{
    ProfReportAllCallback cb = NULL;
    GET_PRODUCT_CALLBACK_RETURN(cb, g_hooks, onProfReportAll);

    int32_t ret = cb();
    if (ret != IOTC_OK) {
        IOTC_LOGW("report all error %d", ret);
    }
    return ret;
}

int32_t ProductProfGetPincode(uint8_t *buf, uint32_t bufLen)
{
    CHECK_RETURN_LOGW(buf != NULL && bufLen != 0, IOTC_ERR_PARAM_INVALID, "param invalid");

    ProfGetPincodeCallback cb = NULL;
    GET_PRODUCT_CALLBACK_RETURN(cb, g_hooks, onProfGetPincode);

    int32_t ret = cb(buf, bufLen);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get pin error %d", ret);
    }
    return ret;
}

int32_t ProductProfGetAcKey(uint8_t *buf, uint32_t bufLen)
{
    CHECK_RETURN_LOGW(buf != NULL && bufLen != 0, IOTC_ERR_PARAM_INVALID, "param invalid");

    ProfGetAcKeyCallback cb = NULL;
    GET_PRODUCT_CALLBACK_RETURN(cb, g_hooks, onProfGetAcKey);

    int32_t ret = cb(buf, bufLen);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get ac key error %d", ret);
    }
    return ret;
}

void ProductProfFree(void *ptr)
{
    CHECK_V_RETURN_LOGW(ptr != NULL, "param invalid");
    (void)UtilsGlobalMutexLock();
    ProfFreeCallback cb = g_hooks.onProfFree;
    UtilsGlobalMutexUnlock();
    if (cb == NULL) {
        IOTC_LOGF("prof free null");
        return;
    }

    cb(ptr);
}

int32_t ProductDevReboot(int8_t res)
{
    DevRebootCallback cb = NULL;
    GET_PRODUCT_CALLBACK_RETURN(cb, g_hooks, onDevReboot);

    int32_t ret = cb(res);
    if (ret != IOTC_OK) {
        IOTC_LOGW("reboot error %d", ret);
    }
    return ret;
}

int32_t ProductDevTrng(uint8_t *buf, uint32_t bufLen)
{
    CHECK_RETURN_LOGW(buf != NULL && bufLen != 0, IOTC_ERR_PARAM_INVALID, "param invalid");

    DevTrngCallback cb = NULL;
    GET_PRODUCT_CALLBACK_RETURN(cb, g_hooks, onDevTrng);

    int32_t ret = cb(buf, bufLen);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get trng error %d", ret);
    }
    return ret;
}
