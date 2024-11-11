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

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "iotc_oh_wifi.h"
#include "iotc_oh_ble.h"
#include "iotc_oh_sdk.h"
#include "iotc_oh_device.h"
#include "securec.h"
#include "cJSON.h"
#include "iotc_conf.h"
#include "iotc_event.h"
#include "iotc_errcode.h"
#include <gtest/gtest.h>
#include "securec.h"
#include "cJSON.h"

#define DEMO_LOG(...) do { \
        printf("DEMO[%s:%u]", __func__, __LINE__); \
        printf(__VA_ARGS__); \
        printf("\r\n"); \
    } while (0)

/**
 * [MUST] MODIFY ME
 * DEV_INFO为产品信息，应与云/APP侧配置一致
 */
static const IotcDeviceInfo DEV_INFO = {
    .sn = "12345678",
    .prodId = "12345",
    .subProdId = "",
    .model = "MODEL",
    .devTypeId = "1234",
    .devTypeName = "DevTypeName",
    .manuId = "123",
    .manuName = "ManuName",
    .fwv = "1.0.0",
    .hwv = "1.0.0",
    .swv = "1.0.0",
    .protType = IOTC_PROT_TYPE_BLE,
};

/**
 * [MUST] MODIFY ME
 * SVC_INFO为产品服务信息，应与云/APP侧配置一致
 */
static const IotcServiceInfo SVC_INFO[] = {
    {"switch", "switch"},
    {"report", "report"},
};

/**
 * [MUST] MODIFY ME
 * PIN_CODE为产品使用的配网PIN码，应与APP侧配置一致
 */
static const char *PIN_CODE = "01234567";

/**
 * [MUST] MODIFY ME
 * AC_KEY为产品厂商的AC KEY，应与云侧配置一致
 */
const uint8_t AC_KEY[IOTC_AC_KEY_LEN] = {
    0x49, 0x3F, 0x45, 0x4A, 0x3A, 0x72, 0x38, 0x7B, 0x36, 0x32, 0x50, 0x3C, 0x49, 0x39, 0x62, 0x38,
    0x72, 0xCB, 0x6D, 0xC5, 0xAE, 0xE5, 0x4A, 0x82, 0xD3, 0xE5, 0x6D, 0xF5, 0x36, 0x82, 0x62, 0xEB,
    0x89, 0x30, 0x6C, 0x88, 0x32, 0x56, 0x23, 0xFD, 0xB8, 0x67, 0x90, 0xA7, 0x7B, 0x61, 0x1E, 0xAE
};

/**
 * [MUST] MODIFY ME
 * 下面代码为开关服务的实现样例
 * SwitchPutCharState函数为控制指令的处理，控制指令报文为{"on":1或0}
 * SwitchGetCharState函数为查询指令的处理
 * g_switch保存了开关的状态
 * 产品可以参考该DEMO并实现自己的服务函数添加到SVC_MAP中
 */
static bool g_switch = false;
static bool g_report = false;

enum {
    BLE_ONLY_TEST_MODE1,
    BLE_ONLY_TEST_MODE2,
    BLE_ONLY_TEST_MODE3,
    BLE_ONLY_TEST_MODE4,
};

static int32_t g_testMode = BLE_ONLY_TEST_MODE1;
enum {
    STEP_1 = 0, /* putcharstate/customsecdata switch-on 1 */
    STEP_2, /* putcharstate/customsecdata switch-on 0 */
    STEP_3, /* putcharstate/customsecdata report-on 0 */

    STEP_MAX,
};

static int32_t g_step[STEP_MAX] = {0};
#define ONE_THOUSAND 1000

static bool g_iotcInitalizedEvent = false;
static bool g_iotcBleSvcStartEvent = false;

static void ClearStep(void)
{
    g_switch = false;
    g_report = false;
    memset_s(&g_step, sizeof(g_step), 0, sizeof(g_step));
}

static bool CheckStepFinish(void)
{
    for (uint32_t i = 0; i < STEP_MAX; i++) {
        if (g_step[i] == 0) {
            return false;
        }
    }

    return true;
}

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

static int32_t WaitIotcInitivalized(void)
{
    int32_t cnt = ONE_THOUSAND;
    while (cnt > 0) {
        if (g_iotcInitalizedEvent) {
            return IOTC_OK;
        }
        cnt--;
        usleep(ONE_THOUSAND);
    }
    return IOTC_ERROR;
}

