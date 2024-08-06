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

#include <gtest/gtest.h>
#include "logger.h"
#include "iotc_ble_def.h"
#include "iotc_conf.h"
#include "iotc_def.h"
#include "iotc_errcode.h"
#include "iotc_event.h"
#include "iotc_prof_def.h"
#include "iotc_oh_ble.h"
#include "iotc_oh_device.h"
#include "iotc_oh_sdk.h"
#include "iotc_oh_wifi.h"

using namespace testing::ext;

class IotcOhSdkTest : public testing::Test {
public:
    IotcOhSdkTest()
    {}
    ~IotcOhSdkTest()
    {}
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override
    {}
    void TearDown() override
    {}
};

void IotcOhSdkTest::SetUpTestCase(void)
{
}

void IotcOhBleTest::TearDownTestCase(void)
{
}

static const IotcDeviceInfo DEV_INFO = {
    .sn = "12345678",
    .prodId = "2F6R0",
    .subProdId = "",
    .model = "DL-01W",
    .devTypeId = "0460",
    .devTypeName = "Table Lamp",
    .manuId = "17C",
    .manuName = "DALEN",
    .fwv = "1.0.0",
    .hwv = "1.0.0",
    .swv = "1.0.0",
    .protType = IOTC_PROT_TYPE_WIFI,
};

static const IotcServiceInfo SVC_INFO[] = {
    {"lightSwitch", "lightSwitch"},
    {"gear", "gear" },
    {"deviceTime", "deviceTime"},
};

static int32_t PutCharState(const IotcCharState state[], uint32_t num)
{
    return IOTC_OK;
}

static int32_t GetCharState(const IotcCharState state[], char *out[], uint32_t len[], uint32_t num)
{
    return IOTC_OK;
}

static int32_t ReportAll(void)
{
    return IOTC_OK;
}

static int32_t GetPincode(uint8_t *buf, uint32_t bufLen)
{
    return IOTC_OK;
}

static int32_t GetAcKey(uint8_t *buf, uint32_t bufLen)
{
    return IOTC_OK;
}

static int32_t NoticeReboot(IotcRebootReason res)
{
    return IOTC_OK;
}

static int32_t GetRootCA(const char **ca[], uint32_t *num)
{
    return IOTC_OK;
}

/**
 * @tc.name: IotcOhMainTest
 * @tc.desc: Test iot connect main.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: The IotcOhMain operates normally.
 */
HWTEST_F(IotcOhSdkTest, IotcOhMainTest, TestSize.Level1)
{
    LOG_INFO(UDMF_TEST, "IotcOhMainTest begin");
    int32_t ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    LOG_INFO(UDMF_TEST, "IotcOhMainTest end");
}

/**
 * @tc.name: IotcOhResetTest
 * @tc.desc: Test iot connect reset.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: The IotcOhReset operates normally.
 */
HWTEST_F(IotcOhSdkTest, IotcOhResetTest, TestSize.Level1)
{
    LOG_INFO(UDMF_TEST, "IotcOhResetTest begin");
    int32_t ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhReset();
    EXPECT_TRUE(ret == IOTC_OK);
    LOG_INFO(UDMF_TEST, "IotcOhResetTest end");
}

/**
 * @tc.name: IotcOhStopTest
 * @tc.desc: Test iot connect stop.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: The IotcOhStop operates normally.
 */
HWTEST_F(IotcOhSdkTest, IotcOhStopTest, TestSize.Level1)
{
    LOG_INFO(UDMF_TEST, "IotcOhStopTest begin");
    int32_t ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
    LOG_INFO(UDMF_TEST, "IotcOhStopTest end");
}

/**
 * @tc.name: IotcOhRestoreTest
 * @tc.desc: Test iot connect restore.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: The IotcOhRestore operates normally.
 */
HWTEST_F(IotcOhSdkTest, IotcOhRestoreTest, TestSize.Level1)
{
    LOG_INFO(UDMF_TEST, "IotcOhRestoreTest begin");
    int32_t ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhRestore();
    EXPECT_TRUE(ret == IOTC_OK);
    LOG_INFO(UDMF_TEST, "IotcOhRestoreTest end");
}

/**
 * @tc.name: IotcOhSetOptionTest
 * @tc.desc: Test iot connect set option.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: The IotcOhSetOption operates normally.
 */
HWTEST_F(IotcOhSdkTest, IotcOhSetOptionTest, TestSize.Level1)
{
    LOG_INFO(UDMF_TEST, "IotcOhSetOptionTest begin");
    int32_t ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_PUT_CHAR_STATE_CALLBACK, PutCharState);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_CHAR_STATE_CALLBACK, GetCharState);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_REPORT_ALL_CALLBACK, ReportAll);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_PINCODE_CALLBACK, GetPincode);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_AC_KEY_CALLBACK, GetAcKey);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DATA_FREE_CALLBACK, cJSON_free);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_REBOOT_CALLBACK, NoticeReboot);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &DEV_INFO);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_SVC_INFO, SVC_INFO, sizeof(SVC_INFO) / sizeof(SVC_INFO[0]));
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_NETCFG_MODE, IOTC_NET_CONFIG_MODE_BLE_SUP);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_NETCFG_TIMEOUT, (10 * 60 * 1000));
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_GET_CERT_CALLBACK, GetRootCA);
    EXPECT_TRUE(ret == IOTC_OK);
    LOG_INFO(UDMF_TEST, "IotcOhSetOptionTest end");
}