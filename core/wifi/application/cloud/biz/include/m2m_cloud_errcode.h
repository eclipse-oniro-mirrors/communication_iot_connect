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
#ifndef M2M_CLOUD_ERRCODE_H
#define M2M_CLOUD_ERRCODE_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CLOUD_ERRCODE_OK = 0,
    CLOUD_ERRCODE_REGISTERED = 2,
    CLOUD_ERRCODE_CODE_EXPIRED = 3,
    CLOUD_ERRCODE_CODE_SECRET_ERR = 5,
    CLOUD_ERRCODE_CODE_DEV_DELETED = 6,
} CloudErrcode;

#ifdef __cplusplus
}
#endif

#endif /* M2M_CLOUD_ERRCODE_H */