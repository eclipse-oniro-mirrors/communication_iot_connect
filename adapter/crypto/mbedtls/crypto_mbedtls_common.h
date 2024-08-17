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
#ifndef MBEDTLS_ADAPTER_COMMON_H
#define MBEDTLS_ADAPTER_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include "iotc_md.h"
#include "iotc_mpi.h"
#include "mbedtls/md.h"
#include "mbedtls/bignum.h"

#ifdef __cplusplus
extern "C" {
#endif

mbedtls_md_type_t GetMbedtlsMdType(IotcMdType type);

mbedtls_mpi *GetMbedtlsMpi(IotcMpi *mpi);

bool IsMdLenValid(IotcMdType type, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_ADAPTER_COMMON_H */