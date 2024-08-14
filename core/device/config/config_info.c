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
#include "config_info.h"
#include "securec.h"
#include "iotc_errcode.h"
#include "utils_mutex_global.h"
#include "utils_assert.h"
#include "comm_def.h"
#include "utils_common.h"
#include "security_store.h"
#include "iotc_store_key.h"

#define STORE_DATA_VER 1
#define STORE_DATA_INFO_V1_RSV_LEN 1703
#define STORE_DATA_INFO_V1_LEN 2048

typedef struct {
    uint8_t ver;
    uint8_t revokeFlag;
    char devId[DEVICE_ID_MAX_STR_LEN + 1];
    char secret[CLOUD_SECRET_MAX_STR_LEN + 1];
    uint8_t psk[CLOUD_PSK_MAX_LEN];
    char url[CLOUD_MAX_URL_LEN + 1];
    char backupUrl[CLOUD_MAX_URL_LEN + 1];
    /* ble */
    char uidHash[BLE_UID_HASH_LEN + 1];
    char authCodeId[BLE_AUTHCODE_ID_LEN + 1];
    uint8_t authCode[BLE_AUTHCODE_LEN];
    /* 预留空间，保证结构体大小为2048，新增成员只能从预留空间中分配 */
    uint8_t netInfoFlag;
    uint8_t rsv[STORE_DATA_INFO_V1_RSV_LEN];
} StoreDataInfoV1;

CHECK_TYPE_SIZE(StoreDataInfoV1, STORE_DATA_INFO_V1_LEN);

typedef struct {
    ConfigInfoKey key;
    uint8_t *buf;
    uint32_t size;
} ConfigInfoItem;

#define CONFIG_KEY_ITEM(_key, _mem) \
    { \
        .key = (_key), \
        .buf = (uint8_t *)&(_mem), \
        .size = sizeof(_mem), \
    }

static StoreDataInfoV1 g_storeInfo;

static SecurityStoreItem g_storeItem[] = {
    { IOTC_STORE_KEY_INT_DEVICE_CONFIG, IOTC_STORE_KEY_CHAR_DEVICE_CONFIG, SECURITY_STORE_FLAG_ENCRYPT,
        (uint8_t *)&g_storeInfo, sizeof(StoreDataInfoV1), sizeof(StoreDataInfoV1) },
};

static const ConfigInfoItem CONFIG_LIST[] = {
    CONFIG_KEY_ITEM(CONFIG_INFO_KEY_REVOKE_FLAG, g_storeInfo.revokeFlag),
    CONFIG_KEY_ITEM(CONFIG_INFO_KEY_DEVICE_ID, g_storeInfo.devId),
    CONFIG_KEY_ITEM(CONFIG_INFO_KEY_LOGIN_SECRET, g_storeInfo.secret),
    CONFIG_KEY_ITEM(CONFIG_INFO_KEY_LOGIN_PSK, g_storeInfo.psk),
    CONFIG_KEY_ITEM(CONFIG_INFO_KEY_LOGIN_URL, g_storeInfo.url),
    CONFIG_KEY_ITEM(CONFIG_INFO_KEY_LOGIN_BACKUP_URL, g_storeInfo.backupUrl),
    CONFIG_KEY_ITEM(CONFIG_INFO_KEY_UID_HASH, g_storeInfo.uidHash),
    CONFIG_KEY_ITEM(CONFIG_INFO_KEY_AUTHCODE_ID, g_storeInfo.authCodeId),
    CONFIG_KEY_ITEM(CONFIG_INFO_KEY_AUTHCODE, g_storeInfo.authCode),
    CONFIG_KEY_ITEM(CONFIG_INFO_KEY_NETINFO_FLAG, g_storeInfo.netInfoFlag),
};

int32_t ConfigInfoInit(void)
{
    (void)memset_s(&g_storeInfo, sizeof(StoreDataInfoV1), 0, sizeof(StoreDataInfoV1));
    int32_t ret = SecurityStoreRegStoreList(g_storeItem, ARRAY_SIZE(g_storeItem));
    if (ret != IOTC_OK) {
        IOTC_LOGW("reg store list error %d", ret);
        return ret;
    }
    ret = SecurityStorePull(IOTC_STORE_KEY_INT_DEVICE_CONFIG);
    if (ret != IOTC_OK) {
        IOTC_LOGF("store data pull error %d", ret);
    }
    /* 填充字符串结束符，避免非法的flash数据造成读越界 */
    g_storeInfo.devId[DEVICE_ID_MAX_STR_LEN] = '\0';
    g_storeInfo.secret[CLOUD_SECRET_MAX_STR_LEN] = '\0';
    g_storeInfo.url[CLOUD_MAX_URL_LEN] = '\0';
    g_storeInfo.backupUrl[CLOUD_MAX_URL_LEN] = '\0';
    g_storeInfo.uidHash[BLE_UID_HASH_LEN] = '\0';
    g_storeInfo.authCodeId[BLE_AUTHCODE_ID_LEN] = '\0';
    return IOTC_OK;
}

void ConfigInfoDeinit(void)
{
    SecurityStoreUnregStoreList(g_storeItem);
}

static const ConfigInfoItem *GetConfigInfoItem(ConfigInfoKey key)
{
    for (uint32_t i = 0; i < ARRAY_SIZE(CONFIG_LIST); ++i) {
        if (CONFIG_LIST[i].key == key) {
            return &CONFIG_LIST[i];
        }
    }
    return NULL;
}

int32_t ConfigInfoGet(ConfigInfoKey key, uint8_t *buf, uint32_t *len)
{
    CHECK_RETURN_LOGW(buf != NULL && len != NULL && *len != 0, IOTC_ERR_PARAM_INVALID, "param invalid");
    const ConfigInfoItem *item = GetConfigInfoItem(key);
    CHECK_RETURN_LOGW(item != NULL && *len >= item->size, IOTC_ERR_PARAM_INVALID, "key invalid %d/%u", key, *len);

    int32_t ret = memcpy_s(buf, *len, item->buf, item->size);
    if (ret != EOK) {
        IOTC_LOGW("copy error %u/%u", *len, item->size);
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    *len = item->size;
    return IOTC_OK;
}

int32_t ConfigInfoSet(ConfigInfoKey key, const uint8_t *buf, uint32_t len)
{
    CHECK_RETURN_LOGW(buf != NULL && len != 0, IOTC_ERR_PARAM_INVALID, "param invalid");
    const ConfigInfoItem *item = GetConfigInfoItem(key);
    CHECK_RETURN_LOGW(item != NULL && len <= item->size, IOTC_ERR_PARAM_INVALID, "key invalid %d/%u", key, len);

    int32_t ret = memcpy_s(item->buf, item->size, buf, len);
    if (ret != EOK) {
        IOTC_LOGW("copy error %u/%u", len, item->size);
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    return IOTC_OK;
}

int32_t ConfigInfoClear(ConfigInfoKey key)
{
    const ConfigInfoItem *item = GetConfigInfoItem(key);
    CHECK_RETURN_LOGW(item != NULL, IOTC_ERR_PARAM_INVALID, "key invalid %d", key);
    (void)memset_s(item->buf, item->size, 0, item->size);
    return IOTC_OK;
}

int32_t ConfigInfoSave(void)
{
    g_storeInfo.ver = STORE_DATA_VER;
    g_storeItem[0].len = sizeof(StoreDataInfoV1);
    return SecurityStoreCommit(IOTC_STORE_KEY_INT_DEVICE_CONFIG);
}