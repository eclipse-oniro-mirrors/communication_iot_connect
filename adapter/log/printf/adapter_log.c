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
#include "adapter_log.h"
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

void AdapterLogOutput(AdapterLogInfo *info)
{
    if (info == NULL || info->fmt == NULL || info->level >= IOTC_LOG_LEVEL_MAX ||
        info->level <= IOTC_LOG_LEVEL_MIN) {
        return;
    }

    const char *tag[IOTC_LOG_LEVEL_DEBUG] = {"IC_FATAL", "IC_ERROR", "IC_WARN", "IC_NOTICE", "IC_INFO", "IC_DEBUG"};
    if (info->funcName != NULL) {
        IotcPrintf("%s:%s:%u, ", tag[info->level - 1], info->funcName, info->line);
    } else {
        IotcPrintf("%s:%s:%u, ", tag[info->level - 1], info->fileName != NULL ? info->fileName : "NULL", info->line);
    }
    vprintf(info->fmt, info->args);
    IotcPrintf("\n");
}