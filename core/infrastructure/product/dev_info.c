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

static IotcDeviceInfo g_deviceInfo;

int32_t ModelDevInfoInit(const IotcDeviceInfo *devInfo)
{
    CHECK_RETURN_LOGW(devInfo != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    ModelDevInfoDeinit();

    struct CopyItem {
        const char **dst;
        const char *src;
    } devInfoItem[] = {
        {&g_deviceInfo.sn, devInfo->sn},
        {&g_deviceInfo.prodId, devInfo->prodId},
        {&g_deviceInfo.subProdId, UtilsIsEmptyStr(devInfo->subProdId) ? DEFAULT_SUB_PRO_ID : devInfo->subProdId},
        {&g_deviceInfo.model, devInfo->model},
        {&g_deviceInfo.devTypeId, devInfo->devTypeId},
        {&g_deviceInfo.devTypeName, devInfo->devTypeName},
        {&g_deviceInfo.manuId, devInfo->manuId},
        {&g_deviceInfo.manuName, devInfo->manuName},
        {&g_deviceInfo.fwv, devInfo->fwv},
        {&g_deviceInfo.hwv, devInfo->hwv},
        {&g_deviceInfo.swv, devInfo->swv},
    };

    uint32_t i = 0;
    for (; i < ARRAY_SIZE(devInfoItem); ++i) {
        *devInfoItem[i].dst = UtilsStrDup(devInfoItem[i].src);
        if (*devInfoItem[i].dst == NULL) {
            break;
        }
    }
    if (i != ARRAY_SIZE(devInfoItem)) {
        IOTC_LOGW("dev info copy error %u", i);
        ModelDevInfoDeinit();
        return IOTC_CORE_COMM_UTILS_ERR_STR_DUP;
    }

    g_deviceInfo.protType = devInfo->protType;
    return IOTC_OK;
}

void ModelDevInfoDeinit(void)
{
    UTILS_FREE_2_NULL(g_deviceInfo.sn);
    UTILS_FREE_2_NULL(g_deviceInfo.prodId);
    UTILS_FREE_2_NULL(g_deviceInfo.subProdId);
    UTILS_FREE_2_NULL(g_deviceInfo.model);
    UTILS_FREE_2_NULL(g_deviceInfo.devTypeId);
    UTILS_FREE_2_NULL(g_deviceInfo.devTypeName);
    UTILS_FREE_2_NULL(g_deviceInfo.manuId);
    UTILS_FREE_2_NULL(g_deviceInfo.manuName);
    UTILS_FREE_2_NULL(g_deviceInfo.fwv);
    UTILS_FREE_2_NULL(g_deviceInfo.hwv);
    UTILS_FREE_2_NULL(g_deviceInfo.swv);
    (void)memset_s(&g_deviceInfo, sizeof(IotcDeviceInfo), 0, sizeof(IotcDeviceInfo));
}

const IotcDeviceInfo *ModelGetDevInfo(void)
{
    return &g_deviceInfo;
}

const char *ModelGetDevSn(void)
{
    return NON_NULL_EMPTY_STR(g_deviceInfo.sn);
}

const char *ModelGetDevProId(void)
{
    return NON_NULL_EMPTY_STR(g_deviceInfo.prodId);
}

const char *ModelGetDevSubProId(void)
{
    return NON_NULL_EMPTY_STR(g_deviceInfo.subProdId);
}

const char *ModelGetDevModel(void)
{
    return NON_NULL_EMPTY_STR(g_deviceInfo.model);
}

const char *ModelGetDevTypeId(void)
{
    return NON_NULL_EMPTY_STR(g_deviceInfo.devTypeId);
}

const char *ModelGetDevTypeName(void)
{
    return NON_NULL_EMPTY_STR(g_deviceInfo.devTypeName);
}

const char *ModelGetDevManuId(void)
{
    return NON_NULL_EMPTY_STR(g_deviceInfo.manuId);
}

const char *ModelGetDevManuName(void)
{
    return NON_NULL_EMPTY_STR(g_deviceInfo.manuName);
}

const char *ModelGetDevFwv(void)
{
    return NON_NULL_EMPTY_STR(g_deviceInfo.fwv);
}

const char *ModelGetDevHwv(void)
{
    return NON_NULL_EMPTY_STR(g_deviceInfo.hwv);
}

const char *ModelGetDevSwv(void)
{
    return NON_NULL_EMPTY_STR(g_deviceInfo.swv);
}

int32_t ModelGetDevProtType(void)
{
    return g_deviceInfo.protType;
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