static void ClearIotcBleSvcStart(void)
{
    g_iotcBleSvcStartEvent = false;
}

static int32_t WaitIotcBleSvcStart(void)
{
    int32_t cnt = ONE_THOUSAND;
    while (cnt > 0) {
        if (g_iotcBleSvcStartEvent) {
            return IOTC_OK;
        }
        cnt--;
        usleep(ONE_THOUSAND);
    }
    return IOTC_ERROR;
}

static void ReportTest(void)
{
    if (g_testMode == BLE_ONLY_TEST_MODE1 || g_testMode == BLE_ONLY_TEST_MODE2) {
        IotcCharState reportInfo;
        (void)memset_s(&reportInfo, sizeof(reportInfo), 0, sizeof(reportInfo));
        reportInfo.svcId = SVC_INFO[1].svcId;
        reportInfo.data = "{\"on\":1}";
        reportInfo.len = strlen(reportInfo.data);
        IotcOhDevReportCharState(&reportInfo, sizeof(reportInfo) / sizeof(reportInfo));
    } else {
        const char *send = "{\"vendor\":[{\"sid\":\"report\",\"data\":{\"on\":1}}],\"seq\":0}";
        IotcOhBleSendCustomSecData((uint8_t *)send, strlen(send));
    }
}

static void CustomSecDataSendStatus(const char *sid, int status)
{
    const char *switch0 = "{\"vendor\":[{\"sid\":\"switch\",\"data\":{\"on\":0}}],\"seq\":0}";
    const char *switch1 = "{\"vendor\":[{\"sid\":\"switch\",\"data\":{\"on\":1}}],\"seq\":0}";
    const char *report0 = "{\"vendor\":[{\"sid\":\"report\",\"data\":{\"on\":0}}],\"seq\":0}";
    const char *report1 = "{\"vendor\":[{\"sid\":\"report\",\"data\":{\"on\":1}}],\"seq\":0}";
    char *send = NULL;
    if (strcmp(sid, "switch") == 0) {
        if (status == 0) {
            send = (char *)switch0;
        } else if (status == 1) {
            send = (char *)switch1;
        }
    } else if (strcmp(sid, "report") == 0) {
        if (status == 0) {
            send = (char *)report0;
        } else if (status == 1) {
            send = (char *)report1;
        }
    }
    IotcOhBleSendCustomSecData((uint8_t *)send, strlen(send));
}

static void CustomSecDataPutOn(cJSON *json)
{
    cJSON *item = cJSON_GetObjectItem(json, "on");
    if (item == NULL || !cJSON_IsNumber(item)) {
        cJSON_Delete(json);
        DEMO_LOG("get on error");
        return;
    }

    int32_t on = cJSON_GetNumberValue(item);
    g_switch = on == 0 ? false : true;
    DEMO_LOG("switch on put %d=>%d", g_switch, on);
    if (on == 0) {
        g_step[STEP_2] = 1;
    } else if (on == 1) {
        g_step[STEP_1] = 1;
    }
    CustomSecDataSendStatus("switch", g_switch);
}

static void CustomSecDataPutReport(cJSON *json)
{
    cJSON *item = cJSON_GetObjectItem(json, "on");
    if (item == NULL || !cJSON_IsNumber(item)) {
        cJSON_Delete(json);
        DEMO_LOG("get on error");
        return;
    }

    int32_t on = cJSON_GetNumberValue(item);
    g_report = on == 0 ? false : true;
    DEMO_LOG("switch report put %d=>%d", g_report, on);
    if (on == 0) {
        g_step[STEP_3] = 1;
    }
    CustomSecDataSendStatus("report", g_report);
}

