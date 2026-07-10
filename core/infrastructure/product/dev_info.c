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
#include "dev_info.h"
#include "securec.h"
#include "iotc_log.h"
#include "iotc_errcode.h"
#include "iotc_mem.h"
#include "utils_common.h"
#include "security_key.h"
#include "utils_assert.h"
#include "iotc_md.h"

#define DEFAULT_SUB_PRO_ID "00"
#define DEFAULT_CUSTOM_DATA "00"
#define DEFAULT_UNIQUEID "00"
#define EMPTY_STR ""

static IotcDeviceInfo *g_deviceInfo = NULL;

int32_t ModelDevInfoInit(const IotcDeviceInfo *devInfo)
{
    CHECK_RETURN_LOGW(devInfo != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    g_deviceInfo = (IotcDeviceInfo *)devInfo;
    if (UtilsIsEmptyStr(devInfo->subProdId)) {
        g_deviceInfo->subProdId = DEFAULT_SUB_PRO_ID;
    }
    if (UtilsIsEmptyStr(devInfo->uniqueId)) {
        g_deviceInfo->uniqueId = DEFAULT_UNIQUEID;
    }
    if (UtilsIsEmptyStr(devInfo->customData)) {
        g_deviceInfo->customData = DEFAULT_CUSTOM_DATA;
    }
    return IOTC_OK;
}

void ModelDevInfoDeinit(void)
{
    g_deviceInfo = NULL;
}

const IotcDeviceInfo *ModelGetDevInfo(void)
{
    return g_deviceInfo;
}

const char *ModelGetDevSn(void)
{
    if (g_deviceInfo == NULL) {
        return EMPTY_STR;
    }
    return NON_NULL_EMPTY_STR(g_deviceInfo->sn);
}

const char *ModelGetDevProId(void)
{
    if (g_deviceInfo == NULL) {
        return EMPTY_STR;
    }
    return NON_NULL_EMPTY_STR(g_deviceInfo->prodId);
}

const char *ModelGetDevSubProId(void)
{
    if (g_deviceInfo == NULL) {
        return EMPTY_STR;
    }
    return NON_NULL_EMPTY_STR(g_deviceInfo->subProdId);
}

const char *ModelGetDevModel(void)
{
    if (g_deviceInfo == NULL) {
        return EMPTY_STR;
    }
    return NON_NULL_EMPTY_STR(g_deviceInfo->model);
}

const char *ModelGetDevTypeId(void)
{
    if (g_deviceInfo == NULL) {
        return EMPTY_STR;
    }
    return NON_NULL_EMPTY_STR(g_deviceInfo->devTypeId);
}

const char *ModelGetDevTypeName(void)
{
    if (g_deviceInfo == NULL) {
        return EMPTY_STR;
    }
    return NON_NULL_EMPTY_STR(g_deviceInfo->devTypeName);
}

const char *ModelGetDevManuId(void)
{
    if (g_deviceInfo == NULL) {
        return EMPTY_STR;
    }
    return NON_NULL_EMPTY_STR(g_deviceInfo->manuId);
}

const char *ModelGetDevManuName(void)
{
    if (g_deviceInfo == NULL) {
        return EMPTY_STR;
    }
    return NON_NULL_EMPTY_STR(g_deviceInfo->manuName);
}

const char *ModelGetDevName(void)
{
    if (g_deviceInfo == NULL) {
        return EMPTY_STR;
    }
    return NON_NULL_EMPTY_STR(g_deviceInfo->devName);
}

const char *ModelGetDevFwv(void)
{
    if (g_deviceInfo == NULL) {
        return EMPTY_STR;
    }
    return NON_NULL_EMPTY_STR(g_deviceInfo->fwv);
}

const char *ModelGetDevHwv(void)
{
    if (g_deviceInfo == NULL) {
        return EMPTY_STR;
    }
    return NON_NULL_EMPTY_STR(g_deviceInfo->hwv);
}

const char *ModelGetDevSwv(void)
{
    if (g_deviceInfo == NULL) {
        return EMPTY_STR;
    }
    return NON_NULL_EMPTY_STR(g_deviceInfo->swv);
}

int32_t ModelGetDevProtType(void)
{
    if (g_deviceInfo == NULL) {
        return IOTC_PROT_TYPE_INVALID;
    }
    return g_deviceInfo->protType;
}

const char *ModelGetDevUniqueId(void)
{
    if (g_deviceInfo == NULL) {
        return EMPTY_STR;
    }
    return NON_NULL_EMPTY_STR(g_deviceInfo->uniqueId);
}
 	 
const char *ModelGetDevCustomData(void)
{
    if (g_deviceInfo == NULL) {
        return EMPTY_STR;
    }
    return NON_NULL_EMPTY_STR(g_deviceInfo->customData);
}

int32_t ModelGetUdid(uint8_t *buf, uint32_t len)
{
    CHECK_RETURN_LOGW(buf != NULL && len != 0 && len >= SECURITY_UDID_LEN, IOTC_ERR_PARAM_INVALID, "param invalid");

    char strBuf[IOTC_MANU_NAME_STR_MAX_LEN + IOTC_MODEL_STR_MAX_LEN + IOTC_SN_STR_MAX_LEN + 1] = {0};
    int32_t ret = sprintf_s(strBuf, sizeof(strBuf), "%s%s%s",
        ModelGetDevManuName(), ModelGetDevModel(), ModelGetDevSn());
    if (ret <= 0) {
        IOTC_LOGW("sprintf error %d", ret);
        return IOTC_ERR_SECUREC_SPRINTF;
    }

    uint8_t sha256Buf[IOTC_MD_SHA256_BYTE_LEN] = {0};
    ret = IotcMdCalc(IOTC_MD_SHA256, (uint8_t *)strBuf, strlen(strBuf), sha256Buf, sizeof(sha256Buf));
    if (ret != IOTC_OK) {
        IOTC_LOGW("calc sha256 error %d", ret);
        return ret;
    }

    if (!UtilsHexify(sha256Buf, sizeof(sha256Buf), (char *)buf, len)) {
        return IOTC_CORE_COMM_UTILS_ERR_HEXIFY;
    }
    return IOTC_OK;
}
