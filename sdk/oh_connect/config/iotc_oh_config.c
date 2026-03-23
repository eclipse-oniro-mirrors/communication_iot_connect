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
#include "iotc_oh_config.h"
#include "config_info.h"
#include "security_store.h"
#include "utils_assert.h"
#include "securec.h"
#include "config_revoke_flag.h"
#include "utils_mutex_global.h"
#include "iotc_kv.h"
#include "utils_common.h"
#include "iotc_errcode.h"

#define STORE_MAX_PATH_LEN 128
static char *g_storePath = NULL;

int32_t IotcOhStoreDataInit(void)
{
    char storePath[STORE_MAX_PATH_LEN + 1] = {0};
    int32_t ret;
    if (g_storePath != NULL) {
        UtilsGlobalMutexLock();
        int32_t ret = strcpy_s(storePath, sizeof(storePath), g_storePath);
        UtilsGlobalMutexUnlock();
        if (ret != IOTC_OK) {
            IOTC_LOGW("copy path error");
            return IOTC_ERR_SECUREC_SPRINTF;
        }
    }
    SecurityStoreCallback callbacks = {
        .onInit = IotcKvInit,
        .onDeinit = IotcKvDeInit,
        .onSetValue = IotcKvSetValue,
        .onGetValue = IotcKvGetValue,
        .onGetValueLen = IotcKvGetLen,
        .onDelValue = IotcKvDelValue,
    };
    if (storePath[0] == '\0') {
        strcpy_s(storePath, sizeof(storePath), SDK_CONFIG_PATH);
    }
    ret = SecurityStoreInit(storePath, &callbacks);
    if (ret != IOTC_OK) {
        IOTC_LOGW("utils store init error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

void IotcOhStoreDataDeinit(void)
{
    (void)UtilsGlobalMutexLock();
    UTILS_FREE_2_NULL(g_storePath);
    UtilsGlobalMutexUnlock();
    SecurityStoreDeinit();
}

int32_t IotcOhStorePathSet(const char *path)
{
    CHECK_RETURN_LOGE(path != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    if (strlen(path) > STORE_MAX_PATH_LEN) {
        IOTC_LOGW("path too long");
        return IOTC_ERR_PARAM_INVALID;
    }

    (void)UtilsGlobalMutexLock();
    UTILS_FREE_2_NULL(g_storePath);
    g_storePath = UtilsStrDup(path);
    if (g_storePath == NULL) {
        IOTC_LOGE("dup path error");
        UtilsGlobalMutexUnlock();
        return IOTC_CORE_COMM_UTILS_ERR_STR_DUP;
    }
    UtilsGlobalMutexUnlock();
    return IOTC_OK;
}