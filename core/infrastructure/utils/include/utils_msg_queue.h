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
#ifndef UTILS_MSG_QUEUE_H
#define UTILS_MSG_QUEUE_H

#include <stdint.h>
#include <stdbool.h>
#include "utils_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UtilsMsgQueue UtilsMsgQueue;

/**
 * @brief 创建消息队列
 *
 * @param capacity [IN] 消息队列最大缓冲数量
 * @param freeValue [IN] 消息队列值释放回调函数，如果传NULL则不需要额外的释放函数
 * @return 非NULL 消息队列操作资源，NULL失败
 */
UtilsMsgQueue *UtilsMsgQueueCreate(uint32_t capacity, QueueFreeValue freeValue);

/**
 * @brief 消息入队
 *
 * @param msgQueue [IN] 待操作的消息队列
 * @param value [IN] 值
 * @param valueLen [IN] 值长度
 * @param timeout [IN] 队列满等待时间，为0不等待
 * @return 0 成功，非0 失败
 */
int32_t UtilsMsgQueuePush(UtilsMsgQueue *msgQueue, const void *value, uint32_t valueLen, uint32_t timeout);

/**
 * @brief 消息入队，使用外部传入地址
 *
 * @param msgQueue [IN] 待操作的消息队列
 * @param value [IN] 值
 * @param valueLen [IN] 值长度
 * @param timeout [IN] 队列满等待时间，为0不等待
 * @return 0 成功，非0 失败
 */
int32_t UtilsMsgQueuePushMem(UtilsMsgQueue *msgQueue, void **value, uint32_t valueLen, uint32_t timeout);

/**
 * @brief 消息队列挂起
 *
 * @param msgQueue [IN] 待操作的消息队列
 * @return 0 成功，非0 失败
 */
int32_t UtilsMsgQueueSuspend(UtilsMsgQueue *msgQueue);

/**
 * @brief 消息队列恢复
 *
 * @param msgQueue [IN] 待操作的消息队列
 * @return 0 成功，非0 失败
 */
void UtilsMsgQueueResume(UtilsMsgQueue *msgQueue);

/**
 * @brief 消息出队，弹出外部传入地址
 *
 * @param msgQueue [IN] 待操作的消息队列
 * @param value [OUT] 输出值
 * @param valueSize [IN] 输出值缓存大小
 * @param valueLen [OUT] 实际输出值长度
 * @param timeout [IN] 队列空等待时间，为0不等待
 * @return 0 成功，非0 失败
 * @attention 出队的元素使用完后记得资源释放
 */
int32_t UtilsMsgQueuePop(UtilsMsgQueue *msgQueue, void *value, uint32_t valueSize, uint32_t *valueLen,
    uint32_t timeout);

/**
 * @brief 消息出队，使用外部传入地址
 *
 * @param msgQueue [IN] 待操作的消息队列
 * @param value [OUT] 输出值
 * @param valueSize [IN] 输出值缓存大小
 * @param valueLen [OUT] 实际输出值长度
 * @param timeout [IN] 队列空等待时间，为0不等待
 * @return 0 成功，非0 失败
 * @attention 出队的元素使用完后记得资源释放
 */
int32_t UtilsMsgQueuePopMem(UtilsMsgQueue *msgQueue, void **value, uint32_t *valueLen, uint32_t timeout);

/**
 * @brief 获取消息队列剩余数据数量
 *
 * @param msgQueue [IN] 待操作的消息队列
 * @return 数量
 */
uint32_t UtilsMsgQueueGetCount(UtilsMsgQueue *msgQueue);

/**
 * @brief 消息队列资源释放
 *
 * @param msgQueueAddr [IN] 待操作的消息队列地址
 */
void UtilsMsgQueueDestroy(UtilsMsgQueue **msgQueueAddr);

#ifdef __cplusplus
}
#endif
#endif /* UTILS_MSG_QUEUE_H */