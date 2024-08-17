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
#include "utils_hash_map.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "securec.h"
#include "iotc_log.h"
#include "iotc_errcode.h"
#include "iotc_mem.h"

#define HASHMAP_NAME_LEN 20

typedef struct HashNode {
    uint8_t *key;
    uint32_t keyLen;
    void *value;
    /* 当key相同时，指向集合中的下一个节点 */
    struct HashNode *next;
} HashNode;

struct HashMap {
    /* hash map 名字 */
    char name[HASHMAP_NAME_LEN];
    /* hash map节点个数 */
    int32_t size;
    /* hash map的桶个数 */
    int32_t bucketSize;
    HashMapFreeValue freeValue;
    /* 二维数组，存key值不重复的node，key重复的node链接在HashNode->next */
    HashNode **hashArr;
};

/*
 * 根据输入的字符串计算哈希值, 由于数据比较少，优化只取末尾四位进行hash计算
 */
#define HASH_FACTOR_ONE 4
#define HASH_FACTOR_TWO 24
static uint32_t Hash(const uint8_t *key, uint32_t keyLen)
{
    uint32_t h = 0;
    uint32_t g;

    uint32_t begin = (keyLen > 4) ? (keyLen - 4) : 0;
    const uint8_t *arBegin = key + begin;
    const uint8_t *arEnd = key + keyLen;

    while (arBegin < arEnd) {
        h = (h << HASH_FACTOR_ONE) + *arBegin;
        if ((g = (h & 0xF0000000))) {
            h = h ^ (g >> HASH_FACTOR_TWO);
            h = h ^ g;
        }
        arBegin++;
    }
    return h;
}

static HashNode *CreateNode(const uint8_t *key, uint32_t keyLen, void *value)
{
    /* 创建一个node节点 */
    HashNode *node = (HashNode *)IotcMalloc(sizeof(HashNode));
    if (node == NULL) {
        return NULL;
    }
    (void)memset_s(node, sizeof(HashNode), 0, sizeof(HashNode));
    uint8_t *keyCopy = (uint8_t *)IotcMalloc(keyLen);
    if (keyCopy == NULL) {
        IotcFree(node);
        return NULL;
    }
    (void)memset_s(keyCopy, keyLen, 0, keyLen);
    if (memcpy_s(keyCopy, keyLen, key, keyLen) != EOK) {
        IotcFree(node);
        IotcFree(keyCopy);
        return NULL;
    }
    node->key = keyCopy;
    node->keyLen = keyLen;
    node->value = value;
    node->next = NULL;
    return node;
}

static void HashMapMallocFree(void *ptr)
{
    IotcFree(ptr);
}

/**
 * @brief 创建一个hash桶数量为n的hash表
 *
 * @param n [IN] hash桶数量 大于0
 * @param name [IN] hash map名称
 * @param freeValue [IN] 值释放函数
 * @return NULL 失败，非NULL 创建的hash map
 */
HashMap *UtilsHashMapCreate(uint32_t n, const char *name, HashMapFreeValue freeValue)
{
    if ((name == NULL) || (n == 0)) {
        IOTC_LOGW("invalid param");
        return NULL;
    }
    HashMap *hashMap = (HashMap *)IotcMalloc(sizeof(HashMap));
    if (hashMap == NULL) {
        IOTC_LOGW("malloc hashMap failed");
        return NULL;
    }
    (void)memset_s(hashMap, sizeof(HashMap), 0, sizeof(HashMap));

    uint32_t hashArrLen = n * sizeof(HashNode *);
    hashMap->hashArr = (HashNode **)IotcMalloc(hashArrLen);
    if (hashMap->hashArr == NULL) {
        IOTC_LOGW("malloc hashMap hashArr failed");
        IotcFree(hashMap);
        return NULL;
    }
    (void)memset_s(hashMap->hashArr, hashArrLen, 0, hashArrLen);

    hashMap->bucketSize = n;
    hashMap->size = 0;
    hashMap->freeValue = (freeValue == NULL) ? HashMapMallocFree : freeValue;

    (void)memset_s(hashMap->name, sizeof(hashMap->name), 0, sizeof(hashMap->name));
    uint32_t nameLen = strlen(name);
    uint32_t cpLen = (nameLen > sizeof(hashMap->name) - 1) ? (sizeof(hashMap->name) - 1) : nameLen;
    if (memcpy_s(hashMap->name, sizeof(hashMap->name) - 1, name, cpLen) != EOK) {
        IOTC_LOGW("init hashMap name fail");
        IotcFree(hashMap->hashArr);
        IotcFree(hashMap);
        return NULL;
    }

    return (HashMap *)hashMap;
}