static int32_t RecvCustomSecDataCallback(const uint8_t *data, uint32_t len)
{
    DEMO_LOG("receive len %u=>%s", len, data);
    cJSON *root = cJSON_Parse((const char *) data);
    if (root == NULL) {
        DEMO_LOG("cjson parse err");
        return -1;
    }
    int32_t arrSize = cJSON_GetArraySize(root);
    if (arrSize < 0) {
        DEMO_LOG("no arry");
        return -1;
    }
    for (int32_t i = 0; i < arrSize; ++i) {
        cJSON *cur = cJSON_GetArrayItem(root, i);
        if (cur == NULL) {
            DEMO_LOG("get arry err");
            break;
        }
        cJSON *dataItem = cJSON_GetObjectItem(cur, "data");
        cJSON *sidItem = cJSON_GetObjectItem(cur, "sid");
        if (dataItem != NULL && sidItem != NULL && sidItem->valuestring != NULL) {
            char *sid = sidItem->valuestring;
            DEMO_LOG("sid=%s", sid);
            if (strcmp(sid, "switch") == 0) {
                CustomSecDataPutOn(dataItem);
            } else if (strcmp(sid, "report") == 0) {
                CustomSecDataPutReport(dataItem);
            }
        }
    }
    
    cJSON_Delete(root);
    return 0;
}

static void PutCharStatePutReport(cJSON *json)
{
    cJSON *item = cJSON_GetObjectItem(json, "on");
    if (item == NULL || !cJSON_IsNumber(item)) {
        cJSON_Delete(json);
        DEMO_LOG("get on error");
        return;
    }

    int32_t on = cJSON_GetNumberValue(item);
    DEMO_LOG("switch report put %d=>%d", g_switch, on);
    g_report = on == 0 ? false : true;
    if (on == 0) {
        g_step[STEP_3] = 1;
    }
}

static void PutCharStatePutOn(cJSON *json)
{
    cJSON *item = cJSON_GetObjectItem(json, "on");
    if (item == NULL || !cJSON_IsNumber(item)) {
        cJSON_Delete(json);
        DEMO_LOG("get on error");
        return;
    }

    int32_t on = cJSON_GetNumberValue(item);
    DEMO_LOG("switch on put %d=>%d", g_switch, on);
    g_switch = on == 0 ? false : true;
    if (on == 0) {
        g_step[STEP_2] = 1;
    } else {
        g_step[STEP_1] = 1;
    }
}

static int32_t SwitchPutCharState(const IotcServiceInfo *svc, const char *data, uint32_t len)
{
    if (data == NULL || len == 0) {
        DEMO_LOG("param invalid");
        return -1;
    }
    cJSON *json = cJSON_Parse(data);
    if (json == NULL) {
        DEMO_LOG("parse error");
        return -1;
    }

    PutCharStatePutOn(json);

    cJSON_Delete(json);
    return 0;
}

static int32_t SwitchGetCharState(const IotcServiceInfo *svc, char **data, uint32_t *len)
{
    if (data == NULL || *data != NULL) {
        DEMO_LOG("param invalid");
        return -1;
    }

    cJSON *json = cJSON_CreateObject();
    if (json == NULL) {
        DEMO_LOG("create obj error");
        return -1;
    }
    if (cJSON_AddNumberToObject(json, "on", g_switch) == NULL) {
        cJSON_Delete(json);
        DEMO_LOG("add num error");
        return -1;
    }
    *data = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    if (*data == NULL) {
        DEMO_LOG("json print error");
        return -1;
    }
    DEMO_LOG("switch on get %d", g_switch);
    *len = strlen(*data);
    return 0;
}

static int32_t ReportPutCharState(const IotcServiceInfo *svc, const char *data, uint32_t len)
{
    if (data == NULL || len == 0) {
        DEMO_LOG("param invalid");
        return -1;
    }
    cJSON *json = cJSON_Parse(data);
    if (json == NULL) {
        DEMO_LOG("parse error");
        return -1;
    }

    PutCharStatePutReport(json);

    cJSON_Delete(json);
    return 0;
}

static int32_t ReportGetCharState(const IotcServiceInfo *svc, char **data, uint32_t *len)
{
    if (data == NULL || *data != NULL) {
        DEMO_LOG("param invalid");
        return -1;
    }

    cJSON *json = cJSON_CreateObject();
    if (json == NULL) {
        DEMO_LOG("create obj error");
        return -1;
    }
    if (cJSON_AddNumberToObject(json, "on", g_report) == NULL) {
        cJSON_Delete(json);
        DEMO_LOG("add num error");
        return -1;
    }
    *data = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    if (*data == NULL) {
        DEMO_LOG("json print error");
        return -1;
    }
    DEMO_LOG("report on get %d", g_report);
    *len = strlen(*data);
    return 0;
}

