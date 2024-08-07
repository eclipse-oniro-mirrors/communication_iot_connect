# IOT_CONNECT

## 说明

响应OpenHarmony生态共建伙伴提出智慧病房、智慧隧道、会议办公等场景的富设备对瘦设备控制的需求。在生态互联互通共建项目中，构建瘦设备上运行的极简连接控制组件：iot_connect，实现在mini/small级别OH设备上的控制和连接。

## DEMO
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
    .prodId = "2F6R0",
    .subProdId = "",
    .model = "DL-01W",
    .devTypeId = "0460",
    .devTypeName = "Table Lamp",
    .manuId = "17C",
    .manuName = "DALEN",
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
 * CA_CERT为云测根证书列表，不使用端云功能可以不关注
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
 * [SHOULD] MODIFY ME
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

    /* 初始化BLE发现控制模块，BLE设备可选 */
    ret = IotcOhBleEnable();
    if (ret != 0) {
        DEMO_LOG("enable ble connect error %d", ret);
        return ret;
    }

    /* 初始化Wi-Fi发现控制模块，Wi-Fi设备可选 */
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
 * [SHOULD] MODIFY ME
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
 * [SHOULD] MODIFY ME
 * IotcOhDemoRestore为iot connect恢复出厂函数，开发者应在设备被重置时调用
 */
