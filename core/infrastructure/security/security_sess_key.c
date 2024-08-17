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
#include "security_sess_key.h"
#include "adapter_mem.h"
#include "securec.h"
#include "utils_assert.h"
#include "iotc_errcode.h"
#include "utils_bit_map.h"
#include "adapter_kdf.h"

typedef enum {
    SESS_KEY_BIT_TRANS_KEY_CREATED = 0,
    SESS_KEY_BIT_AUTH_KEY_CREATED,
    SESS_KEY_BIT_SN1_SET,
    SESS_KEY_BIT_SN2_SET,
} SessKeyBitMap;

struct SessKeyContext {
    uint8_t bitMap;
    uint8_t sn[SESS_SN_LEN * 2];
    uint8_t transKey[SESS_TRANS_KEY_LEN];
    uint8_t authKey[SESS_AUTH_KEY_LEN];
};

SessKeyContext *SecuritySessKeyInit(void)
{
    SessKeyContext *ctx = IotcMalloc(sizeof(SessKeyContext));
    if (ctx == NULL) {
        IOTC_LOGW("malloc error");
        return NULL;
    }
    (void)memset_s(ctx, sizeof(SessKeyContext), 0, sizeof(SessKeyContext));
    return ctx;
}

static void SessKeyParamUpdate(SessKeyContext *ctx)
{
    if (UTILS_IS_BIT_SET(ctx->bitMap, SESS_KEY_BIT_TRANS_KEY_CREATED)) {
        (void)memset_s(ctx->transKey, SESS_TRANS_KEY_LEN, 0, SESS_TRANS_KEY_LEN);
        UTILS_BIT_RESET(ctx->bitMap, SESS_KEY_BIT_TRANS_KEY_CREATED);
    }
    if (UTILS_IS_BIT_SET(ctx->bitMap, SESS_KEY_BIT_AUTH_KEY_CREATED)) {
        (void)memset_s(ctx->authKey, SESS_AUTH_KEY_LEN, 0, SESS_AUTH_KEY_LEN);
        UTILS_BIT_RESET(ctx->bitMap, SESS_KEY_BIT_AUTH_KEY_CREATED);
    }
}