/**
 * [MUST] MODIFY ME
 * SVC_MAP为服务处理函数表
 * 其中svc为SVC_INFO中的服务指针，用于标识是哪个服务
 * putCharState为控制指令处理函数
 * getCharState为查询指令处理函数
 * 开发者应实现自己的服务处理函数并添加到该表中
 */
const struct SvcMap {
    const IotcServiceInfo *svc;
    int32_t (*putCharState)(const IotcServiceInfo *svc, const char *data, uint32_t len);
    int32_t (*getCharState)(const IotcServiceInfo *svc, char **data, uint32_t *len);
} SVC_MAP[] = {
    {&SVC_INFO[0], SwitchPutCharState, SwitchGetCharState},
    {&SVC_INFO[1], ReportPutCharState, ReportGetCharState},
};

/**
 * [SHOULD] MODIFY ME
 * PutCharState为注册到iot connect中的控制指令处理业务回调
 * 当收到控制指令时，该函数会通过SVC_MAP进行SID维度的控制分发
 */
static int32_t PutCharState(const IotcCharState state[], uint32_t num)
{
    if (state == NULL || num == 0) {
        DEMO_LOG("param invalid");
        return -1;
    }

    int32_t ret = 0;
    for (uint32_t i = 0; i < num; ++i) {
        for (uint32_t j = 0; j < (sizeof(SVC_MAP) / sizeof(SVC_MAP[0])); ++j) {
            DEMO_LOG("put char sid:%s data:%s", state[i].svcId, state[i].data);
            if (strcmp(state[i].svcId, SVC_MAP[j].svc->svcId) != 0 || SVC_MAP[j].putCharState == NULL) {
                continue;
            }
            int32_t curRet = SVC_MAP[j].putCharState(SVC_MAP[j].svc, state[i].data, state[i].len);
            if (curRet != 0) {
                ret = curRet;
                DEMO_LOG("put char sid:%s error %d", state[i].svcId, ret);
            }
        }
    }
    return ret;
}

/**
 * [SHOULD] MODIFY ME
 * GetCharState为注册到iot connect中的查询指令处理业务回调
 * 当收到查询指令时，该函数会通过SVC_MAP进行SID维度的查询分发
 */
static int32_t GetCharState(const IotcCharState state[], char *out[], uint32_t len[], uint32_t num)
{
    if (state == NULL || num == 0 || out == NULL || len == NULL) {
        DEMO_LOG("param invalid");
        return -1;
    }

    int32_t ret = 0;
    for (uint32_t i = 0; i < num; ++i) {
        for (uint32_t j = 0; j < (sizeof(SVC_MAP) / sizeof(SVC_MAP[0])); ++j) {
            DEMO_LOG("get char sid:%s", state[i].svcId);
            if (strcmp(state[i].svcId, SVC_MAP[j].svc->svcId) != 0 || SVC_MAP[j].getCharState == NULL) {
                continue;
            }
            int32_t curRet = SVC_MAP[j].getCharState(SVC_MAP[j].svc, &out[i], &len[i]);
            if (curRet != 0) {
                ret = curRet;
                DEMO_LOG("get char sid:%s error %d", state[i].svcId, ret);
            }
        }
    }

    return ret;
}

/**
 * [SHOULD] MODIFY ME
 * ReportAll为注册到iot connect中的全量服务上报业务回调
 * 当设备上线时会通过该接口上报所有可上报服务，同步端侧和APP/云侧服务状态
 */
static int32_t ReportAll(void)
{
    IotcCharState reportInfo[sizeof(SVC_MAP) / sizeof(SVC_MAP[0])];
    int32_t ret;
    (void)memset_s(reportInfo, sizeof(reportInfo), 0, sizeof(reportInfo));
    for (uint32_t i = 0; i < (sizeof(SVC_MAP) / sizeof(SVC_MAP[0])); ++i) {
        reportInfo[i].svcId = SVC_MAP[i].svc->svcId;
        ret = SVC_MAP[i].getCharState(SVC_MAP[i].svc, (char **)&reportInfo[i].data, &reportInfo[i].len);
        if (ret != 0) {
            DEMO_LOG("get char sid:%s error %d", reportInfo[i].svcId, ret);
            break;
        }
    }
    if (ret == 0) {
        ret = IotcOhDevReportCharState(reportInfo, sizeof(reportInfo) / sizeof(reportInfo[0]));
    }

    for (uint32_t i = 0; i < (sizeof(SVC_MAP) / sizeof(SVC_MAP[0])); ++i) {
        if (reportInfo[i].data != NULL) {
            cJSON_free((char *)reportInfo[i].data);
            reportInfo[i].data = NULL;
        }
    }
    return ret;
}

