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
#include "security_speke_nego_ctx.h"
#include "security_speke_defs.h"
#include "iotc_log.h"
#include "securec.h"
#include "adapter_mem.h"
#include "utils_common.h"
#include "security_random.h"
#include "adapter_kdf.h"
#include "iotc_errcode.h"

/* 数据读取基数 */
#define NUMERIC_BASE 16
#define RANDOM_LEN_BIG 32
#define RANDOM_LEN_NORMAL 24
#define HKDF_KEY_HEX_BUF_LEN (HEXIFY_LEN(ADAPTER_MD_SHA256_BYTE_LEN) + 1)
#define SPEKE_EXP "2"
#define MIN_PUB_KEY 2
#define MIN_SALT_LEN 8

/* 3072bit素数 */
#define SPEKE_BIG_PRIME "FFFFFFFFFFFFFFFFC90FDAA22168C234" \
                        "C4C6628B80DC1CD129024E088A67CC74" \
                        "020BBEA63B139B22514A08798E3404DD" \
                        "EF9519B3CD3A431B302B0A6DF25F1437" \
                        "4FE1356D6D51C245E485B576625E7EC6" \
                        "F44C42E9A637ED6B0BFF5CB6F406B7ED" \
                        "EE386BFB5A899FA5AE9F24117C4B1FE6" \
                        "49286651ECE45B3DC2007CB8A163BF05" \
                        "98DA48361C55D39A69163FA8FD24CF5F" \
                        "83655D23DCA3AD961C62F356208552BB" \
                        "9ED529077096966D670C354E4ABC9804" \
                        "F1746C08CA18217C32905E462E36CE3B" \
                        "E39E772C180E86039B2783A2EC07A28F" \
                        "B5C55DF06F4C52C9DE2BCBF695581718" \
                        "3995497CEA956AE515D2261898FA0510" \
                        "15728E5A8AAAC42DAD33170D04507A33" \
                        "A85521ABDF1CBA64ECFB850458DBEF0A" \
                        "8AEA71575D060C7DB3970F85A6E1E4C7" \
                        "ABF5AE8CDB0933D71E8C94E04A25619D" \
                        "CEE3D2261AD2EE6BF12FFA06D98A0864" \
                        "D87602733EC86A64521F2B18177B200C" \
                        "BBE117577A615D6C770988C0BAD946E2" \
                        "08E24FA074E5AB3143DB5BFCE0FD108E" \
                        "4B82D120A93AD2CAFFFFFFFFFFFFFFFF"
/* 2048bit素数 */
#define SPEKE_PRIME     "FFFFFFFFFFFFFFFFC90FDAA22168C234" \
                        "C4C6628B80DC1CD129024E088A67CC74" \
                        "020BBEA63B139B22514A08798E3404DD" \
                        "EF9519B3CD3A431B302B0A6DF25F1437" \
                        "4FE1356D6D51C245E485B576625E7EC6" \
                        "F44C42E9A637ED6B0BFF5CB6F406B7ED" \
                        "EE386BFB5A899FA5AE9F24117C4B1FE6" \
                        "49286651ECE45B3DC2007CB8A163BF05" \
                        "98DA48361C55D39A69163FA8FD24CF5F" \
                        "83655D23DCA3AD961C62F356208552BB" \
                        "9ED529077096966D670C354E4ABC9804" \
                        "F1746C08CA18217C32905E462E36CE3B" \
                        "E39E772C180E86039B2783A2EC07A28F" \
                        "B5C55DF06F4C52C9DE2BCBF695581718" \
                        "3995497CEA956AE515D2261898FA0510" \
                        "15728E5A8AACAA68FFFFFFFFFFFFFFFF"

/* base info, pin 派生时使用 */
static const char *SPEKE_BASE_INFO = "hichain_speke_base_info";
/* session info, 共享密钥派生时使用 */
static const char *SPEKE_SESSION_KEY_INFO = "hichain_speke_sessionkey_info";
/* data info, 业务会话密钥派生时使用 */
static const char *SPEKE_DATA_KEY_INFO = "hichain_return_key";

static AdapterMpi *InitMpiByString(uint8_t radix, const char *s)
{
    AdapterMpi *mpi = AdapterMpiInit();
    if (mpi == NULL) {
        return NULL;
    }

    int32_t ret = AdapterMpiReadString(mpi, radix, s);
    if (ret != IOTC_OK) {
        AdapterMpiFree(mpi);
        return NULL;
    }

    return mpi;
}

