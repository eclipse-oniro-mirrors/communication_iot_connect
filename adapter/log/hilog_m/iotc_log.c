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
#include <stdint.h>
#include <stdio.h>

// #define IotcPrintf(...) { HILOG_INFO(HILOG_MODULE_APP,__VA_ARGS__); }; 
// #define IotcPrintf(...) { printf(__VA_ARGS__); }; 


void IotcLogOutputImpl(uint8_t level, const char *fileName,
    const char *funcName, uint32_t line, const char *fmt, ...)
{
    const char *tag[6] = {"IC_FATAL", "IC_ERROR", "IC_WARN", "IC_NOTICE", "IC_INFO", "IC_DEBUG"};
    if (funcName != NULL) {
        printf("%s:%s:%u, ", tag[level - 1], funcName, line);
    } else {
        printf("%s:%s:%u, ", tag[level - 1], fileName != NULL ? fileName : "NULL", line);
    }
}

