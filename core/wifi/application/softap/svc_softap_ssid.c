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
#include "svc_softap_ssid.h"
#include "utils_assert.h"
#include "comm_def.h"
#include "securec.h"
#include "utils_common.h"
#include "iotc_wifi_def.h"
#include "iotc_svc_dev.h"
#include "iotc_svc.h"
#include "service_proxy.h"
#include "iotc_errcode.h"
#include "dev_info.h"

#define HILINK_SOFTAP_SSID_PREFIX "Hi- "
#define HILINK_SOFTAP_DASH_CHAR '-'
#define HILINK_SOFTAP_VER_CHAR '1'
#define HILINK_SOFTAP_DEV_INFO_MAX_LEN 10
#define HILINK_SOFTAP_MANU_CUT_LEN 5
#define HILINK_SOFTAP_DEV_CUT_LEN 5
#define HILINK_SOFTAP_SN_MIN_LEN 3
#define HILINK_SOFTAP_COVER_CHAR ' '

/* 仅支持字母、数字、空格 */
static void DelInvalidChar(char *str, uint32_t len)
{
    uint32_t i = 0;
    uint32_t j = 0;
    for (; i < len && str[i] != '\0'; ++i) {
        if ((str[i] >= 'a' && str[i] <= 'z') ||
            (str[i] >= 'A' && str[i] <= 'Z') ||
            (str[i] >= '0' && str[i] <= '9') ||
            (str[i] == ' ')) {
            str[j] = str[i];
            ++j;
        }
    }
    str[j] = '\0';
}

static void CutDevInfo(char *manu, uint32_t manuLen, char *devName, uint32_t devNameLen)
{
    if (manuLen + devNameLen <= HILINK_SOFTAP_DEV_INFO_MAX_LEN) {
        return;
    }
    if (manuLen < HILINK_SOFTAP_MANU_CUT_LEN) {
        /* dev name 超长截断 */
        devName[HILINK_SOFTAP_DEV_INFO_MAX_LEN - manuLen] = '\0';
        return;
    }
    if (devNameLen < HILINK_SOFTAP_DEV_CUT_LEN) {
        /* manu name 超长截断 */
        manu[HILINK_SOFTAP_DEV_INFO_MAX_LEN - devNameLen] = '\0';
        return;
    }
    /* 都超长 */
    manu[HILINK_SOFTAP_MANU_CUT_LEN] = '\0';
    devName[HILINK_SOFTAP_DEV_CUT_LEN] = '\0';
}

static int32_t BuildManuNameAndDeviceName(CommBuffer *buf, const IotcDeviceInfo *devInfo)
{
    const char *manu = devInfo->manuName;
    const char *deviceType = devInfo->devTypeName;
    if (UtilsIsEmptyStr(manu)) {
        IOTC_LOGW("empty manu name");
        return IOTC_CORE_PROF_MDL_ERR_DEV_INFO_EMPTY_MANU_NAME;
    }
    if (UtilsIsEmptyStr(deviceType)) {
        IOTC_LOGW("empty dev name");
        return IOTC_CORE_PROF_MDL_ERR_DEV_INFO_EMPTY_DEV_NAME;
    }

    char *manuBuf = UtilsStrDup(manu);
    if (manuBuf == NULL) {
        IOTC_LOGW("dum manu name error");
        return IOTC_CORE_COMM_UTILS_ERR_STR_DUP;
    }
    char *deviceTypeBuf = UtilsStrDup(deviceType);
    if (deviceTypeBuf == NULL) {
        IOTC_LOGW("dum dev name error");
        IotcFree(manuBuf);
        return IOTC_CORE_COMM_UTILS_ERR_STR_DUP;
    }

    /* 删除非法字符 */
    DelInvalidChar(manuBuf, strlen(manuBuf));
    DelInvalidChar(deviceTypeBuf, strlen(deviceTypeBuf));
    /* 截断 */
    CutDevInfo(manuBuf, strlen(manuBuf), deviceTypeBuf, strlen(deviceTypeBuf));

    int32_t ret = sprintf_s((char *)buf->buffer + buf->len, buf->size - buf->len, "%s%c%s", manuBuf,
        HILINK_SOFTAP_DASH_CHAR, deviceTypeBuf);
    IotcFree(manuBuf);
    IotcFree(deviceTypeBuf);
    if (ret <= 0) {
        return IOTC_ERR_SECUREC_SPRINTF;
    }
    buf->len += ret;
    return IOTC_OK;
}

