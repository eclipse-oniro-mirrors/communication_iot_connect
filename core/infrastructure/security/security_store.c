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
#include "security_store.h"
#include "utils_list.h"
#include "utils_bit_map.h"
#include "utils_mutex_global.h"
#include "iotc_log.h"
#include "utils_assert.h"
#include "utils_common.h"
#include "utils_mutex_ex.h"
#include "securec.h"
#include "adapter_md.h"
#include "security_key.h"
#include "adapter_aes.h"
#include "security_random.h"
#include "iotc_errcode.h"

#define MAX_STORE_LIST_ITEM_NUM 128
#define STORE_CIPHER_SALT_LEN 16
#define STORE_CIPHER_GCM_KEY_LEN 16
#define STORE_CIPHER_GCM_IV_LEN 16
#define STORE_CIPHER_VER_1 1

typedef struct {
    ListEntry node;
    uint32_t num;
    SecurityStoreItem *items;
} StoreItemsNode;

typedef struct {
    UtilsExMutex *mutex;
    ListEntry storeList;
    SecurityStoreCallback callback;
} UtilsStoreContext;

typedef enum {
    STORE_OP_READ = 0,
    STORE_OP_WRITE,
    STORE_OP_DEL,
} StoreOp;

typedef struct {
    uint8_t ver;
    uint8_t rsv[3]; /* 对齐 */
    uint8_t salt[STORE_CIPHER_SALT_LEN];
    uint8_t mac[IOTC_MD_SHA256_BYTE_LEN];
    uint32_t len;
} StoreCipherHeader;

/* 该值应为所有版本解密头的最大值 */
#define MAX_STORE_CIPHER_HEADER_LEN (UTILS_MAX(sizeof(StoreCipherHeader), 0))

static const char *IOTC_DEFAULT_PATH = "iotc";

static UtilsStoreContext *GetContext(void)
{
    static UtilsStoreContext ctx;
    return &ctx;
}

#define STORE_CTX_LOCK_RET() \
    do { \
        if (!UtilsExMutexLock(GetContext()->mutex)) { \
            IOTC_LOGW("lock timeout"); \
            return IOTC_ERR_TIMEOUT; \
        } \
    } while (0)

#define STORE_CTX_LOCK_V_RET() \
    do { \
        if (!UtilsExMutexLock(GetContext()->mutex)) { \
            IOTC_LOGW("lock timeout"); \
            return; \
        } \
    } while (0)

#define STORE_CTX_UNLOCK() UtilsExMutexUnlock(GetContext()->mutex)
#define STORE_ITEM_LOGW_CODE(tag, code, item) IOTC_LOGW("%s %d, key:%s/%d len:%u/%u mode:%d", \
    tag, (code), NON_NULL_STR((item)->keyChar), (item)->keyInt, (item)->len, (item)->size, (item)->mode)
#define STORE_ITEM_LOGW(tag, item) IOTC_LOGW("%s, key:%s/%d len:%u/%u mode:%d", \
    tag, NON_NULL_STR((item)->keyChar), (item)->keyInt, (item)->len, (item)->size, (item)->mode)
#define STORE_ITEM_LOGI(tag, item) IOTC_LOGI("%s, key:%s/%d len:%u/%u mode:%d", \
    tag, NON_NULL_STR((item)->keyChar), (item)->keyInt, (item)->len, (item)->size, (item)->mode)

static int32_t UtilsStoreKvInit(const char *tag)
{
    if (GetContext()->callback.onInit != NULL) {
        return GetContext()->callback.onInit(tag);
    }
    return IOTC_ERR_CALLBACK_NULL;
}

static int32_t UtilsStoreKvDeInit(void)
{
    if (GetContext()->callback.onDeinit != NULL) {
        return GetContext()->callback.onDeinit();
    }
    return IOTC_ERR_CALLBACK_NULL;
}

