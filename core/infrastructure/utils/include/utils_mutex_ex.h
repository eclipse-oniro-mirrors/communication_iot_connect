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
#ifndef UTILS_MUTEX_EX_H
#define UTILS_MUTEX_EX_H

#include <stddef.h>
#include <stdbool.h>
#include "adapter_os.h"
#include "utils_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 锁默认超时时间 */
#ifndef UTILS_MUTEX_EX_DEFAULT_TIMEOUT_MS
#define UTILS_MUTEX_EX_DEFAULT_TIMEOUT_MS UTILS_MIN_TO_MS(2)
#endif

/** 扩展锁句柄 */
typedef struct UtilsExMutex UtilsExMutex;

/**
 * @brief 创建互斥锁
 *
 * @return UtilsExMutex*
 */
UtilsExMutex *UtilsCreateExMutex(void);

/**
 * @brief 互斥锁上锁
 *
 * @param mutex [IN] 互斥锁
 * @param timeout [IN] 超时时间
 * @param func [IN] 调用函数
 * @return true成功false失败
 * @attention 内部接口，请勿直接调用，使用MutexLock与MutexLockTimeout
 */
bool UtilsExMutexLockInner(UtilsExMutex *mutex, uint32_t timeout, const char *func);

/**
 * @brief 互斥锁上锁，超时时间为MUTEX_DEFAULT_TIMEOUT_MS
 *
 * @param mutex [IN] 互斥锁
 * @return true成功false失败
 */
#define UtilsExMutexLock(mutex) UtilsExMutexLockInner(mutex, UTILS_MUTEX_EX_DEFAULT_TIMEOUT_MS, __func__)

/**
 * @brief 互斥锁上锁，带超时时间
 *
 * @param mutex [IN] 互斥锁
 * @param timeout [IN] 超时时间，单位ms
 * @return true成功false失败
 */
#define UtilsExMutexLockTimeout(mutex, timeout) UtilsExMutexLockInner(mutex, timeout, __func__)

/**
 * @brief 互斥锁上锁，无限等
 *
 * @param mutex [IN] 互斥锁
 * @return true成功false失败
 */
#define UtilsExMutexLockAnyway(mutex) UtilsExMutexLockTimeout(mutex, IOTC_WAIT_FOREVER)

/**
 * @brief 互斥锁解锁
 *
 * @param mutex [IN] 互斥锁
 * @param func [IN] 调用函数
 * @attention 内部接口，请勿直接调用，使用MutexUnlock
 */
void UtilsExMutexUnlockInner(UtilsExMutex *mutex, const char *func);

/**
 * @brief 互斥锁解锁
 *
 * @param mutex [IN] 互斥锁
 */
#define UtilsExMutexUnlock(mutex) UtilsExMutexUnlockInner(mutex, __func__)

/**
 * @brief 销毁互斥锁
 *
 * @param mutex [IN] 互斥锁
 */
void UtilsDestroyExMutex(UtilsExMutex **mutex);

#ifdef __cplusplus
}
#endif
#endif /* UTILS_MUTEX_EX_H */