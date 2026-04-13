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
#include "sle_profile.h"
#include "securec.h"
#include "utils_common.h"
#include "utils_assert.h"
#include "utils_json.h"
#include "security_random.h"
#include "iotc_svc_dev.h"
#include "service_proxy.h"
#include "iotc_svc.h"
#include "iotc_errcode.h"
#include "sle_conn_device_info.h"
#include <string.h>
#include "comm_def.h"
#include "iotc_mem.h"
#include "sle_print_data.h"
#include "sle_session_mngr.h"
#include "iotc_event.h"

#define SLE_MESSAGES_VAL 1
#define SLE_TYPE_VAL 1
#define SLE_MODE_SUPPORT_VAL 3
#define HEX_CHAR_PER_BYTE 2
#define SLE_INIT_SEQ_NUM 1

static ListEntry g_sleInitialSaltSn1 = LIST_DECLARE_INIT(&g_sleInitialSaltSn1);
#define HEX_CHARS_PER_BYTE 2
static SleInitialSaltSn1Part *SleSessGetSaltSn1ByConnId(uint16_t connId)
{
    ListEntry *item;
    LIST_FOR_EACH_ITEM(item, &g_sleInitialSaltSn1)
    {
        SleInitialSaltSn1Part *saltSn1 = CONTAINER_OF(item, SleInitialSaltSn1Part, node);
        if (connId == saltSn1->connId) {
            return saltSn1;
        }
    }
    return NULL;
}

typedef struct DeviceStatusReport {
    char deviceId[DEVICE_ID_MAX_STR_LEN + 1];
    uint8_t online;
    char *lastChange;
} DeviceStatusReport;

static bool SleIsSessionInitSaltSn1Exist(uint16_t connId)
{
    SleInitialSaltSn1Part *saltSn1 = SleSessGetSaltSn1ByConnId(connId);
    if (saltSn1 == NULL) {
        return false;
    }
    return true;
}

static int32_t SleDelSessInitSaltSn1(uint16_t connId)
{
    SleInitialSaltSn1Part *saltSn1 = SleSessGetSaltSn1ByConnId(connId);
    if (saltSn1 == NULL) {
        return IOTC_OK;
    }
    LIST_REMOVE(&(saltSn1->node));
    if (saltSn1->password != NULL) {
        IotcFree((char *)(saltSn1->password));
        saltSn1->password = NULL;
    }
    IotcFree(saltSn1);
    return IOTC_OK;
}

static int32_t SleCreateSessInitSaltSn1(uint16_t connId, const uint8_t *password, const uint8_t *sn1, uint32_t sn1Len)
{
    SleInitialSaltSn1Part *saltSn1 = (SleInitialSaltSn1Part *)IotcMalloc(sizeof(SleInitialSaltSn1Part));
    if (saltSn1 == NULL) {
        IOTC_LOGW("malloc failed");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }
    (void)memset_s(saltSn1, sizeof(SleInitialSaltSn1Part), 0, sizeof(SleInitialSaltSn1Part));

    saltSn1->connId = connId;

    if (sn1Len > sizeof(saltSn1->sn1)) {
        IOTC_LOGE("sn1 length exceeds buffer size");
        IotcFree(saltSn1);
        return IOTC_ERR_INVALID_PARAM;
    }
    if (memcpy_s(saltSn1->sn1, sizeof(saltSn1->sn1), sn1, sn1Len) != EOK) {
        IOTC_LOGE("memcpy_s failed");
        IotcFree(saltSn1);
        return IOTC_ERR_INVALID_PARAM;
    }

    if (password == NULL) {
        IOTC_LOGE("password is NULL");
        IotcFree(saltSn1);
        return IOTC_ERR_INVALID_PARAM;
    }

    uint8_t *pwdCopy = (uint8_t *)IotcMalloc(BLE_AUTHCODE_LEN);
    if (pwdCopy == NULL) {
        IOTC_LOGE("malloc for password failed");
        IotcFree(saltSn1);
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }

    if (memcpy_s(pwdCopy, BLE_AUTHCODE_LEN, password, BLE_AUTHCODE_LEN) != EOK) {
        IOTC_LOGE("memcpy_s for password failed");
        IotcFree(pwdCopy);
        IotcFree(saltSn1);
        return IOTC_ERR_INVALID_PARAM;
    }

    saltSn1->password = pwdCopy;

    LIST_INSERT(&saltSn1->node, &g_sleInitialSaltSn1);

    return IOTC_OK;
}

