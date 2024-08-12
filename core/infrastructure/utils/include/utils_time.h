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
#ifndef UTILS_TIME_H
#define UTILS_TIME_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t week;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
} UtilsTimeInfo;

int32_t UtilsGetUtcTimeStamp(uint64_t *ts);

int32_t UtilsSetUtcTimeStamp(uint64_t stamp);

int32_t UtilsSetTimezone(int8_t hour, int8_t min);

int32_t UtilsGetUtcTimeInfo(UtilsTimeInfo *info);

int32_t UtilsGetLocalTimeInfo(UtilsTimeInfo *info);

bool UtilsIsTimeSet(void);

int32_t UtilsTimeInit(void);

void UtilsTimeDeinit(void);

#ifdef __cplusplus
}
#endif

#endif /* UTILS_TIME_H */
