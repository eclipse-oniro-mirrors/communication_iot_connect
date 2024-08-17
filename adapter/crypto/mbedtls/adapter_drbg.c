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
#include <stdlib.h>
#include "adapter_drbg.h"
#include "adapter_mem.h"
#include "iotc_errcode.h"
#include "adapter_log.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/platform.h"
#include "mbedtls/entropy.h"
#include "securec.h"

typedef struct {
    IotcTrngCallback trng;
    mbedtls_ctr_drbg_context drbg;
    mbedtls_entropy_context entropy;
} DrbgContext;

/* 此函数为注册到mbedtls的熵源回调函数，按照mbedtls_entropy_f_source_ptr的定义进行实现 */
static int32_t MbedtlsEntropySourceCallback(void *data, unsigned char *output, size_t len, size_t *outLen)
{
    if (output == NULL || outLen == NULL || data == NULL) {
        ADAPTER_LOGW("invalid param");
        return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
    }

    DrbgContext *ctx = (DrbgContext *)data;
    if (ctx->trng == NULL) {
        ADAPTER_LOGW("no trng");
        return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
    }

    int32_t ret = ctx->trng(output, len);
    if (ret != 0) {
        ADAPTER_LOGW("trng error %d", ret);
        return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
    }
    *outLen = len;
    return 0;
}

IotcDrbgContext *IotcDrbgInit(const char *custom, IotcTrngCallback trng)
{
    /* 入参可以为空 */
    DrbgContext *ctx = (DrbgContext *)IotcMalloc(sizeof(DrbgContext));
    if (ctx == NULL) {
        ADAPTER_LOGW("malloc err");
        return NULL;
    }
    (void)memset_s(ctx, sizeof(DrbgContext), 0, sizeof(DrbgContext));
    ctx->trng = trng;
    int32_t ret;

    do {
        mbedtls_entropy_init(&ctx->entropy);
        mbedtls_ctr_drbg_init(&ctx->drbg);

        if (trng != NULL) {
            ret = mbedtls_entropy_add_source(&ctx->entropy, MbedtlsEntropySourceCallback,
                /* at least get 4 byte random epr invoke */
                ctx, 4, MBEDTLS_ENTROPY_SOURCE_STRONG);
            if (ret != 0) {
                ADAPTER_LOGW("add entropy source error [-0x%04x]", -ret);
                break;
            }
        }

        ret = mbedtls_ctr_drbg_seed(&ctx->drbg, mbedtls_entropy_func, &ctx->entropy,
            (const unsigned char *)custom, custom == NULL ? 0 : strlen(custom));
        if (ret != 0) {
            ADAPTER_LOGW("seed error [-0x%04x]", -ret);
            break;
        }

        mbedtls_ctr_drbg_set_prediction_resistance(&ctx->drbg, MBEDTLS_CTR_DRBG_PR_OFF);
        return ctx;
    } while (0);

    IotcDrbgDeinit(ctx);
    return NULL;
}

void IotcDrbgDeinit(IotcDrbgContext *ctx)
{
    if (ctx == NULL) {
        return;
    }

    mbedtls_entropy_free(&((DrbgContext *)ctx)->entropy);
    mbedtls_ctr_drbg_free(&((DrbgContext *)ctx)->drbg);
    IotcFree(ctx);
}

int32_t IotcDrbgRandom(IotcDrbgContext *ctx, uint8_t *out, uint32_t outLen)
{
    if ((ctx == NULL) || (out == NULL) || (outLen == 0)) {
        ADAPTER_LOGW("invalid param\r\n");
        return IOTC_ERR_PARAM_INVALID;
    }

    int32_t ret = mbedtls_ctr_drbg_random((void *)&((DrbgContext *)ctx)->drbg, out, outLen);
    if (ret != 0) {
        ADAPTER_LOGW("get mbedtls random error [-0x%04x]", -ret);
        return IOTC_ADAPTER_CRYPTO_ERR_RANDOM;
    }

    return IOTC_OK;
}