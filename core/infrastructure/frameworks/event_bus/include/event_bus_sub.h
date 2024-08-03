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
#ifndef EVENT_BUS_SUB_H
#define EVENT_BUS_SUB_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 事件处理回调
 *
 * @param event [IN] 事件
 * @param param [IN] 参数
 * @param len [IN] 参数长度
 */
typedef void (*EventBusCallback)(uint32_t event, void *param, uint32_t len);

/**
 * @brief 事件匹配函数
 *
 * @param event [IN] 事件
 * @param param [IN] 参数
 * @return true 事件匹配成功，false 事件匹配失败
 */
typedef bool (*EventMatchFunc)(uint32_t event);

/**
 * @brief 基于事件id订阅事件
 *
 * @param listener [IN] 监听回调
 * @param event [IN] 事件
 * @return 0 成功，非0失败
 */
int32_t EventBusSubscribe(EventBusCallback listener, uint32_t event);

/**
 * @brief 基于事件匹配函数订阅事件
 *
 * @param listener [IN] 监听回调
 * @param match [IN] 匹配函数
 * @return 0 成功，非0失败
 */
int32_t EventBusSubscribeMatch(EventBusCallback listener, EventMatchFunc match);

/**
 * @brief 订阅所有事件
 *
 * @param listener [IN] 监听回调
 * @return 0 成功，非0失败
 */
int32_t EventBusSubscribeAll(EventBusCallback listener);

/**
 * @brief 去除订阅
 *
 * @param listener [IN] 监听回调
 * @return 0 成功，非0失败
 */
int32_t EventBusUnsubscribe(EventBusCallback listener);

#ifdef __cplusplus
}
#endif

#endif /* EVENT_BUS_SUB_H */
