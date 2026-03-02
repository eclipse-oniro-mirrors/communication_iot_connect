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
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include "iotc_log.h"
#include "iotc_errcode.h"

static void IotcPrintf(const char *fmt, ...)
{
    if (fmt == NULL) {
        return;
    }
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    return;
}

void IotcLogOutputImpl(uint8_t level, const char *fileName,
    const char *funcName, uint32_t line, const char *fmt, ...)
{
    if (level > IotcGetLogLevel() || level > IOTC_LOG_LEVEL_DEBUG || level == 0 || fmt == NULL) {
        return;
    }

    const char *tag[IOTC_LOG_LEVEL_DEBUG] = {"IC_FATAL", "IC_ERROR", "IC_WARN", "IC_NOTICE", "IC_INFO", "IC_DEBUG"};
    if (funcName != NULL) {
        IotcPrintf("%s:%s:%u, ", tag[level - 1], funcName, line);
    } else {
        IotcPrintf("%s:%s:%u, ", tag[level - 1], fileName != NULL ? fileName : "NULL", line);
    }

    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    IotcPrintf("\n");
}