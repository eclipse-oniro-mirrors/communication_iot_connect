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
#ifndef ADAPTER_LOG_H
#define ADAPTER_LOG_H

#include <stdint.h>
#include <stdarg.h>
#include "iotc_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t level;
    const char *fileName;
    const char *funcName;
    uint32_t line;
    const char *fmt;
    va_list args;
} AdapterLogInfo;

void AdapterLogOutput(AdapterLogInfo *info);

void AdapterLogOutputInner(uint8_t level, const char *fileName,
    const char *funcName, uint32_t line, const char *fmt, ...);

void AdapterSetLogLevel(uint8_t level);

#ifndef ADAPTER_FILE_NAME
#define ADAPTER_FILE_NAME (__builtin_strrchr("/" __FILE__, '/') + 1)
#endif
#ifndef ADAPTER_FUNC_NAME
#define ADAPTER_FUNC_NAME NULL
#endif

#if IOTC_CONF_LOG_BUILD_LEVEL >= IOTC_LOG_LEVEL_DEBUG
#define ADAPTER_LOGD(...) \
    AdapterLogOutputInner(IOTC_LOG_LEVEL_DEBUG, ADAPTER_FILE_NAME, ADAPTER_FUNC_NAME, __LINE__, __VA_ARGS__)
#else
#define ADAPTER_LOGD(...)
#endif

#if IOTC_CONF_LOG_BUILD_LEVEL >= IOTC_LOG_LEVEL_INFO
#define ADAPTER_LOGI(...) \
    AdapterLogOutputInner(IOTC_LOG_LEVEL_INFO, ADAPTER_FILE_NAME, ADAPTER_FUNC_NAME, __LINE__, __VA_ARGS__)
#else
#define ADAPTER_LOGI(...)
#endif

#if IOTC_CONF_LOG_BUILD_LEVEL >= IOTC_LOG_LEVEL_NOTICE
#define ADAPTER_LOGN(...) \
    AdapterLogOutputInner(IOTC_LOG_LEVEL_NOTICE, ADAPTER_FILE_NAME, ADAPTER_FUNC_NAME, __LINE__, __VA_ARGS__)
#else
#define ADAPTER_LOGN(...)
#endif

#if IOTC_CONF_LOG_BUILD_LEVEL >= IOTC_LOG_LEVEL_WARN
#define ADAPTER_LOGW(...) \
    AdapterLogOutputInner(IOTC_LOG_LEVEL_WARN, ADAPTER_FILE_NAME, ADAPTER_FUNC_NAME, __LINE__, __VA_ARGS__)
#else
#define ADAPTER_LOGW(...)
#endif

#if IOTC_CONF_LOG_BUILD_LEVEL >= IOTC_LOG_LEVEL_ERROR
#define ADAPTER_LOGE(...) \
    AdapterLogOutputInner(IOTC_LOG_LEVEL_ERROR, ADAPTER_FILE_NAME, ADAPTER_FUNC_NAME, __LINE__, __VA_ARGS__)
#else
#define ADAPTER_LOGE(...)
#endif

#if IOTC_CONF_LOG_BUILD_LEVEL >= IOTC_LOG_LEVEL_FATAL
#define ADAPTER_LOGF(...) \
    AdapterLogOutputInner(IOTC_LOG_LEVEL_FATAL, ADAPTER_FILE_NAME, ADAPTER_FUNC_NAME, __LINE__, __VA_ARGS__)
#else
#define ADAPTER_LOGF(...)
#endif

#ifdef __cplusplus
}
#endif

#endif /* ADAPTER_LOG_H */
