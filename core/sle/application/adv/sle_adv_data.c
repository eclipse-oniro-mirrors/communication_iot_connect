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
#include "sle_adv_data.h"
#include "securec.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "utils_assert.h"
#include "iotc_sle.h"
#include "sle_common.h"
#include "iotc_def.h"
#include "iotc_svc_dev.h"
#include "utils_combo.h"
#include "netcfg_service.h"
#include "sle_adv.h"
#include "utils_mutex_global.h"
#include "service_proxy.h"
#include "service_manager.h"
#include "iotc_svc.h"
#include "dev_info.h"
#include "product_adapter.h"

#define ADV_NAME_MOULD "Hi-AAAAABBBBB-XYYYYMMNNNN"
#define UNREG_ADV_NAME_HEAD "Hi"
#define REGED_ADV_NAME_HEAD "HI"
#define ADV_NAME_VER "1"
#define DEFAULT_ADV_TX_POWER 0xF8

#define CUSTOM_NAME_BUF_LEN 64
#define ADV_NAME_SN_LEN 4

#define COMPANY_UUID_BYTES 2
#define CUSTOM_COMPANY_UUID 0xFDEE
#define CUSTOM_ADV_VERSION 1
#define NEARBY_EXTENSION_DEFAULT_VALUE 1
#define CUSTOM_ADV_PROID_LEN 4
#define CUSTOM_ADV_SN_LEN 2
#define ONEHOP_TYPELIST_LEN 2

#define OFFSET_EIGHT_BIT 8
#define ONE_BYTE 1
#define TWO_BYTE 2

enum {
    ADV_TYPE_FLAGS = 0x01,
    ADV_TYPE_CUSTOM = 0x16,
    ADV_TYPE_NAME = 0x09,
};

enum {
    BUSINESS_SLE_NEARBY = 0x01, /* 蓝牙靠近发现业务 */
    BUSINESS_SLE_ONEHOP = 0x06, /* 蓝牙碰一碰业务 */
    BUSINESS_NFC_ONEHOP = 0x07, /* NFC碰一碰业务 */
};

enum {
    NEARBY_EXTENSION_HALF_MODAL = 3, /* 靠近发现拉起半模态 */
    NEARBY_EXTENSION_FA = 6, /* 靠近发现拉起FA */
};

enum {
    ONEHOP_EXTENSION_FA = 0x00, /* 碰一碰业务拉起FA */
    ONEHOP_EXTENSION_HALF_MODAL = 0x01, /* 碰一碰业务拉起半模态 */
};

enum {
    NEARBY_TYPE_SUB_PROID = 0x04,
    NEARBY_TYPE_ADV_POWER = 0x11,
    NEARBY_TYPE_PROID = 0x12,
    NEARBY_TYPE_TRANSMIT = 0xFF,
};

enum {
    HALF_MODAL_EXTEND_TYPE_PROTOCOL = 0x00, /* 可选，半模态弹框模式协议类型，如果不传此字段默认设备已被绑定，弹半模态控制页 */
    HALF_MODAL_EXTEND_TYPE_SN = 0x02, /* 必选，SN类型 */
};

enum {
    FA_EXTEND_TYPE_PROTOCOL = 0x17, /* 必选，FA弹框模式协议类型 */
    FA_EXTEND_TYPE_SN = 0x14, /* 必选，SN类型 */
};

enum {
    ONEHOP_EXTEND_TYPE_PROTOCOL = 0x17, /* 必选，FA弹框模式协议类型 */
    ONEHOP_EXTEND_TYPE_SN = 0x14, /* 必选，SN类型 */
};

