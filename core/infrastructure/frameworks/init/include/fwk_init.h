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
#ifndef FWK_INIT_H
#define FWK_INIT_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef FWK_INIT_BLOCK_SLEEP_TIME_SEC
#define FWK_INIT_BLOCK_SLEEP_TIME_SEC 5
#endif

typedef enum {
    FWK_INIT_POLICY_BLOCK = 0,
    FWK_INIT_FAILED_RETURN,
    FWK_INIT_FAILED_DEINIT_ALL,
} FwkInitPolicy;

typedef enum {
    FWK_INIT_LVL_MIN = 0,
    FWK_INIT_LVL_DEP,
    FWK_INIT_LVL_COMP,
    FWK_INIT_LVL_FWK,
    FWK_INIT_LVL_BIZ,
    FWK_INIT_LVL_MAX,
} FwkInitLevel;

typedef struct {
    FwkInitLevel level;
    const char *name;
    int32_t (*initFunc)(void);
    void (*deinitFunc)(void);
} FwkInitUnit;

int32_t FwkInitAll(FwkInitPolicy policy);

void FwkDeinitAll(void);

int32_t FwkRegInitUnits(const FwkInitUnit *units, uint32_t num, const char *mdlName);

#define FwkRegInitUnitArray(unitArray) \
    FwkRegInitUnits(&((unitArray)[0]), (sizeof(unitArray) / sizeof(FwkInitUnit)), #unitArray)

void FwkUnregInitUnit(const FwkInitUnit *units);

void FwkUnregAll(void);

#ifdef __cplusplus
}
#endif

#endif /* FWK_INIT_H */