/**
 * @brief 插入1个键值对
 *
 * @param hashMapId [IN] 待操作的hash表
 * @param key [IN] 键
 * @param keyLen [IN] 键标识的长度
 * @param value [IN] 值
 * @return 非0 失败，0 成功
 * @attention 如果key相同，则后插入的value会覆盖已有的value
 */
int32_t UtilsHashMapInsert(HashMap *hashMapId, const uint8_t *key, uint32_t keyLen, void *value)
{
    HashMap *hashMap = (HashMap *)hashMapId;
    if ((hashMap == NULL) || (key == NULL) || (keyLen == 0) || (value == NULL)) {
        IOTC_LOGW("invalid param");
        return IOTC_ERROR;
    }
    if (hashMap->hashArr == NULL) {
        IOTC_LOGW("no arr");
        return IOTC_ERROR;
    }

    HashNode *node = CreateNode(key, keyLen, value);
    if (node == NULL) {
        IOTC_LOGW("create");
        return IOTC_ERROR;
    }
    int32_t index = Hash(key, keyLen) % hashMap->bucketSize;
    /* 如果当前位置没有node，就将创建的node加入 */
    if (hashMap->hashArr[index] == NULL) {
        hashMap->hashArr[index] = node;
        hashMap->size++;
        return IOTC_OK;
    }

    HashNode *temp = hashMap->hashArr[index];
    HashNode *prev = temp;
    while (temp != NULL) {
        /* 如果两个key相同，则直接用新node的value覆盖旧的 */
        if ((temp->key != NULL) && (node->key != NULL)) {
            if ((temp->keyLen == node->keyLen) && (memcmp(temp->key, node->key, node->keyLen) == 0)) {
                IOTC_LOGW("Hashmap(%s) HashMapInsert keyLen=%u overwr", hashMap->name, keyLen);
                hashMap->freeValue(temp->value);
                temp->value = node->value;
                IotcFree(node->key);
                IotcFree(node);
                return IOTC_OK;
            }
        }
        prev = temp;
        temp = temp->next;
    }
    prev->next = node;
    hashMap->size++;
    return IOTC_OK;
}

/**
 * @brief 通过key查找value
 *
 * @param hashMapId [IN] 待操作的hash表
 * @param key [IN] 键
 * @param keyLen [IN] 键标识的长度
 * @return NULL 失败或未查询到，非NULL 查询到的值
 * @attention 查询到的value，没查到返回NULL
 */
void *UtilsHashMapGetValue(HashMap *hashMapId, const uint8_t *key, uint32_t keyLen)
{
    HashMap *hashMap = (HashMap *)hashMapId;
    if ((hashMap == NULL) || (key == NULL) || (keyLen == 0)) {
        IOTC_LOGW("invalid param");
        return NULL;
    }
    if (hashMap->hashArr == NULL) {
        IOTC_LOGW("no arr");
        return NULL;
    }
    int32_t index = Hash(key, keyLen) % hashMap->bucketSize;
    HashNode *temp = hashMap->hashArr[index];
    while (temp != NULL) {
        if (temp->key != NULL) {
            if ((temp->keyLen == keyLen) && (memcmp(temp->key, key, keyLen) == 0)) {
                return temp->value;
            }
        }
        temp = temp->next;
    }

    return NULL;
}

/**
 * @brief 释放hash map内存, 只删除hash node 节点
 *
 * @param hashMapId [IN] 待操作的hash表
 */
void UtilsHashMapDelete(HashMap *hashMapId)
{
    HashMap *hashMap = (HashMap *)hashMapId;
    if (hashMap == NULL) {
        IOTC_LOGW("invalid param");
        return;
    }

    for (int32_t i = 0; i < hashMap->bucketSize; i++) {
        HashNode *temp = hashMap->hashArr[i];
        HashNode *prev = temp;
        while (temp != NULL) {
            prev = temp;
            temp = temp->next;
            IotcFree(prev->key);
            hashMap->freeValue(prev->value);
            IotcFree(prev);
        }
    }

    uint32_t hashArrLen = hashMap->bucketSize * sizeof(HashNode *);
    (void)memset_s(hashMap->hashArr, hashArrLen, 0, hashArrLen);
    hashMap->size = 0;
}

