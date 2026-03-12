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
#include "sle_svc_create_session.h"
#include "sle_session_mngr.h"
#include "securec.h"
#include "utils_common.h"
#include "utils_assert.h"
#include "utils_json.h"
#include "security_random.h"
#include "iotc_svc_dev.h"
#include "service_proxy.h"
#include "iotc_svc.h"
#include "iotc_errcode.h"

static int32_t InitRecvSeq(const IotcJson *req, uint32_t *recvSeq)
{
    IotcJson *seqObj = IotcJsonGetObj(req, STR_JSON_SEQ_NUM);
    CHECK_RETURN_LOGE(seqObj != NULL, IOTC_ADAPTER_JSON_ERR_GET_OBJ, "get seq json err");
    int64_t seq = 0;
    int32_t ret = IotcJsonGetNum(seqObj, &seq);
    CHECK_RETURN_LOGE(ret == IOTC_OK, IOTC_ADAPTER_JSON_ERR_GET_NUM, "get seq num err");
    CHECK_RETURN_LOGE(seq >= 0 && seq <= UINT32_MAX, IOTC_CORE_SLE_INVALID_SEQ, "get seq num:%ld overflow", seq);

    *recvSeq = (uint32_t)seq;
    SleSessRecvSeqInit(*recvSeq);
    return IOTC_OK;
}

static int32_t CheckUidHash(const IotcJson *req)
{
    char uidHash[SLE_UID_HASH_LEN + 1] = { 0 };
    int32_t ret = UtilsJsonGetString(req, STR_JSON_UIDHASH, uidHash, sizeof(uidHash));
    CHECK_RETURN_LOGE(ret == IOTC_OK, IOTC_ADAPTER_JSON_ERR_GET_NUM, "get uid hash err");

    DevAuthInfo authInfo = {0};
    bool isAuthInfoExist = false;
    ret = DevSvcProxyGetAuthInfo(&isAuthInfoExist, &authInfo);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get auth info error %d", ret);
        return ret;
    }

    bool isMatch = strcmp(uidHash, authInfo.uidHash) == 0;
    (void)memset_s(&authInfo, sizeof(DevAuthInfo), 0, sizeof(DevAuthInfo));

    CHECK_RETURN_LOGE(isMatch,
        IOTC_CORE_SLE_UIDHASH_NOT_MATCH, "uidHash not match");
    return IOTC_OK;
}

static int32_t GetSn1Hex(const IotcJson *req, uint8_t *out, uint32_t outLen)
{
    char sn1Str[HEXIFY_LEN(RAND_SN_LEN) + 1] = { 0 };
    int32_t ret = UtilsJsonGetString(req, STR_JSON_SN1, sn1Str, sizeof(sn1Str));
    CHECK_RETURN_LOGE(ret == IOTC_OK, IOTC_ADAPTER_JSON_ERR_GET_NUM, "get sn1 str err");

    CHECK_RETURN_LOGE(UtilsUnhexify(sn1Str, strlen(sn1Str), out, outLen),
        IOTC_CORE_COMM_UTILS_ERR_UNHEXIFY, "get sn1 hex err");
    return IOTC_OK;
}

static int32_t OwnerAuth(const IotcJson *req, IotcJson *rsp)
{
    uint8_t sn1[RAND_SN_LEN] = { 0 };
    int32_t ret = GetSn1Hex(req, sn1, sizeof(sn1));
    CHECK_RETURN(ret == IOTC_OK, ret);

    uint8_t sn2[RAND_SN_LEN] = { 0 };
    ret = SecurityRandom(sn2, sizeof(sn2));
    CHECK_RETURN(ret == IOTC_OK, ret);

    ret = SleSessKeyGen(sn1, sizeof(sn1), sn2, sizeof(sn2));
    CHECK_RETURN(ret == IOTC_OK, ret);

    ret = SleSessIdGen();
    CHECK_RETURN(ret == IOTC_OK, ret);

    ret = UtilsJsonAddHexify(rsp, STR_JSON_SESS_ID, SleSessIdGet(), SESSION_ID_LEN);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "add sessId err:%d", ret);

    ret = UtilsJsonAddHexify(rsp, STR_JSON_SN2, sn2, sizeof(sn2));
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "add sn2 err:%d", ret);

    DevAuthInfo authInfo = {0};
    bool isAuthInfoExist = false;
    ret = DevSvcProxyGetAuthInfo(&isAuthInfoExist, &authInfo);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get auth info error %d", ret);
        return ret;
    }

    if (!isAuthInfoExist) {
        IOTC_LOGW("authinfo not exists");
        return IOTC_CORE_SLE_INVALID_AUTHCODE_ID;
    }

    ret = IotcJsonAddStr2Obj(rsp, STR_JSON_AUTHCODE_ID, authInfo.authCodeId);
    (void)memset_s(&authInfo, sizeof(DevAuthInfo), 0, sizeof(DevAuthInfo));
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "add authcode id err:%d", ret);
    return IOTC_OK;
}

