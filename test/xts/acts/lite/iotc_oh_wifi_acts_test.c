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

#include "iotc_ble_def.h"
#include "iotc_conf.h"
#include "iotc_def.h"
#include "iotc_errcode.h"
#include "iotc_event.h"
#include "iotc_prof_def.h"
#include "iotc_oh_device.h"
#include "iotc_oh_sdk.h"
#include "iotc_oh_wifi.h"

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

static int32_t GetRootCA(const char **ca[], uint32_t *num)
{
    return IOTC_OK;
}

/**
 * @tc.desc      : register a test suite, this suite is used to test function
 * @param        : subsystem name is communication
 * @param        : module name is lwip
 * @param        : test suit name is LwipFuncTestSuite
 */
LITE_TEST_SUIT(communication, iot_connect, IotcOhWifiActsTest);

/**
 * @tc.setup     : setup for every testcase
 * @return       : setup result, TRUE is success, FALSE is fail
 */
static BOOL IotcOhWifiActsTestSetUp(void)
{
    printf("IotcOhWifiActsTestSetUp \r\n");
    return TRUE;
}

/**
 * @tc.teardown  : teardown for every testcase
 * @return       : teardown result, TRUE is success, FALSE is fail
 */
static BOOL IotcOhWifiActsTestTearDown(void)
{
    printf("IotcOhWifiActsTestTearDown \r\n");
    return TRUE;
}