static int32_t UtilsStoreKvSetValue(const char *key, const uint8_t *value, uint32_t len)
{
    if (GetContext()->callback.onSetValue != NULL) {
        return GetContext()->callback.onSetValue(key, value, len);
    }
    return IOTC_ERR_CALLBACK_NULL;
}

static int32_t UtilsStoreKvGetValue(const char *key, uint8_t *buf, uint32_t *len)
{
    if (GetContext()->callback.onGetValue != NULL) {
        return GetContext()->callback.onGetValue(key, buf, len);
    }
    return IOTC_ERR_CALLBACK_NULL;
}

static int32_t UtilsStoreKvGetLen(const char *key, uint32_t *len)
{
    if (GetContext()->callback.onGetValueLen != NULL) {
        return GetContext()->callback.onGetValueLen(key, len);
    }
    return IOTC_ERR_CALLBACK_NULL;
}

static int32_t UtilsStoreKvDelValue(const char *key)
{
    if (GetContext()->callback.onDelValue != NULL) {
        return GetContext()->callback.onDelValue(key);
    }
    return IOTC_ERR_CALLBACK_NULL;
}

int32_t SecurityStoreInit(const char *path, SecurityStoreCallback *cb)
{
    CHECK_RETURN_LOGW(cb != NULL && cb->onInit != NULL && cb->onDeinit != NULL &&
        cb->onGetValue != NULL && cb->onSetValue != NULL && cb->onGetValueLen &&
        cb->onDelValue != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    (void)memset_s(GetContext(), sizeof(UtilsStoreContext), 0, sizeof(UtilsStoreContext));
    LIST_INIT(&GetContext()->storeList);
    int32_t ret = memcpy_s(&GetContext()->callback, sizeof(SecurityStoreCallback), cb, sizeof(SecurityStoreCallback));
    if (ret != EOK) {
        return IOTC_ERR_SECUREC_MEMCPY;
    }

    GetContext()->mutex = UtilsCreateExMutex();
    if (GetContext()->mutex == NULL) {
        IOTC_LOGW("create ex mutex error");
        return IOTC_CORE_COMM_UTILS_ERR_EX_MUTEX_CREATE;
    }

    const char *storePath =  UtilsIsEmptyStr(path) ? IOTC_DEFAULT_PATH : path;
    IOTC_LOGI("init store to %s", storePath);
    ret = UtilsStoreKvInit(storePath);
    if (ret != IOTC_OK) {
        IOTC_LOGW("store kv init error");
        SecurityStoreDeinit();
        return ret;
    }
    return IOTC_OK;
}

void SecurityStoreDeinit(void)
{
    (void)UtilsStoreKvDeInit();
    UtilsDestroyExMutex(&GetContext()->mutex);

    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &GetContext()->storeList) {
        StoreItemsNode *node = CONTAINER_OF(item, StoreItemsNode, node);
        LIST_REMOVE(&node->node);
        IotcFree(node);
    }
}

static StoreItemsNode *StoreItemsNodeNew(SecurityStoreItem *list, uint32_t num)
{
    StoreItemsNode *newNode = (StoreItemsNode *)IotcMalloc(sizeof(StoreItemsNode));
    if (newNode == NULL) {
        IOTC_LOGW("malloc error");
        return NULL;
    }

    (void)memset_s(newNode, sizeof(StoreItemsNode), 0, sizeof(StoreItemsNode));
    newNode->items = list;
    newNode->num = num;
    return newNode;
}

int32_t SecurityStoreRegStoreList(SecurityStoreItem *list, uint32_t num)
{
    CHECK_RETURN(list != NULL && num != 0 && num <= MAX_STORE_LIST_ITEM_NUM, IOTC_ERR_PARAM_INVALID);
    STORE_CTX_LOCK_RET();

    /* 去重 */
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &GetContext()->storeList) {
        StoreItemsNode *node = CONTAINER_OF(item, StoreItemsNode, node);
        if (node->items == list) {
            STORE_CTX_UNLOCK();
            return IOTC_OK;
        }
    }

    StoreItemsNode *newNode = StoreItemsNodeNew(list, num);
    if (newNode == NULL) {
        STORE_CTX_UNLOCK();
        return IOTC_CORE_COMM_UTILS_ERR_STORE_CREATE_NEW_NODE;
    }

    LIST_INSERT_BEFORE(&newNode->node, &GetContext()->storeList);

    STORE_CTX_UNLOCK();
    return IOTC_OK;
}