/**
 * @brief 删除由key指定的键值对
 *
 * @param hashMapId [IN] 待操作的hash表
 * @param key [IN] 键
 * @param keyLen [IN] 键标识的长度
 * @return 非0 失败，0 成功
 */
int32_t UtilsHashMapRemove(HashMap *hashMapId, const uint8_t *key, uint32_t keyLen)
{
    HashMap *hashMap = (HashMap *)hashMapId;
    if ((hashMap == NULL) || (key == NULL) || (keyLen == 0)) {
        IOTC_LOGW("invalid param");
        return IOTC_ERROR;
    }
    if (hashMap->hashArr == NULL) {
        IOTC_LOGW("no arr");
        return IOTC_ERROR;
    }

    int32_t index = Hash(key, keyLen) % hashMap->bucketSize;
    HashNode *temp = hashMap->hashArr[index];
    if ((temp == NULL) || (temp->key == NULL)) {
        IOTC_LOGW("null");
        return IOTC_ERROR;
    }

    /* 如果第一个就匹配中 */
    if ((temp->keyLen == keyLen) && (memcmp(temp->key, key, keyLen) == 0)) {
        hashMap->hashArr[index] = temp->next;
        IotcFree(temp->key);
        hashMap->freeValue(temp->value);
        IotcFree(temp);
        hashMap->size--;
        return IOTC_OK;
    }

    HashNode *prev = temp;
    temp = temp->next;
    while (temp != NULL) {
        if ((temp->keyLen == keyLen) && (memcmp(temp->key, key, keyLen) == 0)) {
            prev->next = temp->next;
            IotcFree(temp->key);
            hashMap->freeValue(temp->value);
            IotcFree(temp);
            hashMap->size--;
            return IOTC_OK;
        }
        prev = temp;
        temp = temp->next;
    }
    return IOTC_ERROR;
}

/**
 * @brief 遍历hashmap， 循环调用f
 *
 * @param HashMap *[IN] 待操作的hash表
 * @param key [IN] 键
 * @return 非0 失败，0 成功
 * @attention f函数返回 HASH_MAP_TRAVE_CONTINUE 继续处理下一个元素，
 * f函数返回 HASH_MAP_TRAVE_BREAK 异常中途退出，不处理下一个元素
 * @warning 遍历过程中除了删除当前节点以外不能对hashmap做其他增删操作
 */
int32_t UtilsHashMapIterate(HashMap *hashMapId, const HashMapTraversal traveFunc, ...)
{
    HashMap *hashMap = (HashMap *)hashMapId;
    if ((hashMap == NULL) || (traveFunc == NULL)) {
        IOTC_LOGW("invalid param");
        return HASH_MAP_TRAVE_BREAK;
    }

    va_list argp;
    HashNode *node = NULL;
    for (int32_t i = 0; i < hashMap->bucketSize; i++) {
        node = hashMap->hashArr[i];
        if (node == NULL) {
            continue;
        }

        HashNode *temp = node;
        HashNode *next = NULL;
        while (temp != NULL) {
            if (temp->value == NULL || temp->key == NULL) {
                IOTC_LOGW("hashmap key or value is null");
                temp = temp->next;
                continue;
            }
            /* 遍历中可能删除当前节点，需提前保存next指针，且traveFunc后不再使用temp指针 */
            next = temp->next;
            va_start(argp, traveFunc);
            if (traveFunc(temp->value, argp) != HASH_MAP_TRAVE_CONTINUE) {
                va_end(argp);
                return HASH_MAP_TRAVE_BREAK;
            }
            va_end(argp);
            temp = next;
        }
    }
    return HASH_MAP_TRAVE_CONTINUE;
}

/**
 * @brief 获取hash map的大小
 *
 * @param hashMapId [IN] 待操作的hash表
 * @return hash map 存储的节点个数
 */
uint32_t UtilsHashMapLength(HashMap *hashMapId)
{
    HashMap *hashMap = (HashMap *)hashMapId;
    if (hashMap != NULL) {
        return hashMap->size;
    }

    return 0;
}

/**
 * @brief 释放hash map所有资源
 * @param hashMapIdAddr [IN] 待操作的hash表地址
 */
void UtilsHashMapDestroy(HashMap **hashMapIdAddr)
{
    if (hashMapIdAddr == NULL) {
        IOTC_LOGW("invalid param");
        return;
    }
    HashMap *hashMap = *hashMapIdAddr;
    UtilsHashMapDelete(hashMap);
    IotcFree(hashMap->hashArr);
    IotcFree(hashMap);
    *hashMapIdAddr = NULL;
}