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

class IotcOhDevTest : public testing::Test {
public:
    IotcOhDevTest()
    {}
    ~IotcOhDevTest()
    {}
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override
    {}
    void TearDown() override
    {}
};

void IotcOhDevTest::SetUpTestCase(void)
{
}

void IotcOhDevTest::TearDownTestCase(void)
{
}

static const IotcServiceInfo SVC_INFO[] = {
    {"switch", "switch"},
};

/**
 * @tc.name: IotcOhDevInitTest
 * @tc.desc: Test iot connect device initialization.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: The IotcOhDevInit operates normally.
 */
HWTEST_F(IotcOhDevTest, IotcOhDevInitTest, TestSize.Level1)
{
    LOG_INFO(UDMF_TEST, "IotcOhDevInitTest begin");
    int32_t ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    LOG_INFO(UDMF_TEST, "IotcOhDevInitTest end");
}

/**
 * @tc.name: IotcOhDevDeinitTest
 * @tc.desc: Test iot connect device disable initialization.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: The IotcOhDevDeinit operates normally.
 */
HWTEST_F(IotcOhDevTest, IotcOhDevDeinitTest, TestSize.Level1)
{
    LOG_INFO(UDMF_TEST, "IotcOhDevDeinitTest begin");
    int32_t ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
    LOG_INFO(UDMF_TEST, "IotcOhDevDeinitTest end");
}

/**
 * @tc.name: IotcOhDevReportCharStateTest
 * @tc.desc: Test iot connect report char state.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: The IotcOhDevReportCharState operates normally.
 */
HWTEST_F(IotcOhDevTest, IotcOhDevReportCharStateTest, TestSize.Level1)
{
    LOG_INFO(UDMF_TEST, "IotcOhDevReportCharStateTest begin");
    int32_t ret = 0;
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_SVC_INFO, SVC_INFO, sizeof(SVC_INFO) / sizeof(SVC_INFO[0]));
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    IotcCharState reportInfo[sizeof(SVC_MAP) / sizeof(SVC_MAP[0])] = {0};
    reportInfo[0].svcId = SVC_MAP[0].svc->svcId;
    reportInfo[0].data = "1";
    reportInfo[0].len = 1;
    ret = IotcOhDevReportCharState(reportInfo, 1);
    EXPECT_TRUE(ret == IOTC_OK);
    LOG_INFO(UDMF_TEST, "IotcOhDevReportCharStateTest end");
}