static int32_t AddSessionJsonFields(IotcJson *root, const uint8_t *sn1,
    const IotcConDeviceInfo *connInfo, uint8_t **out, uint32_t *outLen)
{
    int32_t ret = IotcJsonAddNum2Obj(root, "messages", SLE_MESSAGES_VAL);
    if (ret != IOTC_OK) {
        IOTC_LOGE("add svcType err ret=%d", ret);
        return ret;
    }
    ret = IotcJsonAddNum2Obj(root, "type", SLE_TYPE_VAL);
    if (ret != IOTC_OK) {
        IOTC_LOGE("add type err ret=%d", ret);
        return ret;
    }
    ret = IotcJsonAddNum2Obj(root, STR_JSON_MODE_SUPPORT, SLE_MODE_SUPPORT_VAL);
    if (ret != IOTC_OK) {
        IOTC_LOGE("add modeSupport err ret=%d", ret);
        return ret;
    }
    char outBuf[RAND_SN_LEN * HEX_CHAR_PER_BYTE + 1] = { 0 };
    if (!UtilsHexify(sn1, RAND_SN_LEN, outBuf, RAND_SN_LEN * HEX_CHAR_PER_BYTE)) {
        IOTC_LOGE("hexify err");
        return IOTC_CORE_COMM_UTILS_ERR_HEXIFY;
    }
    ret = IotcJsonAddStr2Obj(root, STR_JSON_SN1, outBuf);
    if (ret != IOTC_OK) {
        IOTC_LOGE("add sn1 err ret=%d", ret);
        return ret;
    }
    ret = IotcJsonAddStr2Obj(root, STR_JSON_UIDHASH, connInfo->uidHash);
    if (ret != IOTC_OK) {
        IOTC_LOGE("add uidHash err ret=%d", ret);
        return ret;
    }
    ret = IotcJsonAddStr2Obj(root, STR_JSON_UUID, "000000000");
    if (ret != IOTC_OK) {
        IOTC_LOGE("add uuid err ret=%d", ret);
        return ret;
    }
    ret = IotcJsonAddNum2Obj(root, STR_JSON_SEQ_NUM, SLE_INIT_SEQ_NUM);
    if (ret != IOTC_OK) {
        IOTC_LOGE("add seq err ret=%d", ret);
        return ret;
    }
    char *outStr = UtilsJsonPrintByMalloc(root);
    if (outStr == NULL) {
        IOTC_LOGE("json print err");
        return IOTC_CORE_COMM_UTILS_ERR_JSON_MALLOC_PRINT;
    }
    *out = (uint8_t *)outStr;
    *outLen = strlen(outStr);
    return IOTC_OK;
}

static int32_t BuildSessionIssueJson(const uint8_t *sn1, const IotcConDeviceInfo *connInfo,
    uint8_t **out, uint32_t *outLen)
{
    IotcJson *root = IotcJsonCreate();
    if (root == NULL) {
        IOTC_LOGE("create err");
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }
    int32_t ret = AddSessionJsonFields(root, sn1, connInfo, out, outLen);
    IotcJsonDelete(root);
    return ret;
}

int32_t CreateSvcSessionIssue(uint16_t connId, uint8_t **out, uint32_t *outLen)
{
    CHECK_RETURN_LOGW((out != NULL) && (outLen != NULL), IOTC_ERR_PARAM_INVALID, "invalid param");
    *out = NULL;
    *outLen = 0;

    uint8_t sn1[RAND_SN_LEN] = {0};
    int32_t ret = SecurityRandom(sn1, sizeof(sn1));
    if (ret != IOTC_OK) {
        IOTC_LOGE("random err ret=%d", ret);
        return IOTC_ERROR;
    }

    IotcConDeviceInfo *connInfo = SleGetConnectionInfoByConnId(connId);
    if (connInfo == NULL) {
        IOTC_LOGE("connInfo is NULL");
        return IOTC_ERR_INVALID_PARAM;
    }

    ret = BuildSessionIssueJson(sn1, connInfo, out, outLen);

    if (SleIsSessionInitSaltSn1Exist(connId)) {
        if (SleDelSessInitSaltSn1(connId) != IOTC_OK) {
            IOTC_LOGE("delSessInitSaltSn1 err");
            return IOTC_ERROR;
        }
    }

    if (SleCreateSessInitSaltSn1(connId, (const uint8_t *)connInfo->authCode, sn1, sizeof(sn1)) != IOTC_OK) {
        IOTC_LOGE("createSessInitSaltSn1 err");
        return IOTC_ERROR;
    }

    return ret;
}

