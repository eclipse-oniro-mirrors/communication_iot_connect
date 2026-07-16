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
    if (event == IOTC_CORE_COMM_EVENT_MAIN_INITIALIZED)
    {
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
    while (cnt > 0)
    {
        if (g_iotcInitalizedEvent)
        {
            return IOTC_OK;
        }
        cnt--;
        sleep(1);
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
 * @tc.desc      : register a test suite, this suite is used to test function
 * @param        : subsystem name is communication
 * @param        : module name is lwip
 * @param        : test suit name is LwipFuncTestSuite
 */
LITE_TEST_SUIT(communication, iot_connect, IotcOhDeviceActsTest);

/**
 * @tc.setup     : setup for every testcase
 * @return       : setup result, TRUE is success, FALSE is fail
 */
static BOOL IotcOhDeviceActsTestSetUp(void)
{
    printf("IotcOhDeviceActsTestSetUp \r\n");
    return TRUE;
}

/**
 * @tc.teardown  : teardown for every testcase
 * @return       : teardown result, TRUE is success, FALSE is fail
 */
static BOOL IotcOhDeviceActsTestTearDown(void)
{
    printf("IotcOhDeviceActsTestSetUp \r\n");
    return TRUE;
}

/**
 * @tc.name   IotcOhDeviceActsTest001
 * @tc.number IotcOhDeviceActsTest001
 * @tc.desc   Test device init, run main and deinit success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhDeviceActsTest, IotcOhDeviceActsTest001, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhDeviceActsTest002
 * @tc.number IotcOhDeviceActsTest002
 * @tc.desc   Test device init after main start returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhDeviceActsTest, IotcOhDeviceActsTest002, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhDevInit();
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhDeviceActsTest003
 * @tc.number IotcOhDeviceActsTest003
 * @tc.desc   Test device deinit before init returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhDeviceActsTest, IotcOhDeviceActsTest003, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhDeviceActsTest004
 * @tc.number IotcOhDeviceActsTest004
 * @tc.desc   Test report device char state after init and main success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhDeviceActsTest, IotcOhDeviceActsTest004, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhDevReportCharState(&g_repChar, 1);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhDeviceActsTest005
 * @tc.number IotcOhDeviceActsTest005
 * @tc.desc   Test report device char state before init returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhDeviceActsTest, IotcOhDeviceActsTest005, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevReportCharState(&g_repChar, 1);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhDeviceActsTest006
 * @tc.number IotcOhDeviceActsTest006
 * @tc.desc   Test report device char state after init without main returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhDeviceActsTest, IotcOhDeviceActsTest006, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhDevReportCharState(&g_repChar, 1);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhDeviceActsTest007
 * @tc.number IotcOhDeviceActsTest007
 * @tc.desc   Test report device char state after main without init returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhDeviceActsTest, IotcOhDeviceActsTest007, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhDevReportCharState(&g_repChar, 1);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhDeviceActsTest008
 * @tc.number IotcOhDeviceActsTest008
 * @tc.desc   Test report device char state with NULL state returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhDeviceActsTest, IotcOhDeviceActsTest008, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevReportCharState(NULL, 1);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhDeviceActsTest009
 * @tc.number IotcOhDeviceActsTest009
 * @tc.desc   Test report device char state with zero num returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhDeviceActsTest, IotcOhDeviceActsTest009, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevReportCharState(&g_repChar, 0);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhDeviceActsTest010
 * @tc.number IotcOhDeviceActsTest010
 * @tc.desc   Test report device char state with NULL svc id returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhDeviceActsTest, IotcOhDeviceActsTest010, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevReportCharState(&g_repChar1, 0);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhDeviceActsTest011
 * @tc.number IotcOhDeviceActsTest011
 * @tc.desc   Test report device char state with NULL data returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhDeviceActsTest, IotcOhDeviceActsTest011, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevReportCharState(&g_repChar2, 0);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhDeviceActsTest012
 * @tc.number IotcOhDeviceActsTest012
 * @tc.desc   Test report device char state with zero data length returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhDeviceActsTest, IotcOhDeviceActsTest012, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevReportCharState(&g_repChar3, 0);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

RUN_TEST_SUITE(IotcOhDeviceActsTest);