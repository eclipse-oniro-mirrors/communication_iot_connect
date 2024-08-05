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
#include "utils_time.h"
#include <stdio.h>
#include <limits.h>
#include <math.h>
#include "securec.h"
#include "iotc_log.h"
#include "iotc_errcode.h"
#include "adapter_os.h"
#include "utils_mutex_ex.h"

typedef struct {
    uint8_t hour;
    uint8_t min;
    bool isEast; /* true:东时区(+) false:西时区(-) */
    bool isInit;
} TimeZoneInfo;

typedef struct {
    bool isInit;
    bool isTimeSet;
    UtilsExMutex *mutex;
    uint64_t lastUtcTime;
    uint64_t lastLocalTime;
    uint64_t lastSystemTime;
    TimeZoneInfo timeZone;
} TimeContext;

static TimeContext *GetTimeCtx(void)
{
    static TimeContext ctx = {
        .isInit = false,
        .isTimeSet = false,
        .mutex = NULL,
    };
    return &ctx;
}

#define UTILS_MS_PER_HOUR UTILS_HOUR_TO_MS(1)
#define UTILS_MS_PER_MIN UTILS_MIN_TO_MS(1)
#define UTILS_HOUR_PER_DAY 24
#define UTILS_MONTHS_PER_YEAR 12

#define MONTH_JANUARY 1
#define MONTH_FEBRUARY 2

#define DEFAULT_TIME_ZONE_HOUR 8

#define TIME_LOCK() UtilsExMutexLock(GetTimeCtx()->mutex)
#define TIME_UNLOCK() UtilsExMutexUnlock(GetTimeCtx()->mutex)

static bool IsTimeOverflow(uint64_t deltaTime, uint64_t lastTime)
{
    if (deltaTime > (UINT64_MAX - lastTime)) {
        IOTC_LOGE("utc time overflow");
        return false;
    }
    return true;
}

static int32_t GetDeltaTime(uint64_t lastSystemTime, uint64_t lastTime, uint64_t *deltaTime)
{
    uint32_t curStamp = AdapterGetSysTimeMs();
    uint64_t tmpDeltaTime = (uint64_t)UtilsDeltaTime(curStamp, lastSystemTime);
    if (!IsTimeOverflow(tmpDeltaTime, lastTime)) {
        IOTC_LOGE("time overflow");
        return IOTC_CORE_COMM_UTILS_ERR_TIME_OVERFLOW;
    }
    *deltaTime = tmpDeltaTime;
    return IOTC_OK;
}

int32_t UtilsGetUtcTimeStamp(uint64_t *ts)
{
    if (ts == NULL) {
        IOTC_LOGW("param invalid");
        return IOTC_ERR_PARAM_INVALID;
    }

    uint64_t utcTime = 0;
    int32_t ret = IOTC_OK;
    if (!TIME_LOCK()) {
        return IOTC_ERR_TIMEOUT;
    }
    do {
        TimeContext *ctx = GetTimeCtx();
        if (!ctx->isTimeSet) {
            IOTC_LOGE("timeStamp is not sync");
            ret = IOTC_CORE_COMM_UTILS_ERR_TIME_NOT_SYNC;
            break;
        }

        uint64_t deltaTime = 0;
        ret = GetDeltaTime(ctx->lastSystemTime, ctx->lastUtcTime, &deltaTime);
        if (ret != IOTC_OK) {
            IOTC_LOGE("utc time err");
            break;
        }
        utcTime = (uint64_t)(ctx->lastUtcTime + deltaTime);
        *ts = utcTime;
        IOTC_LOGN("get utc timeStamp[%llu] succ", utcTime);
    } while (false);
    TIME_UNLOCK();

    return ret;
}

static int32_t CalcLocalTimeFromUtcTime(TimeContext *ctx)
{
    if (!ctx->timeZone.isEast) {
        ctx->lastLocalTime = (ctx->lastUtcTime -
            (ctx->timeZone.hour * UTILS_MS_PER_HOUR) - (ctx->timeZone.min * UTILS_MS_PER_MIN));
    } else {
        uint64_t deltaTime = (uint64_t)((ctx->timeZone.hour * UTILS_MS_PER_HOUR) +
            (ctx->timeZone.min * UTILS_MS_PER_MIN));
        if (!IsTimeOverflow(deltaTime, ctx->lastUtcTime)) {
            return IOTC_CORE_COMM_UTILS_ERR_TIME_OVERFLOW;
        }
        ctx->lastLocalTime = ctx->lastUtcTime + deltaTime;
    }
    return IOTC_OK;
}

