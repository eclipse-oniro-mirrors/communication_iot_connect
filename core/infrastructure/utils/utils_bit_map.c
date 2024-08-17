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
#include "utils_bit_map.h"
#include "iotc_mem.h"
#include "comm_def.h"
#include "iotc_log.h"
#include "securec.h"

struct BitMap {
    uint32_t size;
    uint8_t map[];
};

#define UINT8_BIT_NUM 8

BitMap *UtilsCreateBitMap(uint32_t size)
{
    if (size > UTILS_BIT_MAP_MAX_BIT) {
        IOTC_LOGW("invalid bit map size %u", size);
        return NULL;
    }

    uint32_t num = size / UINT8_BIT_NUM + 1;
    uint32_t memSize = sizeof(BitMap) + sizeof(uint8_t) * num;
    BitMap *map = (BitMap *)IotcMalloc(memSize);
    if (map == NULL) {
        IOTC_LOGW("malloc error %u", memSize);
        return NULL;
    }
    (void)memset_s(map, memSize, 0, memSize);
    map->size = size * UINT8_BIT_NUM;
    return map;
}

void UtilsFreeBitMap(BitMap *map)
{
    if (map == NULL) {
        return;
    }
    IotcFree(map);
}

void UtilsBitMapSet(BitMap *map, uint32_t bit)
{
    if (map == NULL || bit >= map->size) {
        return;
    }
    UTILS_BIT_SET(map->map[bit / UINT8_BIT_NUM], bit % UINT8_BIT_NUM);
}

void UtilsBitMapReset(BitMap *map, uint32_t bit)
{
    if (map == NULL || bit >= map->size) {
        return;
    }
    UTILS_BIT_RESET(map->map[bit / UINT8_BIT_NUM], bit % UINT8_BIT_NUM);
}

bool UtilsIsBitSet(BitMap *map, uint32_t bit)
{
    if (map == NULL || bit >= map->size) {
        return false;
    }
    return UTILS_IS_BIT_SET(map->map[bit / UINT8_BIT_NUM], bit % UINT8_BIT_NUM);
}