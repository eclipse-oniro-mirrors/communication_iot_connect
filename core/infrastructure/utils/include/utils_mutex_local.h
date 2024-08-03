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
#ifndef UTILS_MUTEX_LOCAL_H
#define UTILS_MUTEX_LOCAL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *handle;
} UtilsMutexLocal;

int32_t UtilsCreateMutexLocal(UtilsMutexLocal *mutex);

void UtilsDestroyMutexLocal(UtilsMutexLocal *mutex);

bool UtilsMutexLocalLockInner(UtilsMutexLocal *mutex, const char *func);

void UtilsMutexLocalUnlockInner(UtilsMutexLocal *mutex, const char *func);

#define UtilsMutexLocalLock(mutex) UtilsMutexLocalLockInner(mutex, __func__)

#define UtilsMutexLocalUnlock(mutex) UtilsMutexLocalUnlockInner(mutex, __func__)

#ifdef __cplusplus
}
#endif

#endif /* UTILS_MUTEX_LOCAL_H */
