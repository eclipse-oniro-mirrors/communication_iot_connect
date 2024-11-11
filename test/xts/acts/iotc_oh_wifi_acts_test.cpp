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

class IotcOhWifiActsTest : public testing::Test {
public:
    IotcOhWifiActsTest()
    {}
    ~IotcOhWifiActsTest()
    {}
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override
    {}
    void TearDown() override
    {}
};

void IotcOhWifiActsTest::SetUpTestCase(void)
{
    /* Is Wifi turned on */
}

void IotcOhWifiActsTest::TearDownTestCase(void)
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

/**
 * @tc.name: IotcOhWifiActsTest001
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhWifiActsTest, IotcOhWifiActsTest001, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
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
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhWifiActsTest002
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhWifiActsTest, IotcOhWifiActsTest002, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhWifiEnable();
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhWifiActsTest003
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhWifiActsTest, IotcOhWifiActsTest003, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
}