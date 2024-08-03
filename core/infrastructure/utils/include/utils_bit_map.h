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
#ifndef UTILS_BIT_MAP_H
#define UTILS_BIT_MAP_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UTILS_BIT_MAP_MAX_BIT 2048

#define UTILS_BIT(n)              (1UL << (n))
#define UTILS_BIT_CLR(data)       ((data) = 0)
#define UTILS_BIT_SET(data, n)    ((data) |= UTILS_BIT(n))
#define UTILS_BIT_RESET(data, n)  ((data) &= (~UTILS_BIT(n)))
#define UTILS_IS_BIT_SET(data, n) (((data) & UTILS_BIT(n)) != 0)

typedef struct TagBitMap BitMap;

BitMap *UtilsCreateBitMap(uint32_t size);

void UtilsFreeBitMap(BitMap *map);

void UtilsBitMapSet(BitMap *map, uint32_t bit);

void UtilsBitMapReset(BitMap *map, uint32_t bit);

bool UtilsIsBitSet(BitMap *map, uint32_t bit);

#ifdef __cplusplus
}
#endif

#endif /* UTILS_BIT_MAP_H */
