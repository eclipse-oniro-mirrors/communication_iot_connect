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
#include "securec.h"
#include "cJSON.h"

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

#define TEST_SVC_UUID "16F1E600A27743FCA484DD39EF8A9100"
#define TEST_SVC_INDICATE_UUID "16F1E601A27743FCA484DD39EF8A9100"

using namespace testing::ext;

class IotcOhBleActsTest : public testing::Test {
public:
    IotcOhBleActsTest()
    {}
    ~IotcOhBleActsTest()
    {}
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override
    {}
    void TearDown() override
    {}
};

void IotcOhBleActsTest::SetUpTestCase(void)
{
    /* Is Bluetooth turned on */
}

void IotcOhBleActsTest::TearDownTestCase(void)
{
}

#define ONE_THOUSAND 1000

static bool g_iotcInitalizedEvent = false;
static bool g_iotcBleSvcStartEvent = false;

static void TestIotcEventCallback(int32_t event)
{
    if (event == IOTC_CORE_COMM_EVENT_MAIN_INITIALIZED) {
        g_iotcInitalizedEvent = true;
    }
    if (event == IOTC_CORE_COMM_BLE_SVC_START) {
        g_iotcBleSvcStartEvent = true;
    }
}

static void ClearIotcInitivalized(void)
{
    g_iotcInitalizedEvent = false;
}

static int WaitIotcInitivalized(void)
{
    int cnt = ONE_THOUSAND;
    while (cnt > 0) {
        if (g_iotcInitalizedEvent) {
            return IOTC_OK;
        }
        cnt--;
        usleep(ONE_THOUSAND);
    }
    return IOTC_ERROR;
}

static void ClearIotcBleSvcStart(void)
{
    g_iotcBleSvcStartEvent = false;
}

static int WaitIotcBleSvcStart(void)
{
    int cnt = ONE_THOUSAND;
    while (cnt > 0) {
        if (g_iotcBleSvcStartEvent) {
            return IOTC_OK;
        }
        cnt--;
        usleep(ONE_THOUSAND);
    }
    return IOTC_ERROR;
}

static const IotcDeviceInfo DEV_INFO = {
    .sn = "12345678",
    .prodId = "12345",
    .subProdId = "",
    .model = "MODEL",
    .devTypeId = "1234",
    .devTypeName = "Dev Type Name",
    .devName = "One Connect Dev Name",
    .manuId = "123",
    .manuName = "Manu Name",
    .fwv = "1.0.0",
    .hwv = "1.0.0",
    .swv = "1.0.0",
    .protType = IOTC_PROT_TYPE_BLE,
};

/**
 * @tc.name: IotcOhBleActsTest001
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest001, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &DEV_INFO);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ClearIotcBleSvcStart();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcBleSvcStart();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleDisable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleDisable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest002
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest002, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_CORE_COMM_FWK_ERR_MAIN_ALREADY_RUNNING);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest003
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest003, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleDisable();
    EXPECT_TRUE(ret == IOTC_CORE_COMM_FWK_ERR_MAIN_ALREADY_RUNNING);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest004
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest004, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleRelease();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleStartAdv(1);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest005
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest005, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhBleRelease();
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest006
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest006, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleRelease();
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest007
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest007, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleRelease();
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhBleDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest008
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest008, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleRelease();
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhBleDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest009
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest009, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &DEV_INFO);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ClearIotcBleSvcStart();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcBleSvcStart();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleStartAdv(1);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleRelease();
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhBleDisable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest010
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest010, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &DEV_INFO);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ClearIotcBleSvcStart();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcBleSvcStart();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleStartAdv(0);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleStopAdv();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleRelease();
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhBleDisable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest011
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest011, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &DEV_INFO);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ClearIotcBleSvcStart();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcBleSvcStart();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleStartAdv(UINT32_MAX);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleStopAdv();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleRelease();
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhBleDisable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest012
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest012, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &DEV_INFO);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ClearIotcBleSvcStart();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcBleSvcStart();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleStartAdv(UINT32_MAX);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleStartAdv(1);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleStartAdv(0);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleStopAdv();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleRelease();
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhBleDisable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}


/**
 * @tc.name: IotcOhBleActsTest013
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest013, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &DEV_INFO);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ClearIotcBleSvcStart();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcBleSvcStart();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleStartAdv(1);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleStopAdv();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleRelease();
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhBleDisable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest014
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest014, TestSize.Level1)
{
    int ret = 0;
    static const IotcDeviceInfo devInfo = {0};
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest015
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest015, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &DEV_INFO);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ClearIotcBleSvcStart();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcBleSvcStart();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleStopAdv();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleStopAdv();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleRelease();
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhBleDisable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest016
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest016, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhBleStopAdv();
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest017
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest017, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleStopAdv();
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhBleDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest018
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest018, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleStopAdv();
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest019
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest019, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleSendCustomSecData(reinterpret_cast<unsigned char *>("1"), 1);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhBleSendCustomSecData(reinterpret_cast<unsigned char *>("1"), 1);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest020
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest020, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhBleSendCustomSecData(reinterpret_cast<unsigned char *>("1"), 1);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest021
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest021, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleSendCustomSecData(reinterpret_cast<unsigned char *>("1"), 1);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhBleDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest022
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest022, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleSendCustomSecData(reinterpret_cast<unsigned char *>("1"), 1);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest023
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest023, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhBleSendCustomSecData(NULL, 1);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest024
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest024, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhBleSendCustomSecData(reinterpret_cast<unsigned char *>("1"), 0);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest025
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest025, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleSendIndicateData(TEST_SVC_UUID, TEST_SVC_INDICATE_UUID, reinterpret_cast<unsigned char *>("1"), 1);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhBleSendIndicateData(TEST_SVC_UUID, TEST_SVC_INDICATE_UUID, reinterpret_cast<unsigned char *>("1"), 1);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest026
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest026, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhBleSendIndicateData(TEST_SVC_UUID, TEST_SVC_INDICATE_UUID, reinterpret_cast<unsigned char *>("1"), 1);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest027
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest027, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleSendIndicateData(TEST_SVC_UUID, TEST_SVC_INDICATE_UUID, reinterpret_cast<unsigned char *>("1"), 1);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhBleDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest028
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest028, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleSendIndicateData(TEST_SVC_UUID, TEST_SVC_INDICATE_UUID, reinterpret_cast<unsigned char *>("1"), 1);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhBleSendIndicateData(TEST_SVC_UUID, TEST_SVC_INDICATE_UUID, reinterpret_cast<unsigned char *>("1"), 1);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest029
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest029, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhBleSendIndicateData(NULL, TEST_SVC_INDICATE_UUID, reinterpret_cast<unsigned char *>("1"), 1);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest030
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest030, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhBleSendIndicateData(TEST_SVC_UUID, NULL, reinterpret_cast<unsigned char *>("1"), 1);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest031
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest031, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhBleSendIndicateData(TEST_SVC_UUID, TEST_SVC_INDICATE_UUID, NULL, 1);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhBleActsTest032
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhBleActsTest, IotcOhBleActsTest032, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhBleSendIndicateData(TEST_SVC_UUID, TEST_SVC_INDICATE_UUID, reinterpret_cast<unsigned char *>("1"), 0);
    EXPECT_FALSE(ret == IOTC_OK);
}