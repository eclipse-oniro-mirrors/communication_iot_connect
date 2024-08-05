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
#ifndef UTILS_HASHMAP_H
#define UTILS_HASHMAP_H

#include <stdint.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct HashMap HashMap;

typedef enum {
    HASH_MAP_TRAVE_CONTINUE = 0,
    HASH_MAP_TRAVE_BREAK,
} HashMapTravCode;

/**
 * @brief hash map 遍历时的回调函数
 *
 * @param value [IN] hash map值
 * @param argp [IN] 变长参数
 * @return HASH_MAP_TRAVE_CONTINUE 继续处理下个元素
 *         HASH_MAP_TRAVE_BREAK 异常退出不处理下个元素
 */
typedef HashMapTravCode (*HashMapTraversal)(const void *value, va_list argp);

/**
 * @brief hash map 值释放回调函数
 *
 * @param value [IN] hash map值
 */
typedef void (*HashMapFreeValue)(void *value);

/**
 * @brief 创建一个hash桶数量为n的hash表
 *
 * @param n [IN] hash桶数量 大于0
 * @param name [IN] hash map名称
 * @param freeValue [IN] 值释放函数,为NULL则使用默认的释放函数
 * @return NULL 失败，非NULL 创建的hash map
 */
HashMap *UtilsHashMapCreate(uint32_t n, const char *name, HashMapFreeValue freeValue);

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
int32_t UtilsHashMapInsert(HashMap *hashMapId, const uint8_t *key, uint32_t keyLen, void *value);

/**
 * @brief 通过key查找value
 *
 * @param hashMapId [IN] 待操作的hash表
 * @param key [IN] 键
 * @param keyLen [IN] 键标识的长度
 * @return NULL 失败或未查询到，非NULL 查询到的值
 * @attention 查询到的value，没查到返回NULL
 */
void *UtilsHashMapGetValue(HashMap *hashMapId, const uint8_t *key, uint32_t keyLen);

/**
 * @brief 释放hash map内存, 只删除hash node 节点
 *
 * @param hashMapId [IN] 待操作的hash表
 */
void UtilsHashMapDelete(HashMap *hashMapId);

/**
 * @brief 删除由key指定的键值对
 *
 * @param hashMapId [IN] 待操作的hash表
 * @param key [IN] 键
 * @param keyLen [IN] 键标识的长度
 * @return 非0 失败，0 成功
 */
int32_t UtilsHashMapRemove(HashMap *hashMapId, const uint8_t *key, uint32_t keyLen);

/**
 * @brief 遍历hashmap， 循环调用f
 *
 * @param hashMapId [IN] 待操作的hash表
 * @param key [IN] 键
 * @return 非0 失败，0 成功
 * @attention f函数返回 HASH_MAP_TRAVE_CONTINUE 继续处理下一个元素，
 * f函数返回 HASH_MAP_TRAVE_BREAK 异常中途退出，不处理下一个元素
 * @warning 遍历过程中除了删除当前节点以外不能对hashmap做其他增删操作
 */
int32_t UtilsHashMapIterate(HashMap *hashMapId, const HashMapTraversal traveFunc, ...);

/**
 * @brief 获取hash map的大小
 *
 * @param hashMapId [IN] 待操作的hash表
 * @return hash map 存储的节点个数
 */
uint32_t UtilsHashMapLength(HashMap *hashMapId);

/**
 * @brief 释放hash map所有资源
 * @param hashMapIdAddr [IN] 待操作的hash表地址
 */
void UtilsHashMapDestroy(HashMap **hashMapIdAddr);

#ifdef __cplusplus
}
#endif
#endif /* UTILS_HASHMAP_H */