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
#include "iotc_mem.h"
#include <string.h>
#include <stdlib.h>

#define IOTC_MAX_MALLOC_SIZE  (2 * 1024 * 1024)

void *IotcMalloc(uint32_t size)
{
    if (size == 0) {
        return NULL;
    }
    if (size > IOTC_MAX_MALLOC_SIZE) {
        return NULL;
    }
    return malloc(size);
}

void *IotcCalloc(uint32_t num, uint32_t size)
{
    if ((size == 0) || (num == 0)) {
        return NULL;
    }

    if (size > IOTC_MAX_MALLOC_SIZE || num > IOTC_MAX_MALLOC_SIZE / size) {
        return NULL;
    }
    return calloc(num, size);
}

void IotcFree(void *pt)
{
    if (pt == NULL) {
        return;
    }
    free(pt);
}