static int32_t GetSn2Hex(const IotcJson *req, uint8_t *out, uint32_t outLen)
{
    char sn1Str[HEXIFY_LEN(RAND_SN_LEN) + 1] = {0};
    int32_t ret = UtilsJsonGetString(req, STR_JSON_SN2, sn1Str, sizeof(sn1Str));
    CHECK_RETURN_LOGE(ret == IOTC_OK, IOTC_ADAPTER_JSON_ERR_GET_NUM, "get sn2 str err");

    CHECK_RETURN_LOGE(
        UtilsUnhexify(sn1Str, strlen(sn1Str), out, outLen), IOTC_CORE_COMM_UTILS_ERR_UNHEXIFY, "get sn2 hex err");
    return IOTC_OK;
}

static int32_t GetSessHex(const IotcJson *req, uint8_t *out, uint32_t outLen)
{
    char sessStr[HEXIFY_LEN(SESSION_ID_LEN) + 1] = {0};
    int32_t ret = UtilsJsonGetString(req, STR_JSON_SESS_ID, sessStr, sizeof(sessStr));
    CHECK_RETURN_LOGE(ret == IOTC_OK, IOTC_ADAPTER_JSON_ERR_GET_NUM, "get sess str err");

    CHECK_RETURN_LOGE(
        UtilsUnhexify(sessStr, strlen(sessStr), out, outLen), IOTC_CORE_COMM_UTILS_ERR_UNHEXIFY, "get sess hex err");
    return IOTC_OK;
}

static int32_t GenerateSessionKey(uint16_t connId, IotcJson *root, SleInitialSaltSn1Part *saltSn1Info)
{
    uint8_t sn2Bin[RAND_SN_LEN] = { 0 };
    int32_t ret = GetSn2Hex(root, sn2Bin, sizeof(sn2Bin));
    if (ret != IOTC_OK) {
        IOTC_LOGE("get sn2 hex failed, ret=%d", ret);
        return IOTC_ERROR;
    }

    uint8_t sessId[SESSION_ID_LEN] = { 0 };
    if (GetSessHex(root, sessId, sizeof(sessId)) != IOTC_OK) {
        IOTC_LOGE("get sessId err");
        return IOTC_ERROR;
    }

    SleSessionKeyGenParam genParam = {
        .password = saltSn1Info->password,
        .passwordLen = BLE_AUTHCODE_LEN,
        .sn1 = saltSn1Info->sn1,
        .sn1Len = sizeof(saltSn1Info->sn1),
        .sn2 = sn2Bin,
        .sn2Len = sizeof(sn2Bin),
        .sessId = sessId,
        .sessIdLen = sizeof(sessId),
    };

    if (SleSessionKeyGenerate(connId, &genParam) != IOTC_OK) {
        IOTC_LOGE("session key generate err");
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

int32_t GetSleSvcCreateSession(const SleCmdParam *param, uint8_t * const *out, const uint32_t *outLen)
{
    (void)out;
    (void)outLen;
    CHECK_RETURN_LOGW(param != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");

    IotcJson *root = IotcJsonParse((const char *)param->request);
    if (root == NULL) {
        IOTC_LOGE("parse err");
        return IOTC_ERROR;
    }

    int64_t req = 0;
    if (SleJsonGetNum(root, STR_JSON_SEQ_NUM, &req) != IOTC_OK) {
        IOTC_LOGE("get %s err", STR_JSON_SEQ_NUM);
        goto ERROR_EXIT;
    }

    SleSessRecvSeqInit(param->connId, req);

    if (!SleSessionExistsByConnId(param->connId)) {
        IOTC_LOGE("session not exist connId = [%u]", param->connId);
        goto ERROR_EXIT;
    }

    SleInitialSaltSn1Part *saltSn1Info = SleSessGetSaltSn1ByConnId(param->connId);
    if (saltSn1Info == NULL) {
        IOTC_LOGE("saltSn1Info is NULL");
        goto ERROR_EXIT;
    }

    int32_t ret = GenerateSessionKey(param->connId, root, saltSn1Info);
    if (ret != IOTC_OK) {
        IOTC_LOGE("session key generate err");
        goto ERROR_EXIT;
    }

    IOTC_LOGI("sle session create success connId = [%u]", param->connId);
    (void)CreateSvcSessionSuccess(param->connId);

    IotcJsonDelete(root);
    return IOTC_OK;

ERROR_EXIT:
    IotcJsonDelete(root);
    return IOTC_ERROR;
}