void SecurityStoreUnregStoreList(SecurityStoreItem *list)
{
    CHECK_V_RETURN(list != NULL);
    STORE_CTX_LOCK_V_RET();

    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &GetContext()->storeList) {
        StoreItemsNode *node = CONTAINER_OF(item, StoreItemsNode, node);
        if (node->items != list) {
            continue;
        }
        LIST_REMOVE(&node->node);
        IotcFree(node);
        break;
    }

    STORE_CTX_UNLOCK();
    return;
}

static int32_t GetStoreItemByKey(int32_t key, SecurityStoreItem **storeItem)
{
    uint32_t i = 0;
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &GetContext()->storeList) {
        StoreItemsNode *node = CONTAINER_OF(item, StoreItemsNode, node);
        for (i = 0; i < node->num; ++i) {
            if (node->items[i].keyInt == key) {
                *storeItem = &node->items[i];
                return IOTC_OK;
            }
        }
    }
    IOTC_LOGW("invalid key %d", key);
    return IOTC_CORE_COMM_UTILS_ERR_STORE_INVALID_KEY;
}

int32_t SecurityStoreSet(int32_t key, const uint8_t *value, uint32_t len)
{
    CHECK_RETURN(value != NULL && len != 0, IOTC_ERR_PARAM_INVALID);
    STORE_CTX_LOCK_RET();

    SecurityStoreItem *storeItem = NULL;
    int32_t ret = GetStoreItemByKey(key, &storeItem);
    if (ret != IOTC_OK) {
        STORE_CTX_UNLOCK();
        return ret;
    }

    if (storeItem->buffer == NULL || storeItem->size < len) {
        IOTC_LOGW("invalid key size %u/%u", storeItem->size, len);
        STORE_CTX_UNLOCK();
        return IOTC_CORE_COMM_UTILS_ERR_STORE_INVALID_BUF_SIZE;
    }

    ret = memcpy_s(storeItem->buffer, storeItem->size, value, len);
    if (ret != EOK) {
        STORE_CTX_UNLOCK();
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    storeItem->len = len;

    STORE_CTX_UNLOCK();
    return IOTC_OK;
}

int32_t SecurityStoreGet(int32_t key, uint8_t *value, uint32_t *len)
{
    CHECK_RETURN(value != NULL && len != NULL && *len != 0, IOTC_ERR_PARAM_INVALID);
    STORE_CTX_LOCK_RET();

    SecurityStoreItem *storeItem = NULL;
    int32_t ret = GetStoreItemByKey(key, &storeItem);
    if (ret != IOTC_OK) {
        STORE_CTX_UNLOCK();
        return ret;
    }

    if (storeItem->buffer == NULL || *len < storeItem->len) {
        IOTC_LOGW("invalid key buffer %u/%u", storeItem->len, *len);
        STORE_CTX_UNLOCK();
        return IOTC_CORE_COMM_UTILS_ERR_STORE_INVALID_BUF_SIZE;
    }

    ret = memcpy_s(value, *len, storeItem->buffer, storeItem->len);
    if (ret != EOK) {
        STORE_CTX_UNLOCK();
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    *len = storeItem->len;
    STORE_CTX_UNLOCK();
    return IOTC_OK;
}

int32_t SecurityStoreGetLen(int32_t key, uint32_t *len)
{
    CHECK_RETURN_LOGE(len != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    STORE_CTX_LOCK_RET();

    SecurityStoreItem *storeItem = NULL;
    int32_t ret = GetStoreItemByKey(key, &storeItem);
    if (ret == IOTC_OK) {
        *len = storeItem->len;
    }
    
    STORE_CTX_UNLOCK();
    return ret;
}

static int32_t CipherV1HmacVerifyAndCopy(SecurityStoreItem *storeItem, StoreCipherHeader *header,
    uint8_t *cipher, uint32_t cipLen, uint8_t key[SECURITY_HKDF_LOCAL_KEY_LEN])
{
    uint8_t hmac[IOTC_MD_SHA256_BYTE_LEN] = {0};
    IotcHmacParam hmacParam = {IOTC_MD_SHA256, key, SECURITY_HKDF_LOCAL_KEY_LEN, cipher, cipLen};
    int32_t ret = IotcHmacCalc(&hmacParam, hmac, sizeof(hmac));
    if (ret != IOTC_OK) {
        IOTC_LOGW("calc hmac error %d", ret);
        return ret;
    }
    if (memcmp(hmac, header->mac, sizeof(hmac)) != 0) {
        IOTC_LOGW("integrity check error");
        return IOTC_CORE_COMM_UTILS_ERR_STORE_INTEGRITY_CHECK;
    }
    ret = memcpy_s(storeItem->buffer, storeItem->size, cipher, cipLen);
    if (ret != EOK) {
        IOTC_LOGW("invalid cipher len %u/%u", storeItem->size, cipLen);
        return IOTC_CORE_COMM_UTILS_ERR_STORE_INVALID_BUF_SIZE;
    }
    storeItem->len = cipLen;
    return IOTC_OK;
}

static int32_t CipherV1AesGcmDecrypt(SecurityStoreItem *storeItem, StoreCipherHeader *header,
    uint8_t *cipher, uint32_t cipLen, uint8_t key[SECURITY_HKDF_LOCAL_KEY_LEN])
{
    IotcAesGcmParam aesParam = {key, STORE_CIPHER_GCM_KEY_LEN, key + STORE_CIPHER_GCM_KEY_LEN,
        STORE_CIPHER_GCM_IV_LEN, NULL, 0, cipher, cipLen};
    int32_t ret = IotcAesGcmDecrypt(&aesParam, header->mac, IOTC_AES_GCM_TAG_MAX_LEN, storeItem->buffer);
    if (ret != IOTC_OK) {
        IOTC_LOGW("aes gcm dec error %d", ret);
        return ret;
    }
    storeItem->len = cipLen;
    return IOTC_OK;
}

static int32_t CipherV1DecryptAndVerify(SecurityStoreItem *storeItem, uint8_t *buffer, uint32_t len,
    uint8_t mac[IOTC_MD_SHA256_BYTE_LEN], bool check)
{
    if (len < sizeof(StoreCipherHeader)) {
        IOTC_LOGW("invalid v1 len %u", len);
        return IOTC_CORE_COMM_UTILS_ERR_STORE_INVALID_CIPHER;
    }

    StoreCipherHeader *header = (StoreCipherHeader *)buffer;
    uint8_t *cipher = buffer + sizeof(StoreCipherHeader);
    uint32_t cipLen = len - sizeof(StoreCipherHeader);
    if (cipLen != header->len || cipLen > storeItem->size) {
        IOTC_LOGW("invalid v1 cipher len %u/%u/%u", cipLen, header->len, storeItem->size);
        return IOTC_CORE_COMM_UTILS_ERR_STORE_INVALID_CIPHER;
    }

    uint8_t key[SECURITY_HKDF_LOCAL_KEY_LEN] = {0};
    int32_t ret = SecurityGenHkdfLocalKey(header->salt, sizeof(header->salt), key);
    if (ret != IOTC_OK) {
        IOTC_LOGW("gen hkdf key error %d", ret);
        return ret;
    }
    if (storeItem->mode == SECURITY_STORE_FLAG_VERIFY) {
        ret = CipherV1HmacVerifyAndCopy(storeItem, header, cipher, cipLen, key);
    } else if (storeItem->mode == SECURITY_STORE_FLAG_ENCRYPT) {
        ret = CipherV1AesGcmDecrypt(storeItem, header, cipher, cipLen, key);
    } else {
        ret = IOTC_CORE_COMM_UTILS_ERR_STORE_INVALID_MODE;
    }
    (void)memset_s(key, sizeof(key), 0, sizeof(key));
    if (ret == IOTC_OK && check) {
        /* 读出后通过与预期mac比对，确保数据正确写入 */
        if (memcmp(mac, header->mac, IOTC_MD_SHA256_BYTE_LEN) != 0) {
            ret = IOTC_CORE_COMM_UTILS_ERR_STORE_MAC_NOT_SAME;
        }
    }

    return ret;
}

/**
 * @brief 读取加密数据
 *
 * @param storeItem [IN] 存储项
 * @param mac [IN] 验证码，读出的数据验证码需与之匹配，用于数据写入后再读出校验，check为true时不为空
 * @param check [IN] 是否校验验证码
 * @return IOTC_OK成功，其他失败
 */
static int32_t StoreItemReadCipher(SecurityStoreItem *storeItem, uint8_t mac[IOTC_MD_SHA256_BYTE_LEN], bool check)
{
    uint32_t len = 0;
    int32_t ret = UtilsStoreKvGetLen(storeItem->keyChar, &len);
    if (ret != IOTC_OK) {
        STORE_ITEM_LOGW_CODE("kv get len error", ret, storeItem);
        return ret;
    }

    if (len == 0) {
        /* 新设备空文件 */
        storeItem->len = 0;
        STORE_ITEM_LOGI("kv read empty cipher ok", storeItem);
        return IOTC_OK;
    }

    if (len > storeItem->size + MAX_STORE_CIPHER_HEADER_LEN) {
        STORE_ITEM_LOGW_CODE("invalid cipher len", len, storeItem);
        return IOTC_CORE_COMM_UTILS_ERR_STORE_INVALID_KV_LEN;
    }

    uint8_t *buffer = (uint8_t *)IotcCalloc(len, sizeof(uint8_t));
    if (buffer == NULL) {
        IOTC_LOGW("calloc error %u", len);
        return IOTC_ADAPTER_MEM_ERR_CALLOC;
    }

    ret = UtilsStoreKvGetValue(storeItem->keyChar, buffer, &len);
    if (ret != IOTC_OK) {
        STORE_ITEM_LOGW_CODE("kv read error", ret, storeItem);
        IotcFree(buffer);
        return ret;
    }

    if (*buffer == STORE_CIPHER_VER_1) {
        ret = CipherV1DecryptAndVerify(storeItem, buffer, len, mac, check);
    } else {
        ret = IOTC_CORE_COMM_UTILS_ERR_STORE_INVALID_VERSION;
    }

    IotcFree(buffer);
    return ret;
}

static int32_t StoreItemRead(SecurityStoreItem *storeItem)
{
    if (storeItem->buffer == NULL || storeItem->keyChar == NULL || storeItem->size == 0) {
        IOTC_LOGW("invalid store item %d/%s/%u", storeItem->keyInt, NON_NULL_STR(storeItem->keyChar), storeItem->size);
        return IOTC_CORE_COMM_UTILS_ERR_STORE_INVALID_ITEM;
    }

    if (storeItem->mode != SECURITY_STORE_FLAG_PLAIN) {
        return StoreItemReadCipher(storeItem, NULL, false);
    }

    uint32_t len = storeItem->size;
    int32_t ret = UtilsStoreKvGetValue(storeItem->keyChar, storeItem->buffer, &len);
    if (ret != IOTC_OK) {
        STORE_ITEM_LOGW_CODE("kv get error", ret, storeItem);
        return ret;
    }

    storeItem->len = len;
    STORE_ITEM_LOGI("kv get ok", storeItem);
    return IOTC_OK;
}

static int32_t CipherV1GenHmacAndCopy(SecurityStoreItem *storeItem, StoreCipherHeader *header,
    uint8_t *cipherBuf, uint32_t bufLen, uint8_t key[SECURITY_HKDF_LOCAL_KEY_LEN])
{
    int32_t ret = memcpy_s(cipherBuf, bufLen, storeItem->buffer, storeItem->len);
    if (ret != EOK) {
        return IOTC_ERR_SECUREC_MEMCPY;
    }

    IotcHmacParam hmacParam = {IOTC_MD_SHA256, key, SECURITY_HKDF_LOCAL_KEY_LEN,
        storeItem->buffer, storeItem->len};
    ret = IotcHmacCalc(&hmacParam, header->mac, sizeof(header->mac));
    if (ret != IOTC_OK) {
        IOTC_LOGW("calc hmac error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static int32_t CipherV1Encrypt(SecurityStoreItem *storeItem, StoreCipherHeader *header,
    uint8_t *cipherBuf, uint32_t bufLen, uint8_t key[SECURITY_HKDF_LOCAL_KEY_LEN])
{
    /* 前16byte作为秘钥 后16byte作为iv */
    IotcAesGcmParam aesParam = {key, STORE_CIPHER_GCM_KEY_LEN, key + STORE_CIPHER_GCM_KEY_LEN,
        STORE_CIPHER_GCM_IV_LEN, NULL, 0, storeItem->buffer, storeItem->len};
    int32_t ret = IotcAesGcmEncrypt(&aesParam, header->mac, IOTC_AES_GCM_TAG_MAX_LEN, cipherBuf);
    if (ret != IOTC_OK) {
        IOTC_LOGW("aes gcm enc error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static int32_t StoreItemWriteCipher(SecurityStoreItem *storeItem)
{
    if (storeItem->len > UINT32_MAX - sizeof(StoreCipherHeader)) {
        STORE_ITEM_LOGW("invalid item len", storeItem);
        return IOTC_CORE_COMM_UTILS_ERR_STORE_INVALID_ITEM;
    }
    uint32_t len = storeItem->len + sizeof(StoreCipherHeader);
    uint8_t *buf = (uint8_t *)IotcCalloc(len, sizeof(uint8_t));
    if (buf == NULL) {
        IOTC_LOGW("calloc error %u", len);
        return IOTC_ADAPTER_MEM_ERR_CALLOC;
    }

    StoreCipherHeader *header = (StoreCipherHeader *)buf;
    header->ver = STORE_CIPHER_VER_1;
    (void)SecurityRandom(header->salt, sizeof(header->salt));
    header->len = storeItem->len;

    uint8_t key[SECURITY_HKDF_LOCAL_KEY_LEN] = {0};
    int32_t ret = SecurityGenHkdfLocalKey(header->salt, sizeof(header->salt), key);
    if (ret != IOTC_OK) {
        IOTC_LOGW("gen hkdf key error %d", ret);
        IotcFree(buf);
        return ret;
    }

    if (storeItem->mode == SECURITY_STORE_FLAG_VERIFY) {
        ret = CipherV1GenHmacAndCopy(storeItem, header, buf + sizeof(StoreCipherHeader), storeItem->len, key);
    } else if (storeItem->mode == SECURITY_STORE_FLAG_ENCRYPT) {
        ret = CipherV1Encrypt(storeItem, header, buf + sizeof(StoreCipherHeader), storeItem->len, key);
    } else {
        ret = IOTC_CORE_COMM_UTILS_ERR_STORE_INVALID_MODE;
    }
    (void)memset_s(key, sizeof(key), 0, sizeof(key));
    if (ret != IOTC_OK) {
        IotcFree(buf);
        return ret;
    }
    ret = UtilsStoreKvSetValue(storeItem->keyChar, buf, len);
    if (ret != IOTC_OK) {
        IotcFree(buf);
        STORE_ITEM_LOGW_CODE("kv write error", ret, storeItem);
        return ret;
    }
    uint8_t mac[IOTC_MD_SHA256_BYTE_LEN] = {0};
    ret = memcpy_s(mac, sizeof(mac), header->mac, sizeof(header->mac));
    IotcFree(buf);
    if (ret !=  EOK) {
        return IOTC_ERR_SECUREC_MEMCPY;
    }

    /* 数据写入后重新读出，并通过校验读出的mac与写入的mac一致来确认数据正确写入 */
    return StoreItemReadCipher(storeItem, mac, true);
}

static int32_t StoreItemWrite(SecurityStoreItem *storeItem)
{
    if (storeItem->buffer == NULL || storeItem->keyChar == NULL || storeItem->len > storeItem->size) {
        IOTC_LOGW("invalid store item %d/%s/%u/%u", storeItem->keyInt, NON_NULL_STR(storeItem->keyChar),
            storeItem->len, storeItem->size);
        return IOTC_CORE_COMM_UTILS_ERR_STORE_INVALID_ITEM;
    }

    if (storeItem->mode != SECURITY_STORE_FLAG_PLAIN) {
        return StoreItemWriteCipher(storeItem);
    }

    int32_t ret = UtilsStoreKvSetValue(storeItem->keyChar, storeItem->buffer, storeItem->len);
    if (ret != IOTC_OK) {
        STORE_ITEM_LOGW_CODE("kv write error", ret, storeItem);
        return ret;
    }

    STORE_ITEM_LOGI("kv write ok", storeItem);
    return IOTC_OK;
}

static int32_t StoreItemDelete(SecurityStoreItem *storeItem)
{
    if (storeItem->keyChar == NULL) {
        IOTC_LOGW("invalid store item %d/%s", storeItem->keyInt, NON_NULL_STR(storeItem->keyChar));
        return IOTC_CORE_COMM_UTILS_ERR_STORE_INVALID_ITEM;
    }

    int32_t ret = UtilsStoreKvDelValue(storeItem->keyChar);
    if (ret != IOTC_OK) {
        STORE_ITEM_LOGW_CODE("kv del error", ret, storeItem);
        return ret;
    }
    storeItem->len = 0;

    STORE_ITEM_LOGI("kv del ok", storeItem);
    return IOTC_OK;
}

int32_t SecurityStorePull(int32_t key)
{
    STORE_CTX_LOCK_RET();

    SecurityStoreItem *storeItem = NULL;
    int32_t ret = GetStoreItemByKey(key, &storeItem);
    if (ret != IOTC_OK) {
        STORE_CTX_UNLOCK();
        return ret;
    }

    ret = StoreItemRead(storeItem);
    if (ret != IOTC_OK) {
        STORE_CTX_UNLOCK();
        return ret;
    }

    STORE_CTX_UNLOCK();
    return IOTC_OK;
}

int32_t SecurityStoreCommit(int32_t key)
{
    STORE_CTX_LOCK_RET();

    SecurityStoreItem *storeItem = NULL;
    int32_t ret = GetStoreItemByKey(key, &storeItem);
    if (ret != IOTC_OK) {
        STORE_CTX_UNLOCK();
        return ret;
    }

    ret = StoreItemWrite(storeItem);
    if (ret != IOTC_OK) {
        STORE_CTX_UNLOCK();
        return ret;
    }

    STORE_CTX_UNLOCK();
    return IOTC_OK;
}

int32_t SecurityStoreDel(int32_t key)
{
    STORE_CTX_LOCK_RET();

    SecurityStoreItem *storeItem = NULL;
    int32_t ret = GetStoreItemByKey(key, &storeItem);
    if (ret != IOTC_OK) {
        STORE_CTX_UNLOCK();
        return ret;
    }

    ret = StoreItemDelete(storeItem);
    if (ret != IOTC_OK) {
        STORE_CTX_UNLOCK();
        return ret;
    }

    STORE_CTX_UNLOCK();
    return IOTC_OK;
}

static int32_t GetStoreNodeByList(SecurityStoreItem *list, StoreItemsNode **storeNode)
{
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &GetContext()->storeList) {
        StoreItemsNode *node = CONTAINER_OF(item, StoreItemsNode, node);
        if (node->items == list) {
            *storeNode  = node;
            return IOTC_OK;
        }
    }
    IOTC_LOGW("invalid store list");
    return IOTC_CORE_COMM_UTILS_ERR_STORE_INVALID_LIST;
}

static int32_t StoreOpByItem(SecurityStoreItem *storeItem, StoreOp op)
{
    int32_t ret;
    if (op == STORE_OP_READ) {
        ret = StoreItemRead(storeItem);
    } else if (op == STORE_OP_WRITE) {
        ret = StoreItemWrite(storeItem);
    } else {
        ret = StoreItemDelete(storeItem);
    }
    return ret;
}

static int32_t StoreOpByList(SecurityStoreItem *list, StoreOp op)
{
    StoreItemsNode *storeNode = NULL;
    SecurityStoreItem *storeItem = NULL;
    uint32_t index = 0;
    int32_t ret = GetStoreNodeByList(list, &storeNode);
    if (ret != IOTC_OK) {
        return ret;
    }

    for (index = 0; index < storeNode->num; ++index) {
        storeItem = storeNode->items + index;
        ret = StoreOpByItem(storeItem, op);
        if (ret != IOTC_OK) {
            break;
        }
    }
    return ret;
}

int32_t SecurityStorePullList(SecurityStoreItem *list)
{
    CHECK_RETURN(list != NULL, IOTC_ERR_PARAM_INVALID);
    STORE_CTX_LOCK_RET();

    int32_t ret = StoreOpByList(list, STORE_OP_READ);

    STORE_CTX_UNLOCK();
    return ret;
}

int32_t SecurityStoreCommitList(SecurityStoreItem *list)
{
    CHECK_RETURN(list != NULL, IOTC_ERR_PARAM_INVALID);
    STORE_CTX_LOCK_RET();

    int32_t ret = StoreOpByList(list, STORE_OP_WRITE);

    STORE_CTX_UNLOCK();
    return ret;
}

int32_t SecurityStoreDelList(SecurityStoreItem *list)
{
    CHECK_RETURN(list != NULL, IOTC_ERR_PARAM_INVALID);
    STORE_CTX_LOCK_RET();

    int32_t ret = StoreOpByList(list, STORE_OP_DEL);

    STORE_CTX_UNLOCK();
    return ret;
}

static int32_t StoreOpAll(StoreOp op)
{
    int32_t ret = IOTC_OK;
    SecurityStoreItem *storeItem = NULL;
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &GetContext()->storeList) {
        StoreItemsNode *node = CONTAINER_OF(item, StoreItemsNode, node);
        for (uint32_t i = 0; i < node->num; ++i) {
            storeItem = node->items + i;
            ret = StoreOpByItem(storeItem, op);
            if (ret != IOTC_OK) {
                break;
            }
        }
    }
    return ret;
}

int32_t SecurityStorePullAll(void)
{
    STORE_CTX_LOCK_RET();

    int32_t ret = StoreOpAll(STORE_OP_READ);
    STORE_CTX_UNLOCK();
    return ret;
}

int32_t SecurityStoreCommitAll(void)
{
    STORE_CTX_LOCK_RET();

    int32_t ret = StoreOpAll(STORE_OP_WRITE);
    STORE_CTX_UNLOCK();
    return ret;
}

int32_t SecurityStoreDelAll(void)
{
    STORE_CTX_LOCK_RET();
    int32_t ret = StoreOpAll(STORE_OP_DEL);
    STORE_CTX_UNLOCK();
    return ret;
}