/**
 * @tc.name   IotcOhWifiActsTest001
 * @tc.number IotcOhWifiActsTest001
 * @tc.desc   Test WiFi module enable, run main and disable success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest001, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
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
    ret = IotcOhWifiDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest002
 * @tc.number IotcOhWifiActsTest002
 * @tc.desc   Test enable WiFi after main start returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest002, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhWifiEnable();
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest003
 * @tc.number IotcOhWifiActsTest003
 * @tc.desc   Test disable WiFi without enable returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest003, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhWifiDisable();
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest004
 * @tc.number IotcOhWifiActsTest004
 * @tc.desc   Test set WiFi send buffer size with typical values success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest004, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    uint32_t resSize = 2048;
    uint32_t maxSize = 3072;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    resSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_SIZE;
    maxSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhWifiDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest005
 * @tc.number IotcOhWifiActsTest005
 * @tc.desc   Test set WiFi send buffer size with equal res and max success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest005, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    uint32_t resSize = 2048;
    uint32_t maxSize = 2048;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    resSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_SIZE;
    maxSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhWifiDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest006
 * @tc.number IotcOhWifiActsTest006
 * @tc.desc   Test set WiFi send buffer size with max size success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest006, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    uint32_t resSize = 2048;
    uint32_t maxSize = IOTC_CONF_WIFI_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    resSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_SIZE;
    maxSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhWifiDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest007
 * @tc.number IotcOhWifiActsTest007
 * @tc.desc   Test set WiFi send buffer size over max limit success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest007, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    uint32_t resSize = 2048;
    uint32_t maxSize = IOTC_CONF_WIFI_BUFFER_MAX_SIZE + 1024;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    resSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_SIZE;
    maxSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhWifiDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest008
 * @tc.number IotcOhWifiActsTest008
 * @tc.desc   Test set WiFi send buffer size with res greater than max returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest008, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    uint32_t resSize = 2048;
    uint32_t maxSize = 1024;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    resSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_SIZE;
    maxSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhWifiDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest009
 * @tc.number IotcOhWifiActsTest009
 * @tc.desc   Test set WiFi send buffer size with zero max returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest009, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    uint32_t resSize = 2048;
    uint32_t maxSize = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    resSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_SIZE;
    maxSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhWifiDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest010
 * @tc.number IotcOhWifiActsTest010
 * @tc.desc   Test set WiFi send buffer size with both max values success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest010, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    uint32_t resSize = IOTC_CONF_WIFI_BUFFER_MAX_SIZE;
    uint32_t maxSize = IOTC_CONF_WIFI_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    resSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_SIZE;
    maxSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhWifiDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest011
 * @tc.number IotcOhWifiActsTest011
 * @tc.desc   Test set WiFi send buffer size over both max values success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest011, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    uint32_t resSize = IOTC_CONF_WIFI_BUFFER_MAX_SIZE + 1024;
    uint32_t maxSize = IOTC_CONF_WIFI_BUFFER_MAX_SIZE + 2048;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    resSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_SIZE;
    maxSize = IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhWifiDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest012
 * @tc.number IotcOhWifiActsTest012
 * @tc.desc   Test set WiFi send buffer size with zero res returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest012, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    uint32_t resSize = 0;
    uint32_t maxSize = 1024;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhWifiDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest013
 * @tc.number IotcOhWifiActsTest013
 * @tc.desc   Test set WiFi send buffer size without enable returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest013, Function | MediumTest | Level2)
{
    int ret = 0;
    uint32_t resSize = 2048;
    uint32_t maxSize = 3072;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest014
 * @tc.number IotcOhWifiActsTest014
 * @tc.desc   Test set WiFi recv buffer size with typical values success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest014, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    uint32_t resSize = 2048;
    uint32_t maxSize = 3072;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    resSize = IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_SIZE;
    maxSize = IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhWifiDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest015
 * @tc.number IotcOhWifiActsTest015
 * @tc.desc   Test set WiFi recv buffer size with equal res and max success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest015, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    uint32_t resSize = 2048;
    uint32_t maxSize = 2048;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    resSize = IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_SIZE;
    maxSize = IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhWifiDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest016
 * @tc.number IotcOhWifiActsTest016
 * @tc.desc   Test set WiFi recv buffer size with max size success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest016, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    uint32_t resSize = 2048;
    uint32_t maxSize = IOTC_CONF_WIFI_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    resSize = IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_SIZE;
    maxSize = IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhWifiDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest017
 * @tc.number IotcOhWifiActsTest017
 * @tc.desc   Test set WiFi recv buffer size over max limit success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest017, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    uint32_t resSize = 2048;
    uint32_t maxSize = IOTC_CONF_WIFI_BUFFER_MAX_SIZE + 1024;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    resSize = IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_SIZE;
    maxSize = IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhWifiDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest018
 * @tc.number IotcOhWifiActsTest018
 * @tc.desc   Test set WiFi recv buffer size with res greater than max returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest018, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    uint32_t resSize = 2048;
    uint32_t maxSize = 1024;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhWifiDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest019
 * @tc.number IotcOhWifiActsTest019
 * @tc.desc   Test set WiFi recv buffer size with zero max returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest019, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    uint32_t resSize = 2048;
    uint32_t maxSize = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhWifiDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest020
 * @tc.number IotcOhWifiActsTest020
 * @tc.desc   Test set WiFi recv buffer size with both max values success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest020, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    uint32_t resSize = IOTC_CONF_WIFI_BUFFER_MAX_SIZE;
    uint32_t maxSize = IOTC_CONF_WIFI_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    resSize = IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_SIZE;
    maxSize = IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhWifiDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest021
 * @tc.number IotcOhWifiActsTest021
 * @tc.desc   Test set WiFi recv buffer size over both max values success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest021, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    uint32_t resSize = IOTC_CONF_WIFI_BUFFER_MAX_SIZE + 1024;
    uint32_t maxSize = IOTC_CONF_WIFI_BUFFER_MAX_SIZE + 2048;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    resSize = IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_SIZE;
    maxSize = IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_MAX_SIZE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhWifiDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest022
 * @tc.number IotcOhWifiActsTest022
 * @tc.desc   Test set WiFi recv buffer size with zero res returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest022, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    uint32_t resSize = 0;
    uint32_t maxSize = 1024;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhWifiDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest023
 * @tc.number IotcOhWifiActsTest023
 * @tc.desc   Test set WiFi recv buffer size without enable returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest023, Function | MediumTest | Level2)
{
    int ret = 0;
    uint32_t resSize = 2048;
    uint32_t maxSize = 3072;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSize, maxSize);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest024
 * @tc.number IotcOhWifiActsTest024
 * @tc.desc   Test set WiFi netcfg mode to NONE success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest024, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    int32_t mode = IOTC_NET_CONFIG_MODE_NONE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_NETCFG_MODE, mode);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhWifiDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest025
 * @tc.number IotcOhWifiActsTest025
 * @tc.desc   Test set WiFi netcfg mode to SOFTAP success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest025, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    int32_t mode = IOTC_NET_CONFIG_MODE_SOFTAP;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_NETCFG_MODE, mode);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhWifiDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest026
 * @tc.number IotcOhWifiActsTest026
 * @tc.desc   Test set WiFi netcfg mode to BLE SUP success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest026, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    int32_t mode = IOTC_NET_CONFIG_MODE_BLE_SUP;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_NETCFG_MODE, mode);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhWifiDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest027
 * @tc.number IotcOhWifiActsTest027
 * @tc.desc   Test set WiFi netcfg mode to BLE AGT success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest027, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    int32_t mode = IOTC_NET_CONFIG_MODE_BLE_AGT;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_NETCFG_MODE, mode);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhWifiDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest028
 * @tc.number IotcOhWifiActsTest028
 * @tc.desc   Test set WiFi netcfg mode to MAX returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest028, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    int32_t mode = IOTC_NET_CONFIG_MODE_MAX;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_NETCFG_MODE, mode);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhWifiDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest029
 * @tc.number IotcOhWifiActsTest029
 * @tc.desc   Test set WiFi netcfg mode beyond MAX returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest029, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    int32_t mode = IOTC_NET_CONFIG_MODE_MAX + 1;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_NETCFG_MODE, mode);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhWifiDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest030
 * @tc.number IotcOhWifiActsTest030
 * @tc.desc   Test set WiFi netcfg mode without enable returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest030, Function | MediumTest | Level2)
{
    int ret = 0;
    int32_t mode = IOTC_NET_CONFIG_MODE_NONE;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_NETCFG_MODE, mode);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest031
 * @tc.number IotcOhWifiActsTest031
 * @tc.desc   Test set WiFi netcfg timeout with valid value success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest031, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    uint32_t timeout = 60000;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_NETCFG_TIMEOUT, timeout);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhWifiDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest032
 * @tc.number IotcOhWifiActsTest032
 * @tc.desc   Test set WiFi netcfg timeout to zero success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest032, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    uint32_t timeout = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_NETCFG_TIMEOUT, timeout);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhWifiDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest033
 * @tc.number IotcOhWifiActsTest033
 * @tc.desc   Test set WiFi netcfg timeout without enable returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest033, Function | MediumTest | Level2)
{
    int ret = 0;
    uint32_t timeout = 60000;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_NETCFG_TIMEOUT, timeout);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest034
 * @tc.number IotcOhWifiActsTest034
 * @tc.desc   Test register WiFi get cert callback success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest034, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_GET_CERT_CALLBACK, GetRootCA);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhWifiDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest035
 * @tc.number IotcOhWifiActsTest035
 * @tc.desc   Test register WiFi get cert callback with NULL returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest035, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhWifiEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_GET_CERT_CALLBACK, NULL);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhWifiDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhWifiActsTest036
 * @tc.number IotcOhWifiActsTest036
 * @tc.desc   Test register WiFi get cert callback without enable returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhWifiActsTest, IotcOhWifiActsTest036, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_WIFI_GET_CERT_CALLBACK, GetRootCA);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

RUN_TEST_SUITE(IotcOhWifiActsTest);