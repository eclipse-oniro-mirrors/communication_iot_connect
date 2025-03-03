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
#include "iotc_oh_sle_adv_data.h"
#include "securec.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "utils_assert.h"
#include "iotc_svc_sle.h"
#include "iotc_svc_dev.h"
#include "dev_info.h"

#define ADV_NAME_MOULD "OH-AAAAABBBBB-XYYYYYNNNNMPP"
#define UNREG_ADV_NAME_HEAD "Oh"
#define REGED_ADV_NAME_HEAD "OH"
#define ADV_NAME_VER "1"
#define SLE_NETCFG_CAPCITY 0b01100001
#define SLE_NETCFG_CAPCITY_SIZE 1

#define CUSTOM_NAME_BUF_LEN 64
#define ADV_NAME_SN_LEN 4

enum {
    ADV_TYPE_FLAGS = 0x01,
    ADV_TYPE_NAME = 0x09,
};


#pragma pack(1)

typedef struct {
    uint8_t leLimitedDiscoverableMode : 1;
    uint8_t leGeneralDiscoverableMode : 1;
    uint8_t controller : 1;
    uint8_t host : 1;
    uint8_t reserved : 4;
} SleFlagsAdvValue;

typedef struct {
    /* 蓝牙标识 */
    uint8_t len;
    uint8_t type;
    SleFlagsAdvValue value;
} SleFlagsAdvStruct;

typedef struct {
    char buf[sizeof(ADV_NAME_MOULD)];
} SleAdvNameValue;

typedef struct {
    /* 蓝牙名称 */
    uint8_t len;
    uint8_t type;
    SleAdvNameValue value;
} SleAdvRspStruct;

#pragma pack()

static void GenAdvFlags(SleFlagsAdvValue *value)
{
    (void)memset_s(value, sizeof(SleFlagsAdvValue), 0, sizeof(SleFlagsAdvValue));
    value->leGeneralDiscoverableMode = 1;
    value->controller = 1;
}

static int32_t AdvFlagsCopyToBuf(uint8_t *out, uint32_t outSize)
{
    SleFlagsAdvStruct adv = {0};
    adv.len = sizeof(SleFlagsAdvValue) + 1;
    adv.type = ADV_TYPE_FLAGS;
    GenAdvFlags(&adv.value);
    if (memcpy_s(out, outSize, &adv, sizeof(adv)) != EOK) {
        IOTC_LOGE("memcpy_s");
        return IOTC_ERROR;
    }
    return sizeof(adv);
}



static int32_t GenAdvName(SleAdvNameValue *value)
{
    const char *head = DevSvcProxyGetBindStatus() == DEV_BIND_STATUS_BIND ? REGED_ADV_NAME_HEAD : UNREG_ADV_NAME_HEAD;
    char customName[CUSTOM_NAME_BUF_LEN + 1] = {0};
    char advSn[ADV_NAME_SN_LEN + 1] = {0};
    const IotcDeviceInfo *devInfo = ModelGetDevInfo();
    if (devInfo == NULL || devInfo->sn == NULL || devInfo->manuName == NULL || devInfo->devTypeName == NULL ||
        devInfo->prodId == NULL) {
        IOTC_LOGW("get dev info null");
        return IOTC_CORE_PROF_MDL_ERR_DEV_INFO_INVALID;
    }

    uint32_t snLen = strlen(devInfo->sn);
    if (snLen < ADV_NAME_SN_LEN) {
        IOTC_LOGW("sn too short %u", snLen);
        return IOTC_CORE_PROF_MDL_ERR_DEV_INFO_INVALID_SN;
    }

    int32_t ret = sprintf_s(advSn, sizeof(advSn), "%s", GET_STR_TAIL(devInfo->sn, ADV_NAME_SN_LEN));
    if (ret <= 0) {
        IOTC_LOGE("sprintf error %d", ret);
        return IOTC_ERR_SECUREC_SPRINTF;
    }

    ret = sprintf_s(customName, sizeof(customName), "%s%s", devInfo->manuName, devInfo->devTypeName);
    if (ret <= 0) {
        IOTC_LOGE("sprintf error %d", ret);
        return IOTC_ERR_SECUREC_SPRINTF;
    }

    ret = sprintf_s(value->buf, sizeof(value->buf), "%s-%.10s-%s%s%s",
        head, customName, ADV_NAME_VER, devInfo->prodId, advSn);
    if ((ret <= 0) || (ret + SLE_NETCFG_CAPCITY_SIZE) > sizeof(value->buf)) {
        IOTC_LOGE("sprintf error %d,%u,%u", ret, SLE_NETCFG_CAPCITY_SIZE, sizeof(value->buf));
        return IOTC_ERR_SECUREC_SPRINTF;
    }
    value->buf[ret] = SLE_NETCFG_CAPCITY;
    return ret + SLE_NETCFG_CAPCITY_SIZE;
}

static int32_t RspAdvCopyToBuf(uint8_t *out, uint32_t outSize)
{
    SleAdvRspStruct adv = {0};
    adv.type = ADV_TYPE_NAME;
    int32_t len = GenAdvName(&adv.value);
    if (len < 0) {
        return IOTC_ERROR;
    }
    adv.len = len + 1;
    if (memcpy_s(out, outSize, &adv, adv.len + 1) != EOK) {
        IOTC_LOGE("memcpy_s");
        return IOTC_ERROR;
    }
    return adv.len + 1;
}

int32_t IotcOhGetSleAdvData(IotcAdptSleAdvData *advData)
{
    CHECK_RETURN_LOGW(advData != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");
    (void)memset_s(advData, sizeof(IotcAdptSleAdvData), 0, sizeof(IotcAdptSleAdvData));

    int32_t len = 0;
    len = AdvFlagsCopyToBuf(&advData->announceData[advData->announceDataLen], sizeof(advData->announceData) - advData->announceDataLen);
    if (len < 0) {
        IOTC_LOGE("copy");
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    advData->announceDataLen += len;

    len = 0;
    len = RspAdvCopyToBuf(&advData->seekRspData[advData->seekRspDataLen], sizeof(advData->seekRspData) - advData->seekRspDataLen);
    if (len < 0) {
        IOTC_LOGE("copy");
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    advData->seekRspDataLen += len;
    return IOTC_OK;
}