static int32_t AddSessionItem(const IotcJson *req, IotcJson *rsp)
{
    int32_t ret = IotcJsonAddNum2Obj(rsp, STR_JSON_SEQ_NUM, SleSessSendSeqGet());
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "add seq err:%d", ret);

    IotcJson *uuidJson = IotcJsonGetObj(req, STR_JSON_UUID);
    CHECK_RETURN_LOGE(uuidJson != NULL, IOTC_ADAPTER_JSON_ERR_GET_OBJ, "get uuid obj err");
    const char *uuid = IotcJsonGetStr(uuidJson);
    CHECK_RETURN_LOGE(uuid != NULL, IOTC_ADAPTER_JSON_ERR_GET_STRING, "get uuid str err");
    ret = IotcJsonAddStr2Obj(rsp, STR_JSON_UUID, uuid);
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "add uuid err:%d", ret);

    return IOTC_OK;
}

static int32_t ProcessCreateSessionCmd(const IotcJson *req, IotcJson *rsp)
{
    uint32_t recvSeq = 0;
    int32_t ret = InitRecvSeq(req, &recvSeq);
    CHECK_RETURN(ret == IOTC_OK, ret);

    ret = CheckUidHash(req);
    CHECK_RETURN(ret == IOTC_OK, ret);

    ret = OwnerAuth(req, rsp);
    CHECK_RETURN(ret == IOTC_OK, ret);

    ret = AddSessionItem(req, rsp);
    CHECK_RETURN(ret == IOTC_OK, ret);

    SleSessSendSeqUpdate();
    return IOTC_OK;
}

static int32_t SleCreateSession(const uint8_t *in, uint8_t **out, uint32_t *outLen)
{
    IotcJson *req = IotcJsonParse((const char *)in);
    CHECK_RETURN_LOGE(req != NULL, IOTC_ADAPTER_JSON_ERR_PARSE, "parse json err");

    IotcJson *rsp = IotcJsonCreate();
    if (rsp == NULL) {
        IOTC_LOGE("create");
        IotcJsonDelete(req);
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }

    int32_t ret = ProcessCreateSessionCmd(req, rsp);
    IotcJsonDelete(req);
    if (ret != IOTC_OK) {
        IotcJsonDelete(rsp);
        return ret;
    }

    char *outStr = UtilsJsonPrintByMalloc(rsp);
    IotcJsonDelete(rsp);
    if (outStr == NULL) {
        IOTC_LOGE("json print err");
        return IOTC_CORE_COMM_UTILS_ERR_JSON_MALLOC_PRINT;
    }

    *out = (uint8_t *)outStr;
    *outLen = strlen(outStr);
    return IOTC_OK;
}

int32_t GetSleSvcCreateSession(const BtCmdParam *param, uint8_t **out, uint32_t *outLen)
{
    CHECK_RETURN_LOGW((param != NULL) && (param->request != NULL) && (param->requestLen != 0) &&
        (out != NULL) && (outLen != NULL), IOTC_ERR_PARAM_INVALID, "invalid param");

    *out = NULL;
    *outLen = 0;
    int32_t ret = SleCreateSession(param->request, out, outLen);
    if (ret != IOTC_OK) {
        return UtilsGenErrcodeJsonStr(ret, (char **)out, outLen);
    }
    return IOTC_OK;
}