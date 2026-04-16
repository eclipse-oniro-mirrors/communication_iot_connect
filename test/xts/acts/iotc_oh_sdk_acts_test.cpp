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
#include "utils_common.h"

using namespace testing::ext;

class IotcOhSdkActsTest : public testing::Test {
public:
    IotcOhSdkActsTest()
    {}
    ~IotcOhSdkActsTest()
    {}
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override
    {}
    void TearDown() override
    {}
};

void IotcOhSdkActsTest::SetUpTestCase(void)
{
    /* Is Bluetooth turned on */
}

void IotcOhSdkActsTest::TearDownTestCase(void)
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

static bool g_iotcInitalizedEvent2 = false;

static void TestIotcEventCallback2(int32_t event)
{
    if (event == IOTC_CORE_COMM_EVENT_MAIN_INITIALIZED) {
        g_iotcInitalizedEvent2 = true;
    }
}

static void ClearIotcInitivalized2(void)
{
    g_iotcInitalizedEvent2 = false;
}

static int WaitIotcInitivalized2(void)
{
    int cnt = ONE_THOUSAND;
    while (cnt > 0) {
        if (g_iotcInitalizedEvent2) {
            return IOTC_OK;
        }
        cnt--;
        usleep(ONE_THOUSAND);
    }
    return IOTC_ERROR;
}

static int32_t GetRootCA(const char **ca[], uint32_t *num)
{
    (void)ca;
    (void)num;
    return IOTC_OK;
}

static int32_t RecvNetCfgInfoCallback(const char *netInfo, uint32_t len)
{
    (void)netInfo;
    (void)len;
    return 0;
}

static int32_t RecvCustomSecDataCallback(const uint8_t *data, uint32_t len)
{
    return 0;
}

#define TEST_SVC_UUID "16F1E600A27743FCA484DD39EF8A9100"
#define TEST_SVC_INDICATE_UUID "16F1E601A27743FCA484DD39EF8A9100"

