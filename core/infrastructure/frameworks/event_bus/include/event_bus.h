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
#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include <stdint.h>
#include "event_bus_pub.h"
#include "event_bus_sub.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 事件总线模块初始化
 *
 * @return 0成功，非0失败
 */
int32_t EventBusInit(void);

/**
 * @brief 注册事件总线异步执行器
 *
 * @param asyncExecutor [IN] 异步执行器
 * @return 0成功，非0失败
 */
int32_t EventBusRegAsyncExecutor(EventBusAsyncExecutor asyncExecutor);

/**
 * @brief 释放事件总线
 *
 */
void EventBusDeinit(void);

#ifdef __cplusplus
}
#endif

#endif /* EVENT_BUS_H */
