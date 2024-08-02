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
#ifndef FRAMEWORK_MAIN_H
#define FRAMEWORK_MAIN_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "iotc_event.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t IotcFwkMain(uint32_t stackSize);

/* 该接口同步返回，不能由SDK内部线程调用 */
int32_t IotcFwkReset(void);

/* 该接口同步返回，不能由SDK内部线程调用 */
int32_t IotcFwkStop(void);

/* 该接口发布同步事件，仅能由SDK内部线程调用 */
int32_t IotcFwkRestore(void);

void IotcFwkNotifyReset(void);

void IotcFwkNotifyStop(void);

bool IotcIsFwkMainRuning(void);

#define CHECK_MAIN_RUNNING_V_RETURN() \
    do { \
        if (IotcIsFwkMainRuning()) { \
            IOTC_LOGE("iotc main is runing"); \
            return; \
        } \
    } while (0)

#define CHECK_MAIN_RUNNING_RETURN() \
    do { \
        if (IotcIsFwkMainRuning()) { \
            IOTC_LOGE("iotc main is runing"); \
            return IOTC_CORE_COMM_FWK_ERR_MAIN_ALREADY_RUNNING; \
        } \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* FRAMEWORK_MAIN_H */