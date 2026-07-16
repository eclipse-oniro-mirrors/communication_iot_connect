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
#include "iotc_oh_ble.h"
#include "iotc_oh_device.h"
#include "iotc_oh_sdk.h"

#define TEST_SVC_UUID "16F1E600A27743FCA484DD39EF8A9100"
#define TEST_SVC_INDICATE_UUID "16F1E601A27743FCA484DD39EF8A9100"

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
        sleep(1);
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
        sleep(1);
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
    .manuId = "123",
    .manuName = "Manu Name",
    .devName = "One Connect Dev Name",
    .fwv = "1.0.0",
    .hwv = "1.0.0",
    .swv = "1.0.0",
    .protType = IOTC_PROT_TYPE_BLE,
};

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

static int32_t RecvNetCfgInfoCallback(const char *netInfo, uint32_t len)
{
    return 0;
}

static int32_t RecvCustomSecDataCallback(const uint8_t *data, uint32_t len)
{
    return 0;
}

/**
 * @tc.desc      : register a test suite, this suite is used to test function
 * @param        : subsystem name is communication
 * @param        : module name is lwip
 * @param        : test suit name is LwipFuncTestSuite
 */
LITE_TEST_SUIT(communication, iot_connect, IotcOhBleActsTest);

/**
 * @tc.setup     : setup for every testcase
 * @return       : setup result, TRUE is success, FALSE is fail
 */
static BOOL IotcOhBleActsTestSetUp(void)
{   
    printf("IotcOhBleActsTestSetUp \r\n");
    return TRUE;
}

/**
 * @tc.teardown  : teardown for every testcase
 * @return       : teardown result, TRUE is success, FALSE is fail
 */
static BOOL IotcOhBleActsTestTearDown(void)
{   
    printf("IotcOhBleActsTestTearDown \r\n");
    return TRUE;
}


