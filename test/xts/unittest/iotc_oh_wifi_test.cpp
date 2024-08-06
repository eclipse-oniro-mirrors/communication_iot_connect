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

class IotcOhWifiTest : public testing::Test {
public:
    IotcOhWifiTest()
    {}
    ~IotcOhWifiTest()
    {}
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override
    {}
    void TearDown() override
    {}
};

void IotcOhWifiTest::SetUpTestCase(void)
{
    /* Is Wifi turned on */
}

void IotcOhWifiTest::TearDownTestCase(void)
{
}

/**
 * @tc.name: IotcOhWifiEnableTest
 * @tc.desc: Test iot connect wifi enable.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: The IotcOhWifiEnable operates normally.
 */
HWTEST_F(IotcOhWifiTest, IotcOhWifiEnableTest, TestSize.Level1)
{
    LOG_INFO(UDMF_TEST, "IotcOhWifiEnableTest begin");
    int32_t ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    LOG_INFO(UDMF_TEST, "IotcOhWifiEnableTest end");
}

/**
 * @tc.name: IotcOhWifiDisableTest
 * @tc.desc: Test iot connect wifi disable.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: The IotcOhWifiDisable operates normally.
 */
HWTEST_F(IotcOhWifiTest, IotcOhWifiDisableTest, TestSize.Level1)
{
    LOG_INFO(UDMF_TEST, "IotcOhWifiDisableTest begin");
    int32_t ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
    LOG_INFO(UDMF_TEST, "IotcOhWifiDisableTest end");
}