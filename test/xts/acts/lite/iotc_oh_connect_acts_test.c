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

#include "securec.h"
#include "hctest.h"
#include "cmsis_os2.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "cJSON.h"

#include "iotc_conf.h"
#include "iotc_def.h"
#include "iotc_errcode.h"
#include "iotc_event.h"
#include "iotc_prof_def.h"
#include "iotc_oh_device.h"
#include "iotc_oh_sdk.h"
#include "utils_common.h"

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
        sleep(1);
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
        sleep(1);
    }
    return IOTC_ERROR;
}

static int32_t PutCharState(const IotcCharState state[], uint32_t num)
{
    return 0;
}

static int32_t GetCharState(const IotcCharState state[], char *out[], uint32_t len[], uint32_t num)
{
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
    .manuId = "17C",
    .manuName = "DALEN",
    .devName = "One Connect Dev Name",
    .fwv = "1.0.0",
    .hwv = "1.0.0",
    .swv = "1.0.0",
    .protType = IOTC_PROT_TYPE_BLE,
};

static IotcServiceInfo g_svcInfo[] = {
    {"switch", "switch"},
};


/**
 * @tc.desc      : register a test suite, this suite is used to test function
 * @param        : subsystem name is communication
 * @param        : module name is lwip
 * @param        : test suit name is LwipFuncTestSuite
 */
LITE_TEST_SUIT(communication, iot_connect, IotcOhConnectActsTest);

/**
 * @tc.setup     : setup for every testcase
 * @return       : setup result, TRUE is success, FALSE is fail
 */
static BOOL IotcOhConnectActsTestSetUp(void)
{
    printf("IotcOhConnectActsTestSetUp \r\n");
    return TRUE;
}

/**
 * @tc.teardown  : teardown for every testcase
 * @return       : teardown result, TRUE is success, FALSE is fail
 */
static BOOL IotcOhConnectActsTestTearDown(void)
{
    printf("IotcOhConnectActsTestTearDown \r\n");
    return TRUE;
}

