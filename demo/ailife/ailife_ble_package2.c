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
#include "iotc_oh_ble.h"
#include "iotc_oh_wifi.h"
#include "iotc_oh_connect.h"
#include "iotc_oh_device.h"
#include "securec.h"
#include "cJSON.h"
#include "adapter_os.h"
#include "iotc_conf.h"

#define HILINK_PSK_LEN  16
#define PINCODE_LEN     8

#define ADV_TIMEOUT     UINT32_MAX

#define DEMO_LOG(...) do { \
        printf("DEMO[%s:%u]", __func__, __LINE__); \
        printf(__VA_ARGS__); \
        printf("\r\n"); \
    } while (0)


static const IotcDeviceInfo DEV_INFO = {
    .sn = "0123456788",
    .prodId = "2F6R",
    .subProdId = "",
    .model = "DL-3HW",
    .devTypeId = "046",
    .devTypeName = "Table Lamp",
    .manuId = "17C",
    .manuName = "DALEN",
    .fwv = "1.0.0",
    .hwv = "1.0.0",
    .swv = "1.0.0",
    .protType = IOTC_PROT_TYPE_BLE,
};

static const IotcServiceInfo SVC_INFO[] = {
    {"switch", "switch"},
};

const uint8_t AC_KEY[IOTC_AC_KEY_LEN] = {
    0x49, 0x3F, 0x45, 0x4A, 0x3A, 0x72, 0x38, 0x7B, 0x36, 0x32, 0x50, 0x3C, 0x49, 0x39, 0x62, 0x38,
    0x72, 0xCB, 0x6D, 0xC5, 0xAE, 0xE5, 0x4A, 0x82, 0xD3, 0xE5, 0x6D, 0xF5, 0x36, 0x82, 0x62, 0xEB,
    0x89, 0x30, 0x6C, 0x88, 0x32, 0x56, 0x23, 0xFD, 0xB8, 0x67, 0x90, 0xA7, 0x7B, 0x61, 0x1E, 0xAE
};

#define BATTERY_SVC_UUID "180F"
#define BATTERY_SVC_LEVEL_CHAR_UUID "2A19"

#define TEST_SVC_UUID "16F1E600A27743FCA484DD39EF8A9100"
#define TEST_SVC_READ_UUID "16F1E601A27743FCA484DD39EF8A9100"
#define TEST_SVC_WRITE_UUID "16F1E602A27743FCA484DD39EF8A9100"

static int32_t BleGattTestCharReadFunc(uint8_t *buff, uint32_t *len);
static int32_t BleGattTestCharWriteFunc(uint8_t *buff, uint32_t len);
static int32_t BleGattBatteryLevelCharReadFunc(uint8_t *buff, uint32_t *len);
static int32_t BleGattBatteryLevelWriteFunc(uint8_t *buff, uint32_t len);

static const IotcBleGattProfileChar g_gattTestChar[] = {
    {
        .uuid = TEST_SVC_WRITE_UUID,
        .permission = IOTC_BLE_GATT_PERMISSION_READ_ENCRYPTED | IOTC_BLE_GATT_PERMISSION_WRITE_ENCRYPTED,
        .property = IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_WRITE,
        .readFunc = NULL,
        .writeFunc = BleGattTestCharWriteFunc,
        .indicateFunc = NULL,
        .desc = NULL,
        .descNum = 0,
    },
    {
        .uuid = TEST_SVC_READ_UUID,
        .permission = IOTC_BLE_GATT_PERMISSION_READ_ENCRYPTED | IOTC_BLE_GATT_PERMISSION_WRITE_ENCRYPTED,
        .property = IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_READ | IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_INDICATE,
        .readFunc = BleGattTestCharReadFunc,
        .writeFunc = NULL,
        .indicateFunc = NULL,
        .desc = NULL,
        .descNum = 0,
    }
};

static const IotcBleGattProfileChar g_gattBatteryLevelChar[] = {
    {
        .uuid = BATTERY_SVC_LEVEL_CHAR_UUID,
        .permission = IOTC_BLE_GATT_PERMISSION_READ_ENCRYPTED | IOTC_BLE_GATT_PERMISSION_WRITE_ENCRYPTED,
        .property = IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_READ | IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_WRITE |
            IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_NOTIFY,
        .readFunc = BleGattBatteryLevelCharReadFunc,
        .writeFunc = BleGattBatteryLevelWriteFunc,
        .indicateFunc = NULL,
        .desc = NULL,
        .descNum = 0,
    },
};

