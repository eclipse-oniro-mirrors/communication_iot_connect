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
#include <stddef.h>
#include "iotc_errcode.h"
#include "adapter_mpi.h"
#include "mbedtls/bignum.h"
#include "adapter_mem.h"
#include "adapter_log.h"

AdapterMpi *AdapterMpiInit(void)
{
    mbedtls_mpi *mpi = (mbedtls_mpi *)AdapterMalloc(sizeof(mbedtls_mpi));
    if (mpi == NULL) {
        ADAPTER_LOGW("malloc error");
        return NULL;
    }
    mbedtls_mpi_init(mpi);
    return mpi;
}

mbedtls_mpi *GetMbedtlsMpi(AdapterMpi *mpi)
{
    return mpi;
}

void AdapterMpiFree(AdapterMpi *mpi)
{
    if (mpi == NULL) {
        return;
    }
    mbedtls_mpi_free(GetMbedtlsMpi(mpi));
    AdapterFree(mpi);
}

int32_t AdapterMpiExpMod(AdapterMpi *x, AdapterMpi *a, AdapterMpi *e, AdapterMpi *n)
{
    if ((x == NULL) || (a == NULL) || (e == NULL) || (n == NULL)) {
        ADAPTER_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    int32_t ret = mbedtls_mpi_exp_mod(GetMbedtlsMpi(x), GetMbedtlsMpi(a), GetMbedtlsMpi(e), GetMbedtlsMpi(n), NULL);
    if (ret != 0) {
        ADAPTER_LOGW("exp mod error [-0x%04x]", -ret);
        return  IOTC_ADAPTER_CRYPTO_ERR_MPI_EXP_MOD;
    }
    return IOTC_OK;
}

int32_t AdapterMpiCmpInt(AdapterMpi *x, int64_t z)
{
    if (x == NULL) {
        ADAPTER_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    return mbedtls_mpi_cmp_int(GetMbedtlsMpi(x), z);
}

int32_t AdapterMpiSubInt(AdapterMpi *x, AdapterMpi *a, int64_t b)
{
    if ((x == NULL) || (a == NULL)) {
        ADAPTER_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    int32_t ret = mbedtls_mpi_sub_int(GetMbedtlsMpi(x), GetMbedtlsMpi(a), b);
    if (ret != 0) {
        ADAPTER_LOGW("sub int error [-0x%04x]", -ret);
        return  IOTC_ADAPTER_CRYPTO_ERR_MPI_SUB_INT;
    }
    return IOTC_OK;
}

int32_t AdapterMpiCmpMpi(AdapterMpi *x, AdapterMpi *y)
{
    if ((x == NULL) || (y == NULL)) {
        ADAPTER_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    return mbedtls_mpi_cmp_mpi(GetMbedtlsMpi(x), GetMbedtlsMpi(y));
}

int32_t AdapterMpiReadString(AdapterMpi *mpi, uint8_t radix, const char *s)
{
    /* 2和16代表进制 */
    if ((mpi == NULL) || (s == NULL) || (radix < 2) || (radix > 16)) {
        ADAPTER_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    int32_t ret = mbedtls_mpi_read_string(GetMbedtlsMpi(mpi), radix, s);
    if (ret != 0) {
        ADAPTER_LOGW("read string error [-0x%04x]", -ret);
        return IOTC_ADAPTER_CRYPTO_ERR_MPI_READ_STRING;
    }
    return IOTC_OK;
}

int32_t AdapterMpiWriteString(AdapterMpi *mpi, uint8_t radix, char *buf, uint32_t *bufLen)
{
    /* 2和16代表进制*/
    if ((mpi == NULL) || (buf == NULL) || (radix < 2) || (radix > 16) || (bufLen == NULL) ||
        (*bufLen == 0)) {
        ADAPTER_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    size_t oLen = 0;
    int32_t ret = mbedtls_mpi_write_string(GetMbedtlsMpi(mpi), radix, buf, *bufLen, &oLen);
    if (ret != 0) {
        ADAPTER_LOGW("write string error [-0x%04x]", -ret);
        return IOTC_ADAPTER_CRYPTO_ERR_MPI_WRITE_STRING;
    }
    *bufLen = oLen;
    return IOTC_OK;
}

int32_t AdapterMpiReadBinary(AdapterMpi *mpi, const uint8_t *buf, uint32_t bufLen)
{
    if ((mpi == NULL) || (buf == NULL) || (bufLen == 0)) {
        ADAPTER_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    int32_t ret = mbedtls_mpi_read_binary(GetMbedtlsMpi(mpi), buf, bufLen);
    if (ret != 0) {
        ADAPTER_LOGW("read binary error [-0x%04x]", -ret);
        return IOTC_ADAPTER_CRYPTO_ERR_MPI_READ_BINARY;
    }
    return IOTC_OK;
}

int32_t AdapterMpiWriteBinary(AdapterMpi *mpi, uint8_t *buf, uint32_t bufLen)
{
    if ((mpi == NULL) || (buf == NULL) || (bufLen == 0)) {
        ADAPTER_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    int32_t ret = mbedtls_mpi_write_binary(GetMbedtlsMpi(mpi), buf, bufLen);
    if (ret != 0) {
        ADAPTER_LOGW("write binary error [-0x%04x]", -ret);
        return IOTC_ADAPTER_CRYPTO_ERR_MPI_WRITE_BINARY;
    }
    return IOTC_OK;
}