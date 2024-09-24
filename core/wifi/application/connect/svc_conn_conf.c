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
#include "svc_conn_conf.h"
#include "iotc_errcode.h"
#include "utils_assert.h"
#include "security_store.h"
#include "iotc_store_key.h"
#include "securec.h"
#include "utils_common.h"

typedef enum {
    NET_INFO_FLAG_NEED_VERIFY = 1,
    NET_INFO_FLAG_CHECKED,
} NetInfoFlag;

#define CONNECT_DATA_VER 1
#define CONNECT_DATA_INFO_V1_RSV_LEN 126
#define CONNECT_DATA_INFO_V1_LEN 128

typedef struct {
    uint8_t ver;
    uint8_t netFlag;
    /* 预留空间，保证结构体大小为2048，新增成员只能从预留空间中分配 */
    uint8_t rsv[CONNECT_DATA_INFO_V1_RSV_LEN];
} ConnDataInfoV1;

CHECK_TYPE_SIZE(ConnDataInfoV1, CONNECT_DATA_INFO_V1_LEN);

static ConnDataInfoV1 g_connDataInfo;
static SecurityStoreItem g_storeItem[] = {
    { IOTC_STORE_KEY_INT_WIFI_CONN_CONFIG, IOTC_STORE_KEY_CHAR_WIFI_CONN_CONFIG, SECURITY_STORE_FLAG_VERIFY,
        (uint8_t *)&g_connDataInfo, sizeof(ConnDataInfoV1), sizeof(ConnDataInfoV1) },
};

int32_t ConnectConfigInfoInit(void)
{
    (void)memset_s(&g_connDataInfo, sizeof(ConnDataInfoV1), 0, sizeof(ConnDataInfoV1));
    int32_t ret = SecurityStoreRegStoreList(g_storeItem, ARRAY_SIZE(g_storeItem));
    if (ret != IOTC_OK) {
        IOTC_LOGW("reg store list error %d", ret);
        return ret;
    }
    ret = SecurityStorePull(IOTC_STORE_KEY_INT_WIFI_CONN_CONFIG);
    if (ret != IOTC_OK) {
        IOTC_LOGF("store data pull error %d", ret);
    }

    return IOTC_OK;
}

void ConnectConfigInfoDeinit(void)
{
    SecurityStoreUnregStoreList(g_storeItem);
}

static int32_t ConfigSetNetInfoFlag(uint8_t flag)
{
    g_connDataInfo.ver = CONNECT_DATA_VER;
    g_connDataInfo.netFlag = flag;
    g_storeItem[0].len = sizeof(ConnDataInfoV1);
    int32_t ret = SecurityStoreCommit(IOTC_STORE_KEY_INT_WIFI_CONN_CONFIG);
    if (ret != IOTC_OK) {
        IOTC_LOGW("set flag error %u/%d", flag, ret);
        return ret;
    }

    IOTC_LOGI("save conn flag %u", flag);
    return IOTC_OK;
}

int32_t ConfigSetNetInfoNeedVerify(void)
{
    return ConfigSetNetInfoFlag(NET_INFO_FLAG_NEED_VERIFY);
}

int32_t ConfigSetNetInfoChecked(void)
{
    return ConfigSetNetInfoFlag(NET_INFO_FLAG_CHECKED);
}

int32_t ConfigClearNetInfoFlag(void)
{
    return ConfigSetNetInfoFlag(0);
}

static bool ConfigCheckNetInfoFlag(uint8_t flag)
{
    return g_connDataInfo.netFlag == flag;
}

bool ConfigIsNetInfoNeedVerify(void)
{
    return ConfigCheckNetInfoFlag(NET_INFO_FLAG_NEED_VERIFY);
}

bool ConfigIsNetInfoChecked(void)
{
    return ConfigCheckNetInfoFlag(NET_INFO_FLAG_CHECKED);
}