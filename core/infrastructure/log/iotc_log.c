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
#include "iotc_log.h"
#include <stddef.h>
#include "adapter_log.h"

static uint8_t g_logLevel = IOT_CONF_LOG_DEFAULT_LEVEL;

void IotcLogOutputInner(uint8_t level, const char *fileName,
    const char *funcName, uint32_t line, const char *fmt, ...)
{
    if (level > g_logLevel || fmt == NULL) {
        return;
    }

    AdapterLogInfo info = {
        .level = level,
        .fileName = fileName,
        .funcName = funcName,
        .line = line,
        .fmt = fmt,
    };
    va_start(info.args, fmt);
    AdapterLogOutput(&info);
    va_end(info.args);
}

void IotcSetLogLevel(uint8_t level)
{
    if (level >  IOTC_LOG_LEVEL_MAX) {
        IOTC_LOGW("invalid log level %u", level);
        return;
    }
    AdapterSetLogLevel(level);
    IOTC_LOGN("set log level to %u", level);
    g_logLevel = level;
}

uint8_t IotcGetLogLevel(void)
{
    return g_logLevel;
}