int32_t SecuritySessKeyAddSn(SessKeyContext *ctx, const uint8_t sn[SESS_SN_LEN], SessKeySnType type)
{
    CHECK_RETURN_LOGW(ctx != NULL && sn != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    int32_t ret;
    if (type == SESS_KEY_SN1) {
        ret = memcpy_s(ctx->sn, SESS_SN_LEN, sn, SESS_SN_LEN);
    } else {
        ret = memcpy_s(ctx->sn + SESS_SN_LEN, SESS_SN_LEN, sn, SESS_SN_LEN);
    }
    if (ret != EOK) {
        return IOTC_ERR_SECUREC_MEMCPY;
    }

    SessKeyParamUpdate(ctx);
    if (type == SESS_KEY_SN1) {
        UTILS_BIT_SET(ctx->bitMap, SESS_KEY_BIT_SN1_SET);
    } else {
        UTILS_BIT_SET(ctx->bitMap, SESS_KEY_BIT_SN2_SET);
    }
    return IOTC_OK;
}

int32_t SecuritySessKeyGenTransKey(SessKeyContext *ctx, const uint8_t *pwd, uint32_t pwdLen,
    uint8_t transKey[SESS_TRANS_KEY_LEN])
{
    CHECK_RETURN_LOGW(ctx != NULL && pwd != NULL && pwdLen != 0 && transKey != NULL,
        IOTC_ERR_PARAM_INVALID, "param invalid");

    int32_t ret;
    if (UTILS_IS_BIT_SET(ctx->bitMap, SESS_KEY_BIT_TRANS_KEY_CREATED)) {
        ret = memcpy_s(transKey, SESS_TRANS_KEY_LEN, ctx->transKey, SESS_TRANS_KEY_LEN);
        if (ret != EOK) {
            return IOTC_ERR_SECUREC_MEMCPY;
        }
        return IOTC_OK;
    }

    if (!UTILS_IS_BIT_SET(ctx->bitMap, SESS_KEY_BIT_SN1_SET) || !UTILS_IS_BIT_SET(ctx->bitMap, SESS_KEY_BIT_SN2_SET)) {
        IOTC_LOGW("trans key not ready %u", ctx->bitMap);
        return IOTC_CORE_COMM_SEC_ERR_SESS_KEY_NOT_READY;
    }
    SessKeyParamUpdate(ctx);

    IotcPbkdf2HmacParam param = {
        .md = IOTC_MD_SHA256,
        .password = pwd,
        .passwordLen = pwdLen,
        .salt = ctx->sn,
        .saltLen = sizeof(ctx->sn),
        /* 迭代次数为1依赖pwd的安全性 */
        .iterCount = 1,
    };
    ret = IotcPkcs5Pbkdf2Hmac(&param, transKey, SESS_TRANS_KEY_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGW("pkcs5pbkdf2hmac gen trans key error %d", ret);
        return ret;
    }

    ret = memcpy_s(ctx->transKey, SESS_TRANS_KEY_LEN, transKey, SESS_TRANS_KEY_LEN);
    if (ret != EOK) {
        (void)memset_s(transKey, SESS_TRANS_KEY_LEN, 0, SESS_TRANS_KEY_LEN);
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    UTILS_BIT_SET(ctx->bitMap, SESS_KEY_BIT_TRANS_KEY_CREATED);
    return IOTC_OK;
}

int32_t SecuritySessKeyGenAuthKey(SessKeyContext *ctx, uint8_t authKey[SESS_AUTH_KEY_LEN])
{
    CHECK_RETURN_LOGW(ctx != NULL && authKey != NULL,
        IOTC_ERR_PARAM_INVALID, "param invalid");
    
    int32_t ret;
    if (UTILS_IS_BIT_SET(ctx->bitMap, SESS_KEY_BIT_AUTH_KEY_CREATED)) {
        ret = memcpy_s(authKey, SESS_AUTH_KEY_LEN, ctx->authKey, SESS_AUTH_KEY_LEN);
        if (ret != EOK) {
            return IOTC_ERR_SECUREC_MEMCPY;
        }
        return IOTC_OK;
    }

    if (!UTILS_IS_BIT_SET(ctx->bitMap, SESS_KEY_BIT_SN1_SET) || !UTILS_IS_BIT_SET(ctx->bitMap, SESS_KEY_BIT_SN2_SET) ||
        !UTILS_IS_BIT_SET(ctx->bitMap, SESS_KEY_BIT_TRANS_KEY_CREATED)) {
        IOTC_LOGW("auth key not ready %u", ctx->bitMap);
        return IOTC_CORE_COMM_SEC_ERR_SESS_KEY_NOT_READY;
    }

    IotcPbkdf2HmacParam param = {
        .md = IOTC_MD_SHA256,
        .password = ctx->transKey,
        .passwordLen = SESS_TRANS_KEY_LEN,
        .salt = ctx->sn,
        .saltLen = sizeof(ctx->sn),
        /* 迭代次数为1依赖pwd的安全性 */
        .iterCount = 1,
    };

    ret = IotcPkcs5Pbkdf2Hmac(&param, authKey, SESS_AUTH_KEY_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGW("pkcs5pbkdf2hmac gen auth key error %d", ret);
        return ret;
    }

    ret = memcpy_s(ctx->authKey, sizeof(ctx->authKey), authKey, SESS_AUTH_KEY_LEN);
    if (ret != EOK) {
        (void)memset_s(authKey, SESS_AUTH_KEY_LEN, 0, SESS_AUTH_KEY_LEN);
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    UTILS_BIT_SET(ctx->bitMap, SESS_KEY_BIT_AUTH_KEY_CREATED);
    return IOTC_OK;
}

void SecuritySessDestroy(SessKeyContext *ctx)
{
    CHECK_V_RETURN(ctx != NULL);

    (void)memset_s(ctx, sizeof(SessKeyContext), 0, sizeof(SessKeyContext));
    IotcFree(ctx);
}
