## Combo Connect DEMO 说明

Combo Connect DEMO适用于使用BLE辅助配网、端云连接、局域网本地控制的设备，DEMO示例代码：
```
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "iotc_oh_wifi.h"
#include "iotc_oh_ble.h"
#include "iotc_oh_connect.h"
#include "iotc_oh_device.h"
#include "securec.h"
#include "cJSON.h"
#include "iotc_conf.h"

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
    .devTypeName = "Dev Type Name",
    .manuId = "123",
    .manuName = "Manu Name",
    .fwv = "1.0.0",
    .hwv = "1.0.0",
    .swv = "1.0.0",
    .protType = IOTC_PROT_TYPE_WIFI,
};

/**
 * [MUST] MODIFY ME
 * SVC_INFO为产品服务信息，应与云/APP侧配置一致
 */
static const IotcServiceInfo SVC_INFO[] = {
    {"switch", "switch"},
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
 * CA_CERT为云测根证书列表
 */
static const char *CERT1 = "-----BEGIN CERTIFICATE-----\r\n" \
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\r\n" \
    "-----END CERTIFICATE-----\r\n"
static const char *CA_CERT[] = {CERT1};

/**
 * [MUST] MODIFY ME
 * 下面代码为开关服务的实现样例
 * SwitchPutCharState函数为控制指令的处理，控制指令报文为{"on":1或0}
 * SwitchGetCharState函数为查询指令的处理
 * g_switch保存了开关的状态
 * 产品可以参考该DEMO并实现自己的服务函数添加到SVC_MAP中
 */
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
    IotcCharState reportInfo[sizeof(SVC_MAP) / sizeof(SVC_MAP[0])] = {0};
    int32_t ret;
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
 * [SHOULD] MODIFY ME
 * GetRootCA为注册到iot connect中的获取端云证书回调
 */
static int32_t GetRootCA(const char **ca[], uint32_t *num)
{
    if (ca == NULL || num == NULL) {
        DEMO_LOG("param invalid");
        return -1;
    }

    *ca = CA_CERT;
    *num = sizeof(CA_CERT) / sizeof(CA_CERT[0]);
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
    } while (0);

/**
 * [MUST] USE ME
 * IotcOhDemoEntry为iot connect业务入口，开发者应在业务进程启动时调用
 */
int32_t IotcOhDemoEntry(void)
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

    /* 初始化Wi-Fi发现控制模块 */
    ret = IotcOhWifiEnable();
    if (ret != 0) {
        DEMO_LOG("enable wifi connect error %d", ret);
        return ret;
    }

    /* 配置iot connect必要的回调 */
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_PUT_CHAR_STATE_CALLBACK, PutCharState);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_GET_CHAR_STATE_CALLBACK, GetCharState);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_REPORT_ALL_CALLBACK, ReportAll);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_GET_PINCODE_CALLBACK, GetPincode);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_GET_AC_KEY_CALLBACK, GetAcKey);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_DATA_FREE_CALLBACK, cJSON_free);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_REBOOT_CALLBACK, NoticeReboot);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_WIFI_GET_CERT_CALLBACK, GetRootCA);
    /* 配置设备信息与服务信息 */
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_DEV_INFO, &DEV_INFO);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_SVC_INFO, SVC_INFO, sizeof(SVC_INFO) / sizeof(SVC_INFO[0]));
    /* 配置配网模式，根据业务场景选择 */
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_WIFI_NETCFG_MODE, IOTC_NET_CONFIG_MODE_BLE_SUP);
    /* 配置配网超时时间 */
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_WIFI_NETCFG_TIMEOUT, (10 * 60 * 1000));

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
void IotcOhDemoExit(void)
{
    int32_t ret = IotcOhStop();
    if (ret != 0) {
        DEMO_LOG("iotc stop error %d", ret);
    }

    ret = IotcOhWifiDisable();
    if (ret != 0) {
        DEMO_LOG("iotc wifi disable error %d", ret);
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

/**
 * [MUST] USE ME
 * IotcOhDemoRestore为iot connect恢复出厂函数，开发者应在设备被重置时调用
 */
void IotcOhDemoRestore(void)
{
    DEMO_LOG("restore");
    IotcOhRestore();
}

/**
 * [MUST] USE ME
 * IotcOhDemoReport为设备数据变化上报的DEMO函数，开发者应在设备服务发生主动变化时调用该接口上报
 */
void IotcOhDemoReport(const char *sid)
{
    DEMO_LOG("report");
    if (sid == NULL) {
        DEMO_LOG("report sid null");
        return;
    }
    IotcCharState reportInfo = {0};
    int32_t ret = -1;
    for (uint32_t i = 0; i < (sizeof(SVC_MAP) / sizeof(SVC_MAP[0])); ++i) {
        if (SVC_MAP[i].svc->svcId == NULL || SVC_MAP[i].getCharState == NULL ||
            strcmp(sid, SVC_MAP[i].svc->svcId) != 0) {
            continue;
        }
        reportInfo.svcId = SVC_MAP[i].svc->svcId;
        ret = SVC_MAP[i].getCharState(SVC_MAP[i].svc, (char **)&reportInfo.data, &reportInfo.len);
        if (ret != 0) {
            DEMO_LOG("get char sid:%s error %d", sid, ret);
        }
        break;
    }
    if (ret == 0) {
        ret = IotcOhDevReportCharState(&reportInfo, 1);
        if (ret != 0) {
            DEMO_LOG("report char sid:%s error %d", sid, ret);
        } else {
            DEMO_LOG("report char sid:%s success", sid);
        }
    }

    if (reportInfo.data != NULL) {
        cJSON_free((char *)reportInfo.data);
    }
    return;
}
```