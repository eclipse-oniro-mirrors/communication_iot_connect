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
#ifndef UTILS_QUEUE_H
#define UTILS_QUEUE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_QUEUE_VALUE_LEN 1024

typedef struct UtilsQueue UtilsQueue;

/**
 * @brief 队列值释放回调函数
 *
 * @param value [IN] 互斥锁
 * @attention 不释放value本身
 */
typedef void (*QueueFreeValue)(void *value);

/**
 * @brief 创建队列
 *
 * @param capacity [IN] 队列最大缓冲数量
 * @param freeValue [IN] 队列值释放回调函数，如果传NULL则不需要额外的释放函数
 * @return 非NULL 队列操作资源，NULL失败
 */
UtilsQueue *UtilsQueueCreate(uint32_t capacity, QueueFreeValue freeValue);

/**
 * @brief 入队
 *
 * @param queue [IN] 待操作的队列
 * @param value [IN] 值
 * @param valueLen [IN] 值长度
 * @return 0 成功，非0 失败
 */
int32_t UtilsQueuePush(UtilsQueue *queue, const void *value, uint32_t valueLen);

/**
 * @brief 入队，使用外部传入地址
 *
 * @param queue [IN] 待操作的队列
 * @param value [IN] 值
 * @param valueLen [IN] 值长度
 * @return 0 成功，非0 失败
 */
int32_t UtilsQueuePushMem(UtilsQueue *queue, void **value, uint32_t valueLen);

/**
 * @brief 出队
 *
 * @param queue [IN] 待操作的队列
 * @param value [OUT] 输出值
 * @param valueSize [IN] 输出值缓存大小
 * @param valueLen [OUT] 实际输出值长度
 * @return 0 成功，非0 失败
 * @attention 出队的元素使用完后记得资源释放
 */
int32_t UtilsQueuePop(UtilsQueue *queue, void *value, uint32_t valueSize, uint32_t *valueLen);

/**
 * @brief 出队，弹出外部传入地址
 *
 * @param queue [IN] 待操作的队列
 * @param value [OUT] 输出值
 * @param valueSize [IN] 输出值缓存大小
 * @param valueLen [OUT] 实际输出值长度
 * @return 0 成功，非0 失败
 * @attention 出队的元素使用完后记得资源释放
 */
int32_t UtilsQueuePopMem(UtilsQueue *queue, void **value, uint32_t *valueLen);

/**
 * @brief 获取队列剩余数据数量
 *
 * @param queue [IN] 待操作的队列
 * @return 数量
 */
uint32_t UtilsQueueGetCount(UtilsQueue *queue);

/**
 * @brief 队列是否满
 *
 * @param queue [IN] 待操作的队列
 * @return true 已满，false 未满
 */
bool UtilsQueueIsFull(UtilsQueue *queue);

/**
 * @brief 队列资源释放
 *
 * @param queueAddr [IN] 待操作的队列地址
 */
void UtilsQueueDestroy(UtilsQueue **queueAddr);

#ifdef __cplusplus
}
#endif
#endif /* UTILS_QUEUE_H */