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
#ifndef IOTC_LOG_H
#define IOTC_LOG_H

#include <stddef.h>
#include <stdint.h>
#include "iotc_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef IOTC_FILE_NAME
#define IOTC_FILE_NAME (__builtin_strrchr("/" __FILE__, '/') + 1)
#endif
#ifndef IOTC_FUNC_NAME
#define IOTC_FUNC_NAME (NULL)
#endif
#ifndef IOTC_CONF_LITEOS_M_LOG_PRINTF
#define IOTC_CONF_LITEOS_M_LOG_PRINTF 1
#endif

void IotcLogOutputImpl(uint8_t level, const char *fileName,
    const char *funcName, uint32_t line, const char *fmt, ...);

void IotcSetLogLevel(uint8_t level);

uint8_t IotcGetLogLevel(void);

#define IotcPrintf(...) \
    do { \
        printf(__VA_ARGS__); \
        printf("\n"); \
    } while (0)

#if IOTC_CONF_LOG_BUILD_LEVEL >= IOTC_LOG_LEVEL_DEBUG
#if IOTC_CONF_LITEOS_M_LOG_PRINTF
#define IOTC_LOGD(...) \
    do {
        IotcLogOutputImpl(IOTC_LOG_LEVEL_DEBUG, IOTC_FILE_NAME, IOTC_FUNC_NAME, __LINE__, __VA_ARGS__);
        IotcPrintf(__VA_ARGS__);
    } while (0)
#else
#define IOTC_LOGD(...) IotcLogOutputImpl(IOTC_LOG_LEVEL_DEBUG, IOTC_FILE_NAME, IOTC_FUNC_NAME, __LINE__, __VA_ARGS__)
#endif //IOTC_CONF_LITEOS_M_LOG_PRINTF
#else
#define IOTC_LOGD(...)
#endif

#if IOTC_CONF_LOG_BUILD_LEVEL >= IOTC_LOG_LEVEL_INFO
#if IOTC_CONF_LITEOS_M_LOG_PRINTF
#define IOTC_LOGI(...) \
    do { \
        IotcLogOutputImpl(IOTC_LOG_LEVEL_INFO, IOTC_FILE_NAME, IOTC_FUNC_NAME, __LINE__, __VA_ARGS__); \
        IotcPrintf(__VA_ARGS__); \
    } while (0)
#else
#define IOTC_LOGI(...) IotcLogOutputImpl(IOTC_LOG_LEVEL_INFO, IOTC_FILE_NAME, IOTC_FUNC_NAME, __LINE__, __VA_ARGS__)
#endif //IOTC_CONF_LITEOS_M_LOG_PRINTF
#else
#define IOTC_LOGI(...)
#endif

#if IOTC_CONF_LOG_BUILD_LEVEL >= IOTC_LOG_LEVEL_NOTICE
#if IOTC_CONF_LITEOS_M_LOG_PRINTF
#define IOTC_LOGN(...) \
    do {
        IotcLogOutputImpl(IOTC_LOG_LEVEL_NOTICE, IOTC_FILE_NAME, IOTC_FUNC_NAME, __LINE__, __VA_ARGS__);
        IotcPrintf(__VA_ARGS__);
    } while (0)
#else
#define IOTC_LOGN(...) IotcLogOutputImpl(IOTC_LOG_LEVEL_NOTICE, IOTC_FILE_NAME, IOTC_FUNC_NAME, __LINE__, __VA_ARGS__)
#endif
#else
#define IOTC_LOGN(...)
#endif

#if IOTC_CONF_LOG_BUILD_LEVEL >= IOTC_LOG_LEVEL_WARN
#if IOTC_CONF_LITEOS_M_LOG_PRINTF
#define IOTC_LOGW(...) \
    do { \
        IotcLogOutputImpl(IOTC_LOG_LEVEL_WARN, IOTC_FILE_NAME, IOTC_FUNC_NAME, __LINE__, __VA_ARGS__); \
        IotcPrintf(__VA_ARGS__); \
    } while (0)
#else
#define IOTC_LOGW(...) IotcLogOutputImpl(IOTC_LOG_LEVEL_WARN, IOTC_FILE_NAME, IOTC_FUNC_NAME, __LINE__, __VA_ARGS__)
#endif
#else
#define IOTC_LOGW(...)
#endif

#if IOTC_CONF_LOG_BUILD_LEVEL >= IOTC_LOG_LEVEL_ERROR
#if IOTC_CONF_LITEOS_M_LOG_PRINTF
#define IOTC_LOGE(...) \
    do {
        IotcLogOutputImpl(IOTC_LOG_LEVEL_ERROR, IOTC_FILE_NAME, IOTC_FUNC_NAME, __LINE__, __VA_ARGS__);
        IotcPrintf(__VA_ARGS__);
    } while (0)
#else
#define IOTC_LOGE(...) IotcLogOutputImpl(IOTC_LOG_LEVEL_ERROR, IOTC_FILE_NAME, IOTC_FUNC_NAME, __LINE__, __VA_ARGS__)
#endif
#else
#define IOTC_LOGE(...)
#endif

#if IOTC_CONF_LOG_BUILD_LEVEL >= IOTC_LOG_LEVEL_FATAL
#if IOTC_CONF_LITEOS_M_LOG_PRINTF
#define IOTC_LOGF(...) \
    do {
        IotcLogOutputImpl(IOTC_LOG_LEVEL_FATAL, IOTC_FILE_NAME, IOTC_FUNC_NAME, __LINE__, __VA_ARGS__);
        IotcPrintf(__VA_ARGS__);
    } while (0)
#else
#define IOTC_LOGF(...) IotcLogOutputImpl(IOTC_LOG_LEVEL_FATAL, IOTC_FILE_NAME, IOTC_FUNC_NAME, __LINE__, __VA_ARGS__)
#endif
#else
#define IOTC_LOGF(...)
#endif

#ifdef __cplusplus
}
#endif

#endif /*IOTC_LOG_H */