static int32_t InitNegoCtxPrime(PrimeType primeType, AdapterMpi **outPrimeMpi)
{
    const char *prime = (primeType == PRIME_NORMAL ? SPEKE_PRIME : SPEKE_BIG_PRIME);

    AdapterMpi *primeMpi = InitMpiByString(NUMERIC_BASE, prime);
    if (primeMpi == NULL) {
        IOTC_LOGE("NegoCtx prime mpi init err");
        return IOTC_ADAPTER_CRYPTO_ERR_MPI_INIT;
    }

    *outPrimeMpi = primeMpi;
    return IOTC_OK;
}

static char *GenerateSpekeNegoRandom(PrimeType primeType)
{
    if ((primeType != PRIME_NORMAL) && (primeType != PRIME_BIG)) {
        return NULL;
    }
    uint32_t randomLen = (primeType == PRIME_NORMAL ? RANDOM_LEN_NORMAL : RANDOM_LEN_BIG);
    uint8_t *random = (uint8_t *)AdapterMalloc(randomLen);
    if (random == NULL) {
        IOTC_LOGE("NegoCtx random malloc err");
        return NULL;
    }
    (void)memset_s(random, randomLen, 0, randomLen);
    int32_t ret = SecurityRandom(random, randomLen);
    if (ret != IOTC_OK) {
        IOTC_LOGE("NegoCtx random gen err:%d", ret);
        AdapterFree(random);
        return NULL;
    }

    uint32_t randomStrLen = HEXIFY_LEN(randomLen) + 1;
    char *randomStr = (char *)AdapterMalloc(randomStrLen);
    if (randomStr == NULL) {
        IOTC_LOGE("NegoCtx random str malloc err");
        AdapterFree(random);
        return NULL;
    }
    (void)memset_s(randomStr, randomStrLen, 0, randomStrLen);
    if (!UtilsHexify(random, randomLen, randomStr, randomStrLen)) {
        IOTC_LOGE("NegoCtx random hexify err");
        AdapterFree(random);
        AdapterFree(randomStr);
        return NULL;
    }

    AdapterFree(random);
    return randomStr;
}

static int32_t InitNegoCtxRandom(PrimeType primeType, AdapterMpi **outRandomMpi)
{
    char *randomStr = GenerateSpekeNegoRandom(primeType);
    if (randomStr == NULL) {
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_RANDOM_GEN;
    }

    AdapterMpi *randomMpi = InitMpiByString(NUMERIC_BASE, randomStr);
    if (randomMpi == NULL) {
        IOTC_LOGE("NegoCtx random mpi init err");
        AdapterFree(randomStr);
        return IOTC_ADAPTER_CRYPTO_ERR_MPI_INIT;
    }

    AdapterFree(randomStr);
    *outRandomMpi = randomMpi;
    return IOTC_OK;
}

static int32_t NegoGenHkdfSecret(const uint8_t *salt, uint32_t saltLen,
    const uint8_t *pinCode, uint32_t pinCodeLen, char sha256Hex[HKDF_KEY_HEX_BUF_LEN])
{
    uint8_t sha256Dec[ADAPTER_MD_SHA256_BYTE_LEN] = { 0 };
    AdapterHkdfParam param = {
        .md             = ADAPTER_MD_SHA256,
        .salt           = salt,
        .saltLen        = saltLen,
        .info           = (uint8_t *)SPEKE_BASE_INFO,
        .infoLen        = strlen(SPEKE_BASE_INFO),
        .material       = pinCode,
        .materialLen    = pinCodeLen,
    };
    int32_t ret = AdapterHkdf(&param, sha256Dec, ADAPTER_MD_SHA256_BYTE_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGE("NegoCtx init pubKey cal secret err:%d", ret);
        return ret;
    }

    if (!UtilsHexify(sha256Dec, ADAPTER_MD_SHA256_BYTE_LEN, sha256Hex, HKDF_KEY_HEX_BUF_LEN)) {
        IOTC_LOGE("NegoCtx init pubKey hexify secret err");
        (void)memset_s(sha256Dec, ADAPTER_MD_SHA256_BYTE_LEN, 0, ADAPTER_MD_SHA256_BYTE_LEN);
        return IOTC_CORE_COMM_UTILS_ERR_HEXIFY;
    }
    (void)memset_s(sha256Dec, ADAPTER_MD_SHA256_BYTE_LEN, 0, ADAPTER_MD_SHA256_BYTE_LEN);

    return IOTC_OK;
}