/**
 * [SHOULD] MODIFY ME
 * GetPincode为注册到iot connect中的获取PIN码回调
 * 设备配网或执行进场控制时，会通过该接口获取PIN码用来和APP协商会话秘钥
 */
static int32_t GetPincode(uint8_t *buf, uint32_t bufLen)
{
    if (buf == NULL || bufLen > IOTC_PINCODE_LEN) {
        DEMO_LOG("param invalid");
        return -1;
    }

    int32_t ret = memcpy_s(buf, bufLen, PIN_CODE, strlen(PIN_CODE));
    if (ret != EOK) {
        return -1;
    }
    return 0;
}

/**
 * [SHOULD] MODIFY ME
 * GetAcKey为注册到iot connect中的获取AC KEY回调
 */
static int32_t GetAcKey(uint8_t *buf, uint32_t bufLen)
{
    if (buf == NULL || bufLen > IOTC_AC_KEY_LEN) {
        DEMO_LOG("param invalid");
        return -1;
    }
    int32_t ret = memcpy_s(buf, bufLen, AC_KEY, sizeof(AC_KEY));
    if (ret != EOK) {
        return -1;
    }
    return 0;
}

/**
 * [MUST] MODIFY ME
 * NoticeReboot为注册到iot connect中的重启回调，开发者应在通知重启时重启设备或进程
 */
static int32_t NoticeReboot(IotcRebootReason res)
{
    DEMO_LOG("notice reboot res %d", res);
    return 0;
}

#define SET_OH_SDK_OPTION(ret, option, ...) \
    do { \
        (ret) = IotcOhSetOption((option), __VA_ARGS__); \
        if ((ret) != 0) { \
            DEMO_LOG("set option %d error %d", (option), (ret)); \
            return ret; \
        } \
    } while (0)

/**
 * [MUST] USE ME
 * IotcOhDemoEntry为iot connect业务入口，开发者应在业务进程启动时调用
 */
static int32_t IotcOhDemoEntry(void)
{
    /* 初始化设备信息模块 */
    int32_t ret = IotcOhDevInit();
    if (ret != 0) {
        DEMO_LOG("init device error %d", ret);
        return ret;
    }

    /* 初始化BLE发现控制模块 */
    ret = IotcOhBleEnable();
    if (ret != 0) {
        DEMO_LOG("enable ble connect error %d", ret);
        return ret;
    }

    /* 配置iot connect必要的回调 */
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, TestIotcEventCallback);
    if (g_testMode == BLE_ONLY_TEST_MODE4) {
        SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_BLE_RECV_CUSTOM_DATA_CALLBACK, RecvCustomSecDataCallback);
    }
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_PUT_CHAR_STATE_CALLBACK, PutCharState);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_GET_CHAR_STATE_CALLBACK, GetCharState);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_REPORT_ALL_CALLBACK, ReportAll);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_GET_PINCODE_CALLBACK, GetPincode);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_GET_AC_KEY_CALLBACK, GetAcKey);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_DATA_FREE_CALLBACK, cJSON_free);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_REBOOT_CALLBACK, NoticeReboot);
    /* 配置设备信息与服务信息 */
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_DEV_INFO, &DEV_INFO);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_SVC_INFO, SVC_INFO, sizeof(SVC_INFO) / sizeof(SVC_INFO[0]));
    /* 启动后的广播超时时间 */
    if (g_testMode == BLE_ONLY_TEST_MODE1 || g_testMode == BLE_ONLY_TEST_MODE3) {
        SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_BLE_START_UP_ADV_TIMEOUT, (10 * 60 * 1000));
    }
    /* 拉起iot connect业务线程 */
    ret = IotcOhMain();
    if (ret != 0) {
        DEMO_LOG("iotc oh main error %d", ret);
        return ret;
    }
    DEMO_LOG("iotc oh main success");
    return ret;
}

