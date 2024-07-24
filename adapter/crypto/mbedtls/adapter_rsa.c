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
#include "crypto_mbedtls_common.h"
#include "adapter_rsa.h"
#include "mbedtls/rsa.h"
#include "adapter_mem.h"
#include "securec.h"
#include "adapter_log.h"

#define MBEDTLS_RADIX_NUM_BASE 16

typedef struct {
    AdapterRsaPkcs1Mode padding;
    AdapterMdType md;
    mbedtls_rsa_context rsa;
    int32_t (*rng)(uint8_t *out, uint32_t len);
} RsaContext;

AdapterRsaContext *AdapterRsaInit(AdapterRsaPkcs1Mode padding, AdapterMdType md)
{
    RsaContext *ctx = (RsaContext *)AdapterMalloc(sizeof(RsaContext));
    if (ctx == NULL) {
        ADAPTER_LOGW("malloc error");
        return NULL;
    }
    (void)memset_s(ctx, sizeof(RsaContext), 0, sizeof(RsaContext));
    ctx->padding = padding;
    ctx->md = md;
#if IOTC_CONF_ADAPTER_MBEDTLS_MAJOR_VERSION == 2
    mbedtls_rsa_init(&ctx->rsa, padding, GetMbedtlsMdType(md));
#else
    mbedtls_rsa_init(&ctx->rsa);
    mbedtls_rsa_set_padding(&ctx->rsa, padding, GetMbedtlsMdType(md));
#endif
    return ctx;
}

void AdapterRsaFree(AdapterRsaContext *ctx)
{
    if (ctx == NULL) {
        ADAPTER_LOGW("invalid param");
        return;
    }
    mbedtls_rsa_free(&((RsaContext *)ctx)->rsa);
    AdapterFree(ctx);
}