enum {
    PROTOCOL_POP_CONTRL_BOX = 0, /* 弹设备控制框 */
    PROTOCOL_WIFI_NETCFG = 1, /* 弹wifi配网框 */
    PROTOCOL_DIRECT_DEV_REG = 2, /* 直连云设备（插网线，蜂窝等）app代理注册弹框 */
    PROTOCOL_WIFI_OFFLINE = 3, /* wifi设备wifi离线，请求更换路由器弹框，（member 不弹框） */
    PROTOCOL_DEV_ABNORMAL = 4, /* 设备异常弹窗 */
    PROTOCOL_SLE_ONLY_REG = 5, /* 纯蓝牙设备代理注册弹框 */
    PROTOCOL_PROXY_ACTIVATE = 6, /* 代理激活弹窗，适用于SLE+4G/SLE+WIFI单品 */
    PROTOCOL_SLE_ONLY_POP = 7, /* 蓝牙设备只弹窗配对不注册上云 */
    PROTOCOL_SLE_HIBEACON_REG = 8, /* 蓝牙Hibeacon设备代理注册弹框 */
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
    uint8_t memberSupport : 1;
    uint8_t resv : 1;
    uint8_t mode : 6;
} AdvProtocol;

typedef struct {
    uint16_t custom : 1;
    uint16_t proId : 1;
    uint16_t subProId : 1;
    uint16_t proForm : 1;
    uint16_t advPower : 1;
    uint16_t bussinessType : 1;
    uint16_t sleFeature : 1;
    uint16_t nfcFeature : 1;
    uint16_t randId : 1;
    uint16_t resv : 6;
    uint16_t cascade : 1;
} OneHopTypeList;

typedef union {
    struct {
        uint8_t defaultValue : 1;
        uint8_t mode : 7;
    } nearby;
    uint8_t onehop;
} AdvExtension;

typedef struct {
    /* 基础字段 */
    uint8_t uuid[COMPANY_UUID_BYTES];
    uint8_t version;
    uint8_t business;
    AdvExtension extension;
    /* 扩展字段 */
    union {
        /* 靠近发现扩展字段 */
        struct {
            struct {
                /* TV字段 */
                uint8_t subProIdType;
                uint8_t subProId;
                uint8_t advPowerType;
                int8_t advPower;
                uint8_t proIdType;
                uint8_t proId[CUSTOM_ADV_PROID_LEN];
                /* 透传字段 */
                uint8_t transmitType;
            } base;
            union {
                struct {
                    uint8_t protocolType;
                    AdvProtocol protocol;
                    uint8_t snType;
                    uint8_t sn[CUSTOM_ADV_SN_LEN];
                } halfModal;
                struct {
                    uint8_t protocolType;
                    uint8_t protocolLen;
                    AdvProtocol protocol;
                    uint8_t snType;
                    uint8_t snLen;
                    uint8_t sn[CUSTOM_ADV_SN_LEN];
                } fa;
            } transmit;
        } nearby;
        /* 碰一碰扩展字段 */
        struct {
            OneHopTypeList typeList;
            struct {
                uint8_t len;
                uint8_t protocolType;
                uint8_t protocolLen;
                AdvProtocol protocol;
                uint8_t snType;
                uint8_t snLen;
                uint8_t sn[CUSTOM_ADV_SN_LEN];
            } custom;
            uint8_t proId[CUSTOM_ADV_PROID_LEN];
            uint8_t subProId;
            int8_t advPower;
        } onehop;
    } extend;
} SleCustomAdvValue;

typedef struct {
    char buf[sizeof(ADV_NAME_MOULD)];
} SleAdvNameValue;

typedef struct {
    /* 蓝牙标识 */
    uint8_t len;
    uint8_t type;
    SleFlagsAdvValue value;
} SleFlagsAdvStruct;

typedef struct {
    /* 蓝牙自定义广播数据数据 */
    uint8_t len;
    uint8_t type;
    SleCustomAdvValue value;
} SleCustomAdvStruct;

typedef struct {
    /* 蓝牙名称 */
    uint8_t len;
    uint8_t type;
    SleAdvNameValue value;
} SleAdvRspStruct;

#pragma pack()

static SleSvcAdvDataType g_sleAdvType = IOTC_SLE_ADV_TYPE_NEARBY_HALF_MODAL;
static CustomAdvDataCb g_customAdvDataCb = NULL;

