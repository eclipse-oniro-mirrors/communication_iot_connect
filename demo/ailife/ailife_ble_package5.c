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
#include "iotc_oh_connect.h"
#include "iotc_oh_device.h"
#include "securec.h"
#include "cJSON.h"

#define HILINK_PSK_LEN  16
#define PINCODE_LEN     8

#define ADV_TIMEOUT     UINT32_MAX

#define DEMO_LOG(...) do { \
        printf("DEMO[%s:%u]", __func__, __LINE__); \
        printf(__VA_ARGS__); \
        printf("\r\n"); \
    } while (0)


static const IotcDeviceInfo DEV_INFO = {
    .sn = "FFEE3333",
    .prodId = "2EKT",
    .subProdId = "",
    .model = "D12",
    .devTypeId = "094",
    .devTypeName = "SmartElectricScooter",
    .manuId = "17C",
    .manuName = "LEQI",
    .fwv = "1.0.0",
    .hwv = "1.0.0",
    .swv = "1.0.0",
    .protType = IOTC_PROT_TYPE_BLE,
};

static const IotcServiceInfo SVC_INFO[] = {
    {"lightSwitch", "lightSwitch"},
    {"gear", "gear" },
    {"deviceTime", "deviceTime"},
};

const uint8_t AC_KEY[IOTC_AC_KEY_LEN] = {
    0x49, 0x3F, 0x45, 0x4A, 0x3A, 0x72, 0x38, 0x7B, 0x36, 0x32, 0x50, 0x3C, 0x49, 0x39, 0x62, 0x38,
    0x72, 0xCB, 0x6D, 0xC5, 0xAE, 0xE5, 0x4A, 0x82, 0xD3, 0xE5, 0x6D, 0xF5, 0x36, 0x82, 0x62, 0xEB,
    0x89, 0x30, 0x6C, 0x88, 0x32, 0x56, 0x23, 0xFD, 0xB8, 0x67, 0x90, 0xA7, 0x7B, 0x61, 0x1E, 0xAE
};

static bool g_switch = false;

static void ModifyLedSwitch(bool onOff)
{
    g_switch = onOff;
    void LedGreenOnOff(bool onOff);
    LedGreenOnOff(g_switch);
}

static int SwitchPutCharState(const IotcServiceInfo *svc, const char *data, uint32_t len)
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
    ModifyLedSwitch(on == 0 ? false : true);

    cJSON_Delete(json);
    return 0;
}

static int SwitchGetCharState(const IotcServiceInfo *svc, char **data, uint32_t *len)
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

static int g_gear = 0;

static int GearPutCharState(const IotcServiceInfo *svc, const char *data, uint32_t len)
{
    if (data == NULL || len == 0) {
        return -1;
    }
    cJSON *json = cJSON_Parse(data);
    if (json == NULL) {
        return -1;
    }

    cJSON *item = cJSON_GetObjectItem(json, "gear");
    if (item == NULL || !cJSON_IsNumber(item)) {
        cJSON_Delete(json);
        return -1;
    }

    int gear = cJSON_GetNumberValue(item);
    DEMO_LOG("gear val put %d=>%d\n", g_gear, gear);
    g_gear = gear;
    if (gear == 2) {
        int32_t AdapterSleepMs(uint32_t ms);
        DEMO_LOG("sleep wait watch dog");
        AdapterSleepMs(2 * 60 *1000);
    }

    cJSON_Delete(json);
    return 0;
}

static int GearGetCharState(const IotcServiceInfo *svc, char **data, uint32_t *len)
{
    if (data == NULL || *data != NULL) {
        return -1;
    }

    cJSON *json = cJSON_CreateObject();
    if (json == NULL) {
        return -1;
    }

    if (cJSON_AddNumberToObject(json, "gear", g_gear) == NULL) {
        cJSON_Delete(json);
        return -1;
    }

    *data = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    if (*data == NULL) {
        return -1;
    }
    DEMO_LOG("gear get %d\n", g_gear);
    *len = strlen(*data);
    return 0;
}

static int32_t ReportAll(void);

static int32_t DevTimePutCharState(const IotcServiceInfo *svc, const char *data, uint32_t len)
{
    if (data == NULL || len == 0) {
        return -1;
    }

    ReportAll();
    return 0;
}

const struct SvcMap {
    const IotcServiceInfo *svc;
    int (*putCharState)(const IotcServiceInfo *svc, const char *data, uint32_t len);
    int (*getCharState)(const IotcServiceInfo *svc, char **data, uint32_t *len);
} SVC_MAP[] = {
    {&SVC_INFO[0], SwitchPutCharState, SwitchGetCharState},
    {&SVC_INFO[1], GearPutCharState, GearGetCharState},
    {&SVC_INFO[2], DevTimePutCharState, NULL},
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
            int curRet = SVC_MAP[j].putCharState(SVC_MAP[j].svc, state[i].data, state[i].len);
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
            int curRet = SVC_MAP[j].getCharState(SVC_MAP[j].svc, &out[i], &len[i]);
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
        case IOTC_CORE_COMM_EVENT_MAIN_RESTORE:
            g_gear = 0;
            ModifyLedSwitch(false);
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

int32_t IotcOhDemoEntry(void)
{
    static const IotcOhProfCallback PROF_CALLBACK = {
        .onPutCharState = PutCharState,
        .onGetCharState = GetCharState,
        .onReportAll = ReportAll,
        .onGetPincode = GetPincode,
        .onGetAcKey = GetAcKey,
        .onProfFree = cJSON_free,
    };
    static const IotcOhDevCallback DEVICE_CALLBACK = {
        .onReboot = NoticeReboot,
        .onTrng = NULL,
    };
    static const IotcOhDevInitParam DEVICE_INIT_PARAM = {
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

    static IotcOhBleInitParam BLE_INIT_PARAM = {
        .flag = 0,
        .advTimeoutMs = (10 * 60 * 1000),
        .svcList = NULL,
        .exCallback = NULL,
    };

    ret = IotcOhBleEnable(&BLE_INIT_PARAM);
    if (ret != 0) {
        DEMO_LOG("enable ble connect error %d", ret);
        return ret;
    }

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

void IotcOhDemoExit(void)
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

void PressButton1(void)
{
    DEMO_LOG("button 1 press");
    (void)IotcOhRestore();
    (void)IotcOhReset();
}

void PressButton2(void)
{
    DEMO_LOG("button 2 press");
    ModifyLedSwitch(!g_switch);
    ReportAll();
}