static IotcBleGattProfileChar g_gattTestChar[] = {
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

static IotcBleGattProfileSvc g_bleGattSvcBuf[] = {
    {
        .uuid = TEST_SVC_UUID,
        .character = g_gattTestChar,
        .charNum = (sizeof(g_gattTestChar) / sizeof(IotcBleGattProfileChar)),
    },
};

static IotcBleGattProfileSvcList g_bleGattSvcList = {
    .svc = g_bleGattSvcBuf,
    .svcNum = sizeof(g_bleGattSvcBuf) / sizeof(IotcBleGattProfileSvc),
};

static int32_t PutCharState(const IotcCharState state[], uint32_t num)
{
    return 0;
}

static int32_t GetCharState(const IotcCharState state[], char *out[], uint32_t len[], uint32_t num)
{
    (void)state;
    (void)out;
    (void)len;
    (void)num;
    return 0;
}

static int32_t ReportAll(void)
{
    return 0;
}

static int32_t GetPincode(uint8_t *buf, uint32_t bufLen)
{
    return 0;
}

static int32_t GetAcKey(uint8_t *buf, uint32_t bufLen)
{
    return 0;
}

static int32_t NoticeReboot(IotcRebootReason res)
{
    return 0;
}

int32_t DevTrng(uint8_t *buf, uint32_t len)
{
    return 0;
}

static IotcDeviceInfo g_devInfo = {
    .sn = "12345678",
    .prodId = "2F6R0",
    .subProdId = "",
    .model = "DL-01W",
    .devTypeId = "0460",
    .devTypeName = "Table Lamp",
    .devName = "One Connect Dev Name",
    .manuId = "17C",
    .manuName = "DALEN",
    .fwv = "1.0.0",
    .hwv = "1.0.0",
    .swv = "1.0.0",
    .protType = IOTC_PROT_TYPE_BLE,
};

static IotcServiceInfo g_svcInfo[] = {
    {"switch", "switch"},
};

/**
 * @tc.name: IotcOhSdkActsTest001
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest001, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhReset();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest002
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest002, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhReset();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    ClearIotcInitivalized();
    ret = IotcOhReset();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest003
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest003, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_AC_KEY_CALLBACK, GetAcKey);
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhReset();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhRestore();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhRestore();
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhReset();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest004
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest004, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhReset();
    EXPECT_TRUE(ret == IOTC_SDK_MNGR_ERR_MAIN_TASK_NOT_EXISTS);
}

/**
 * @tc.name: IotcOhSdkActsTest005
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest005, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest006
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest006, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhReset();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhReset();
    EXPECT_TRUE(ret == IOTC_SDK_MNGR_ERR_MAIN_TASK_NOT_EXISTS);
}

/**
 * @tc.name: IotcOhSdkActsTest007
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest007, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhReset();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest008
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest008, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_CORE_COMM_FWK_ERR_MAIN_ALREADY_RUNNING);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest009
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest009, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhRestore();
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest010
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest010, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhRestore();
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest011
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest011, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_LOG_LEVEL, IOTC_LOG_LEVEL_MIN);
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest012
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest012, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_LOG_LEVEL, IOTC_LOG_LEVEL_MAX);
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest013
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest013, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_LOG_LEVEL, IOTC_LOG_LEVEL_MAX);
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest014
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest014, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_LOG_LEVEL, IOTC_LOG_LEVEL_NOTICE);
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest015
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest015, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_LOG_LEVEL, 100);
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest016
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest016, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_LOG_LEVEL, 257);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest017
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest017, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_MAIN_TASK_SIZE, 0);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest018
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest018, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_MAIN_TASK_SIZE, 32 * 1024);
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest019
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest019, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_MONITOR_TASK_SIZE, 0);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest020
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest020, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_MONITOR_TASK_SIZE, 32 * 1024);
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest021
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest021, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_CONFIG_PATH, NULL);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest022
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest022, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_CONFIG_PATH, "");
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest023
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest023, TestSize.Level1)
{
    int ret = 0;
    char buf[130] = {0};
    memset_s(buf, sizeof(buf), 'c', sizeof(buf) - 1);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_CONFIG_PATH, buf);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest024
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest024, TestSize.Level1)
{
    int ret = 0;
    char buf[129] = {0};
    memset_s(buf, sizeof(buf), 'c', sizeof(buf) - 1);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_CONFIG_PATH, buf);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_CONFIG_PATH, "iotc");
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest025
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest025, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, NULL);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest026
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest026, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_UNREG_EVENT_LISTENER, NULL);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest027
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest027, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback2);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_AC_KEY_CALLBACK, GetAcKey);
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ClearIotcInitivalized2();
    ret = IotcOhMain();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized2();
    EXPECT_TRUE(ret == IOTC_OK);
    ClearIotcInitivalized();
    ClearIotcInitivalized2();
    ret = IotcOhReset();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized2();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhRestore();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhStop();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_UNREG_EVENT_LISTENER, TestIotcEventCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_UNREG_EVENT_LISTENER, TestIotcEventCallback2);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest028
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest028, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    uint32_t resSize = 2048;
    uint32_t maxSize = 3072;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    EXPECT_TRUE(ret == IOTC_OK);
    resSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_SIZE;
    maxSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest029
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest029, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    uint32_t resSize = 2048;
    uint32_t maxSize = 2048;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    EXPECT_TRUE(ret == IOTC_OK);
    resSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_SIZE;
    maxSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest030
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest030, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    uint32_t resSize = 2048;
    uint32_t maxSize = IOTC_CONF_WIFI_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    EXPECT_TRUE(ret == IOTC_OK);
    resSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_SIZE;
    maxSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest031
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest031, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    uint32_t resSize = 2048;
    uint32_t maxSize = IOTC_CONF_WIFI_BUFFER_MAX_SIZE + 1024;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    EXPECT_TRUE(ret == IOTC_OK);
    resSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_SIZE;
    maxSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest032
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest032, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    uint32_t resSize = 2048;
    uint32_t maxSize = 1024;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    EXPECT_FALSE(ret == IOTC_OK);
    resSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_SIZE;
    maxSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest033
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest033, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    uint32_t resSize = 2048;
    uint32_t maxSize = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    EXPECT_FALSE(ret == IOTC_OK);
    resSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_SIZE;
    maxSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest034
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest034, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    uint32_t resSize = IOTC_CONF_WIFI_BUFFER_MAX_SIZE;
    uint32_t maxSize = IOTC_CONF_WIFI_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    EXPECT_TRUE(ret == IOTC_OK);
    resSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_SIZE;
    maxSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest035
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest035, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    uint32_t resSize = IOTC_CONF_WIFI_BUFFER_MAX_SIZE + 1024;
    uint32_t maxSize = IOTC_CONF_WIFI_BUFFER_MAX_SIZE + 2048;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    EXPECT_TRUE(ret == IOTC_OK);
    resSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_SIZE;
    maxSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest036
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest036, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    uint32_t resSize = 0;
    uint32_t maxSize = 1024;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest037
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest037, TestSize.Level1)
{
    int ret = 0;
    uint32_t resSize = 2048;
    uint32_t maxSize = 3072;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest038
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest038, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    uint32_t resSize = 2048;
    uint32_t maxSize = 3072;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    EXPECT_TRUE(ret == IOTC_OK);
    resSize = IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_SIZE;
    maxSize = IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest039
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest039, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    uint32_t resSize = 2048;
    uint32_t maxSize = 2048;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    EXPECT_TRUE(ret == IOTC_OK);
    resSize = IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_SIZE;
    maxSize = IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest040
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest040, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    uint32_t resSize = 2048;
    uint32_t maxSize = IOTC_CONF_WIFI_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    EXPECT_TRUE(ret == IOTC_OK);
    resSize = IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_SIZE;
    maxSize = IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest041
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest041, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    uint32_t resSize = 2048;
    uint32_t maxSize = IOTC_CONF_WIFI_BUFFER_MAX_SIZE + 1024;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    EXPECT_TRUE(ret == IOTC_OK);
    resSize = IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_SIZE;
    maxSize = IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest042
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest042, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    uint32_t resSize = 2048;
    uint32_t maxSize = 1024;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest043
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest043, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    uint32_t resSize = 2048;
    uint32_t maxSize = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest044
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest044, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    uint32_t resSize = IOTC_CONF_WIFI_BUFFER_MAX_SIZE;
    uint32_t maxSize = IOTC_CONF_WIFI_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    EXPECT_TRUE(ret == IOTC_OK);
    resSize = IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_SIZE;
    maxSize = IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest045
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest045, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    uint32_t resSize = IOTC_CONF_WIFI_BUFFER_MAX_SIZE + 1024;
    uint32_t maxSize = IOTC_CONF_WIFI_BUFFER_MAX_SIZE + 2048;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    EXPECT_TRUE(ret == IOTC_OK);
    resSize = IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_SIZE;
    maxSize = IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest046
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest046, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    uint32_t resSize = 0;
    uint32_t maxSize = 1024;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest047
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest047, TestSize.Level1)
{
    int ret = 0;
    uint32_t resSize = 2048;
    uint32_t maxSize = 3072;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest048
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest048, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    int32_t mode = IOTC_NET_CONFIG_MODE_NONE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_NETCFG_MODE, mode);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest049
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest049, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    int32_t mode = IOTC_NET_CONFIG_MODE_SOFTAP;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_NETCFG_MODE, mode);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest050
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest050, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    int32_t mode = IOTC_NET_CONFIG_MODE_BLE_SUP;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_NETCFG_MODE, mode);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest051
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest051, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    int32_t mode = IOTC_NET_CONFIG_MODE_BLE_AGT;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_NETCFG_MODE, mode);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest052
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest052, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    int32_t mode = IOTC_NET_CONFIG_MODE_MAX;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_NETCFG_MODE, mode);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest053
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest053, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    int32_t mode = IOTC_NET_CONFIG_MODE_MAX + 1;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_NETCFG_MODE, mode);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest054
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest054, TestSize.Level1)
{
    int ret = 0;
    int32_t mode = IOTC_NET_CONFIG_MODE_NONE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_NETCFG_MODE, mode);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest055
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest055, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    uint32_t timeout = 60000;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_NETCFG_TIMEOUT, timeout);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest056
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest056, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    uint32_t timeout = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_NETCFG_TIMEOUT, timeout);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest057
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest057, TestSize.Level1)
{
    int ret = 0;
    uint32_t timeout = 60000;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_NETCFG_TIMEOUT, timeout);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest058
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest058, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_GET_CERT_CALLBACK, GetRootCA);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest059
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest059, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_GET_CERT_CALLBACK, NULL);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhWifiDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest060
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest060, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_GET_CERT_CALLBACK, GetRootCA);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest061
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest061, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_RECV_NETCFG_CALLBACK, RecvNetCfgInfoCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest062
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest062, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_RECV_NETCFG_CALLBACK, NULL);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhBleDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest063
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest063, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_RECV_NETCFG_CALLBACK, RecvNetCfgInfoCallback);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest064
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest064, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_RECV_CUSTOM_DATA_CALLBACK, RecvCustomSecDataCallback);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest065
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest065, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_RECV_CUSTOM_DATA_CALLBACK, NULL);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhBleDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest066
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest066, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_RECV_CUSTOM_DATA_CALLBACK, RecvCustomSecDataCallback);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest067
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest067, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    uint32_t timeout = 60000;
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_START_UP_ADV_TIMEOUT, timeout);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest068
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest068, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    uint32_t timeout = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_START_UP_ADV_TIMEOUT, timeout);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest069
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest069, TestSize.Level1)
{
    int ret = 0;
    uint32_t timeout = 60000;
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_START_UP_ADV_TIMEOUT, timeout);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest070
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest070, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_GATT_PROFILE_SVC_LIST, &g_bleGattSvcList);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest071
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest071, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_GATT_PROFILE_SVC_LIST, NULL);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhBleDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest072
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest072, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    g_bleGattSvcList.svc = NULL;
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_GATT_PROFILE_SVC_LIST, &g_bleGattSvcList);
    EXPECT_FALSE(ret == IOTC_OK);
    g_bleGattSvcList.svc = g_bleGattSvcBuf;
    ret = IotcOhBleDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest073
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest073, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    EXPECT_TRUE(ret == IOTC_OK);
    g_bleGattSvcList.svcNum = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_GATT_PROFILE_SVC_LIST, &g_bleGattSvcList);
    EXPECT_FALSE(ret == IOTC_OK);
    g_bleGattSvcList.svcNum = sizeof(g_bleGattSvcBuf) / sizeof(IotcBleGattProfileSvc);
    ret = IotcOhBleDisable();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest074
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest074, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_GATT_PROFILE_SVC_LIST, &g_bleGattSvcList);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest075
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest075, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_GATT_PROFILE_SVC_LIST, &g_bleGattSvcList);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest076
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest076, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_PUT_CHAR_STATE_CALLBACK, PutCharState);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest077
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest077, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_PUT_CHAR_STATE_CALLBACK, NULL);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest078
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest078, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_PUT_CHAR_STATE_CALLBACK, PutCharState);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest079
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest079, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_CHAR_STATE_CALLBACK, GetCharState);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest080
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest080, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_CHAR_STATE_CALLBACK, NULL);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest081
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest081, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_CHAR_STATE_CALLBACK, GetCharState);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest082
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest082, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_REPORT_ALL_CALLBACK, ReportAll);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest083
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest083, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_REPORT_ALL_CALLBACK, NULL);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest084
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest084, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_REPORT_ALL_CALLBACK, ReportAll);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest085
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest085, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_PINCODE_CALLBACK, GetPincode);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest086
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest086, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_PINCODE_CALLBACK, NULL);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest087
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest087, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_PINCODE_CALLBACK, GetPincode);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest088
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest088, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_AC_KEY_CALLBACK, GetAcKey);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest089
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest089, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_AC_KEY_CALLBACK, NULL);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest090
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest090, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_AC_KEY_CALLBACK, GetAcKey);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest091
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest091, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DATA_FREE_CALLBACK, cJSON_free);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest092
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest092, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DATA_FREE_CALLBACK, NULL);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest093
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest093, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DATA_FREE_CALLBACK, cJSON_free);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest094
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest094, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_REBOOT_CALLBACK, NoticeReboot);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest095
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest095, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_REBOOT_CALLBACK, NULL);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest096
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest096, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_REBOOT_CALLBACK, NoticeReboot);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest097
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest097, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_REBOOT_CALLBACK, DevTrng);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest098
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest098, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_REBOOT_CALLBACK, NULL);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest099
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest099, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_REBOOT_CALLBACK, DevTrng);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest100
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest100, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest101
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest101, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, NULL);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest102
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest102, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    g_devInfo.sn = NULL;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.sn = "12345678";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest103
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest103, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    g_devInfo.prodId = NULL;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.prodId = "2F6R0";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest104
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest104, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    g_devInfo.subProdId = NULL;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_TRUE(ret == IOTC_OK);
    g_devInfo.subProdId = "";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest105
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest105, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    g_devInfo.model = NULL;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.model = "DL-01W";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest106
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest106, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    g_devInfo.devTypeId = NULL;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.devTypeId = "0460";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest107
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest107, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    g_devInfo.devTypeName = NULL;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.devTypeName = "Table Lamp";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest108
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest108, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    g_devInfo.manuId = NULL;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.manuId = "17C";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest109
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest109, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    g_devInfo.manuName = NULL;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.manuName = "DALEN";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest110
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest110, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    g_devInfo.fwv = NULL;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.fwv = "1.0.0";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest111
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest111, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    g_devInfo.hwv = NULL;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.hwv = "1.0.0";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest112
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest112, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    g_devInfo.swv = NULL;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.swv = "1.0.0";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest113
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest113, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    char tmpStr[UTILS_MIN_STR_LEN] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', UTILS_MIN_STR_LEN - 1);
    g_devInfo.sn = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.sn = "12345678";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest114
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest114, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    char tmpStr[IOTC_OH_PRO_ID_STR_LEN] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_PRO_ID_STR_LEN - 1);
    g_devInfo.prodId = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.prodId = "2F6R0";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest115
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest115, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    char tmpStr[UTILS_MIN_STR_LEN] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', UTILS_MIN_STR_LEN - 1);
    g_devInfo.model = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.model = "DL-01W";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest116
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest116, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    char tmpStr[IOTC_OH_DEV_TYPE_ID_STR_LEN] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_DEV_TYPE_ID_STR_LEN - 1);
    g_devInfo.devTypeId = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.devTypeId = "0460";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest117
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest117, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    char tmpStr[UTILS_MIN_STR_LEN] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', UTILS_MIN_STR_LEN - 1);
    g_devInfo.devTypeName = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.devTypeName = "Table Lamp";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest118
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest118, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    char tmpStr[IOTC_OH_MANU_ID_STR_LEN] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_MANU_ID_STR_LEN - 1);
    g_devInfo.manuId = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.manuId = "17C";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest119
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest119, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    char tmpStr[UTILS_MIN_STR_LEN] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', UTILS_MIN_STR_LEN - 1);
    g_devInfo.manuName = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.manuName = "17C";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest120
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest120, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    char tmpStr[UTILS_MIN_STR_LEN] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', UTILS_MIN_STR_LEN - 1);
    g_devInfo.fwv = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.fwv = "1.0.0";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest121
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest121, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    char tmpStr[UTILS_MIN_STR_LEN] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', UTILS_MIN_STR_LEN - 1);
    g_devInfo.hwv = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.hwv = "1.0.0";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest122
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest122, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    char tmpStr[UTILS_MIN_STR_LEN] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', UTILS_MIN_STR_LEN - 1);
    g_devInfo.swv = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.swv = "1.0.0";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest123
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest123, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    char tmpStr[IOTC_OH_SN_STR_MAX_LEN + 2] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_SN_STR_MAX_LEN + 1);
    g_devInfo.sn = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.sn = "12345678";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest124
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest124, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    char tmpStr[IOTC_OH_PRO_ID_STR_LEN + 2] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_PRO_ID_STR_LEN + 1);
    g_devInfo.prodId = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.prodId = "2F6R0";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest125
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest125, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    char tmpStr[IOTC_OH_MODEL_STR_MAX_LEN + 2] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_MODEL_STR_MAX_LEN + 1);
    g_devInfo.model = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.model = "DL-01W";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest126
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest126, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    char tmpStr[IOTC_OH_DEV_TYPE_ID_STR_LEN + 2] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_DEV_TYPE_ID_STR_LEN + 1);
    g_devInfo.devTypeId = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.devTypeId = "0460";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest127
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest127, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    char tmpStr[IOTC_OH_DEV_TYPE_NAME_STR_MAX_LEN + 2] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_DEV_TYPE_NAME_STR_MAX_LEN + 1);
    g_devInfo.devTypeName = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.devTypeName = "Table Lamp";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest128
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest128, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    char tmpStr[IOTC_OH_MANU_ID_STR_LEN + 2] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_MANU_ID_STR_LEN + 1);
    g_devInfo.manuId = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.manuId = "17C";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest129
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest129, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    char tmpStr[IOTC_OH_MANU_NAME_STR_MAX_LEN + 2] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_MANU_NAME_STR_MAX_LEN + 1);
    g_devInfo.manuName = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.manuName = "17C";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest130
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest130, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    char tmpStr[IOTC_OH_FIRMWARE_VER_STR_MAX_LEN + 2] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_FIRMWARE_VER_STR_MAX_LEN + 1);
    g_devInfo.fwv = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.fwv = "1.0.0";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest131
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest131, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    char tmpStr[IOTC_OH_FIRMWARE_VER_STR_MAX_LEN + 2] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_FIRMWARE_VER_STR_MAX_LEN + 1);
    g_devInfo.hwv = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.hwv = "1.0.0";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest132
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest132, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    char tmpStr[IOTC_OH_FIRMWARE_VER_STR_MAX_LEN + 2] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_FIRMWARE_VER_STR_MAX_LEN + 1);
    g_devInfo.swv = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.swv = "1.0.0";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest133
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest133, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    g_devInfo.protType = IOTC_PROT_TYPE_VIRTUAL + 1;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
    g_devInfo.protType = IOTC_PROT_TYPE_BLE;
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest134
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest134, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    EXPECT_FALSE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest135
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest135, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_SVC_INFO, g_svcInfo, sizeof(g_svcInfo) / sizeof(g_svcInfo[0]));
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest136
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest136, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_SVC_INFO, g_svcInfo, 0);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest137
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest137, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_SVC_INFO, NULL, 1);
    EXPECT_FALSE(ret == IOTC_OK);
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest138
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest138, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    g_svcInfo[0].svcId = NULL;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_SVC_INFO, g_svcInfo, sizeof(g_svcInfo) / sizeof(g_svcInfo[0]));
    EXPECT_FALSE(ret == IOTC_OK);
    g_svcInfo[0].svcId = "switch";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest139
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest139, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    g_svcInfo[0].svcType = NULL;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_SVC_INFO, g_svcInfo, sizeof(g_svcInfo) / sizeof(g_svcInfo[0]));
    EXPECT_FALSE(ret == IOTC_OK);
    g_svcInfo[0].svcType = "switch";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest140
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest140, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    char tmpStr[UTILS_MIN_STR_LEN] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', UTILS_MIN_STR_LEN - 1);
    g_svcInfo[0].svcId = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_SVC_INFO, g_svcInfo, sizeof(g_svcInfo) / sizeof(g_svcInfo[0]));
    EXPECT_FALSE(ret == IOTC_OK);
    g_svcInfo[0].svcId = "switch";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest141
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest141, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    char tmpStr[UTILS_MIN_STR_LEN] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', UTILS_MIN_STR_LEN - 1);
    g_svcInfo[0].svcType = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_SVC_INFO, g_svcInfo, sizeof(g_svcInfo) / sizeof(g_svcInfo[0]));
    EXPECT_FALSE(ret == IOTC_OK);
    g_svcInfo[0].svcType = "switch";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest142
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest142, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    char tmpStr[IOTC_OH_SVC_ID_STR_MAX_LEN + 2] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_SVC_ID_STR_MAX_LEN + 1);
    g_svcInfo[0].svcId = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_SVC_INFO, g_svcInfo, sizeof(g_svcInfo) / sizeof(g_svcInfo[0]));
    EXPECT_FALSE(ret == IOTC_OK);
    g_svcInfo[0].svcId = "switch";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest143
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest143, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhDevInit();
    EXPECT_TRUE(ret == IOTC_OK);
    char tmpStr[IOTC_OH_SVC_TYPE_STR_MAX_LEN + 2] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_SVC_TYPE_STR_MAX_LEN + 1);
    g_svcInfo[0].svcType = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_SVC_INFO, g_svcInfo, sizeof(g_svcInfo) / sizeof(g_svcInfo[0]));
    EXPECT_FALSE(ret == IOTC_OK);
    g_svcInfo[0].svcType = "switch";
    ret = IotcOhDevDeinit();
    EXPECT_TRUE(ret == IOTC_OK);
}

/**
 * @tc.name: IotcOhSdkActsTest144
 * @tc.desc: Test iot connect.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcOhSdkActsTest, IotcOhSdkActsTest144, TestSize.Level1)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_SVC_INFO, g_svcInfo, sizeof(g_svcInfo) / sizeof(g_svcInfo[0]));
    EXPECT_FALSE(ret == IOTC_OK);
}