static SleSvcAdvDataType SleGetAdvType(void)
{
    return g_sleAdvType;
}

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

static uint32_t GenCustomUuid(SleCustomAdvValue *value)
{
    value->uuid[0] = (uint8_t)CUSTOM_COMPANY_UUID;
    value->uuid[1] = (uint8_t)(CUSTOM_COMPANY_UUID >> OFFSET_EIGHT_BIT);
    return sizeof(value->uuid);
}

static uint32_t GenCustomVersion(SleCustomAdvValue *value)
{
    value->version = CUSTOM_ADV_VERSION;
    return sizeof(value->version);
}

static uint32_t GenBusiness(SleCustomAdvValue *value)
{
    if ((SleGetAdvType() == IOTC_SLE_ADV_TYPE_NEARBY_HALF_MODAL) ||
        (SleGetAdvType() == IOTC_SLE_ADV_TYPE_NEARBY_FA)) {
        value->business = BUSINESS_SLE_NEARBY;
    } else if ((SleGetAdvType() == IOTC_SLE_ADV_TYPE_ONEHOP_HALF_MODAL) ||
               (SleGetAdvType() == IOTC_SLE_ADV_TYPE_ONEHOP_FA)) {
        value->business = BUSINESS_SLE_ONEHOP;
    }
    return sizeof(value->business);
}

static uint32_t GenExtension(SleCustomAdvValue *value)
{
    if (SleGetAdvType() == IOTC_SLE_ADV_TYPE_NEARBY_HALF_MODAL) {
        value->extension.nearby.defaultValue = NEARBY_EXTENSION_DEFAULT_VALUE;
        value->extension.nearby.mode = NEARBY_EXTENSION_HALF_MODAL;
    } else if (SleGetAdvType() == IOTC_SLE_ADV_TYPE_NEARBY_FA) {
        value->extension.nearby.defaultValue = NEARBY_EXTENSION_DEFAULT_VALUE;
        value->extension.nearby.mode = NEARBY_EXTENSION_FA;
    } else if (SleGetAdvType() == IOTC_SLE_ADV_TYPE_ONEHOP_HALF_MODAL) {
        value->extension.onehop = ONEHOP_EXTENSION_HALF_MODAL;
    } else if (SleGetAdvType() == IOTC_SLE_ADV_TYPE_ONEHOP_FA) {
        value->extension.onehop = ONEHOP_EXTENSION_FA;
    }
    return sizeof(value->extension);
}

static uint32_t GenExtendNearbyBase(SleCustomAdvValue *value)
{
    const char *proId = ModelGetDevProId();
    if (proId == NULL || strlen(proId) < CUSTOM_ADV_PROID_LEN) {
        IOTC_LOGE("get pro id invalid");
        return 0;
    }
    value->extend.nearby.base.subProIdType = NEARBY_TYPE_SUB_PROID;
    value->extend.nearby.base.subProId = GetSleSubProIdInt();
    value->extend.nearby.base.advPowerType = NEARBY_TYPE_ADV_POWER;
    if (ProductGetSurfacePower(&value->extend.nearby.base.advPower) != IOTC_OK) {
        IOTC_LOGN("set default power");
        value->extend.nearby.base.advPower = DEFAULT_ADV_TX_POWER;
    }
    value->extend.nearby.base.proIdType = NEARBY_TYPE_PROID;
    if (memcpy_s(value->extend.nearby.base.proId, CUSTOM_ADV_PROID_LEN,
        proId, CUSTOM_ADV_PROID_LEN) != EOK) {
        IOTC_LOGW("memcpy error");
        return 0;
    }
    value->extend.nearby.base.transmitType = NEARBY_TYPE_TRANSMIT;
    return sizeof(value->extend.nearby.base);
}

