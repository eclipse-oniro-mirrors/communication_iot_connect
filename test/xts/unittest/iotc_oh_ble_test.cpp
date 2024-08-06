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

class IotcOhBleTest : public testing::Test {
public:
    IotcOhBleTest()
    {}
    ~IotcOhBleTest()
    {}
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override
    {}
    void TearDown() override
    {}
};

void IotcOhBleTest::SetUpTestCase(void)
{
    /* Is Bluetooth turned on */
}

void IotcOhBleTest::TearDownTestCase(void)
{
}

#define TEST_SVC_UUID "16F1E600A27743FCA484DD39EF8A9100"
#define TEST_SVC_INDICATE_UUID "16F1E601A27743FCA484DD39EF8A9100"

static const IotcBleGattProfileChar g_gattTestChar[] = {
    {
        .uuid = TEST_SVC_INDICATE_UUID,
        .permission = IOTC_BLE_GATT_PERMISSION_READ_ENCRYPTED | IOTC_BLE_GATT_PERMISSION_WRITE_ENCRYPTED,
        .property = IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_INDICATE,
        .readFunc = NULL,
        .writeFunc = NULL,
        .indicateFunc = NULL,
        .desc = NULL,
        .descNum = 0,
    }
};

static const IotcBleGattProfileSvc g_bleGattSvcBuf[] = {
    {
        .uuid = TEST_SVC_UUID,
        .character = g_gattTestChar,
        .charNum = (sizeof(g_gattTestChar) / sizeof(IotcBleGattProfileChar)),
    },
};

static const IotcBleGattProfileSvcList g_bleGattSvcList = {
    .svc = g_bleGattSvcBuf,
    .svcNum = sizeof(g_bleGattSvcBuf) / sizeof(IotcBleGattProfileSvc),
};

/**
 * @tc.name: IotcOhBleEnableTest
 * @tc.desc: Test iot connect ble enable.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: The IotcOhBleEnable operates normally.
 */
HWTEST_F(IotcOhBleTest, IotcOhBleEnableTest, TestSize.Level1)
{
    LOG_INFO(UDMF_TEST, "IotcOhBleEnableTest begin");
    int32_t ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    LOG_INFO(UDMF_TEST, "IotcOhBleEnableTest end");
}

/**
 * @tc.name: IotcOhBleDisableTest
 * @tc.desc: Test iot connect ble disable.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: The IotcOhBleDisable operates normally.
 */
HWTEST_F(IotcOhBleTest, IotcOhBleDisableTest, TestSize.Level1)
{
    LOG_INFO(UDMF_TEST, "IotcOhBleDisableTest begin");
    int32_t ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleDisable();
    EXPECT_TRUE(ret == IOTC_OK);
    LOG_INFO(UDMF_TEST, "IotcOhBleDisableTest end");
}

/**
 * @tc.name: IotcOhBleReleaseTest
 * @tc.desc: Test iot connect ble release.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: The IotcOhBleRelease operates normally.
 */
HWTEST_F(IotcOhBleTest, IotcOhBleReleaseTest, TestSize.Level1)
{
    LOG_INFO(UDMF_TEST, "IotcOhBleReleaseTest begin");
    int32_t ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleRelease();
    EXPECT_TRUE(ret == IOTC_OK);
    LOG_INFO(UDMF_TEST, "IotcOhBleReleaseTest end");
}

/**
 * @tc.name: IotcOhBleStartAdvTest
 * @tc.desc: Test iot connect ble adv start.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: The IotcOhBleStartAdv operates normally.
 */
HWTEST_F(IotcOhBleTest, IotcOhBleStartAdvTest, TestSize.Level1)
{
    LOG_INFO(UDMF_TEST, "IotcOhBleStartAdvTest begin");
    int32_t ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleStartAdv(1000);
    EXPECT_TRUE(ret == IOTC_OK);
    LOG_INFO(UDMF_TEST, "IotcOhBleStartAdvTest end");
}

/**
 * @tc.name: IotcOhBleStopAdvTest
 * @tc.desc: Test iot connect ble adv stop.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: The IotcOhBleStopAdv operates normally.
 */
HWTEST_F(IotcOhBleTest, IotcOhBleStopAdvTest, TestSize.Level1)
{
    LOG_INFO(UDMF_TEST, "IotcOhBleStopAdvTest begin");
    int32_t ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleStopAdv();
    EXPECT_TRUE(ret == IOTC_OK);
    LOG_INFO(UDMF_TEST, "IotcOhBleStopAdvTest end");
}

/**
 * @tc.name: IotcOhBleSendCustomSecDataTest
 * @tc.desc: Test iot connect ble send custom sec data.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: The IotcOhBleSendCustomSecData operates normally.
 */
HWTEST_F(IotcOhBleTest, IotcOhBleSendCustomSecDataTest, TestSize.Level1)
{
    LOG_INFO(UDMF_TEST, "IotcOhBleSendCustomSecDataTest begin");
    int32_t ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleSendCustomSecData((const uint8_t *)"1", 1);
    EXPECT_TRUE(ret == IOTC_OK);
    LOG_INFO(UDMF_TEST, "IotcOhBleSendCustomSecDataTest end");
}

/**
 * @tc.name: IotcOhBleSendIndicateDataTest
 * @tc.desc: Test iot connect ble send custom sec data.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: The IotcOhBleSendIndicateData operates normally.
 */
HWTEST_F(IotcOhBleTest, IotcOhBleSendIndicateDataTest, TestSize.Level1)
{
    LOG_INFO(UDMF_TEST, "IotcOhBleSendIndicateDataTest begin");
    int32_t ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_BLE_GATT_PROFILE_SVC_LIST, &g_bleGattSvcList);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleSendIndicateData(TEST_SVC_UUID, TEST_SVC_INDICATE_UUID, (const uint8_t *)"1", 1);
    EXPECT_TRUE(ret == IOTC_OK);
    LOG_INFO(UDMF_TEST, "IotcOhBleSendIndicateDataTest end");
}