void IotcOhDemoRestore(void)
{
    DEMO_LOG("restore");
    IotcOhRestore();
}
```

互联互通服务启动后，所有的业务交互都通过注册的回调进行，服务的主动上报使用接口`IotcOhDevReportCharState`

## 接口说明
**oh connect**为iot connect 提供的互联互通协议实现
对外头文件目录为`interfaces/kits/common`与`interfaces/kits/oh_connect`

### 1. 部件运行管理

部件运行管理包括部件运行、复位、停止等接口，接口定义在`iotc_oh_sdk.h`内，部件运行前应先参考2、3、4章节使能对应的能力

#### 1.1 部件业务入口

**函数原型：**
`int32_t IotcOhMain(void)`

**说明：**
部件业务入口，调用该接口后部件会拉起自己的任务线程，并且大部分预配置类的接口不再可用

**返回值：**
类型：`int32_t`
值：`0`成功，其他失败，详见`iotc_errcode.h`

#### 1.2 部件复位

**函数原型：**
`int32_t IotcOhReset(void)`

**说明：**
部件复位，调用后部件会重置所有组件业务的运行状态，仅在部件运行时有效，该接口为同步接口，返回即表示复位成功/失败

**返回值：**
类型：`int32_t`
值：`0`成功，其他失败，详见`iotc_errcode.h`

#### 1.3 部件停止

**函数原型：**
`int32_t IotcOhStop(void)`

**说明：**
停止部件的运行，仅在部件运行时有效，该接口为同步接口，返回即表示停止成功/失败

**返回值：**
类型：`int32_t`
值：`0`成功，其他失败，详见`iotc_errcode.h`

#### 1.4 恢复出厂

**函数原型：**
`int32_t IotcOhRestore(void)`

**说明：**
通知所有业务恢复出厂，仅在部件运行时有效，该接口为同步接口，返回即表示恢复出厂成功/失败

**返回值：**
类型：`int32_t`
值：`0`成功，其他失败，详见`iotc_errcode.h`

#### 1.5 配置部件运行参数

**函数原型：**
`int32_t IotcOhSetOption(int32_t option, ...);`
**说明：**
配置部件运行时的参数

**参数列表：**

| 参数| 类型 | 说明 |
| --- | --- | --- |
| int32_t option | 输入 | 待配置的参数类型，详见1.6配置选项章节 |

#### 1.7 配置选项

配置选项定义在`IotcOhOptionType`枚举，并通过`IotcOhSetOption`接口使用

| 配置项| 枚举 | 参数 | 使用样例 | 备注 |
| --- | --- | --- | --- | --- |
| 日志等级 | `IOTC_OH_OPTION_部件_LOG_LEVEL` | uint32_t | uint32 logLevel = 7;<br>IotcOhSetOption(IOTC_OH_OPTION_部件_LOG_LEVEL, logLevel); | 部件日志输出除了受该运行时日志等级约束以外，还受到编译时等级约束，不会输出高于日志编译等级的日志 |
| 主线程栈大小 | `IOTC_OH_OPTION_部件_MAIN_TASK_SIZE` | uint32_t | uint32 mainTaskSize= 0x4000;<br />IotcOhSetOption(IOTC_OH_OPTION_部件_MAIN_TASK_SIZE, mainTaskSize); |  |
| 监控线程栈大小 | `IOTC_OH_OPTION_部件_MONITOR_TASK_SIZE` | uint32_t | uint32 monitorTaskSize= 0x1000;<br />IotcOhSetOption(IOTC_OH_OPTION_部件_MONITOR_TASK_SIZE, monitorTaskSize); |  |
| 配置文件路径 | `IOTC_OH_OPTION_部件_CONFIG_PATH` | const char * | const char *path = "/config/iotc";<br />IotcOhSetOption(IOTC_OH_OPTION_部件_CONFIG_PATH, path ); |  |
| 事件监听注册 | `IOTC_OH_OPTION_部件_REG_EVENT_LISTENER` | IotcOhEventCallback | void YourEventListener(int32_t event);<br />IotcOhSetOption(IOTC_OH_OPTION_部件_REG_EVENT_LISTENER, YourEventListener); |  |
| 事件监听去注册 | `IOTC_OH_OPTION_部件_UNREG_EVENT_LISTENER` | IotcOhEventCallback | void YourEventListener(int32_t event);<br />IotcOhSetOption(IOTC_OH_OPTION_部件_UNREG_EVENT_LISTENER, YourEventListener); |  |<br>

### 2. BLE Connect能力

#### 2.1 BLE Connect能力使能

**函数原型：**
`int32_t IotcOhBleEnable(void)`

**说明：**
使能BLE发现、连接、控制的能力，该接口仅做使能，调用不会触发任何业务流程，该接口应在iot connect运行前调用

**返回值：**
类型：`int32_t`
值：`0`成功，其他失败，详见`iotc_errcode.h`

#### 2.2 BLE Connect能力关闭

**函数原型：**
`int32_t IotcOhBleDisable(void)`

**说明：**
关闭iot connect的BLE发现、连接、控制的能力，用于释放调用`IotcOhBleEnable`时申请的资源，该接口无法在iot connect运行时调用。

**返回值：**
类型：`int32_t`
值：`0`成功，其他失败，详见`iotc_errcode.h`

#### 2.3 发送BLE发现广播

**函数原型：**
`int32_t IotcOhBleStartAdv(uint32_t ms);`

**说明：**
启动BLE广播发现，根据初始化参数的不同，iot connect启动后会自动发送一段时间广播，后续需要调用该接口激活广播，用于配合外部按键，实现按键触发的场景

**参数列表：**

| 参数| 类型 | 说明 |
| --- | --- | --- |
| uint32_t ms | 输入 | 广播时长，单位ms，gatt的连接不会刷新、延长该时间|

**返回值：**
类型：`int32_t`
值：`0`成功，其他失败，详见`iotc_errcode.h`

#### 2.4 停止BLE发现广播

**函数原型：**
`int32_t IotcOhBleStopAdv(void)`

**说明：**
在BLE广播发现期间，调用该接口可以停止广播

**返回值：**
类型：`int32_t`
值：`0`成功，其他失败，详见`iotc_errcode.h`

#### 2.5 发送`customSecData`数据

**函数原型：**
`int32_t IotcOhBleSendCustomSecData(const uint8_t *data, uint32_t len);`

**说明：**
部分场景下，需要使用`customSecData`服务通道，可以调用该接口，sdk完成数据加密后发送给对端

**参数列表：**

| 参数| 类型 | 说明 |
| --- | --- | --- |
| const uint8_t *data | 输入 | 待发送的数据 |
| uint32_t len | 输入 | 数据长度 |

**返回值：**
类型：`int32_t`
值：`0`成功，其他失败，详见`iotc_errcode.h`

#### 2.6 发送BLE Indicate数据

**函数原型：**
`int32_t IotcOhBleSendIndicateData(const char *svcUuid, const char *charUuid, const uint8_t *value, uint32_t valueLen)`

**说明：**
当使用部件管理GATT服务时，可以使用该接口发送BLE Indicate数据

**参数列表：**

| 参数| 类型 | 说明 |
| --- | --- | --- |
| const char *svcUuid | 输入 | Ble gatt svc uuid |
| const char *charUuid | 输入 |  Ble gatt svc character UUID |
| const uint8_t *value| 输入 | 待发送的数据 |
| uint32_t valueLen| 输入 | 数据长度 |

**返回值：**
类型：`int32_t`
值：`0`成功，其他失败，详见`iotc_errcode.h`

#### 2.7 配置选项

配置选项定义在`IotcOhOptionType`枚举，并通过`IotcOhSetOption`接口使用

| 配置项| 枚举 | 参数 | 使用样例 | 备注 |
| --- | --- | --- | --- | --- |
| 配置BLE Connect能力在配网完成后退出 | `IOTC_OH_OPTION_BLE_EXIT_AFTER_NETCFG` | 无 | IotcOhSetOption(IOTC_OH_OPTION_BLE_EXIT_AFTER_NETCFG); |  |
| 注册接收配网信息的业务回调 | `IOTC_OH_OPTION_BLE_RECV_NETCFG_CALLBACK` | IotcRecvNetCfgInfoCallback | int32_t YourNetCfgInfoCallback(const char *netInfo, uint32_t len);<br />IotcOhSetOption(IOTC_OH_OPTION_BLE_RECV_NETCFG_CALLBACK, YourNetCfgInfoCallback); |  |
| 注册`customSecData`服务数据业务回调 | `IOTC_OH_OPTION_BLE_RECV_CUSTOM_DATA_CALLBACK` | IotcRecvCustomSecDataCallback | int32_t YourRecvCustomSecDataCallback(const uint8_t *data, uint32_t len);<br />IotcOhSetOption(IOTC_OH_OPTION_BLE_RECV_CUSTOM_DATA_CALLBACK, YourRecvCustomSecDataCallback); |  |
| 配置启动后BLE广播时长 | `IOTC_OH_OPTION_BLE_START_UP_ADV_TIMEOUT` | uint32_t | uint32_t advTimeout = 10 * 60 * 1000;<br />IotcOhSetOption(IOTC_OH_OPTION_BLE_START_UP_ADV_TIMEOUT, advTimeout ); | 单位ms |
| 注册GATT服务列表                    | `IOTC_OH_OPTION_BLE_GATT_PROFILE_SVC_LIST` | IotcBleGattProfileSvcList | static const IotcBleGattProfileSvcList *YOUR_GATT_LIST = XXX;<br />IotcOhSetOption(IOTC_OH_OPTION_BLE_GATT_PROFILE_SVC_LIST, YOUR_GATT_LIST ); | GATT服务列表的指针及内部指针应为静态常量 |<br>

#### 2.8 结构体/枚举定义

**IotcBleGattProfileSvcList：**

| 成员| 描述 |
| --- | --- |
| const IotcBleGattProfileSvc *svc | ble gatt服务 |
| uint8_t svcNum | ble gatt 服务数量 |

**IotcBleGattProfileSvc：**

| 成员| 描述 |
| --- | --- |
| const char *uuid | ble gatt 服务 uuid |
| const IotcBleGattProfileChar *character | ble gatt 属性 |
| uint8_t charNum | ble gatt 属性数量 |

**IotcBleGattProfileChar：**

| 成员| 描述 |
| --- | --- |
| const char *uuid | ble gatt 属性 uuid |
| uint32_t permission | ble gatt 读写权限 |
| uint32_t property | ble gatt 特征属性 |
| readFunc | ble gatt 读回调 |
| writeFunc | ble gatt 写回调 |
| indicateFunc | ble gatt 指示回调 |
| const IotcBleGattProfileDesc *desc | 描述特征列表 |
| uint8_t descNum | 描述特征列表数量 |

**IotcBleGattProfileDesc：**

| 成员| 描述 |
| --- | --- |
| const char *uuid | ble gatt 属性 uuid |
| uint32_t permission | ble gatt 读写权限 |
| readFunc | ble gatt 读回调 |
| writeFunc | ble gatt 写回调 |

**IotcBleGattProperties：**

| 成员| 描述 |
| --- | --- |
| IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_BROADCAST | 可广播 |
| IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_READ | 可读 |
| IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_WRITE_NO_RSP | 可不响应写入 |
| IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_WRITE | 可写入 |
| IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_NOTIFY | 支持通知 |
| IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_INDICATE | 支持指示 |
| IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_SIGNED_WRITE | 支持带签名写入 |
| IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_EXTENDED_PROPERTY | 具有拓展属性 |

**IotcBleGattPermission：**

| 成员| 描述 |
| --- | --- |
| IOTC_BLE_GATT_PERMISSION_READ | 可读 |
| IOTC_BLE_GATT_PERMISSION_READ_ENCRYPTED | 加密可读 |
| IOTC_BLE_GATT_PERMISSION_READ_ENCRYPTED_MITM | 中间人保护可读 |
| IOTC_BLE_GATT_PERMISSION_WRITE | 可写 |
| IOTC_BLE_GATT_PERMISSION_WRITE_ENCRYPTED | 加密可写 |
| IOTC_BLE_GATT_PERMISSION_WRITE_ENCRYPTED_MITM | 中间人保护可写 |
| IOTC_BLE_GATT_PERMISSION_WRITE_SIGNED | 签名可写 |
| IOTC_BLE_GATT_PERMISSION_WRITE_SIGNED_MITM | 中间人保护签名可写 |

### 3. Wi-Fi Connect 能力

#### 3.1 Wi-Fi Connect能力使能

**函数原型：**
`int32_t IotcOhWifiEnable(void)`

**说明：**
使能基于Wi-Fi/LAN的发现、连接、控制的能力，该接口仅做使能，调用不会触发任何业务流程，该接口应在iot connect运行前调用

**返回值：**
类型：`int32_t`
值：`0`成功，其他失败，详见`iotc_errcode.h`

#### 3.2 Wi-Fi Connect能力关闭

**函数原型：**
`int32_t IotcOhWifiDisable(void)`

**说明：**
关闭iot connect的Wi-Fi发现、连接、控制的能力，用于释放调用`IotcOhWifiEnable`时申请的资源，该接口无法在iot connect运行时调用。

**返回值：**
类型：`int32_t`
值：`0`成功，其他失败，详见`iotc_errcode.h`

#### 3.3 配置选项

配置选项定义在`IotcOhOptionType`枚举，并通过`IotcOhSetOption`接口使用

| 配置项                         | 枚举                                    | 参数                                                         | 使用样例                                                     | 备注   |
| ------------------------------ | --------------------------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ | ------ |
| 配置发送缓冲区大小             | `IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE`  | 个数：2<br />参数1：uint32_t 常驻缓冲区大小<br />参数2：uint32_t 最大缓冲区大小 | uint32_t resSendBufferSize = 0x4000;<br />uint32_t maxSendBufferSize = 0x10000;<br />IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSendBufferSize , maxSendBufferSize); |        |
| 配置接收缓冲区大小             | `IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE`  | 个数：2<br />参数1：uint32_t 常驻缓冲区大小<br />参数2：uint32_t 最大缓冲区大小 | uint32_t resSendBufferSize = 0x4000;<br />uint32_t maxSendBufferSize = 0x10000;<br />IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSendBufferSize , maxSendBufferSize); |        |
| 配置配网模式                   | `IOTC_OH_OPTION_WIFI_NETCFG_MODE`       | int32_t                                                      | int32_t mode = IOTC_NET_CONFIG_MODE_SOFTAP;<br />IotcOhSetOption(IOTC_OH_OPTION_WIFI_NETCFG_MODE, mode); |        |
| 配置配网超时时长               | `IOTC_OH_OPTION_WIFI_NETCFG_TIMEOUT`    | uint32_t                                                     | uint32_t netCfgTimeout = 10 * 60 * 1000;<br />IotcOhSetOption(IOTC_OH_OPTION_WIFI_NETCFG_TIMEOUT, netCfgTimeout); | 单位ms |
| 注册端云根CA证书获取的业务回调 | `IOTC_OH_OPTION_WIFI_GET_CERT_CALLBACK` | IotcOhWifiGetCertCallback                                    | int32_t YourWifiGetCertCallback(const char **ca[], uint32_t *num);<br />IotcOhSetOption(IOTC_OH_OPTION_WIFI_GET_CERT_CALLBACK, YourWifiGetCertCallback); |        |<br>

#### 3.3 结构体/枚举定义

**IotcNetConfigMode：**

| 成员| 描述 |
| --- | --- |
| IOTC_NET_CONFIG_MODE_NONE | 无需配网，自身有配网/联网能力的设备选择 |
| IOTC_NET_CONFIG_MODE_SOFTAP | 使用SoftAP配网 |
| IOTC_NET_CONFIG_MODE_BLE_SUP | 使用BLE辅助配网 |
| IOTC_NET_CONFIG_MODE_BLE_AGT | 使用BLE辅助配网并代理注册设备 |

### 4. 设备信息/服务/控制能力

#### 4.1 设备管理模块使能

**函数原型：**
`int32_t IotcOhDevInit(void)`

**说明：**
配置设备信息，并注册设备服务/控制等相关业务回调，该接口应在iot connect运行前调用

**返回值：**
类型：`int32_t`
值：`0`成功，其他失败，详见`iotc_errcode.h`

#### 4.2 设备管理模块去使能

**函数原型：**
`int32_t IotcOhDevDeinit(void)`

**说明：**
释放调用`IotcOhDevInit`时申请的资源，该接口无法在iot connect运行时调用

**返回值：**
类型：`int32_t`
值：`0`成功，其他失败，详见`iotc_errcode.h`

#### 4.3 服务主动上报

**函数原型：**`int32_t IotcOhDevReportCharState(const IotcCharState state[], uint32_t num)`

**说明：**
上报设备的服务信息，应在设备的服务信息发生变化/事件服务发生时调用

**参数列表：**

| 参数| 类型 | 说明 |
| --- | --- | --- |
| const IotcCharState state[]| 输入 | 上报服务列表，详见`IotcCharState`结构体定义，应为常量指针|
| uint32_t num| 输入 | 上报服务数量 |

**返回值：**
类型：`int32_t`
值：`0`成功，其他失败，详见`iotc_errcode.h`

#### 4.3 配置选项

配置选项定义在`IotcOhOptionType`枚举，并通过`IotcOhSetOption`接口使用

| 配置项                         | 枚举                                            | 参数                                                         | 使用样例                                                     | 备注             |
| ------------------------------ | ----------------------------------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ | ---------------- |
| 配置服务信息修改业务回调       | `IOTC_OH_OPTION_DEVICE_PUT_CHAR_STATE_CALLBACK` | IotcDevProfPutCharState                                      | int32_t YourPutCharState(const IotcCharState state[], uint32_t num);<br />IotcOhSetOption(IOTC_OH_OPTION_DEVICE_PUT_CHAR_STATE_CALLBACK, YourPutCharState); | 必须配置         |
| 配置服务信息查询业务回调       | `IOTC_OH_OPTION_DEVICE_GET_CHAR_STATE_CALLBACK` | IotcDevProfGetCharState                                      | int32_t YourGetCharState(const IotcCharState state[], char *out[], uint32_t len[], uint32_t num);<br />IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_CHAR_STATE_CALLBACK, YourGetCharState); | 必须配置         |
| 配置全量可上报服务上报业务回调 | `IOTC_OH_OPTION_DEVICE_REPORT_ALL_CALLBACK`     | IotcDevProfReportAll                                         | int32_t YourReportAll(void);<br />IotcOhSetOption(IOTC_OH_OPTION_DEVICE_REPORT_ALL_CALLBACK, YourReportAll); | 必须配置         |
| 配置获取PIN码回调              | `IOTC_OH_OPTION_DEVICE_GET_PINCODE_CALLBACK`    | IotcDevProfGetPincode                                        | int32_t YourGetPincode(uint8_t *buf, uint32_t bufLen);<br />IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_PINCODE_CALLBACK, YourGetPincode); | 单位ms，必须配置 |
| 配置获取厂商AC KEY业务回调     | `IOTC_OH_OPTION_DEVICE_GET_AC_KEY_CALLBACK`     | IotcDevProfGetAcKey                                          | int32_t YourGetAcKey(uint8_t *buf, uint32_t bufLen);<br />IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_AC_KEY_CALLBACK, YourGetAcKey); | 必须配置         |
| 配置服务信息资源释放回调       | `IOTC_OH_OPTION_DEVICE_DATA_FREE_CALLBACK`      | IotcDevProfFree                                              | int32_t YourProfDataFree(void *ptr);<br />IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DATA_FREE_CALLBACK, YourProfDataFree); | 必须配置         |
| 配置设备重启回调               | `IOTC_OH_OPTION_DEVICE_REBOOT_CALLBACK`         | IotcDevReboot                                                | int32_t YourDevReboot(int32_t res);<br />IotcOhSetOption(IOTC_OH_OPTION_DEVICE_REBOOT_CALLBACK, YourDevReboot); | 必须配置         |
| 配置硬件随机数回调             | `IOTC_OH_OPTION_DEVICE_TRNG_CALLBACK`           | IotcDevTrng                                                  | int32_t YourDevTrng(uint8_t *buf, uint32_t len);<br />IotcOhSetOption(IOTC_OH_OPTION_DEVICE_TRNG_CALLBACK, YourDevTrng); |                  |
| 配置设备信息                   | `IOTC_OH_OPTION_DEVICE_DEV_INFO`                | const IotcDeviceInfo *                                       | static const IotcDeviceInfo DEV_INFO = {...};<br />IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &DEV_INFO ); | 必须配置         |
| 配置服务信息                   | `IOTC_OH_OPTION_DEVICE_SVC_INFO`                | 个数：2<br />参数1：const IotcServiceInfo *<br />参数2：uint32_t 服务个数 | static const IotcDeviceInfo SVC_INFO= {...};<br />uint32_t svcNum = sizeof(SVC_INFO) / sizeof(SVC_INFO[0])<br />IotcOhSetOption(IOTC_OH_OPTION_DEVICE_SVC_INFO, DEV_INFO, svcNum ); | 必须配置         |<br>

#### 4.4 结构体/枚举定义

**IotcCharState：**

| 成员| 描述 |
| --- | --- |
| svcId | 服务id，应为以`\0`结束的字符串 |
| data| 服务属性值，应为以`\0`结束的json字符串，格式为`{"属性1":xxx,"属性2":xxx}` |
| len | 服务属性值字符串长度|

**IotcDeviceInfo:**

| 成员| 描述 |
| --- | --- |
| const char *sn | 设备SN，字符串长度(0,40] |
| const char *prodId| 设备产品ID，字符串长度5，需严格与云测定义一致 |
| const char *subProdId | 设备子产品ID，字符串长度2，可以为`NULL`|
| const char *model | 设备型号，字符串长度(0,32]，需严格与云测定义一致 |
| const char *devTypeId |设备类型ID，字符串长度4，需严格与云测定义一致|
| const char *devTypeName | 设备类型名称，字符串长度(0,32]|
| const char *manuId | 厂商ID，字符串长度3，需严格与云测定义一致|
| const char *manuName | 厂商名，字符串长度(0,32]|
| const char *fwv | 固件版本号，字符串长度(0,64]|
| const char *hwv | 硬件版本号，字符串长度(0,64]|
| const char *swv | 软件版本号，字符串长度(0,64]|
| int8_t protType | 设备协议类型，详见`IotcProtType`定义，需严格与云测定义一致|

*注意：`manuName `与`devTypeName`用于BLE广播与SoftAp的SSID拼接，过长可能导致截断，其中BLE广播要求两个字段长度和小于10，SoftAP SSID要求两个字段和小于14字节*

**IotcServiceInfo:**

| 成员| 描述 |
| --- | --- |
| const char *svcType | 服务类型，以`\0`结束的字符串，需严格与云测定义一致 |
| const char *svcId| 服务id，以`\0`结束的字符串，需严格与云测定义一致 |

**IotcRebootReason:**

| 成员| 描述 |
| --- | --- |
| IOTC_REBOOT_WATCH_DOG_TIMEOUT| 软狗超时，部件内部的开门狗超时，业务不可用|
| IOTC_REBOOT_RESTORE| 设备恢复出厂|