static void GetAdvProtocol(AdvProtocol *protocol)
{
    DevBindStatus bindStatus = DevSvcProxyGetBindStatus();
    bool onlineStatus = DevSvcProxyGetOnlineStatus();

    (void)memset_s(protocol, sizeof(AdvProtocol), 0, sizeof(AdvProtocol));
    protocol->memberSupport = 0;
    if (bindStatus == DEV_BIND_STATUS_BIND) {
        if (UtilsGetComboType() == COMBO_TYPE_COMBO) {
            protocol->mode = onlineStatus ? PROTOCOL_POP_CONTRL_BOX : PROTOCOL_WIFI_OFFLINE;
        } else if (UtilsGetComboType() == COMBO_TYPE_SLE_ONLY) {
            protocol->mode = PROTOCOL_POP_CONTRL_BOX;
        }
    } else {
        if (UtilsGetComboType() == COMBO_TYPE_COMBO) {
            if (GetNetCfgMode() == IOTC_NET_CONFIG_MODE_SLE_AGT) {
                protocol->mode = PROTOCOL_PROXY_ACTIVATE;
            } else if (GetNetCfgMode() == IOTC_NET_CONFIG_MODE_SLE_SUP) {
                protocol->mode = PROTOCOL_WIFI_NETCFG;
            }
        } else if (UtilsGetComboType() == COMBO_TYPE_SLE_ONLY) {
            protocol->mode = PROTOCOL_SLE_ONLY_REG;
        }
    }
}

static uint32_t GenExtendNearbyTransmitHalfModal(SleCustomAdvValue *value)
{
    const char *sn = ModelGetDevSn();
    if (sn == NULL || strlen(sn) < CUSTOM_ADV_SN_LEN) {
        IOTC_LOGW("get sn error");
        return 0;
    }

    value->extend.nearby.transmit.halfModal.protocolType = HALF_MODAL_EXTEND_TYPE_PROTOCOL;
    GetAdvProtocol(&value->extend.nearby.transmit.halfModal.protocol);
    value->extend.nearby.transmit.halfModal.snType = HALF_MODAL_EXTEND_TYPE_SN;
    if (memcpy_s(value->extend.nearby.transmit.halfModal.sn, CUSTOM_ADV_SN_LEN,
        GET_STR_TAIL(sn, CUSTOM_ADV_SN_LEN), CUSTOM_ADV_SN_LEN) != EOK) {
        IOTC_LOGE("memcpy_s");
        return 0;
    }
    return sizeof(value->extend.nearby.transmit.halfModal);
}

static uint32_t GenExtendNearbyTransmitFa(SleCustomAdvValue *value)
{
    const char *sn = ModelGetDevSn();
    if (sn == NULL || strlen(sn) < CUSTOM_ADV_SN_LEN) {
        IOTC_LOGW("get sn error");
        return 0;
    }

    value->extend.nearby.transmit.fa.protocolType = FA_EXTEND_TYPE_PROTOCOL;
    value->extend.nearby.transmit.fa.protocolLen = sizeof(AdvProtocol);
    GetAdvProtocol(&value->extend.nearby.transmit.fa.protocol);
    value->extend.nearby.transmit.fa.snType = FA_EXTEND_TYPE_SN;
    value->extend.nearby.transmit.fa.snLen = CUSTOM_ADV_SN_LEN;
    if (memcpy_s(value->extend.nearby.transmit.fa.sn, CUSTOM_ADV_SN_LEN,
        GET_STR_TAIL(sn, CUSTOM_ADV_SN_LEN), CUSTOM_ADV_SN_LEN) != EOK) {
        IOTC_LOGE("memcpy_s");
        return 0;
    }
    return sizeof(value->extend.nearby.transmit.fa);
}

static uint32_t GenExtendNearby(SleCustomAdvValue *value)
{
    uint32_t len = 0;
    if (SleGetAdvType() == IOTC_SLE_ADV_TYPE_NEARBY_HALF_MODAL) {
        len += GenExtendNearbyBase(value);
        len += GenExtendNearbyTransmitHalfModal(value);
    } else if (SleGetAdvType() == IOTC_SLE_ADV_TYPE_NEARBY_FA) {
        len += GenExtendNearbyBase(value);
        len += GenExtendNearbyTransmitFa(value);
    }
    return len;
}

