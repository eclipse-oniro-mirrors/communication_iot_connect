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
#ifndef EVENT_BUS_PUB_H
#define EVENT_BUS_PUB_H

#include <stdint.h>
#include <stdbool.h>
#include "comm_def.h"
#include "iotc_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 异步事件参数释放函数
 *
 * @param event [IN] 事件
 * @param param [IN] 参数
 * @param len [IN] 参数长度
 */
typedef void (*EventBusParamFreeHandler)(uint32_t event, void *param, uint32_t len);

/**
 * @brief 异步执行函数
 *
 * @param param [IN] 参数
 */
typedef void (*EventBusAsyncHandler)(void *param);

/**
 * @brief 异步执行器
 *
 * @param handler [IN] 执行的处理函数
 * @param param [IN] 参数
 * @return 0成功，非0失败
 */
typedef int32_t (*EventBusAsyncExecutor)(EventBusAsyncHandler handler, void *param);

/**
 * @brief 发布同步事件
 *
 * @param event [IN] 事件
 * @param name [IN] 事件名称
 * @param param [IN] 参数
 * @param len [IN] 参数长度
 */
void EventBusPublishSyncInner(uint32_t event, const char *name, void *param, uint32_t len);

/**
 * @brief 发布异步事件
 *
 * @param event [IN] 事件
 * @param name [IN] 事件名称
 * @param param [IN] 参数
 * @param len [IN] 参数长度
 * @param freeFunc [IN] 资源释放函数
 * @return 0成功，非0失败
 */
int32_t EventBusPublishAsyncInner(uint32_t event, const char *name, void *param, uint32_t len,
    EventBusParamFreeHandler freeFunc);

#if (IOTC_CONF_DEVICE_LEVEL > IOTC_DEVICE_LEVEL_MINI) || \
    (IOTC_CONF_BUILD_TYPE == IOTC_BUILD_TYPE_DEBUG)
#define EventBusPublishSync(event, param, len) EventBusPublishSyncInner(event, #event, param, len)
#define EventBusPublishAsync(event, param, len, freeFunc) \
    EventBusPublishAsyncInner(event, #event, param, len, freeFunc)
#else
#define EventBusPublishSync(event, param, len) EventBusPublishSyncInner(event, NULL, param, len)
#define EventBusPublishAsync(event, param, len, freeFunc) \
    EventBusPublishAsyncInner(event, NULL, param, len, freeFunc)
#endif

#ifdef __cplusplus
}
#endif

#endif /* EVENT_BUS_PUB_H */