/**
 * [SHOULD] USE ME
 * IotcOhDemoExit为iot connect业务退出函数，开发者应在不使用相关业务并需要释放iot connect资源时调用
 */
static void IotcOhDemoExit(void)
{
    int32_t ret = IotcOhStop();
    if (ret != 0) {
        DEMO_LOG("iotc stop error %d", ret);
    }

    ret = IotcOhBleDisable();
    if (ret != 0) {
        DEMO_LOG("iotc ble disable error %d", ret);
    }

    ret = IotcOhDevDeinit();
    if (ret != 0) {
        DEMO_LOG("iotc dev info deinit error %d", ret);
    }
}

using namespace testing::ext;

class IotcBleOnlyDctsTest : public testing::Test {
public:
    IotcBleOnlyDctsTest()
    {}
    ~IotcBleOnlyDctsTest()
    {}
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override
    {}
    void TearDown() override
    {}
};

void IotcBleOnlyDctsTest::SetUpTestCase(void)
{
    /* Is Bluetooth turned on */
}

void IotcBleOnlyDctsTest::TearDownTestCase(void)
{
}

#define ONE_THOUSAND 1000

static int32_t WaitStepFinish(void)
{
    bool report = false;
    uint32_t cnt = 60 * 1000;
    while (cnt > 0) {
        if (!report && g_step[STEP_2] == 1) {
            usleep(ONE_THOUSAND * ONE_THOUSAND);
            ReportTest();
            report = true;
        }
        if (CheckStepFinish()) {
            usleep(ONE_THOUSAND * ONE_THOUSAND);
            return IOTC_OK;
        }
        cnt--;
        usleep(ONE_THOUSAND);
    }
    return IOTC_ERROR;
}

/**
 * @tc.name: IotcBleOnlyDctsTest001
 * @tc.desc: Test iot connect ble.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcBleOnlyDctsTest, IotcBleOnlyDctsTest001, TestSize.Level1)
{
    g_testMode = BLE_ONLY_TEST_MODE1;
    ClearStep();
    ClearIotcInitivalized();
    ClearIotcBleSvcStart();
    int32_t ret = IotcOhDemoEntry();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcBleSvcStart();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitStepFinish();
    EXPECT_TRUE(ret == IOTC_OK);
    IotcOhDemoExit();
}

/**
 * @tc.name: IotcBleOnlyDctsTest002
 * @tc.desc: Test iot connect ble.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcBleOnlyDctsTest, IotcBleOnlyDctsTest002, TestSize.Level1)
{
    g_testMode = BLE_ONLY_TEST_MODE2;
    ClearStep();
    ClearIotcInitivalized();
    ClearIotcBleSvcStart();
    int32_t ret = IotcOhDemoEntry();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcBleSvcStart();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = IotcOhBleStartAdv(10 * 60 * 1000);
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitStepFinish();
    EXPECT_TRUE(ret == IOTC_OK);
    IotcOhDemoExit();
}

/**
 * @tc.name: IotcBleOnlyDctsTest003
 * @tc.desc: Test iot connect ble.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcBleOnlyDctsTest, IotcBleOnlyDctsTest003, TestSize.Level1)
{
    g_testMode = BLE_ONLY_TEST_MODE3;
    ClearStep();
    ClearIotcInitivalized();
    ClearIotcBleSvcStart();
    int32_t ret = IotcOhDemoEntry();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcBleSvcStart();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitStepFinish();
    EXPECT_TRUE(ret == IOTC_OK);
    IotcOhDemoExit();
}

/**
 * @tc.name: IotcBleOnlyDctsTest004
 * @tc.desc: Test iot connect ble.
 * @tc.in: Test module, Test number, Test levels.
 * @tc.out: Zero
 * @tc.type: FUNC
 * @tc.require: 1
 */
HWTEST_F(IotcBleOnlyDctsTest, IotcBleOnlyDctsTest004, TestSize.Level1)
{
    g_testMode = BLE_ONLY_TEST_MODE4;
    ClearStep();
    ClearIotcInitivalized();
    ClearIotcBleSvcStart();
    int32_t ret = IotcOhDemoEntry();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcInitivalized();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitIotcBleSvcStart();
    EXPECT_TRUE(ret == IOTC_OK);
    ret = WaitStepFinish();
    EXPECT_TRUE(ret == IOTC_OK);
    IotcOhDemoExit();
}