static int32_t CalNegoGenerator(const NegoContext *context,
    const uint8_t *pinCode, uint32_t pinCodeLen, AdapterMpi **outGenerator)
{
    AdapterMpi *exp = InitMpiByString(NUMERIC_BASE, SPEKE_EXP);
    if (exp == NULL) {
        IOTC_LOGE("NegoCtx init generator exp mpi init err");
        return IOTC_ADAPTER_CRYPTO_ERR_MPI_INIT;
    }

    char sha256Hex[HKDF_KEY_HEX_BUF_LEN] = { 0 };
    int32_t ret = NegoGenHkdfSecret(context->salt, context->saltLen, pinCode, pinCodeLen, sha256Hex);
    if (ret != IOTC_OK) {
        AdapterMpiFree(exp);
        return ret;
    }
    AdapterMpi *secret = InitMpiByString(NUMERIC_BASE, sha256Hex);
    (void)memset_s(sha256Hex, sizeof(sha256Hex), 0, sizeof(sha256Hex));
    if (secret == NULL) {
        IOTC_LOGE("NegoCtx init generator secret mpi init err");
        AdapterMpiFree(exp);
        return IOTC_ADAPTER_CRYPTO_ERR_MPI_INIT;
    }

    AdapterMpi *generator = AdapterMpiInit();
    if (generator == NULL) {
        IOTC_LOGE("NegoCtx init generator mpi init err");
        AdapterMpiFree(exp);
        AdapterMpiFree(secret);
        return IOTC_ADAPTER_CRYPTO_ERR_MPI_INIT;
    }
    ret = AdapterMpiExpMod(generator, secret, exp, context->prime);
    if (ret != IOTC_OK) {
        IOTC_LOGE("NegoCtx init generator cal err:%d", ret);
        AdapterMpiFree(exp);
        AdapterMpiFree(secret);
        AdapterMpiFree(generator);
        return ret;
    }

    AdapterMpiFree(exp);
    AdapterMpiFree(secret);
    *outGenerator = generator;
    return IOTC_OK;
}

