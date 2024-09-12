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

using namespace testing::ext;

class IotcOhDeviceActsTest : public testing::Test {
public:
    IotcOhDeviceActsTest()
    {}
    ~IotcOhDeviceActsTest()
    {}
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override
    {}
    void TearDown() override
    {}
};

void IotcOhDeviceActsTest::SetUpTestCase(void)
{
    /* Is Wifi turned on */
}

void IotcOhDeviceActsTest::TearDownTestCase(void)
{
}

#define ONE_THOUSAND 1000

static bool g_iotcInitalizedEvent = false;

static void TestIotcEventCallback(int32_t event)
{
    if (event == IOTC_CORE_COMM_EVENT_MAIN_INITIALIZED) {
        g_iotcInitalizedEvent = true;
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

static IotcCharState g_repChar = {
    .svcId = "test",
    .data = "1",
    .len = 1,
};

static IotcCharState g_repChar1 = {
    .svcId = NULL,
    .data = "1",
    .len = 1,
};

static IotcCharState g_repChar2 = {
    .svcId = "test",
    .data = NULL,
    .len = 1,
};

static IotcCharState g_repChar3 = {
    .svcId = "test",
    .data = "1",
    .len = 0,
};

/**
 * @tc.name: IotcOhDeviceActsTest001
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhDeviceActsTest, IotcOhDeviceActsTest001, TestSize.Level1)
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
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhDeviceActsTest002
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhDeviceActsTest, IotcOhDeviceActsTest002, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhDevInit();
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhDeviceActsTest003
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhDeviceActsTest, IotcOhDeviceActsTest003, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhDeviceActsTest004
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhDeviceActsTest, IotcOhDeviceActsTest004, TestSize.Level1)
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
    ret = IotcOhDevReportCharState(&g_repChar, 1);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhDeviceActsTest005
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhDeviceActsTest, IotcOhDeviceActsTest005, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevReportCharState(&g_repChar, 1);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhDeviceActsTest006
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhDeviceActsTest, IotcOhDeviceActsTest006, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhDevReportCharState(&g_repChar, 1);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhDeviceActsTest007
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhDeviceActsTest, IotcOhDeviceActsTest007, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhDevReportCharState(&g_repChar, 1);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhDeviceActsTest008
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhDeviceActsTest, IotcOhDeviceActsTest008, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevReportCharState(NULL, 1);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhDeviceActsTest009
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhDeviceActsTest, IotcOhDeviceActsTest009, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevReportCharState(&g_repChar, 0);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhDeviceActsTest010
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhDeviceActsTest, IotcOhDeviceActsTest010, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevReportCharState(&g_repChar1, 0);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhDeviceActsTest011
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhDeviceActsTest, IotcOhDeviceActsTest011, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevReportCharState(&g_repChar2, 0);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhDeviceActsTest012
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhDeviceActsTest, IotcOhDeviceActsTest012, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevReportCharState(&g_repChar3, 0);
    EXPECT_FALSE(ret == IOTC_OK);
}