/**
 * @tc.name   IotcOhBleActsTest001
 * @tc.number IotcOhBleActsTest001
 * @tc.desc   Test BLE module enable, run main and disable success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest001, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &DEV_INFO);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ClearIotcBleSvcStart();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcBleSvcStart();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest002
 * @tc.number IotcOhBleActsTest002
 * @tc.desc   Test enable BLE before main start returns already running error.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest002, Function | MediumTest | Level2)
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
    ret = IotcOhBleEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_CORE_COMM_FWK_ERR_MAIN_ALREADY_RUNNING);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest003
 * @tc.number IotcOhBleActsTest003
 * @tc.desc   Test disable BLE without main start returns already running error.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest003, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_CORE_COMM_FWK_ERR_MAIN_ALREADY_RUNNING);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest004
 * @tc.number IotcOhBleActsTest004
 * @tc.desc   Test start BLE adv after release returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest004, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleRelease();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleStartAdv(1);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest005
 * @tc.number IotcOhBleActsTest005
 * @tc.desc   Test release BLE without enable returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest005, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhBleRelease();
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest006
 * @tc.number IotcOhBleActsTest006
 * @tc.desc   Test release BLE during main run returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest006, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleRelease();
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest007
 * @tc.number IotcOhBleActsTest007
 * @tc.desc   Test release BLE with enable but without main returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest007, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleRelease();
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhBleDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest008
 * @tc.number IotcOhBleActsTest008
 * @tc.desc   Test release BLE after main stop returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest008, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleRelease();
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhBleDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest009
 * @tc.number IotcOhBleActsTest009
 * @tc.desc   Test start BLE adv with 1ms duration success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest009, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &DEV_INFO);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ClearIotcBleSvcStart();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcBleSvcStart();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleStartAdv(1);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleRelease();
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhBleDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest010
 * @tc.number IotcOhBleActsTest010
 * @tc.desc   Test start BLE adv with 0ms duration and stop success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest010, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &DEV_INFO);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ClearIotcBleSvcStart();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcBleSvcStart();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleStartAdv(0);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleStopAdv();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleRelease();
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhBleDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest011
 * @tc.number IotcOhBleActsTest011
 * @tc.desc   Test start BLE adv with UINT32_MAX duration and stop success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest011, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &DEV_INFO);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ClearIotcBleSvcStart();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcBleSvcStart();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleStartAdv(UINT32_MAX);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleStopAdv();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleRelease();
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhBleDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest012
 * @tc.number IotcOhBleActsTest012
 * @tc.desc   Test start BLE adv consecutively with different durations success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest012, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &DEV_INFO);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ClearIotcBleSvcStart();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcBleSvcStart();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleStartAdv(UINT32_MAX);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleStartAdv(1);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleStartAdv(0);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleStopAdv();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleRelease();
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhBleDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}


/**
 * @tc.name   IotcOhBleActsTest013
 * @tc.number IotcOhBleActsTest013
 * @tc.desc   Test start BLE adv with 1ms duration and stop success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest013, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &DEV_INFO);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ClearIotcBleSvcStart();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcBleSvcStart();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleStartAdv(1);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleStopAdv();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleRelease();
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhBleDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest014
 * @tc.number IotcOhBleActsTest014
 * @tc.desc   Test set device info with empty IotcDeviceInfo returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest014, Function | MediumTest | Level2)
{
    int ret = 0;
    static const IotcDeviceInfo devInfo = {0};
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &devInfo);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest015
 * @tc.number IotcOhBleActsTest015
 * @tc.desc   Test stop BLE adv twice in sequence success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest015, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhDevInit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &DEV_INFO);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ClearIotcBleSvcStart();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcBleSvcStart();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleStopAdv();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleStopAdv();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleRelease();
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhBleDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhDevDeinit();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest016
 * @tc.number IotcOhBleActsTest016
 * @tc.desc   Test stop BLE adv without enable returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest016, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhBleStopAdv();
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest017
 * @tc.number IotcOhBleActsTest017
 * @tc.desc   Test stop BLE adv after enable without main returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest017, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleStopAdv();
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhBleDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest018
 * @tc.number IotcOhBleActsTest018
 * @tc.desc   Test stop BLE adv after main start without peer returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest018, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleStopAdv();
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest019
 * @tc.number IotcOhBleActsTest019
 * @tc.desc   Test send BLE custom sec data after enable and main success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest019, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleSendCustomSecData((unsigned char *)"1", 1);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhBleSendCustomSecData((unsigned char *)"1", 1);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest020
 * @tc.number IotcOhBleActsTest020
 * @tc.desc   Test send BLE custom sec data without enable returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest020, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhBleSendCustomSecData((unsigned char *)"1", 1);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest021
 * @tc.number IotcOhBleActsTest021
 * @tc.desc   Test send BLE custom sec data after enable without main returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest021, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleSendCustomSecData((unsigned char *)"1", 1);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhBleDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest022
 * @tc.number IotcOhBleActsTest022
 * @tc.desc   Test send BLE custom sec data after main without peer returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest022, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleSendCustomSecData((unsigned char *)"1", 1);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest023
 * @tc.number IotcOhBleActsTest023
 * @tc.desc   Test send BLE custom sec data with NULL data returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest023, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhBleSendCustomSecData(NULL, 1);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest024
 * @tc.number IotcOhBleActsTest024
 * @tc.desc   Test send BLE custom sec data with zero length returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest024, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhBleSendCustomSecData((unsigned char *)"1", 0);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest025
 * @tc.number IotcOhBleActsTest025
 * @tc.desc   Test send BLE indicate data twice after main success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest025, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleSendIndicateData(TEST_SVC_UUID, TEST_SVC_INDICATE_UUID, (unsigned char *)"1", 1);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhBleSendIndicateData(TEST_SVC_UUID, TEST_SVC_INDICATE_UUID, (unsigned char *)"1", 1);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest026
 * @tc.number IotcOhBleActsTest026
 * @tc.desc   Test send BLE indicate data without enable returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest026, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhBleSendIndicateData(TEST_SVC_UUID, TEST_SVC_INDICATE_UUID, (unsigned char *)"1", 1);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest027
 * @tc.number IotcOhBleActsTest027
 * @tc.desc   Test send BLE indicate data after enable without main returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest027, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleSendIndicateData(TEST_SVC_UUID, TEST_SVC_INDICATE_UUID, (unsigned char *)"1", 1);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhBleDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest028
 * @tc.number IotcOhBleActsTest028
 * @tc.desc   Test send BLE indicate data twice after main without peer returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest028, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ClearIotcInitivalized();
    ret = IotcOhMain();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = WaitIotcInitivalized();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleSendIndicateData(TEST_SVC_UUID, TEST_SVC_INDICATE_UUID, (unsigned char *)"1", 1);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhBleSendIndicateData(TEST_SVC_UUID, TEST_SVC_INDICATE_UUID, (unsigned char *)"1", 1);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhStop();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest029
 * @tc.number IotcOhBleActsTest029
 * @tc.desc   Test send BLE indicate data with NULL svc uuid returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest029, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhBleSendIndicateData(NULL, TEST_SVC_INDICATE_UUID, (unsigned char *)"1", 1);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest030
 * @tc.number IotcOhBleActsTest030
 * @tc.desc   Test send BLE indicate data with NULL char uuid returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest030, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhBleSendIndicateData(TEST_SVC_UUID, NULL, (unsigned char *)"1", 1);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest031
 * @tc.number IotcOhBleActsTest031
 * @tc.desc   Test send BLE indicate data with NULL value returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest031, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhBleSendIndicateData(TEST_SVC_UUID, TEST_SVC_INDICATE_UUID, NULL, 1);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest032
 * @tc.number IotcOhBleActsTest032
 * @tc.desc   Test send BLE indicate data with zero length returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest032, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhBleSendIndicateData(TEST_SVC_UUID, TEST_SVC_INDICATE_UUID, (unsigned char *)"1", 0);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest033
 * @tc.number IotcOhBleActsTest033
 * @tc.desc   Test register BLE netcfg receive callback success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest033, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_RECV_NETCFG_CALLBACK, RecvNetCfgInfoCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest034
 * @tc.number IotcOhBleActsTest034
 * @tc.desc   Test register BLE netcfg receive callback with NULL returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest034, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_RECV_NETCFG_CALLBACK, NULL);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhBleDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest035
 * @tc.number IotcOhBleActsTest035
 * @tc.desc   Test register BLE netcfg receive callback without enable returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest035, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_RECV_NETCFG_CALLBACK, RecvNetCfgInfoCallback);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest036
 * @tc.number IotcOhBleActsTest036
 * @tc.desc   Test register BLE custom data receive callback success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest036, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_RECV_CUSTOM_DATA_CALLBACK, RecvCustomSecDataCallback);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest037
 * @tc.number IotcOhBleActsTest037
 * @tc.desc   Test register BLE custom data receive callback with NULL returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest037, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_RECV_CUSTOM_DATA_CALLBACK, NULL);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhBleDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest038
 * @tc.number IotcOhBleActsTest038
 * @tc.desc   Test register BLE custom data receive callback without enable returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest038, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_RECV_CUSTOM_DATA_CALLBACK, RecvCustomSecDataCallback);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest039
 * @tc.number IotcOhBleActsTest039
 * @tc.desc   Test set BLE startup adv timeout with valid value success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest039, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    uint32_t timeout = 60000;
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_START_UP_ADV_TIMEOUT, timeout);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest040
 * @tc.number IotcOhBleActsTest040
 * @tc.desc   Test set BLE startup adv timeout with zero success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest040, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    uint32_t timeout = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_START_UP_ADV_TIMEOUT, timeout);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest041
 * @tc.number IotcOhBleActsTest041
 * @tc.desc   Test set BLE startup adv timeout without enable returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest041, Function | MediumTest | Level2)
{
    int ret = 0;
    uint32_t timeout = 60000;
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_START_UP_ADV_TIMEOUT, timeout);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest042
 * @tc.number IotcOhBleActsTest042
 * @tc.desc   Test set BLE GATT profile service list success.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest042, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_GATT_PROFILE_SVC_LIST, &g_bleGattSvcList);
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhBleDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest043
 * @tc.number IotcOhBleActsTest043
 * @tc.desc   Test set BLE GATT profile service list with NULL returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest043, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_GATT_PROFILE_SVC_LIST, NULL);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    ret = IotcOhBleDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest044
 * @tc.number IotcOhBleActsTest044
 * @tc.desc   Test set BLE GATT profile service list with NULL svc returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest044, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    g_bleGattSvcList.svc = NULL;
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_GATT_PROFILE_SVC_LIST, &g_bleGattSvcList);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_bleGattSvcList.svc = g_bleGattSvcBuf;
    ret = IotcOhBleDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest045
 * @tc.number IotcOhBleActsTest045
 * @tc.desc   Test set BLE GATT profile service list with zero svc num returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest045, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhBleEnable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
    g_bleGattSvcList.svcNum = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_GATT_PROFILE_SVC_LIST, &g_bleGattSvcList);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
    g_bleGattSvcList.svcNum = sizeof(g_bleGattSvcBuf) / sizeof(IotcBleGattProfileSvc);
    ret = IotcOhBleDisable();
    TEST_ASSERT_EQUAL_INT(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest046
 * @tc.number IotcOhBleActsTest046
 * @tc.desc   Test set BLE GATT profile service list without enable returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest046, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_GATT_PROFILE_SVC_LIST, &g_bleGattSvcList);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

/**
 * @tc.name   IotcOhBleActsTest047
 * @tc.number IotcOhBleActsTest047
 * @tc.desc   Test set BLE GATT profile service list without enable returns failure.
 * @tc.type   FUNCTION
 * @tc.size   MEDIUMTEST
 * @tc.level  LEVEL2
 */
LITE_TEST_CASE(IotcOhBleActsTest, IotcOhBleActsTest047, Function | MediumTest | Level2)
{
    int ret = 0;
    ret = IotcOhSetOption(IOTC_OH_OPTION_BLE_GATT_PROFILE_SVC_LIST, &g_bleGattSvcList);
    TEST_ASSERT_NOT_EQUAL(ret, IOTC_OK);
}

RUN_TEST_SUITE(IotcOhBleActsTest);