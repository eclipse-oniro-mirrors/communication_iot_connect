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
#include "securec.h"
#include "iotc_log.h"
#include "hilog/log.h"

#define IOTC_LOG_LINE_MAX_LENGTH   512
#define IOTC_LOG_DOMAIN 0xD000101 /* 此值需确认 */
#define IOTC_LOG_TAG "iotc"

void AdapterLogOutput(AdapterLogInfo *info)
{
    if (info == NULL || info->fmt == NULL || info->level >= IOTC_LOG_LEVEL_MAX ||
        info->level <= IOTC_LOG_LEVEL_MIN) {
        return;
    }
    const LogLevel level[IOTC_LOG_LEVEL_DEBUG] = {LOG_FATAL, LOG_ERROR, LOG_WARN, LOG_INFO, LOG_INFO, LOG_DEBUG};
    const char *tag[IOTC_LOG_LEVEL_DEBUG] = {"IC_FATAL", "IC_ERROR", "IC_WARN", "IC_NOTICE", "IC_INFO", "IC_DEBUG"};

    char line[IOTC_LOG_LINE_MAX_LENGTH + 1] = { 0 };
    int32_t ret = 0;
    uint32_t next = 0;
    if (info->funcName != NULL) {
        ret = sprintf_s(&line[next], sizeof(line) - next, "%s:%s:%u, ",
            tag[info->level - 1], info->funcName, info->line);
    } else {
        ret = sprintf_s(&line[next], sizeof(line) - next, "%s:%s:%u, ",
            tag[info->level - 1], info->fileName != NULL ? info->fileName : "NULL", info->line);
    }
    if (ret <= 0) {
        HiLogPrint(LOG_CORE, level[info->level - 1], IOTC_LOG_DOMAIN, IOTC_LOG_TAG, "%{public}s", "too long");
        return;
    }
    next += ret;
    ret = vsprintf_s(&line[next], sizeof(line) - next, info->fmt, info->args);
    if (ret <= 0) {
        HiLogPrint(LOG_CORE, level[info->level - 1], IOTC_LOG_DOMAIN, IOTC_LOG_TAG, "%{public}s", "too long");
        return;
    }
    (void)HiLogPrint(LOG_CORE, level[info->level - 1], IOTC_LOG_DOMAIN, IOTC_LOG_TAG, "%{public}s", line);
}

void IotcLogOutputImpl(uint8_t level, const char *fileName,
    const char *funcName, uint32_t line, const char *fmt, ...)
{
    if (level > IotcGetLogLevel() || level > IOTC_LOG_LEVEL_DEBUG || level == 0 || fmt == NULL) {
        return;
    }

    const LogLevel level[IOTC_LOG_LEVEL_DEBUG] = {LOG_FATAL, LOG_ERROR, LOG_WARN, LOG_INFO, LOG_INFO, LOG_DEBUG};
    const char *tag[IOTC_LOG_LEVEL_DEBUG] = {"IC_FATAL", "IC_ERROR", "IC_WARN", "IC_NOTICE", "IC_INFO", "IC_DEBUG"};

    char line[IOTC_LOG_LINE_MAX_LENGTH + 1] = { 0 };
    int32_t ret = 0;
    uint32_t next = 0;
    if (funcName != NULL) {
        ret = sprintf_s(&line[next], sizeof(line) - next, "%s:%s:%u, ", tag[level - 1], funcName, line);
    } else {
        ret = sprintf_s(&line[next], sizeof(line) - next, "%s:%s:%u, ", tag[level - 1],
            fileName != NULL ? fileName : "NULL", line);
    }
    if (ret <= 0) {
        HiLogPrint(LOG_CORE, level[level - 1], IOTC_LOG_DOMAIN, IOTC_LOG_TAG, "%{public}s", "too long");
        return;
    }
    next += ret;
    va_list ap;
    va_start(ap, fmt);
    ret = vsprintf_s(&line[next], sizeof(line) - next, fmt, ap);
    va_end(ap);
    if (ret <= 0) {
        HiLogPrint(LOG_CORE, level[info->level - 1], IOTC_LOG_DOMAIN, IOTC_LOG_TAG, "%{public}s", "too long");
        return;
    }
    (void)HiLogPrint(LOG_CORE, level[info->level - 1], IOTC_LOG_DOMAIN, IOTC_LOG_TAG, "%{public}s", line);
}