static uint32_t GenExtendOnehop(SleCustomAdvValue *value)
{
    const IotcDeviceInfo *devInfo = ModelGetDevInfo();
    if (devInfo == NULL || devInfo->sn == NULL || devInfo->prodId == NULL ||
        strlen(devInfo->sn) < CUSTOM_ADV_SN_LEN ||  strlen(devInfo->prodId) < CUSTOM_ADV_PROID_LEN) {
        IOTC_LOGW("get dev info invalid");
        return 0;
    }

    (void)memset_s(&value->extend.onehop.typeList, sizeof(OneHopTypeList), 0, sizeof(OneHopTypeList));
    value->extend.onehop.typeList.custom = 1;
    value->extend.onehop.typeList.proId = 1;
    value->extend.onehop.typeList.subProId = 1;
    value->extend.onehop.typeList.advPower = 1;
    value->extend.onehop.custom.len = sizeof(value->extend.onehop.custom) - 1;
    value->extend.onehop.custom.protocolType = ONEHOP_EXTEND_TYPE_PROTOCOL;
    value->extend.onehop.custom.protocolLen = sizeof(value->extend.onehop.custom.protocol);
    GetAdvProtocol(&value->extend.onehop.custom.protocol);
    value->extend.onehop.custom.snType = ONEHOP_EXTEND_TYPE_SN;
    value->extend.onehop.custom.snLen = sizeof(value->extend.onehop.custom.sn);
    if (memcpy_s(value->extend.onehop.custom.sn, CUSTOM_ADV_SN_LEN,
        GET_STR_TAIL(devInfo->sn, CUSTOM_ADV_SN_LEN), CUSTOM_ADV_SN_LEN) != EOK) {
        IOTC_LOGE("memcpy_s");
        return 0;
    }
    if (memcpy_s(value->extend.onehop.proId, CUSTOM_ADV_PROID_LEN,
        devInfo->prodId, CUSTOM_ADV_PROID_LEN) != EOK) {
        IOTC_LOGE("memcpy_s");
        return 0;
    }
    value->extend.onehop.subProId = GetSleSubProIdInt();
    if (ProductGetSurfacePower(&value->extend.onehop.advPower) != IOTC_OK) {
        IOTC_LOGN("set defalut power");
        value->extend.onehop.advPower = DEFAULT_ADV_TX_POWER;
    }
    return sizeof(value->extend.onehop);
}

static uint32_t GetCustomAdvValue(SleCustomAdvValue *value)
{
    uint32_t len = 0;
    len += GenCustomUuid(value);
    len += GenCustomVersion(value);
    len += GenBusiness(value);
    len += GenExtension(value);
    if ((SleGetAdvType() == IOTC_SLE_ADV_TYPE_NEARBY_HALF_MODAL) ||
        (SleGetAdvType() == IOTC_SLE_ADV_TYPE_NEARBY_FA)) {
        len += GenExtendNearby(value);
    } else if ((SleGetAdvType() == IOTC_SLE_ADV_TYPE_ONEHOP_HALF_MODAL) ||
               (SleGetAdvType() == IOTC_SLE_ADV_TYPE_ONEHOP_FA)) {
        len += GenExtendOnehop(value);
    }

    return len;
}

static int32_t CustomAdvCopyToBuf(uint8_t *out, uint32_t outSize)
{
    SleCustomAdvStruct adv = {0};
    adv.type = ADV_TYPE_CUSTOM;
    uint32_t len = GetCustomAdvValue(&adv.value);
    adv.len = len + 1;
    if (memcpy_s(out, outSize, &adv, adv.len + 1) != EOK) {
        IOTC_LOGE("memcpy_s");
        return IOTC_ERROR;
    }
    return adv.len + 1;
}