static int32_t InitNegoCtxPubKey(NegoContext *context, const uint8_t *pinCode, uint32_t pinCodeLen)
{
    AdapterMpi *generator = NULL;
    int32_t ret = CalNegoGenerator(context, pinCode, pinCodeLen, &generator);
    if (ret != IOTC_OK) {
        return ret;
    }

    AdapterMpi *pubKeyMpi = AdapterMpiInit();
    if (pubKeyMpi == NULL) {
        IOTC_LOGE("NegoCtx pubKey mpi init err");
        AdapterMpiFree(generator);
        return IOTC_ADAPTER_CRYPTO_ERR_MPI_INIT;
    }
    ret = AdapterMpiExpMod(pubKeyMpi, generator, context->random, context->prime);
    if (ret != IOTC_OK) {
        IOTC_LOGE("NegoCtx pubKey cal err:%d", ret);
        AdapterMpiFree(generator);
        AdapterMpiFree(pubKeyMpi);
        return ret;
    }
    AdapterMpiFree(generator);

    uint8_t *pubKey = (uint8_t *)AdapterMalloc(PRIME_KEY_LEN);
    if (pubKey == NULL) {
        IOTC_LOGE("NegoCtx pubKey malloc err");
        AdapterMpiFree(pubKeyMpi);
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(pubKey, PRIME_KEY_LEN, 0, PRIME_KEY_LEN);
    ret = AdapterMpiWriteBinary(pubKeyMpi, pubKey, PRIME_KEY_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGE("NegoCtx pubKey write err:%d", ret);
        AdapterMpiFree(pubKeyMpi);
        AdapterFree(pubKey);
        return ret;
    }

    AdapterMpiFree(pubKeyMpi);
    context->pubKey = pubKey;
    context->pubKeyLen = PRIME_KEY_LEN;
    return IOTC_OK;
}

static bool CheckInitParamValid(const uint8_t *pinCode, uint32_t pinCodeLen,
    const uint8_t *salt, uint32_t saltLen, PrimeType primeType)
{
    if ((pinCode == NULL) || (pinCodeLen == 0)) {
        return false;
    }
    if ((salt == NULL) || (saltLen > MAX_SALT_LEN) || (saltLen < MIN_SALT_LEN)) {
        return false;
    }
    if ((primeType != PRIME_NORMAL) && (primeType != PRIME_BIG)) {
        return false;
    }
    return true;
}

NegoContext *NegoContextInit(const uint8_t *pinCode, uint32_t pinCodeLen,
    const uint8_t *salt, uint32_t saltLen, PrimeType primeType)
{
    if (!CheckInitParamValid(pinCode, pinCodeLen, salt, saltLen, primeType)) {
        return NULL;
    }

    NegoContext *negoContext = (NegoContext *)AdapterMalloc(sizeof(NegoContext));
    if (negoContext == NULL) {
        IOTC_LOGE("NegoCtx malloc err");
        return NULL;
    }
    (void)memset_s(negoContext, sizeof(NegoContext), 0, sizeof(NegoContext));

    /* 生成本端挑战值 challenge */
    int32_t ret = SecurityRandom(negoContext->localChallenge, CHALLENGE_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGE("NegoCtx gen challenge err:%d", ret);
        NegoContextFree(negoContext);
        return NULL;
    }

    /* 拷贝盐值 salt */
    ret = memcpy_s(negoContext->salt, MAX_SALT_LEN, salt, saltLen);
    if (ret != EOK) {
        NegoContextFree(negoContext);
        return NULL;
    }
    negoContext->saltLen = saltLen;

    /* 使用固定质数 */
    ret = InitNegoCtxPrime(primeType, &negoContext->prime);
    if (ret != IOTC_OK) {
        NegoContextFree(negoContext);
        return NULL;
    }

    /* 生成本端私钥 sk */
    ret = InitNegoCtxRandom(primeType, &negoContext->random);
    if (ret != IOTC_OK) {
        NegoContextFree(negoContext);
        return NULL;
    }

    /* 生成本端公钥 pk */
    ret = InitNegoCtxPubKey(negoContext, pinCode, pinCodeLen);
    if (ret != IOTC_OK) {
        NegoContextFree(negoContext);
        return NULL;
    }

    return negoContext;
}

void NegoContextFree(NegoContext *context)
{
    if (context == NULL) {
        IOTC_LOGW("double free negoCtx");
        return;
    }

    if (context->pubKey != NULL) {
        AdapterFree(context->pubKey);
        context->pubKey = NULL;
    }
    if (context->random != NULL) {
        AdapterMpiFree(context->random);
        context->random = NULL;
    }
    if (context->prime != NULL) {
        AdapterMpiFree(context->prime);
        context->prime = NULL;
    }

    (void)memset_s(context, sizeof(NegoContext), 0, sizeof(NegoContext));
    AdapterFree(context);
}

int32_t NegoContextSetRemoteChallenge(NegoContext *context, const uint8_t *challenge, uint32_t challengeLen)
{
    if ((context == NULL) || (challenge == NULL) || (challengeLen == 0) || (challengeLen > CHALLENGE_LEN)) {
        return IOTC_ERR_PARAM_INVALID;
    }

    int32_t ret = memcpy_s(context->remoteChallenge, CHALLENGE_LEN, challenge, challengeLen);
    if (ret != EOK) {
        return IOTC_ERR_SECUREC_MEMCPY;
    }

    return IOTC_OK;
}

static int32_t VerifyRemotePubKey(AdapterMpi *prime, AdapterMpi *pubKey)
{
    int32_t ret = AdapterMpiCmpInt(pubKey, MIN_PUB_KEY);
    if (ret < 0) {
        IOTC_LOGE("remote pubKey too small:%d", ret);
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_PUBKEY;
    }

    AdapterMpi *result = AdapterMpiInit();
    if (result == NULL) {
        IOTC_LOGE("verify remote pubKey mpi init err");
        return IOTC_ADAPTER_CRYPTO_ERR_MPI_INIT;
    }
    ret = AdapterMpiSubInt(result, prime, MIN_PUB_KEY);
    if (ret != IOTC_OK) {
        IOTC_LOGE("remote pubKey subtract err:%d", ret);
        AdapterMpiFree(result);
        return ret;
    }
    ret = AdapterMpiCmpMpi(pubKey, result);
    if (ret > 0) {
        IOTC_LOGE("remote pubKey too large:%d", ret);
        AdapterMpiFree(result);
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_PUBKEY;
    }

    AdapterMpiFree(result);
    return IOTC_OK;
}

static int32_t CalSharedKey(const NegoContext *context, const uint8_t *pubKey, uint32_t pubKeyLen,
    uint8_t **sharedKey, uint32_t *sharedKeyLen)
{
    AdapterMpi *remotePubKey = AdapterMpiInit();
    if (remotePubKey == NULL) {
        IOTC_LOGE("NegoCtx remote pubKey mpi init err");
        return IOTC_ADAPTER_CRYPTO_ERR_MPI_INIT;
    }
    int32_t ret = AdapterMpiReadBinary(remotePubKey, pubKey, pubKeyLen);
    if (ret != IOTC_OK) {
        IOTC_LOGE("NegoCtx remote pubKey mpi read err:%d", ret);
        AdapterMpiFree(remotePubKey);
        return ret;
    }
    ret = VerifyRemotePubKey(context->prime, remotePubKey);
    if (ret != IOTC_OK) {
        AdapterMpiFree(remotePubKey);
        return ret;
    }

    AdapterMpi *sharedKeyMpi = AdapterMpiInit();
    if (sharedKeyMpi == NULL) {
        IOTC_LOGE("NegoCtx cal sharedKey mpi init err");
        AdapterMpiFree(remotePubKey);
        return IOTC_ADAPTER_CRYPTO_ERR_MPI_INIT;
    }
    /* 共享密钥计算: sharedKey = remotePubKey ^ random mod prime */
    ret = AdapterMpiExpMod(sharedKeyMpi, remotePubKey, context->random, context->prime);
    if (ret != IOTC_OK) {
        IOTC_LOGE("NegoCtx sharedKey cal err:%d", ret);
        AdapterMpiFree(remotePubKey);
        AdapterMpiFree(sharedKeyMpi);
        return ret;
    }
    AdapterMpiFree(remotePubKey);

    uint32_t keyLen = PRIME_KEY_LEN;
    uint8_t *key = (uint8_t *)AdapterCalloc(keyLen, sizeof(uint8_t));
    if (key == NULL) {
        IOTC_LOGE("NegoCtx sharedKey calloc err");
        AdapterMpiFree(sharedKeyMpi);
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    ret = AdapterMpiWriteBinary(sharedKeyMpi, key, keyLen);
    AdapterMpiFree(sharedKeyMpi);
    if (ret != IOTC_OK) {
        IOTC_LOGE("NegoCtx sharedKey write err:%d", ret);
        (void)memset_s(key, keyLen, 0, keyLen);
        AdapterFree(key);
        return ret;
    }

    *sharedKey = key;
    *sharedKeyLen = keyLen;
    return IOTC_OK;
}

static int32_t GenSessionKey(NegoContext *context, uint8_t *sharedKey, uint32_t sharedKeyLen)
{
    AdapterHkdfParam param = {
        .md             = ADAPTER_MD_SHA256,
        .salt           = context->salt,
        .saltLen        = context->saltLen,
        .info           = (uint8_t *)SPEKE_SESSION_KEY_INFO,
        .infoLen        = strlen(SPEKE_SESSION_KEY_INFO),
        .material       = sharedKey,
        .materialLen    = sharedKeyLen,
    };
    uint8_t sessionKey[SESSION_KEY_LEN + SESSION_KEY_LEN] = { 0 };
    uint32_t keyLen = SESSION_KEY_LEN + SESSION_KEY_LEN;
    int32_t ret = AdapterHkdf(&param, sessionKey, keyLen);
    if (ret != IOTC_OK) {
        IOTC_LOGE("NegoCtx gen session key err:%d", ret);
        return ret;
    }

    ret = memcpy_s(context->identityEncKey, SESSION_KEY_LEN, sessionKey, SESSION_KEY_LEN);
    if (ret != EOK) {
        (void)memset_s(sessionKey, keyLen, 0, keyLen);
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    ret = memcpy_s(context->hmacKey, SESSION_KEY_LEN, sessionKey + SESSION_KEY_LEN, SESSION_KEY_LEN);
    (void)memset_s(sessionKey, keyLen, 0, keyLen);
    if (ret != EOK) {
        (void)memset_s(context->identityEncKey, SESSION_KEY_LEN, 0, SESSION_KEY_LEN);
        return IOTC_ERR_SECUREC_MEMCPY;
    }

    return IOTC_OK;
}

int32_t NegoContextGenSessionKey(NegoContext *context, const uint8_t *pubKey, uint32_t pubKeyLen)
{
    if ((context == NULL) || (pubKey == NULL) || (pubKeyLen == 0)) {
        return IOTC_ERR_PARAM_INVALID;
    }

    uint8_t *sharedKey = NULL;
    uint32_t sharedKeyLen = 0;
    int32_t ret = CalSharedKey(context, pubKey, pubKeyLen, &sharedKey, &sharedKeyLen);
    if ((ret != IOTC_OK) || (sharedKey == NULL) || (sharedKeyLen == 0)) {
        return ret;
    }

    ret = GenSessionKey(context, sharedKey, sharedKeyLen);
    (void)memset_s(sharedKey, sharedKeyLen, 0, sharedKeyLen);
    AdapterFree(sharedKey);
    return ret;
}

static int32_t GenNegoHmac(const uint8_t *hmacKey, const uint8_t challenge1[CHALLENGE_LEN],
    const uint8_t challenge2[CHALLENGE_LEN], uint8_t *output, uint32_t outputLen)
{
    uint8_t challenge[CHALLENGE_LEN + CHALLENGE_LEN] = { 0 };
    int32_t ret = memcpy_s(challenge, CHALLENGE_LEN, challenge1, CHALLENGE_LEN);
    if (ret != EOK) {
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    ret = memcpy_s(challenge + CHALLENGE_LEN, CHALLENGE_LEN, challenge2, CHALLENGE_LEN);
    if (ret != EOK) {
        return IOTC_ERR_SECUREC_MEMCPY;
    }

    AdapterHmacParam param = {
        .md         = ADAPTER_MD_SHA256,
        .key        = hmacKey,
        .keyLen     = SESSION_KEY_LEN,
        .data       = challenge,
        .dataLen    = sizeof(challenge),
    };
    ret = AdapterHmacCalc(&param, output, outputLen);
    if (ret != IOTC_OK) {
        IOTC_LOGE("NegoCtx gen hmac err:%d", ret);
    }

    return ret;
}

int32_t NegoContextGenHmac(const NegoContext *context, uint8_t *output, uint32_t outputLen)
{
    if ((context == NULL) || (output == NULL) || (outputLen < HMAC_LEN)) {
        return IOTC_ERR_PARAM_INVALID;
    }

    return GenNegoHmac(context->hmacKey, context->localChallenge, context->remoteChallenge, output, outputLen);
}

int32_t NegoContextVerifyHmac(const NegoContext *context, const uint8_t *hmac, uint32_t hmacLen)
{
    if ((context == NULL) || (hmac == NULL) || (hmacLen != HMAC_LEN)) {
        return IOTC_ERR_PARAM_INVALID;
    }

    uint8_t calHmac[HMAC_LEN] = { 0 };
    int32_t ret = GenNegoHmac(context->hmacKey, context->remoteChallenge, context->localChallenge, calHmac, HMAC_LEN);
    if (ret != IOTC_OK) {
        return ret;
    }
    ret = memcmp(hmac, calHmac, hmacLen);
    if (ret != 0) {
        IOTC_LOGE("NegoCtx verify hmac err:%d", ret);
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_VERIFY_HMAC;
    }

    return IOTC_OK;
}

int32_t NegoContextGenDataEncKey(const NegoContext *context, uint8_t *output, uint32_t outputLen)
{
    if ((context == NULL) || (output == NULL) || (outputLen < SESSION_KEY_LEN)) {
        return IOTC_ERR_PARAM_INVALID;
    }

    AdapterHkdfParam param = {
        .md             = ADAPTER_MD_SHA256,
        .salt           = context->salt,
        .saltLen        = context->saltLen,
        .info           = (uint8_t *)SPEKE_DATA_KEY_INFO,
        .infoLen        = strlen(SPEKE_DATA_KEY_INFO),
        .material       = context->identityEncKey,
        .materialLen    = SESSION_KEY_LEN,
    };
    int32_t ret = AdapterHkdf(&param, output, SESSION_KEY_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGE("NegoCtx gen data key err:%d", ret);
        return ret;
    }

    return IOTC_OK;
}