static void SetDefaultTimezone(TimeZoneInfo *timeZone)
{
    timeZone->isEast = true;
    timeZone->hour = DEFAULT_TIME_ZONE_HOUR;
    timeZone->min = 0;
    timeZone->isInit = true;
    IOTC_LOGN("Set Default Timezone succ");
    return;
}

int32_t UtilsSetUtcTimeStamp(uint64_t stamp)
{
    uint32_t curStamp = AdapterGetSysTimeMs();
    int32_t ret = IOTC_OK;

    if (!TIME_LOCK()) {
        return IOTC_ERR_TIMEOUT;
    }
    do {
        TimeContext *ctx = GetTimeCtx();
        if (!ctx->timeZone.isInit) {
            SetDefaultTimezone(&ctx->timeZone);
        }

        ctx->lastUtcTime = stamp;
        ctx->lastSystemTime = curStamp;
        ret = CalcLocalTimeFromUtcTime(ctx);
        if (ret != IOTC_OK) {
            IOTC_LOGE("calc local time err, ret=%d", ret);
            break;
        }

        if (!ctx->isTimeSet) {
            ctx->isTimeSet = true;
        }
        IOTC_LOGN("Set Utc TimeStamp[%llu] succ", stamp);
    } while (false);
    TIME_UNLOCK();

    return ret;
}

int32_t UtilsSetTimezone(int8_t hour, int8_t min)
{
    if (!TIME_LOCK()) {
        return IOTC_ERR_TIMEOUT;
    }
    TimeContext *ctx = GetTimeCtx();
    ctx->timeZone.isEast = (hour > 0) ? true : false;
    ctx->timeZone.hour = (uint8_t)abs(hour);
    ctx->timeZone.min = (uint8_t)min;
    ctx->timeZone.isInit = true;
    TIME_UNLOCK();
    IOTC_LOGN("set timezone[hour:%d,min:%d] succ", hour, min);
    return IOTC_OK;
}

/* 通过年月日解析星期 */
static int32_t GetWeekDayFromDate(uint16_t year, uint8_t month, uint8_t day)
{
    /* 1/2月份进行转换 */
    if ((month == MONTH_JANUARY) || (month == MONTH_FEBRUARY)) {
        month += UTILS_MONTHS_PER_YEAR;
        year--;
    }

    if (month > (UTILS_MONTHS_PER_YEAR + MONTH_FEBRUARY)) {
        return IOTC_CORE_COMM_UTILS_ERR_TIME_GET_WEEKDAY;
    }

    /* 利用基姆拉尔森公式计算星期，数字为公式中的常量 */
    uint8_t week = (day + 2 * month + 3 * (month + 1) / 5 + year +
        year / 4 - year / 100 + year / 400) % 7;
    return week;
}

/* 将系统timeSec转换为日期 */
static int32_t GetDate(uint64_t timeSec, UtilsTimeInfo *info)
{
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
    uint32_t e;
    uint32_t f;

    /* 利用算法转换时间为日期，数字4,102032,146097,2442113均为公式中的常量 */
    a = (uint32_t)((4 * timeSec + 102032) / 146097 + 15);
    b = (uint32_t)(timeSec + 2442113 + a - (a / 4));
    if (b > (UINT_MAX / 20)) {
        return IOTC_CORE_COMM_UTILS_ERR_TIME_GET_DATA;
    }
    /* 数字20,365,2442,7305均为公式中的常量 */
    c = (20 * b - 2442) / 7305;
    d = b - 365 * c - (c / 4);
    if (d > (UINT_MAX / 1000)) {
        return IOTC_CORE_COMM_UTILS_ERR_TIME_GET_DATA;
    }
    /* 数字30,30601,601,1000均为公式中的常量 */
    e = d * 1000 / 30601;
    f = d - e * 30 - e * 601 / 1000;

    /* 一月和二月被当做是前一年的第13和14个月,数字4716,4715均为公式中的常量 */
    if (e <= 13) {
        c -= 4716;
        e -= 1;
    } else {
        c -= 4715;
        e -= 13;
    }

    info->year = (uint16_t)c;
    info->month = (uint8_t)e;
    info->day = (uint8_t)f;
    return IOTC_OK;
}