static int32_t GenAdvName(SleAdvNameValue *value)
{
    const char *head = (DevSvcProxyGetBindStatus() == DEV_BIND_STATUS_BIND) ?
        REGED_ADV_NAME_HEAD : UNREG_ADV_NAME_HEAD;
    char customName[CUSTOM_NAME_BUF_LEN + 1] = {0};
    char advSn[ADV_NAME_SN_LEN + 1] = {0};
    const IotcDeviceInfo *devInfo = ModelGetDevInfo();
    if (devInfo == NULL || devInfo->sn == NULL || devInfo->manuName == NULL || devInfo->devTypeName == NULL ||
        devInfo->prodId == NULL || devInfo->subProdId == NULL) {
        IOTC_LOGW("get dev info null");
        return IOTC_CORE_PROF_MDL_ERR_DEV_INFO_INVALID;
    }

    int32_t snLen = strlen(devInfo->sn);
    if (snLen < ADV_NAME_SN_LEN) {
        IOTC_LOGW("set mac to adv sn");
    } else {
        if (sprintf_s(advSn, sizeof(advSn), "%s", GET_STR_TAIL(devInfo->sn, ADV_NAME_SN_LEN)) <= 0) {
            IOTC_LOGE("sprintf_s");
            return IOTC_ERROR;
        }
    }
    if (sprintf_s(customName, sizeof(customName), "%s%s",
        devInfo->manuName, devInfo->devTypeName) <= 0) {
        IOTC_LOGE("sprintf_s");
        return IOTC_ERROR;
    }
    if (sprintf_s(value->buf, sizeof(value->buf), "%s-%.10s-%s%s%s%s",
        head, customName, ADV_NAME_VER, devInfo->prodId, devInfo->subProdId, advSn) <= 0) {
        IOTC_LOGE("sprintf_s");
        return IOTC_ERROR;
    }

    return strlen(value->buf);
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

static int32_t GetSleAilifeAdvDataInner(IotcAdptSleAdvData *advData)
{
    CHECK_RETURN_LOGW(advData != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");
    (void)memset_s(advData, sizeof(IotcAdptSleAdvData), 0, sizeof(IotcAdptSleAdvData));

    int32_t len = 0;
    len = AdvFlagsCopyToBuf(&advData->announceData[advData->announceDataLen],
        sizeof(advData->announceData) - advData->announceDataLen);
    if (len < 0) {
        IOTC_LOGE("copy");
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    advData->announceDataLen += len;
    if (SleGetAdvType() != IOTC_SLE_ADV_TYPE_ONLY_NAME) {
        len = CustomAdvCopyToBuf(&advData->announceData[advData->announceDataLen],
            sizeof(advData->announceData) - advData->announceDataLen);
        if (len < 0) {
            IOTC_LOGE("copy");
            return IOTC_ERR_SECUREC_MEMCPY;
        }
        advData->announceDataLen += len;
    }

    len = 0;
    len = RspAdvCopyToBuf(&advData->seekRspData[advData->seekRspDataLen],
        sizeof(advData->seekRspData) - advData->seekRspDataLen);
    if (len < 0) {
        IOTC_LOGE("copy");
        return IOTC_ERR_SECUREC_MEMCPY;
    }
    advData->seekRspDataLen += len;
    return IOTC_OK;
}

int32_t GetSleAdvData(IotcAdptSleAdvData *advData)
{
    if (!UtilsGlobalMutexLock()) {
        IOTC_LOGW("glock error");
        return IOTC_ERR_TIMEOUT;
    }
    CustomAdvDataCb customAdvDataCb = g_customAdvDataCb;
    UtilsGlobalMutexUnlock();
    if ((SleGetAdvType() == IOTC_SLE_ADV_TYPE_CUSTOM) && (customAdvDataCb != NULL)) {
        return customAdvDataCb(advData);
    } else {
        return GetSleAilifeAdvDataInner(advData);
    }
}

void SleSetAdvType(SleSvcAdvDataType type)
{
    g_sleAdvType = type;
    IOTC_LOGN("set adv type to %d", type);
}