/**
 * @tc.name   IotcOhConnectActsTest001
 * @tc.number IotcOhConnectActsTest001
 * @tc.desc   Test SDK reset after main start success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest001, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhReset();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest002
 * @tc.number IotcOhConnectActsTest002
 * @tc.desc   Test SDK reset after main start twice success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest002, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhReset();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    ClearIotcInitivalized();
    ret = IotcOhReset();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest003
 * @tc.number IotcOhConnectActsTest003
 * @tc.desc   Test SDK restore and reset cycle success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest003, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_AC_KEY_CALLBACK, GetAcKey);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_REBOOT_CALLBACK, NoticeReboot);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhReset();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhRestore();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhRestore();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhReset();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest004
 * @tc.number IotcOhConnectActsTest004
 * @tc.desc   Test SDK reset before main returns main task not exists error.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest004, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhReset();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_SDK_MNGR_ERR_MAIN_TASK_NOT_EXISTS);
}

/**
 * @tc.name   IotcOhConnectActsTest005
 * @tc.number IotcOhConnectActsTest005
 * @tc.desc   Test SDK stop before main start returns success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest005, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest006
 * @tc.number IotcOhConnectActsTest006
 * @tc.desc   Test SDK reset after stop returns main task not exists error.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest006, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhReset();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhReset();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_SDK_MNGR_ERR_MAIN_TASK_NOT_EXISTS);
}

/**
 * @tc.name   IotcOhConnectActsTest007
 * @tc.number IotcOhConnectActsTest007
 * @tc.desc   Test SDK stop twice after reset returns success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest007, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhReset();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest008
 * @tc.number IotcOhConnectActsTest008
 * @tc.desc   Test SDK main called twice returns already running error.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest008, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_CORE_COMM_FWK_ERR_MAIN_ALREADY_RUNNING);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest009
 * @tc.number IotcOhConnectActsTest009
 * @tc.desc   Test SDK restore before main returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest009, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhRestore();
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest010
 * @tc.number IotcOhConnectActsTest010
 * @tc.desc   Test SDK restore after stop returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest010, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhRestore();
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest011
 * @tc.number IotcOhConnectActsTest011
 * @tc.desc   Test set SDK log level to MIN success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest011, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_LOG_LEVEL, IOTC_LOG_LEVEL_MIN);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest012
 * @tc.number IotcOhConnectActsTest012
 * @tc.desc   Test set SDK log level to MAX success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest012, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_LOG_LEVEL, IOTC_LOG_LEVEL_MAX);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest013
 * @tc.number IotcOhConnectActsTest013
 * @tc.desc   Test set SDK log level to MAX success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest013, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_LOG_LEVEL, IOTC_LOG_LEVEL_MAX);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest014
 * @tc.number IotcOhConnectActsTest014
 * @tc.desc   Test set SDK log level to NOTICE success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest014, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_LOG_LEVEL, IOTC_LOG_LEVEL_NOTICE);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest015
 * @tc.number IotcOhConnectActsTest015
 * @tc.desc   Test set SDK log level to custom value success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest015, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_LOG_LEVEL, 100);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest016
 * @tc.number IotcOhConnectActsTest016
 * @tc.desc   Test set SDK log level with invalid value returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest016, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_LOG_LEVEL, 257);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest017
 * @tc.number IotcOhConnectActsTest017
 * @tc.desc   Test set SDK main task size to zero returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest017, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_MAIN_TASK_SIZE, 0);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest018
 * @tc.number IotcOhConnectActsTest018
 * @tc.desc   Test set SDK main task size with valid value success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest018, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_MAIN_TASK_SIZE, 32 * 1024);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest019
 * @tc.number IotcOhConnectActsTest019
 * @tc.desc   Test set SDK monitor task size to zero returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest019, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_MONITOR_TASK_SIZE, 0);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest020
 * @tc.number IotcOhConnectActsTest020
 * @tc.desc   Test set SDK monitor task size with valid value success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest020, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_MONITOR_TASK_SIZE, 32 * 1024);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest021
 * @tc.number IotcOhConnectActsTest021
 * @tc.desc   Test set SDK config path with NULL returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest021, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_CONFIG_PATH, NULL);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest022
 * @tc.number IotcOhConnectActsTest022
 * @tc.desc   Test set SDK config path with empty string success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest022, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_CONFIG_PATH, "");
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest023
 * @tc.number IotcOhConnectActsTest023
 * @tc.desc   Test set SDK config path exceeding max length returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest023, Function | MediumTest | Level2)
{
    int ret = 0;
    char buf[130] = {0};
    memset_s(buf, sizeof(buf), 'c', sizeof(buf) - 1);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_CONFIG_PATH, buf);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest024
 * @tc.number IotcOhConnectActsTest024
 * @tc.desc   Test set SDK config path with max length success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest024, Function | MediumTest | Level2)
{
    int ret = 0;
    char buf[129] = {0};
    memset_s(buf, sizeof(buf), 'c', sizeof(buf) - 1);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_CONFIG_PATH, buf);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_CONFIG_PATH, "iotc");
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest025
 * @tc.number IotcOhConnectActsTest025
 * @tc.desc   Test register SDK event listener with NULL returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest025, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, NULL);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest026
 * @tc.number IotcOhConnectActsTest026
 * @tc.desc   Test unregister SDK event listener before register returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest026, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_UNREG_EVENT_LISTENER, NULL);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest027
 * @tc.number IotcOhConnectActsTest027
 * @tc.desc   Test register two SDK event listeners and unregister success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest027, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback2);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_AC_KEY_CALLBACK, GetAcKey);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ClearIotcInitivalized2();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized2();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ClearIotcInitivalized2();
    ret = IotcOhReset();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized2();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhRestore();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_UNREG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_UNREG_EVENT_LISTENER, TestIotcEventCallback2);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}


/**
 * @tc.name   IotcOhConnectActsTest028
 * @tc.number IotcOhConnectActsTest028
 * @tc.desc   Test register device put char state callback success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest028, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_PUT_CHAR_STATE_CALLBACK, PutCharState);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest029
 * @tc.number IotcOhConnectActsTest029
 * @tc.desc   Test register device put char state callback with NULL returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest029, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_PUT_CHAR_STATE_CALLBACK, NULL);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest030
 * @tc.number IotcOhConnectActsTest030
 * @tc.desc   Test register device put char state callback without init returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest030, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_PUT_CHAR_STATE_CALLBACK, PutCharState);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest031
 * @tc.number IotcOhConnectActsTest031
 * @tc.desc   Test register device get char state callback success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest031, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_CHAR_STATE_CALLBACK, GetCharState);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest032
 * @tc.number IotcOhConnectActsTest032
 * @tc.desc   Test register device get char state callback with NULL returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest032, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_CHAR_STATE_CALLBACK, NULL);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest033
 * @tc.number IotcOhConnectActsTest033
 * @tc.desc   Test register device get char state callback without init returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest033, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_CHAR_STATE_CALLBACK, GetCharState);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest034
 * @tc.number IotcOhConnectActsTest034
 * @tc.desc   Test register device report all callback success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest034, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_REPORT_ALL_CALLBACK, ReportAll);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest035
 * @tc.number IotcOhConnectActsTest035
 * @tc.desc   Test register device report all callback with NULL returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest035, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_REPORT_ALL_CALLBACK, NULL);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest036
 * @tc.number IotcOhConnectActsTest036
 * @tc.desc   Test register device report all callback without init returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest036, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_REPORT_ALL_CALLBACK, ReportAll);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest037
 * @tc.number IotcOhConnectActsTest037
 * @tc.desc   Test register device get pincode callback success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest037, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_PINCODE_CALLBACK, GetPincode);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest038
 * @tc.number IotcOhConnectActsTest038
 * @tc.desc   Test register device get pincode callback with NULL returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest038, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_PINCODE_CALLBACK, NULL);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest039
 * @tc.number IotcOhConnectActsTest039
 * @tc.desc   Test register device get pincode callback without init returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest039, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_PINCODE_CALLBACK, GetPincode);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest040
 * @tc.number IotcOhConnectActsTest040
 * @tc.desc   Test register device get ac key callback success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest040, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_AC_KEY_CALLBACK, GetAcKey);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest041
 * @tc.number IotcOhConnectActsTest041
 * @tc.desc   Test register device get ac key callback with NULL returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest041, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_AC_KEY_CALLBACK, NULL);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest042
 * @tc.number IotcOhConnectActsTest042
 * @tc.desc   Test register device get ac key callback without init returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest042, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_AC_KEY_CALLBACK, GetAcKey);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest043
 * @tc.number IotcOhConnectActsTest043
 * @tc.desc   Test register device data free callback success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest043, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DATA_FREE_CALLBACK, cJSON_free);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest044
 * @tc.number IotcOhConnectActsTest044
 * @tc.desc   Test register device data free callback with NULL returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest044, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DATA_FREE_CALLBACK, NULL);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest045
 * @tc.number IotcOhConnectActsTest045
 * @tc.desc   Test register device data free callback without init returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest045, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DATA_FREE_CALLBACK, cJSON_free);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest046
 * @tc.number IotcOhConnectActsTest046
 * @tc.desc   Test register device reboot callback success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest046, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_REBOOT_CALLBACK, NoticeReboot);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest047
 * @tc.number IotcOhConnectActsTest047
 * @tc.desc   Test register device reboot callback with NULL returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest047, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_REBOOT_CALLBACK, NULL);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest048
 * @tc.number IotcOhConnectActsTest048
 * @tc.desc   Test register device reboot callback without init returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest048, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_REBOOT_CALLBACK, NoticeReboot);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest049
 * @tc.number IotcOhConnectActsTest049
 * @tc.desc   Test register device trng callback success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest049, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_REBOOT_CALLBACK, DevTrng);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest050
 * @tc.number IotcOhConnectActsTest050
 * @tc.desc   Test register device trng callback with NULL returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest050, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_REBOOT_CALLBACK, NULL);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest051
 * @tc.number IotcOhConnectActsTest051
 * @tc.desc   Test register device trng callback without init returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest051, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_REBOOT_CALLBACK, DevTrng);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest052
 * @tc.number IotcOhConnectActsTest052
 * @tc.desc   Test set device info with valid value success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest052, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest053
 * @tc.number IotcOhConnectActsTest053
 * @tc.desc   Test set device info with NULL returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest053, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, NULL);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest054
 * @tc.number IotcOhConnectActsTest054
 * @tc.desc   Test set device info with NULL sn returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest054, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    g_devInfo.sn = NULL;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.sn = "12345678";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest055
 * @tc.number IotcOhConnectActsTest055
 * @tc.desc   Test set device info with NULL prod id returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest055, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    g_devInfo.prodId = NULL;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.prodId = "2F6R0";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest056
 * @tc.number IotcOhConnectActsTest056
 * @tc.desc   Test set device info with NULL sub prod id success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest056, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    g_devInfo.subProdId = NULL;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    g_devInfo.subProdId = "";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest057
 * @tc.number IotcOhConnectActsTest057
 * @tc.desc   Test set device info with NULL model returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest057, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    g_devInfo.model = NULL;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.model = "DL-01W";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest058
 * @tc.number IotcOhConnectActsTest058
 * @tc.desc   Test set device info with NULL dev type id returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest058, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    g_devInfo.devTypeId = NULL;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.devTypeId = "0460";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest059
 * @tc.number IotcOhConnectActsTest059
 * @tc.desc   Test set device info with NULL dev type name returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest059, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    g_devInfo.devTypeName = NULL;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.devTypeName = "Table Lamp";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest060
 * @tc.number IotcOhConnectActsTest060
 * @tc.desc   Test set device info with NULL manu id returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest060, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    g_devInfo.manuId = NULL;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.manuId = "17C";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest061
 * @tc.number IotcOhConnectActsTest061
 * @tc.desc   Test set device info with NULL manu name returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest061, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    g_devInfo.manuName = NULL;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.manuName = "DALEN";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest062
 * @tc.number IotcOhConnectActsTest062
 * @tc.desc   Test set device info with NULL firmware version returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest062, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    g_devInfo.fwv = NULL;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.fwv = "1.0.0";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest063
 * @tc.number IotcOhConnectActsTest063
 * @tc.desc   Test set device info with NULL hardware version returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest063, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    g_devInfo.hwv = NULL;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.hwv = "1.0.0";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest064
 * @tc.number IotcOhConnectActsTest064
 * @tc.desc   Test set device info with NULL software version returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest064, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    g_devInfo.swv = NULL;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.swv = "1.0.0";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest065
 * @tc.number IotcOhConnectActsTest065
 * @tc.desc   Test set device info with min length sn returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest065, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    char tmpStr[UTILS_MIN_STR_LEN] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', UTILS_MIN_STR_LEN - 1);
    g_devInfo.sn = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.sn = "12345678";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest066
 * @tc.number IotcOhConnectActsTest066
 * @tc.desc   Test set device info with min length prod id returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest066, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    char tmpStr[IOTC_OH_PRO_ID_STR_LEN] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_PRO_ID_STR_LEN - 1);
    g_devInfo.prodId = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.prodId = "2F6R0";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest067
 * @tc.number IotcOhConnectActsTest067
 * @tc.desc   Test set device info with min length model returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest067, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    char tmpStr[UTILS_MIN_STR_LEN] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', UTILS_MIN_STR_LEN - 1);
    g_devInfo.model = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.model = "DL-01W";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest068
 * @tc.number IotcOhConnectActsTest068
 * @tc.desc   Test set device info with min length dev type id returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest068, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    char tmpStr[IOTC_OH_DEV_TYPE_ID_STR_LEN] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_DEV_TYPE_ID_STR_LEN - 1);
    g_devInfo.devTypeId = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.devTypeId = "0460";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest069
 * @tc.number IotcOhConnectActsTest069
 * @tc.desc   Test set device info with min length dev type name returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest069, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    char tmpStr[UTILS_MIN_STR_LEN] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', UTILS_MIN_STR_LEN - 1);
    g_devInfo.devTypeName = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.devTypeName = "Table Lamp";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest070
 * @tc.number IotcOhConnectActsTest070
 * @tc.desc   Test set device info with min length manu id returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest070, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    char tmpStr[IOTC_OH_MANU_ID_STR_LEN] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_MANU_ID_STR_LEN - 1);
    g_devInfo.manuId = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.manuId = "17C";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest071
 * @tc.number IotcOhConnectActsTest071
 * @tc.desc   Test set device info with min length manu name returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest071, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    char tmpStr[UTILS_MIN_STR_LEN] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', UTILS_MIN_STR_LEN - 1);
    g_devInfo.manuName = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.manuName = "17C";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest072
 * @tc.number IotcOhConnectActsTest072
 * @tc.desc   Test set device info with min length firmware version returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest072, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    char tmpStr[UTILS_MIN_STR_LEN] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', UTILS_MIN_STR_LEN - 1);
    g_devInfo.fwv = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.fwv = "1.0.0";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest073
 * @tc.number IotcOhConnectActsTest073
 * @tc.desc   Test set device info with min length hardware version returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest073, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    char tmpStr[UTILS_MIN_STR_LEN] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', UTILS_MIN_STR_LEN - 1);
    g_devInfo.hwv = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.hwv = "1.0.0";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest074
 * @tc.number IotcOhConnectActsTest074
 * @tc.desc   Test set device info with min length software version returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest074, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    char tmpStr[UTILS_MIN_STR_LEN] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', UTILS_MIN_STR_LEN - 1);
    g_devInfo.swv = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.swv = "1.0.0";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest075
 * @tc.number IotcOhConnectActsTest075
 * @tc.desc   Test set device info with sn exceeding max length returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest075, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    char tmpStr[IOTC_OH_SN_STR_MAX_LEN + 2] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_SN_STR_MAX_LEN + 1);
    g_devInfo.sn = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.sn = "12345678";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest076
 * @tc.number IotcOhConnectActsTest076
 * @tc.desc   Test set device info with prod id exceeding max length returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest076, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    char tmpStr[IOTC_OH_PRO_ID_STR_LEN + 2] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_PRO_ID_STR_LEN + 1);
    g_devInfo.prodId = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.prodId = "2F6R0";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest077
 * @tc.number IotcOhConnectActsTest077
 * @tc.desc   Test set device info with model exceeding max length returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest077, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    char tmpStr[IOTC_OH_MODEL_STR_MAX_LEN + 2] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_MODEL_STR_MAX_LEN + 1);
    g_devInfo.model = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.model = "DL-01W";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest078
 * @tc.number IotcOhConnectActsTest078
 * @tc.desc   Test set device info with dev type id exceeding max length returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest078, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    char tmpStr[IOTC_OH_DEV_TYPE_ID_STR_LEN + 2] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_DEV_TYPE_ID_STR_LEN + 1);
    g_devInfo.devTypeId = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.devTypeId = "0460";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest079
 * @tc.number IotcOhConnectActsTest079
 * @tc.desc   Test set device info with dev type name exceeding max length returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest079, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    char tmpStr[IOTC_OH_DEV_TYPE_NAME_STR_MAX_LEN + 2] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_DEV_TYPE_NAME_STR_MAX_LEN + 1);
    g_devInfo.devTypeName = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.devTypeName = "Table Lamp";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest080
 * @tc.number IotcOhConnectActsTest080
 * @tc.desc   Test set device info with manu id exceeding max length returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest080, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    char tmpStr[IOTC_OH_MANU_ID_STR_LEN + 2] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_MANU_ID_STR_LEN + 1);
    g_devInfo.manuId = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.manuId = "17C";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest081
 * @tc.number IotcOhConnectActsTest081
 * @tc.desc   Test set device info with manu name exceeding max length returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest081, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    char tmpStr[IOTC_OH_MANU_NAME_STR_MAX_LEN + 2] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_MANU_NAME_STR_MAX_LEN + 1);
    g_devInfo.manuName = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.manuName = "17C";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest082
 * @tc.number IotcOhConnectActsTest082
 * @tc.desc   Test set device info with firmware version exceeding max length returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest082, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    char tmpStr[IOTC_OH_FIRMWARE_VER_STR_MAX_LEN + 2] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_FIRMWARE_VER_STR_MAX_LEN + 1);
    g_devInfo.fwv = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.fwv = "1.0.0";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest083
 * @tc.number IotcOhConnectActsTest083
 * @tc.desc   Test set device info with hardware version exceeding max length returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest083, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    char tmpStr[IOTC_OH_FIRMWARE_VER_STR_MAX_LEN + 2] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_FIRMWARE_VER_STR_MAX_LEN + 1);
    g_devInfo.hwv = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.hwv = "1.0.0";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest084
 * @tc.number IotcOhConnectActsTest084
 * @tc.desc   Test set device info with software version exceeding max length returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest084, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    char tmpStr[IOTC_OH_FIRMWARE_VER_STR_MAX_LEN + 2] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_FIRMWARE_VER_STR_MAX_LEN + 1);
    g_devInfo.swv = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.swv = "1.0.0";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest085
 * @tc.number IotcOhConnectActsTest085
 * @tc.desc   Test set device info with invalid prot type returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest085, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    g_devInfo.protType = IOTC_PROT_TYPE_VIRTUAL + 1;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.protType = IOTC_PROT_TYPE_BLE;
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest086
 * @tc.number IotcOhConnectActsTest086
 * @tc.desc   Test set device info without init returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest086, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest087
 * @tc.number IotcOhConnectActsTest087
 * @tc.desc   Test set device service info with valid value success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest087, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_SVC_INFO, g_svcInfo, sizeof(g_svcInfo) / sizeof(g_svcInfo[0]));
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest088
 * @tc.number IotcOhConnectActsTest088
 * @tc.desc   Test set device service info with zero num returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest088, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_SVC_INFO, g_svcInfo, 0);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest089
 * @tc.number IotcOhConnectActsTest089
 * @tc.desc   Test set device service info with NULL returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest089, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_SVC_INFO, NULL, 1);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest090
 * @tc.number IotcOhConnectActsTest090
 * @tc.desc   Test set device service info with NULL svc id returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest090, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    g_svcInfo[0].svcId = NULL;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_SVC_INFO, g_svcInfo, sizeof(g_svcInfo) / sizeof(g_svcInfo[0]));
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_svcInfo[0].svcId = "switch";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest091
 * @tc.number IotcOhConnectActsTest091
 * @tc.desc   Test set device service info with NULL svc type returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest091, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    g_svcInfo[0].svcType = NULL;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_SVC_INFO, g_svcInfo, sizeof(g_svcInfo) / sizeof(g_svcInfo[0]));
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_svcInfo[0].svcType = "switch";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest092
 * @tc.number IotcOhConnectActsTest092
 * @tc.desc   Test set device service info with min length svc id returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest092, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    char tmpStr[UTILS_MIN_STR_LEN] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', UTILS_MIN_STR_LEN - 1);
    g_svcInfo[0].svcId = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_SVC_INFO, g_svcInfo, sizeof(g_svcInfo) / sizeof(g_svcInfo[0]));
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_svcInfo[0].svcId = "switch";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest093
 * @tc.number IotcOhConnectActsTest093
 * @tc.desc   Test set device service info with min length svc type returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest093, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    char tmpStr[UTILS_MIN_STR_LEN] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', UTILS_MIN_STR_LEN - 1);
    g_svcInfo[0].svcType = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_SVC_INFO, g_svcInfo, sizeof(g_svcInfo) / sizeof(g_svcInfo[0]));
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_svcInfo[0].svcType = "switch";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest094
 * @tc.number IotcOhConnectActsTest094
 * @tc.desc   Test set device service info with svc id exceeding max length returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest094, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    char tmpStr[IOTC_OH_SVC_ID_STR_MAX_LEN + 2] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_SVC_ID_STR_MAX_LEN + 1);
    g_svcInfo[0].svcId = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_SVC_INFO, g_svcInfo, sizeof(g_svcInfo) / sizeof(g_svcInfo[0]));
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_svcInfo[0].svcId = "switch";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest095
 * @tc.number IotcOhConnectActsTest095
 * @tc.desc   Test set device service info with svc type exceeding max length returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest095, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    char tmpStr[IOTC_OH_SVC_TYPE_STR_MAX_LEN + 2] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_SVC_TYPE_STR_MAX_LEN + 1);
    g_svcInfo[0].svcType = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_SVC_INFO, g_svcInfo, sizeof(g_svcInfo) / sizeof(g_svcInfo[0]));
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_svcInfo[0].svcType = "switch";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest096
 * @tc.number IotcOhConnectActsTest096
 * @tc.desc   Test set device service info without init returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest096, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_SVC_INFO, g_svcInfo, sizeof(g_svcInfo) / sizeof(g_svcInfo[0]));
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest097
 * @tc.number IotcOhConnectActsTest097
 * @tc.desc   Test set device info with NULL dev name returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest097, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    g_devInfo.devName = NULL;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.devName = "One Connect Dev Name";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest098
 * @tc.number IotcOhConnectActsTest098
 * @tc.desc   Test set device info with min length dev name returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest098, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    char tmpStr[UTILS_MIN_STR_LEN] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', UTILS_MIN_STR_LEN - 1);
    g_devInfo.devName = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.devName = "One Connect Dev Name";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhConnectActsTest099
 * @tc.number IotcOhConnectActsTest099
 * @tc.desc   Test set device info with dev name exceeding max length returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhConnectActsTest, IotcOhConnectActsTest099, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    char tmpStr[IOTC_OH_DEV_NAME_STR_MAX_LEN + 2] = {0};
    (void)memset_s(tmpStr, sizeof(tmpStr), 0, sizeof(tmpStr));
    (void)memset_s(tmpStr, sizeof(tmpStr), 'c', IOTC_OH_DEV_NAME_STR_MAX_LEN + 1);
    g_devInfo.devName = tmpStr;
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &g_devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_devInfo.devName = "One Connect Dev Name";
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

RUN_TEST_SUITE(IotcOhConnectActsTest);