int AdapterRsaParamImport(AdapterRsaContext *ctx, const AdapterRsaParam *param)
{
    if ((ctx == NULL) || (param == NULL)) {
        ADAPTER_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    int32_t ret = mbedtls_rsa_import(&((RsaContext *)ctx)->rsa,
        GetMbedtlsMpi(param->n), GetMbedtlsMpi(param->p),
        GetMbedtlsMpi(param->q), GetMbedtlsMpi(param->d), GetMbedtlsMpi(param->e));
    if (ret != 0) {
        ADAPTER_LOGW("import error[-0x%04x]", -ret);
        return IOTC_ADAPTER_CRYPTO_ERR_RSA_PARAM_IMPORT;
    }
    return IOTC_OK;
}

int AdapterRsaPkcs1Verify(AdapterRsaContext *ctx, const AdapterRsaVerifyParam *param)
{
    if (ctx == NULL || param == NULL || param->hash == NULL || param->hashLen == 0 ||
        param->sig == NULL || param->sigLen != mbedtls_rsa_get_len(&((RsaContext *)ctx)->rsa) ||
        (param->md != ADAPTER_MD_NONE && !IsMdLenValid(param->md, param->hashLen))) {
        ADAPTER_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }
    mbedtls_rsa_context *rsa = &((RsaContext *)ctx)->rsa;
    int32_t ret = mbedtls_rsa_check_pubkey(rsa);
    if (ret != 0) {
        ADAPTER_LOGW("check error [-0x%04x]", -ret);
        return IOTC_ADAPTER_CRYPTO_ERR_RSA_VERIFY;
    }

    ret = mbedtls_rsa_pkcs1_verify(rsa,
#if IOTC_CONF_ADAPTER_MBEDTLS_MAJOR_VERSION == 2
        NULL, NULL, MBEDTLS_RSA_PUBLIC,
#endif
        GetMbedtlsMdType(param->md), param->hashLen, param->hash, param->sig);
    if (ret != 0) {
        ADAPTER_LOGW("verify error [-0x%04x]", -ret);
        return IOTC_ADAPTER_CRYPTO_ERR_RSA_VERIFY;
    }

    return IOTC_OK;
}

static int32_t RngForMbedtls(void *param, unsigned char *out, size_t len)
{
    RsaContext *ctx = (RsaContext *)param;
    if ((ctx == NULL) || (ctx->rng == NULL) || (out == NULL) || (len == 0)) {
        return -1;
    }
    return ctx->rng(out, len);
}

int AdapterRsaPkcs1Decrypt(const AdapterRsaCryptParam *param, uint8_t *buf, uint32_t *len)
{
    if (param == NULL || param->ctx == NULL || param->input == NULL || buf == NULL ||
        param->inLen != mbedtls_rsa_get_len(&(((RsaContext *)(param->ctx))->rsa)) || len == NULL || *len == 0) {
        ADAPTER_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    RsaContext *ctx = (RsaContext *)param->ctx;
    int32_t ret;
    if (param->mode == ADAPTER_RSA_OP_PRIVATE) {
        if (param->rng == NULL) {
            ADAPTER_LOGW("invalid rng");
            return IOTC_ERR_PARAM_INVALID;
        }
        ctx->rng = param->rng;
        ret = mbedtls_rsa_check_privkey(&ctx->rsa);
    } else {
#if IOTC_CONF_ADAPTER_MBEDTLS_MAJOR_VERSION == 2
        ret = mbedtls_rsa_check_pubkey(&ctx->rsa);
#else
        ADAPTER_LOGW("mode not support public key");
        return IOTC_ERR_NOT_SUPPORT;
#endif
    }
    if (ret != 0) {
        ADAPTER_LOGW("check error [-0x%04x]", -ret);
        return IOTC_ADAPTER_CRYPTO_ERR_RSA_DEC;
    }

    size_t oLen = 0;
    ret = mbedtls_rsa_pkcs1_decrypt(&ctx->rsa, RngForMbedtls, ctx,
#if IOTC_CONF_ADAPTER_MBEDTLS_MAJOR_VERSION == 2
        param->mode,
#endif
        &oLen, param->input, buf, *len);
    if (ret != 0) {
        ADAPTER_LOGW("decrypt error [-0x%04x]", -ret);
        return IOTC_ADAPTER_CRYPTO_ERR_RSA_DEC;
    }
    *len = oLen;
    return IOTC_OK;
}

int AdapterRsaPkcs1Encrypt(const AdapterRsaCryptParam *param, uint8_t *buf, uint32_t len)
{
    if (param == NULL || param->ctx == NULL || param->input == NULL || buf == NULL ||
        len < mbedtls_rsa_get_len(&(((RsaContext *)(param->ctx))->rsa))) {
        ADAPTER_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    RsaContext *ctx = (RsaContext *)param->ctx;
    int32_t ret;
    if (param->mode == ADAPTER_RSA_OP_PRIVATE) {
#if IOTC_CONF_ADAPTER_MBEDTLS_MAJOR_VERSION == 2
        ret = mbedtls_rsa_check_privkey(&ctx->rsa);
#else
        ADAPTER_LOGW("mode not support private key");
        return IOTC_ERR_NOT_SUPPORT;
#endif
    } else {
        ret = mbedtls_rsa_check_pubkey(&ctx->rsa);
    }
    if (ret != 0) {
        ADAPTER_LOGW("check error [-0x%04x]", -ret);
        return IOTC_ADAPTER_CRYPTO_ERR_RSA_ENC;
    }
    bool isNeedRng = false;
    if ((param->mode == ADAPTER_RSA_OP_PUBLIC) || (ctx->padding == ADAPTER_RSA_PKCS1_V21)) {
        if (param->rng == NULL) {
            ADAPTER_LOGW("invalid rng");
            return IOTC_ERR_PARAM_INVALID;
        }
        ctx->rng = param->rng;
        isNeedRng = true;
    }

    ret = mbedtls_rsa_pkcs1_encrypt(&ctx->rsa, isNeedRng ? RngForMbedtls : NULL, isNeedRng ? ctx : NULL,
#if IOTC_CONF_ADAPTER_MBEDTLS_MAJOR_VERSION == 2
        param->mode,
#endif
        param->inLen, param->input, buf);
    if (ret != 0) {
        ADAPTER_LOGW("encrypt error [-0x%04x]", -ret);
        return IOTC_ADAPTER_CRYPTO_ERR_RSA_ENC;
    }
    return IOTC_OK;
}