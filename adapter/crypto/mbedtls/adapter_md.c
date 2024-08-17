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
#include "iotc_errcode.h"
#include "adapter_mem.h"
#include "adapter_md.h"
#include "crypto_mbedtls_common.h"
#include "mbedtls/md.h"
#include "securec.h"
#include "adapter_log.h"

typedef struct {
    IotcMdType type;
    mbedtls_md_context_t mdCtx;
} MdContext;

mbedtls_md_type_t GetMbedtlsMdType(IotcMdType type)
{
    switch (type) {
        case IOTC_MD_NONE:
            return MBEDTLS_MD_NONE;
        case IOTC_MD_SHA256:
            return MBEDTLS_MD_SHA256;
        case IOTC_MD_SHA384:
            return MBEDTLS_MD_SHA384;
        case IOTC_MD_SHA512:
            return MBEDTLS_MD_SHA512;
        default:
            ADAPTER_LOGW("invalid mode");
            return MBEDTLS_MD_NONE;
    }
}

bool IsMdLenValid(IotcMdType type, uint32_t len)
{
    if (type == IOTC_MD_SHA256) {
        return len >= IOTC_MD_SHA256_BYTE_LEN;
    } else if (type == IOTC_MD_SHA384) {
        return len >= IOTC_MD_SHA384_BYTE_LEN;
    } else if (type == IOTC_MD_SHA512) {
        return len >= IOTC_MD_SHA512_BYTE_LEN;
    }
    ADAPTER_LOGW("invalid type %d", type);
    return false;
}

int32_t IotcMdCalc(IotcMdType type, const uint8_t *inData, uint32_t inLen, uint8_t *md, uint32_t mdLen)
{
    if (inData == NULL || inLen == 0 || md == NULL || !IsMdLenValid(type, mdLen)) {
        ADAPTER_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    int32_t ret = mbedtls_md(mbedtls_md_info_from_type(GetMbedtlsMdType(type)), inData, inLen, md);
    if (ret != 0) {
        ADAPTER_LOGW("mbedtls md calc error [-0x%04x]", -ret);
        return IOTC_ADAPTER_CRYPTO_ERR_MD_CALC;
    }
    return IOTC_OK;
}

int32_t IotcHmacCalc(const IotcHmacParam *param, uint8_t *hmac, uint32_t hmacLen)
{
    if ((param == NULL) || (param->key == NULL) || (param->keyLen == 0) ||
        (param->data == NULL) || (param->dataLen == 0) || (hmac == NULL) ||
        (!IsMdLenValid(param->md, hmacLen))) {
        ADAPTER_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    int32_t ret = mbedtls_md_hmac(mbedtls_md_info_from_type(GetMbedtlsMdType(param->md)), param->key, param->keyLen,
        param->data, param->dataLen, hmac);
    if (ret != 0) {
        ADAPTER_LOGW("mbedtls hmac calc error [-0x%04x]", -ret);
        return IOTC_ADAPTER_CRYPTO_ERR_HMAC_CALC;
    }
    return IOTC_OK;
}

IotcMdContext *IotcMdInit(IotcMdType type)
{
    MdContext *ctx = (MdContext *)AdapterMalloc(sizeof(MdContext));
    if (ctx == NULL) {
        ADAPTER_LOGW("malloc error");
        return NULL;
    }
    (void)memset_s(ctx, sizeof(MdContext), 0, sizeof(MdContext));

    ctx->type = type;
    mbedtls_md_init(&ctx->mdCtx);
    /* 0不使用hmac */
    int32_t ret = mbedtls_md_setup(&ctx->mdCtx, mbedtls_md_info_from_type(GetMbedtlsMdType(type)), 0);
    if (ret != 0) {
        ADAPTER_LOGW("mbedtls md setup error [-0x%04x]", -ret);
        mbedtls_md_free(&ctx->mdCtx);
        IotcMdFree(ctx);
        return NULL;
    }
    ret = mbedtls_md_starts(&ctx->mdCtx);
    if (ret != 0) {
        ADAPTER_LOGW("mbedtls md start error [-0x%04x]", -ret);
        mbedtls_md_free(&ctx->mdCtx);
        IotcMdFree(ctx);
        return NULL;
    }

    return ctx;
}

int32_t IotcMdUpdate(IotcMdContext *ctx, const uint8_t *inData, uint32_t inLen)
{
    if (ctx == NULL) {
        ADAPTER_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    int32_t ret = mbedtls_md_update(&((MdContext *)ctx)->mdCtx, inData, inLen);
    if (ret != 0) {
        ADAPTER_LOGW("mbedtls md update error [-0x%04x]", -ret);
        return IOTC_ADAPTER_CRYPTO_ERR_MD_UPDATE;
    }

    return IOTC_OK;
}

int32_t IotcMdFinish(IotcMdContext *ctx, uint8_t *outData, uint32_t outLen)
{
    if (ctx == NULL || !IsMdLenValid(((MdContext *)ctx)->type, outLen)) {
        ADAPTER_LOGW("invalid param");
        return IOTC_ERR_PARAM_INVALID;
    }

    int32_t ret = mbedtls_md_finish(&((MdContext *)ctx)->mdCtx, outData);
    if (ret != 0) {
        ADAPTER_LOGW("mbedtls md calc error [-0x%04x]", -ret);
        return IOTC_ADAPTER_CRYPTO_ERR_MD_CALC;
    }
    return IOTC_OK;
}

void IotcMdFree(IotcMdContext *ctx)
{
    if (ctx == NULL) {
        return;
    }
    mbedtls_md_free(&((MdContext *)ctx)->mdCtx);
    AdapterFree(ctx);
}