static const IotcBleGattProfileSvc g_bleGattSvcBuf[] = {
    {
        .uuid = BATTERY_SVC_UUID,
        .character = g_gattBatteryLevelChar,
        .charNum = (sizeof(g_gattBatteryLevelChar) / sizeof(IotcBleGattProfileChar)),
    },
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

static bool g_batteryLevelCharNotifyTestEnable = false; /* 收到write 1 开启，0 关闭, 开启后每3秒发送一次 */
static bool g_testCharIndicateTestEnable = false; /* 收到write 1 开启，0 关闭，开启后每3秒发送一次 */
static uint8_t g_TestCharCnt = 0;
static uint8_t g_batteryLevel = 0;
#define BATTERY_LEVEL_MAX  101

static int32_t BleGattTestCharReadFunc(uint8_t *buff, uint32_t *len)
{
    if (buff == NULL || len == NULL) {
        DEMO_LOG("param invalid");
        return -1;
    }
    DEMO_LOG("*len=%u", *len);
    buff[0] = g_TestCharCnt;
    *len = 1;
    g_TestCharCnt++;
    return 0;
}

static int32_t BleGattTestCharWriteFunc(uint8_t *buff, uint32_t len)
{
    if (buff == NULL || len == 0) {
        DEMO_LOG("param invalid");
        return -1;
    }
    DEMO_LOG("len=%u,buff=%02x", len, buff[0]);
    if (buff[0] == 0) {
        g_testCharIndicateTestEnable = false;
    } else {
        g_testCharIndicateTestEnable = true;
    }
    return 0;
}

static int32_t BleGattBatteryLevelCharReadFunc(uint8_t *buff, uint32_t *len)
{
    if (buff == NULL || len == NULL) {
        DEMO_LOG("param invalid");
        return -1;
    }
    DEMO_LOG("*len=%u", *len);
    buff[0] = g_batteryLevel;
    *len = 1;
    g_batteryLevel++;
    if (g_batteryLevel >= BATTERY_LEVEL_MAX) {
        g_batteryLevel = 0;
    }
    return 0;
}

static int32_t BleGattBatteryLevelWriteFunc(uint8_t *buff, uint32_t len)
{
    if (buff == NULL || len == 0) {
        DEMO_LOG("param invalid");
        return -1;
    }
    DEMO_LOG("len=%u,buff=%02x", len, buff[0]);
    if (buff[0] == 0) {
        g_batteryLevelCharNotifyTestEnable = false;
    } else {
        g_batteryLevelCharNotifyTestEnable = true;
    }
    return 0;
}

static void BleGattTestTask(void *arg)
{
    DEMO_LOG("BleGattTestTask enter");
    int32_t ret = 0;
    while (1) {
        if (g_batteryLevelCharNotifyTestEnable) {
            ret = IotcOhBleSendIndicateData(BATTERY_SVC_UUID, BATTERY_SVC_LEVEL_CHAR_UUID,
                &g_batteryLevel, sizeof(g_batteryLevel));
            DEMO_LOG("battery level send ret = %d", ret);
            g_batteryLevel++;
            if (g_batteryLevel >= BATTERY_LEVEL_MAX) {
                g_batteryLevel = 0;
            }
        }
        if (g_TestCharIndicateTestEnable) {
            ret = IotcOhBleSendIndicateData(TEST_SVC_UUID, TEST_SVC_READ_UUID, &g_TestCharCnt, sizeof(g_TestCharCnt));
            DEMO_LOG("test char send ret = %d", ret);
            g_TestCharCnt++;
        }
        AdapterSleepMs(3000);  // 避免频繁，休眠3s
    }
}

static void StartBleGattTestTask(void)
{
    AdapterTaskParam task = {
        .arg = NULL,
        .func = BleGattTestTask,
        .name = "BleGattTestTask",
        .prio = ADAPTER_TASK_PRIORITY_MID,
        .stackSize = 2048,
    };

    AdapterTaskId *id = AdapterCreateTask(&task);
    if (id == NULL) {
        DEMO_LOG("create iotc main task error %u", task.stackSize);
        return;
    }
    DEMO_LOG("create BleGattTestTask success");
}

static bool g_switch = false;

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

    cJSON *item = cJSON_GetObjectItem(json, "on");
    if (item == NULL || !cJSON_IsNumber(item)) {
        cJSON_Delete(json);
        DEMO_LOG("get on error");
        return -1;
    }

    int32_t on = cJSON_GetNumberValue(item);
    DEMO_LOG("switch on put %d=>%d", g_switch, on);
    g_switch = on == 0 ? false : true;

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

const struct SvcMap {
    const IotcServiceInfo *svc;
    int32_t (*putCharState)(const IotcServiceInfo *svc, const char *data, uint32_t len);
    int32_t (*getCharState)(const IotcServiceInfo *svc, char **data, uint32_t *len);
} SVC_MAP[] = {
    {&SVC_INFO[0], SwitchPutCharState, SwitchGetCharState},
};

static int32_t PutCharState(const IotcCharState state[], uint32_t num)
{
    if (state == NULL || num == 0) {
        DEMO_LOG("param invalid");
        return -1;
    }

    int32_t ret = 0;
    bool found = false;
    for (uint32_t i = 0; i < num; ++i) {
        DEMO_LOG("put char sid:%s data:%s", state[i].svcId, state[i].data);
        for (uint32_t j = 0; j < (sizeof(SVC_MAP) / sizeof(SVC_MAP[0])); ++j) {
            if (strcmp(state[i].svcId, SVC_MAP[j].svc->svcId) != 0 || SVC_MAP[j].putCharState == NULL) {
                continue;
            }
            found = true;
            int32_t curRet = SVC_MAP[j].putCharState(SVC_MAP[j].svc, state[i].data, state[i].len);
            if (curRet != 0) {
                ret = curRet;
                DEMO_LOG("put char sid:%s error %d", state[i].svcId, ret);
            }
        }
    }
    return ret != 0 ? ret : (found ? 0 : -1);
}

static int32_t GetCharState(const IotcCharState state[], char *out[], uint32_t len[], uint32_t num)
{
    if (state == NULL || num == 0 || out == NULL || len == NULL) {
        DEMO_LOG("param invalid");
        return -1;
    }

    int32_t ret = 0;
    bool found = false;
    for (uint32_t i = 0; i < num; ++i) {
        DEMO_LOG("get char sid:%s", state[i].svcId);
        for (uint32_t j = 0; j < (sizeof(SVC_MAP) / sizeof(SVC_MAP[0])); ++j) {
            if (strcmp(state[i].svcId, SVC_MAP[j].svc->svcId) != 0 || SVC_MAP[j].getCharState == NULL) {
                continue;
            }
            found = true;
            int32_t curRet = SVC_MAP[j].getCharState(SVC_MAP[j].svc, &out[i], &len[i]);
            if (curRet != 0) {
                ret = curRet;
                DEMO_LOG("get char sid:%s error %d", state[i].svcId, ret);
            }
        }
    }

    return ret != 0 ? ret : (found ? 0 : -1);
}

static int32_t ReportAll(void)
{
    IotcCharState reportInfo[sizeof(SVC_MAP) / sizeof(SVC_MAP[0])] = {0};
    int32_t ret = 0;
    uint32_t rptNum = 0;
    for (uint32_t i = 0; i < (sizeof(SVC_MAP) / sizeof(SVC_MAP[0])); ++i) {
        if (SVC_MAP[i].getCharState == NULL) {
            continue;
        }
        reportInfo[rptNum].svcId = SVC_MAP[i].svc->svcId;
        ret = SVC_MAP[i].getCharState(SVC_MAP[rptNum].svc, (char **)&reportInfo[rptNum].data, &reportInfo[rptNum].len);
        if (ret != 0) {
            DEMO_LOG("get char sid:%s error %d", reportInfo[rptNum].svcId, ret);
            break;
        }
        rptNum++;
    }
    if (ret == 0) {
        ret = IotcOhDevReportCharState(reportInfo, rptNum);
    }

    for (uint32_t i = 0; i < (sizeof(SVC_MAP) / sizeof(SVC_MAP[0])); ++i) {
        if (reportInfo[i].data != NULL) {
            cJSON_free((char *)reportInfo[i].data);
            reportInfo[i].data = NULL;
        }
    }
    return ret;
}

static int32_t GetPincode(uint8_t *buf, uint32_t bufLen)
{
    if (buf == NULL || bufLen > IOTC_PINCODE_LEN) {
        DEMO_LOG("param invalid");
        return -1;
    }

    unsigned char pskBuf[HILINK_PSK_LEN] = {0};
    extern void HiLinkGetPsk(unsigned char *psk, unsigned short len);
    HiLinkGetPsk(pskBuf, HILINK_PSK_LEN);

    int32_t ret = memcpy_s(buf, bufLen, pskBuf, PINCODE_LEN);
    if (ret != EOK) {
        return -1;
    }
    return 0;
}

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

static void DemoBleEventListener(int32_t event)
{
    int32_t ret = 0;
    switch (event) {
        case IOTC_CORE_BLE_EVENT_GATT_DISCONNECT:
            ret = IotcOhBleStartAdv(ADV_TIMEOUT);
            break;
        default:
            return;
    }
    DEMO_LOG("event[%d] ret:%d", event, ret);
}

static int32_t NoticeReboot(IotcRebootReason res)
{
    DEMO_LOG("notice reboot res %d", res);
    void sys_reset(void);
    sys_reset();
    return 0;
}

static int32_t RecvNetCfgInfo(const char *netInfo, uint32_t len)
{
    if (netInfo == NULL) {
        DEMO_LOG("param invalid");
        return -1;
    }
    DEMO_LOG("recv netcfg info: %s", netInfo);
    void LedFalsh(int i);
    LedFalsh(0);
    return 0;
}

static int32_t IotcOhDemoEntry(void)
{
    static const IotcOhProfCallback PROF_CALLBACK = {
        .onPutCharState = PutCharState,
        .onGetCharState = GetCharState,
        .onReportAll = ReportAll,
        .onGetPincode = GetPincode,
        .onGetAcKey = GetAcKey,
        .onProfFree = cJSON_free,
    };
    static IotcOhDevCallback DEVICE_CALLBACK = {
        .onReboot = NoticeReboot,
        .onTrng = NULL,
    };
    static IotcOhDevInitParam DEVICE_INIT_PARAM = {
        .devInfo = &DEV_INFO,
        .svcInfo = SVC_INFO,
        .svcNum = sizeof(SVC_INFO) / sizeof(SVC_INFO[0]),
        .profCb = &PROF_CALLBACK,
        .devCb = &DEVICE_CALLBACK,
    };
    int32_t ret = IotcOhDevInit(&DEVICE_INIT_PARAM);
    if (ret != 0) {
        DEMO_LOG("init device error %d", ret);
        return ret;
    }

    static const IotcOhBleExCallback BLE_EX_CALLBACK = {
        .onRecvNetCfgInfo = RecvNetCfgInfo,
        .onRecvCustomSecData = NULL
    };

    static const IotcOhBleInitParam BLE_INIT_PARAM = {
        .flag = 0,
        .advTimeoutMs = (10 * 60 * 1000),
        .svcList = &g_bleGattSvcList,
        .exCallback = &BLE_EX_CALLBACK,
    };

    ret = IotcOhBleEnable(&BLE_INIT_PARAM);
    if (ret != 0) {
        DEMO_LOG("enable ble connect error %d", ret);
        return ret;
    }
    StartBleGattTestTask();
    void UtilsComboSetWifiFlag(bool flag);
    UtilsComboSetWifiFlag(true);
    void SetNetCfgMode(IotcNetConfigMode mode);
    SetNetCfgMode(IOTC_NET_CONFIG_MODE_BLE_SUP);

    ret = IotcOhRegEventListener(DemoBleEventListener);
    if (ret != 0) {
        DEMO_LOG("reg event listener error %d", ret);
        return ret;
    }

    ret = IotcOhMain();
    if (ret != 0) {
        DEMO_LOG("iotc oh main error %d", ret);
        return ret;
    }
    DEMO_LOG("iotc oh main success");
    return ret;
}

static void IotcOhDemoExit(void)
{
    int32_t ret = IotcOhStop();
    if (ret != 0) {
        DEMO_LOG("iotc stop error %d", ret);
    }

    ret = IotcOhUnregEventListener(DemoBleEventListener);
    if (ret != 0) {
        DEMO_LOG("unreg event listener error %d", ret);
    }

    ret = IotcOhBleDisable();
    if (ret != 0) {
        DEMO_LOG("iotc wifi disable error %d", ret);
    }

    ret = IotcOhDevDeinit();
    if (ret != 0) {
        DEMO_LOG("iotc dev info deinit error %d", ret);
    }
}

#if IOTC_CONF_MEM_DEBUG
void AdapterMemDump(void);
#endif

static void PressButton1(void)
{
    DEMO_LOG("button 1 press");
#if IOTC_CONF_MEM_DEBUG
    AdapterMemDump();
#endif
    IotcOhReset();
}

static void PressButton2(void)
{
    DEMO_LOG("button 2 press");
#if IOTC_CONF_MEM_DEBUG
    AdapterMemDump();
#endif
    IotcOhDemoExit();
#if IOTC_CONF_MEM_DEBUG
    DEMO_LOG("iotc demo exit");
    AdapterMemDump();
#endif 
}