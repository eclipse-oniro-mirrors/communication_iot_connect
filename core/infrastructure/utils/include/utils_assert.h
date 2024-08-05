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
#ifndef UTILS_ASSERT_H
#define UTILS_ASSERT_H
#include "iotc_log.h"

#define CHECK_RETURN(cond, ret) do { \
    if (!(cond)) { \
        return (ret); \
    } \
} while (0)

#define CHECK_RETURN_LOGW(cond, ret, ...) do { \
    if (!(cond)) { \
        IOTC_LOGW(__VA_ARGS__); \
        return (ret); \
    } \
} while (0)

#define CHECK_RETURN_LOGE(cond, ret, ...) do { \
    if (!(cond)) { \
        IOTC_LOGE(__VA_ARGS__); \
        return (ret); \
    } \
} while (0)

#define CHECK_V_RETURN(cond) do { \
    if (!(cond)) { \
        return; \
    } \
} while (0)

#define CHECK_V_RETURN_LOGW(cond, ...) do { \
    if (!(cond)) { \
        IOTC_LOGW(__VA_ARGS__); \
        return; \
    } \
} while (0)

#define CHECK_V_RETURN_LOGE(cond, ...) do { \
    if (!(cond)) { \
        IOTC_LOGE(__VA_ARGS__); \
        return; \
    } \
} while (0)

#endif