static int32_t BuildProdId(CommBuffer *buf, const IotcDeviceInfo *devInfo)
{
    const char *proId = devInfo->prodId;
    const char *subProId = devInfo->subProdId;
    if (strlen(proId) != IOTC_PRO_ID_STR_LEN) {
        IOTC_LOGW("invalid pro id");
        return IOTC_CORE_PROF_MDL_ERR_DEV_INFO_INVALID_PRO_ID;
    }
    if (strlen(subProId) != IOTC_SUB_PRO_ID_STR_LEN) {
        IOTC_LOGW("invalid sub pro id");
        return IOTC_CORE_PROF_MDL_ERR_DEV_INFO_INVALID_SUB_PRO_ID;
    }

    int32_t ret = sprintf_s((char *)buf->buffer + buf->len, buf->size - buf->len, "%s%s", proId, subProId);
    if (ret <= 0) {
        return IOTC_ERR_SECUREC_SPRINTF;
    }
    buf->len += ret;
    return IOTC_OK;
}

static int32_t BuildSoftapSn(CommBuffer *buf, const IotcDeviceInfo *devInfo)
{
    const char *sn = devInfo->sn;
    if (strlen(sn) < HILINK_SOFTAP_SN_MIN_LEN) {
        IOTC_LOGW("invalid sn len");
        return IOTC_CORE_PROF_MDL_ERR_DEV_INFO_INVALID_SN;
    }

    uint32_t pos = strlen(sn) - HILINK_SOFTAP_SN_MIN_LEN;
    int32_t ret = sprintf_s((char *)buf->buffer + buf->len, buf->size - buf->len, "%s", sn + pos);
    if (ret <= 0) {
        return IOTC_ERR_SECUREC_SPRINTF;
    }
    buf->len += ret;
    return IOTC_OK;
}

/**
 * hilink softap ssid规则：Hi- AAAAA-BBBBB-VYYYYSSNNNEX
 * 其中:
 *      A: 厂商名
 *      B: 设备名，A+B不超过10字符，超过则截断至最少5字符
 *      V: 版本号
 *      Y: 四字符 product id
 *      S: 两字符sub product id
 *      N: SN后三位，不足三位取随机字符补齐
 *      E: 能力集，无能力取空格
 *      X: 空格补齐32字符
 */
static int32_t BuildHiLinkSsid(CommBuffer *buf)
{
    const IotcDeviceInfo *devInfo = ModelGetDevInfo();
    if (devInfo == NULL || devInfo->devTypeName == NULL || devInfo->manuName == NULL || devInfo->prodId == NULL ||
        devInfo->sn == NULL || devInfo->subProdId == NULL) {
        IOTC_LOGW("get dev info null");
        return IOTC_CORE_PROF_MDL_ERR_DEV_INFO_INVALID;
    }

    int32_t ret = strcpy_s((char *)buf->buffer, buf->size, HILINK_SOFTAP_SSID_PREFIX);
    if (ret != EOK) {
        return IOTC_ERR_SECUREC_STRCPY;
    }
    buf->len = strlen(HILINK_SOFTAP_SSID_PREFIX);
    ret = BuildManuNameAndDeviceName(buf, devInfo);
    if (ret != IOTC_OK) {
        return ret;
    }

    buf->buffer[buf->len++] = HILINK_SOFTAP_DASH_CHAR;
    buf->buffer[buf->len++] = HILINK_SOFTAP_VER_CHAR;

    ret = BuildProdId(buf, devInfo);
    if (ret != IOTC_OK) {
        return ret;
    }

    ret = BuildSoftapSn(buf, devInfo);
    if (ret != IOTC_OK) {
        return ret;
    }

    /* 能力集为空 */
    buf->buffer[buf->len++] = HILINK_SOFTAP_COVER_CHAR;
    /* 补齐空格 */
    for (; buf->len < buf->size; ++buf->len) {
        buf->buffer[buf->len] = HILINK_SOFTAP_COVER_CHAR;
    }

    return IOTC_OK;
}

int32_t BuildHiLinkSoftapSsid(uint8_t *ssid, uint32_t *len)
{
    CHECK_RETURN(ssid != NULL && len != NULL && *len >= IOTC_WIFI_SSID_MAX_LEN, IOTC_ERR_PARAM_INVALID);

    uint32_t size = *len;
    (void)memset_s(ssid, size, 0, size);
    CommBuffer buf = {ssid, 0, size};

    int32_t ret = BuildHiLinkSsid(&buf);
    if (ret == IOTC_OK) {
        *len = buf.len;
    }
    return ret;
}