static int32_t ConvertTime(uint64_t timeMs, UtilsTimeInfo *info)
{
    if ((info == NULL) || (timeMs == 0)) {
        IOTC_LOGW("param invalid");
        return IOTC_ERR_PARAM_INVALID;
    }

    uint64_t timeSec = (timeMs / UTILS_MS_PER_SECOND);
    info->sec = (uint8_t)(timeSec % UTILS_SEC_PER_MIN);
    timeSec = (uint32_t)(timeSec / UTILS_SEC_PER_MIN);
    info->min = (uint8_t)(timeSec % UTILS_MIN_PER_HOUR);
    timeSec = (uint32_t)(timeSec / UTILS_MIN_PER_HOUR);
    info->hour = (uint8_t)(timeSec % UTILS_HOUR_PER_DAY);
    timeSec = (uint32_t)(timeSec / UTILS_HOUR_PER_DAY);

    if (GetDate(timeSec, info) != IOTC_OK) {
        IOTC_LOGE("get date err");
        return IOTC_CORE_COMM_UTILS_ERR_TIME_GET_DATA;
    }
    int32_t weekDay = GetWeekDayFromDate(info->year, info->month, info->day);
    if (weekDay == IOTC_CORE_COMM_UTILS_ERR_TIME_GET_WEEKDAY) {
        IOTC_LOGE("get weekDay err");
        return IOTC_CORE_COMM_UTILS_ERR_TIME_GET_WEEKDAY;
    }
    info->week = (uint8_t)weekDay;
    info->week = (uint8_t)(1 << info->week);
    return IOTC_OK;
}

static int32_t GetTimeInfo(UtilsTimeInfo *info, TimeContext *ctx, uint64_t lastTime)
{
    if (!ctx->isTimeSet) {
        IOTC_LOGE("timeStamp is not sync");
        return IOTC_CORE_COMM_UTILS_ERR_TIME_NOT_SYNC;
    }

    uint64_t deltaTime = 0;
    int32_t ret = GetDeltaTime(ctx->lastSystemTime, lastTime, &deltaTime);
    if (ret != IOTC_OK) {
        IOTC_LOGE("get deltaTime err");
        return ret;
    }
    ret = ConvertTime((deltaTime + lastTime), info);
    if (ret != IOTC_OK) {
        IOTC_LOGE("Convert Time err");
        return ret;
    }
    IOTC_LOGN("Get Time[%d-%d-%d %02d:%02d:%02d, week:%d]succ",
        info->year, info->month, info->day,
        info->hour, info->min, info->sec, info->week);
    return IOTC_OK;
}

int32_t UtilsGetUtcTimeInfo(UtilsTimeInfo *info)
{
    if (info == NULL) {
        IOTC_LOGW("param invalid");
        return IOTC_ERR_PARAM_INVALID;
    }

    int32_t ret = IOTC_OK;
    if (!TIME_LOCK()) {
        return IOTC_ERR_TIMEOUT;
    }
    ret = GetTimeInfo(info, GetTimeCtx(), GetTimeCtx()->lastUtcTime);
    TIME_UNLOCK();
    return ret;
}

int32_t UtilsGetLocalTimeInfo(UtilsTimeInfo *info)
{
    if (info == NULL) {
        IOTC_LOGW("param invalid");
        return IOTC_ERR_PARAM_INVALID;
    }

    int32_t ret = IOTC_OK;
    if (!TIME_LOCK()) {
        return IOTC_ERR_TIMEOUT;
    }
    ret = GetTimeInfo(info, GetTimeCtx(), GetTimeCtx()->lastLocalTime);
    TIME_UNLOCK();
    return ret;
}

bool UtilsIsTimeSet(void)
{
    return GetTimeCtx()->isTimeSet;
}

int32_t UtilsTimeInit(void)
{
    TimeContext *ctx = GetTimeCtx();
    if (ctx->isInit) {
        IOTC_LOGN("time is initialized");
        return IOTC_OK;
    }
    (void)memset_s(ctx, sizeof(TimeContext), 0, sizeof(TimeContext));

    ctx->mutex = UtilsCreateExMutex();
    if (ctx->mutex == NULL) {
        IOTC_LOGE("mutex create err");
        return IOTC_CORE_COMM_UTILS_ERR_EX_MUTEX_CREATE;
    }
    ctx->isInit = true;
    IOTC_LOGN("time init succ");
    return IOTC_OK;
}

static void CleanTimeCtx(TimeContext *ctx)
{
    ctx->isInit = false;
    ctx->isTimeSet = false;
    ctx->lastUtcTime = 0;
    ctx->lastLocalTime = 0;
    ctx->lastSystemTime = 0;
    (void)memset_s(&ctx->timeZone, sizeof(TimeZoneInfo), 0, sizeof(TimeZoneInfo));
}

int32_t UtilsTimeDeinit(void)
{
    TimeContext *ctx = GetTimeCtx();
    if (!TIME_LOCK()) {
        return IOTC_ERR_TIMEOUT;
    }
    CleanTimeCtx(ctx);
    TIME_UNLOCK();

    if (ctx->mutex != NULL) {
        UtilsDestroyExMutex(&ctx->mutex);
    }
    IOTC_LOGN("time deinit succ");
    return IOTC_OK;
}