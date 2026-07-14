# BLE Only 设备基于Hi3863平台DEMO编译构建

## 一、开发环境准备

### 1.1 硬件要求

| 设备类型 | 配置要求                                                         |
| ---- | ------------------------------------------------------------ |
| 开发板  | BearPi-pico H3863（Hi3863 芯片），需配备 USB 数据线（用于烧录和调试），以下简称为H3863 |

### 1.2 软件要求

| 软件/工具         | 版本要求                    | 用途说明                                     |
| ------------- | ----------------------- | ---------------------------------------- |
| OpenHarmony源码 | 5.1.0 release           | 基础系统源码，为 Hi3863 平台提供编译底座                 |
| IoTConnect组件  | 最新master分支              | 提供蓝牙单模通信核心能力，支撑设备与APP/云侧数据交互             |
| 通用互联APP       | 最新master分支              | 鸿蒙生态控制入口，运行在HarmonyOS手机，用于蓝牙配网、设备控制及状态查看 |
| 编译工具链         | arm-none-eabi-gcc 9.3.1 | Hi3863 芯片的 ARM 架构编译工具链                   |
| hb构建工具        | 0.4.6及以上                | OpenHarmony轻量级设备编译构建工具，简化编译流程            |
| Python        | 3.8~3.9                 | 运行hb工具及编译脚本，支撑自动化构建流程                    |
| Git           | 2.25及以上                 | 克隆仓库代码                                   |

## 二、示例代码

[BLE Only代码](https://gitcode.com/ohos-oneconnect/applications_sample_iot_connect_samples/blob/master/ble/liteos/ws63/iotc_oh_demo_ble.c)

## 三、代码开发详解

### 3.1 云平台注册

- 登录OpenHarmony统一互联云平台，注册产品信息，获取产品ID（prodId）、厂商ID（manuId）及AC密钥，需与Demo中设备信息配置严格一致。

- 创建物模型，添加所需服务，定义服务ID、类型及数据格式，与Demo服务配置匹配。

### 3.2 DEMO编写

创建自己的demo文件(.c文件)

#### 3.2.1 新建工程

#### 3.2.2 配置设备基础信息

在Demo文件中配置设备身份信息、配网PIN码及加密AC密钥，需与云平台注册信息完全一致，开发测试阶段可复用示例配置，量产时需替换为实际信息。

```c
// 设备基础信息配置（单模核心：仅启用BLE协议）
static IotcDeviceInfo DEV_INFO = {
    .sn = "FFEE3333",          // 设备唯一序列号，量产建议拼接MAC地址
    .prodId = "00007",         // 云平台注册产品ID
    .subProdId = "",           // 无子产品时留空
    .model = "PD31",           // 设备型号，对应PD31平台
    .devTypeId = "1007",       // 云平台设备类型ID
    .devTypeName = "TabletPD31",// 设备类型名称，APP显示用
    .manuId = "104",           // 云平台注册厂商ID
    .manuName = "OpenValley",  // 厂商名称，APP显示用
    .devName = "OneConnectName",// 设备名称，APP端显示
    .fwv = "1.0.0",            // 固件版本
    .hwv = "1.0.0",            // 硬件版本
    .swv = "1.0.0",            // 软件版本
    .protType = IOTC_PROT_TYPE_BLE, // 蓝牙单模协议，区别于双模的BLE+WiFi
};

// 配网PIN码：8位数字，与APP配网时输入一致
static const char *PIN_CODE = "01234567";

// 端云通信加密AC密钥：32字节（示例为48字节，需按IOTC_AC_KEY_LEN修正）
const uint8_t AC_KEY[IOTC_AC_KEY_LEN] = {
    0x49, 0x3F, 0x45, 0x4A, 0x3A, 0x72, 0x38, 0x7B, 
    // 省略部分字节，实际需与云平台密钥完全一致
};
```

#### 3.2.3 服务定义与处理模块（核心业务）

本地控物模型是通用互联APP固定的，固定对应"switch"、"gps"、"allService"，定义设备支持的服务列表及处理函数映射表，本Demo仅含开关和GPS服务，开关服务支持指令接收（Put）和状态查询（Get），GPS服务仅支持状态上报（Get）,"allService"服务写法可以参考[wifi_ble_combo_hi3863](./wifi_ble_combo_hi3863.md)。

```c
// 服务列表：与云平台物模型服务ID、类型严格对齐
static const IotcServiceInfo SVC_INFO[] = {
    {"switch", "switch"},    // 开关服务：控制LED灯
    {"gps","gps"},           // GPS服务：上报定位数据
};

// 服务处理函数映射表：关联服务与对应业务逻辑
const struct SvcMap {
    const IotcServiceInfo *svc;          // 服务信息
    int32_t (*putCharState)(const IotcServiceInfo *svc, const char *data, uint32_t len); // 指令接收
    int32_t (*getCharState)(const IotcServiceInfo *svc, char **data, uint32_t *len);     // 状态查询
} SVC_MAP[] = {
    {&SVC_INFO[0], SwitchPutCharState, SwitchGetCharState},  // 开关服务（支持Put/Get）
    {&SVC_INFO[1], NULL, GpsGetCharState},                   // GPS服务（仅支持Get）
};
```

#### 3.2.4 核心服务解析

##### 3.2.4.1 硬件控制模块（GPIO与LED）

实现GPIO初始化及LED控制函数，配置H3863开发板GPIO5为输出模式，用于接收开关指令驱动LED亮灭。

```c
// 全局变量：存储开关状态（false=关闭，true=开启）
static bool g_switch = false;

// GPIO初始化：配置GPIO5为输出，初始熄灭LED
static void GpioInitTask(void)
{
    IoTGpioInit(LED_TASK_GPIO);                    // 初始化GPIO5
    IoTGpioSetDir(LED_TASK_GPIO, IOT_GPIO_DIR_OUT); // 设置为输出模式
    IoTGpioSetOutputVal(LED_TASK_GPIO, 0);         // 初始低电平（LED灭）
}

// LED控制：根据指令设置GPIO电平
static void SetGpio(int value)
{
    IoTGpioSetOutputVal(LED_TASK_GPIO, value); // 1=亮，0=灭（需适配硬件极性）
}
```

##### 3.2.4.2 开关服务（switch）

处理APP下发的开关控制指令及状态查询请求，核心逻辑为JSON解析、状态更新及GPIO控制。

```c
// 开关指令接收（Put）：解析APP指令，控制LED
static int32_t SwitchPutCharState(const IotcServiceInfo *svc, const char *data, uint32_t len)
{   
    (void)svc; // 屏蔽未使用参数警告
    DEMO_LOG("SwitchPutCharState in");
    if (data == NULL || len == 0) { return -1; } // 参数校验

    cJSON *json = cJSON_Parse(data); // 解析APP下发JSON指令
    if (json == NULL) { return -1; }

    cJSON *item = cJSON_GetObjectItem(json, "on"); // 提取"on"字段（0=关，1=开）
    if (item == NULL || !cJSON_IsNumber(item)) {
        cJSON_Delete(json);
        return -1;
    }

    int32_t on = cJSON_GetNumberValue(item);
    g_switch = on == 0 ? false : true; // 更新状态
    SetGpio(on); // 控制LED

    cJSON_Delete(json); // 释放JSON对象，避免内存泄漏
    return 0;
}

// 开关状态查询（Get）：上报当前LED状态给APP
static int32_t SwitchGetCharState(const IotcServiceInfo *svc, char **data, uint32_t *len)
{
    DEMO_LOG("SwitchGetCharState in");
    if (data == NULL || *data != NULL) { return -1; }

    cJSON *json = cJSON_CreateObject(); // 创建JSON响应对象
    if (json == NULL) { return -1; }

    cJSON_AddNumberToObject(json, "on", g_switch); // 封装状态（0/1）
    *data = cJSON_PrintUnformatted(json); // 转换为无格式JSON字符串
    *len = strlen(*data);

    cJSON_Delete(json);
    return 0;
}
```

##### 3.2.4.3 GPS服务（gps）

仅支持状态上报，用固定经纬度模拟GPS数据（实际场景需对接GPS模块实时读取），封装为JSON格式返回给APP。

```c
// 全局变量：模拟GPS经纬度
static double g_latitude = 30.268681;
static double g_longitude = 120.110690;

// GPS状态上报（Get）：返回当前定位数据
static int32_t GpsGetCharState(const IotcServiceInfo *svc, char **data, uint32_t *len)
{
    DEMO_LOG("GpsGetCharState in");
    if (data == NULL || *data != NULL) { return -1; }

    cJSON *json = cJSON_CreateObject();
    if (json == NULL) { return -1; }

    // 创建经纬度数值对象，保证高精度
    cJSON *latitudeItem = cJSON_CreateNumber(g_latitude);
    cJSON *longitudeItem = cJSON_CreateNumber(g_longitude);
    if (latitudeItem == NULL || longitudeItem == NULL) {
        cJSON_Delete(json);
        return -1;
    }

    cJSON_AddItemToObject(json, "latitude", latitudeItem);
    cJSON_AddItemToObject(json, "longitude", longitudeItem);
    *data = cJSON_PrintUnformatted(json);
    *len = strlen(*data);

    cJSON_Delete(json);
    return 0;
}
```

##### 3.2.4.4 服务分发与全量上报

实现指令分发回调函数，根据服务ID匹配对应处理逻辑；提供全量状态上报接口，批量采集所有服务状态并上报至APP/云侧。

- **指令分发**：遍历服务映射表，将APP指令分发至对应服务的Put/Get函数。

- **全量上报**：批量采集状态并调用组件API上报，上报后释放JSON内存。
  
  ```c
  static int32_t ReportAll(void)
  {
      DEMO_LOG("ReportAll in");
      // 定义上报数据数组，大小与服务映射表一致，初始化清零避免野指针
      IotcCharState reportInfo[sizeof(SVC_MAP) / sizeof(SVC_MAP[0])] = {0};
      int32_t ret = 0;          // 函数返回值，存储错误码
      uint32_t rptNum = 0;      // 有效上报服务数量（过滤无Get接口的服务）
  
      // 遍历所有服务映射项，收集可上报的服务状态
      for (uint32_t i = 0; i < (sizeof(SVC_MAP) / sizeof(SVC_MAP[0])); ++i) {
          DEMO_LOG("i is %d",i);
          // 跳过无状态查询接口（getCharState为NULL）的服务
          if (SVC_MAP[i].getCharState == NULL) {
              DEMO_LOG("i is %d getCharState is null",i);
              continue;
          }
          // 关联当前服务ID，用于APP/云侧识别服务类型
          reportInfo[rptNum].svcId = SVC_MAP[i].svc->svcId;
          DEMO_LOG("i is %d svcId is %s",i,SVC_MAP[i].svc->svcId);
          // 调用对应服务的Get接口，获取状态数据（JSON格式）并填充至上报数组
          ret = SVC_MAP[i].getCharState(SVC_MAP[rptNum].svc, (char **)&reportInfo[rptNum].data, &reportInfo[rptNum].len);
          DEMO_LOG("ret is %d",ret);
          // 若获取状态失败，打印日志并跳出循环，终止上报流程
          if (ret != 0) {
              DEMO_LOG("get char sid:%s error %d", reportInfo[rptNum].svcId, ret);
              break;
          }
          rptNum++; // 有效上报数量自增
      }
  
      // 若状态收集无异常，调用组件API批量上报所有有效服务状态
      if (ret == 0) {
          ret = IotcOhDevReportCharState(reportInfo, rptNum);
      }
  
      // 遍历上报数组，释放cJSON分配的内存（避免内存泄漏）
      for (uint32_t i = 0; i < (sizeof(SVC_MAP) / sizeof(SVC_MAP[0])); ++i) {
          if (reportInfo[i].data != NULL) {
              cJSON_free((char *)reportInfo[i].data);
              reportInfo[i].data = NULL; // 置空防止重复释放
          }
      }
  
      return ret; // 返回最终执行结果
  }
  ```

#### 3.2.5 IoTConnect组件对接

##### 3.2.5.1 核心回调注册

向IoTConnect组件注册指令处理、安全认证、全量上报等回调函数，组件收到APP指令或触发对应事件时自动调用。

```c
// 注册指令接收回调（APP下发控制指令时触发）
SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_PUT_CHAR_STATE_CALLBACK, PutCharState);
// 注册状态查询回调（APP查询状态时触发）
SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_GET_CHAR_STATE_CALLBACK, GetCharState);
// 注册全量状态上报回调（设备上线时触发）
SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_REPORT_ALL_CALLBACK, ReportAll);
// 注册PIN码获取回调（配网鉴权时触发）
SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_GET_PINCODE_CALLBACK, GetPincode);
// 注册AC KEY获取回调（端云加密时触发）
SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_GET_AC_KEY_CALLBACK, GetAcKey);
// 注册重启回调（组件通知设备重启时触发）
SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_REBOOT_CALLBACK, NoticeReboot);
```

##### 3.2.5.2 组件初始化与启动

初始化设备管理模块、启用BLE功能，配置设备/服务信息及BLE参数，启动组件核心流程，适配蓝牙单模场景。

```c
int32_t IotcOhDemoEntry(void)
{
    DEMO_LOG("IotcOhDemoEntry in");
    int32_t ret = IotcOhDevInit(); // 初始化设备管理模块
    if (ret != 0) { return ret; }

    ret = IotcOhBleEnable(); // 仅启用BLE模块（单模核心）
    if (ret != 0) { return ret; }

    // 注册核心回调
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_PUT_CHAR_STATE_CALLBACK, PutCharState);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_GET_CHAR_STATE_CALLBACK, GetCharState);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_REPORT_ALL_CALLBACK, ReportAll);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_GET_PINCODE_CALLBACK, GetPincode);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_GET_AC_KEY_CALLBACK, GetAcKey);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_DATA_FREE_CALLBACK, cJSON_free);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_REBOOT_CALLBACK, NoticeReboot);

    // 配置设备与服务信息
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_DEV_INFO, &DEV_INFO);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_SVC_INFO, SVC_INFO, sizeof(SVC_INFO)/sizeof(SVC_INFO[0]));

    // 配置BLE广播超时（需与ADV_TIMEOUT统一，避免冲突）
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_BLE_START_UP_ADV_TIMEOUT, (1000 * 60 * 1000));
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_SDK_REG_EVENT_LISTENER, DemoBleEventListener);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_SDK_CONFIG_PATH, "/data/app/iotc");

    ret = IotcOhMain(); // 启动组件核心线程
    return ret;
}
```

#### 3.2.6 事件监听与设备管理

##### 3.2.6.1 BLE事件监听

监听BLE核心事件，在组件初始化完成或连接断开时重启广播，确保设备始终可被APP发现。

```c
static void DemoBleEventListener(int32_t event)
{
    DEMO_LOG("DemoBleEventListener in");
    int32_t ret = 0;
    switch (event) {
        case IOTC_CORE_COMM_EVENT_MAIN_INITIALIZED: // 组件初始化完成
        case IOTC_CORE_BLE_EVENT_GATT_DISCONNECT:   // BLE连接断开
            ret = IotcOhBleStartAdv(ADV_TIMEOUT);   // 重启广播（永不超时）
            break;
        default:
            return;
    }
    DEMO_LOG("event[%d] ret:%d", event, ret);
}
```

##### 3.2.6.2 设备重启回调

组件通知设备重启时触发，示例为空实现，可根据需求添加数据保存、模块重置等逻辑。

```c
static int32_t NoticeReboot(IotcRebootReason res)
{
    DEMO_LOG("notice reboot res %d", res);
    return 0;
}
```

#### 3.2.7 任务启动

基于CMSIS-OS2创建业务线程，延时等待BLE模块就绪后，初始化组件及GPIO，通过循环延时降低CPU占用。

```c
int IotTask(void)
{
    DEMO_LOG("sleep 10 seconds to wait ble ready.");
    osDelay(10000); // 延时10秒，等待BLE模块初始化
    IotcOhDemoEntry(); // 启动IoTConnect组件
    GpioInitTask();    // 初始化GPIO（LED）
    while (true) {
        osDelay(1000); // 每秒延时，降低CPU占用
    }
    return 0;
}

// 系统启动入口：创建业务线程
static void IOTBleExampleEntry(void)
{
    osThreadAttr_t attr;
    attr.name = "IotTask";          // 线程名称
    attr.attr_bits = 0U;            // 线程属性标志
    attr.cb_mem = NULL;             // 回调内存
    attr.cb_size = 0U;              // 回调内存大小
    attr.stack_mem = NULL;          // 栈内存（系统分配）
    attr.stack_size = 0x4000;       // 栈大小：16KB（单模需求）
    attr.priority = 19;             // 中等优先级

    if (osThreadNew((osThreadFunc_t)IotTask, NULL, &attr) == NULL) {
        printf("[BLEExample] Failed to create LedTask!\n");
    }
}

SYS_RUN(IOTBleExampleEntry); // 注册为系统启动任务，PD31开机自动执行
```

### 3.3 BUILD.gn配置

- 核心作用
  
  `BUILD.gn`是 OpenHarmony/LiteOS-M 平台的编译配置文件（基于 GN 构建系统），该文件的核心作用：
  
  - 定义 Demo 的编译类型（静态库`static_library`）；
  - 指定 Demo 的源码文件、依赖库、头文件路径；
  - 配置编译选项（如警告等级、宏定义、安全编译规则）；
  - 关联 IoTConnect 核心组件，确保编译时能正确链接依赖。
  
  配置前需确保以下路径 / 组件已存在（与`build.gn`中的路径对应），否则会导致编译失败：
  
  - IoTConnect 组件已部署到`//foundation/communication/iot_connect/`目录；
  
  - ws63 SDK 源码已部署到`//device/soc/hisilicon/ws63v100/sdk/`目录；
  
  - 第三方依赖（cJSON、bounds_checking_function）已存在于 OpenHarmony 源码对应路径；
  
  - H3863 GPIO / 蓝牙驱动源码已部署到对应路径。

- 配置解析
  
  详细BUILD.gn配置的注释如下所示

```gn
# 导入IoTConnect组件核心配置文件，加载组件编译依赖及宏定义
import("//foundation/communication/iot_connect/iotc.gni")

# 定义静态库编译目标，名称为"iotc_ble_demo"，用于生成Demo核心静态库
static_library("iotc_ble_demo") {
  # 指定Demo核心源码文件，仅包含蓝牙单模Demo主文件
  sources = [
   "iotc_oh_demo_ble.c",
  ]

  # 配置依赖库，确保编译时能链接所需第三方库及核心组件
  deps = [
    // 安全函数库，提供内存操作等安全校验接口（如memcpy_s）
    "//third_party/bounds_checking_function:libsec_shared",
    // cJSON静态库，用于Demo中JSON数据的解析与生成（开关/GPS数据处理）
    "//build/lite/config/component/cJSON:cjson_static",
    // IoTConnect核心静态库，提供蓝牙通信、设备管理等核心能力
    "//foundation/communication/iot_connect:iotc_static",
  ]

  # 配置头文件搜索路径，确保编译时能找到依赖组件的头文件
  include_dirs = [
    // IoTConnect组件通用接口头文件路径
    "//foundation/communication/iot_connect/interfaces/kits/common",
    // IoTConnect OH连接模式接口头文件路径
    "//foundation/communication/iot_connect/interfaces/kits/oh_connect",
    // IoTConnect适配器层头文件路径，适配底层硬件与系统
    "//foundation/communication/iot_connect/adapter/include",
    // LittleFS文件系统头文件路径，用于组件配置文件存储
    "//device/soc/hisilicon/ws63v100/sdk/open_source/littlefs/v2.5.0",
    // 硬件外设内部API头文件路径，支撑GPIO等硬件操作
    "//base/iothardware/peripheral/interfaces/inner_api",
  ]

  # 配置C语言编译选项，强化编译检查与安全防护
  cflags = [
    "-ftrapv",                  // 检测整数溢出并触发陷阱
    "-Werror",                  // 将所有警告视为错误，强制修正潜在问题
    "-Wextra",                  // 开启额外警告检查，提升代码规范性
    "-Wshadow",                 // 检测变量隐藏问题（如局部变量覆盖全局变量）
    "-fstack-protector-all",    // 开启全栈保护，防止栈溢出攻击
    "-D_FORTIFY_SOURCE=2",      // 增强内存操作函数安全性（如strcpy、memcpy）
    "-Wformat=2",               // 强化格式化字符串检查，防止格式注入漏洞
    "-Wfloat-equal",            // 警告浮点数直接相等比较，避免精度问题
    "-Wdate-time",              // 警告代码中包含日期时间，确保版本一致性
    "-Wno-error=unused-parameter", // 不将未使用参数视为错误（适配部分回调接口）
    "-Wall",                    // 开启所有基础警告检查
    "-fPIC",                    // 生成位置无关代码，适配动态链接需求
    "-Wunused-parameter",       // 警告未使用参数，提醒优化代码
  ]
}
```

## 四、DEMO编译

BLE Only（Hi3863平台）的编译方法以及步骤请参考示例代码仓的[README.md](https://gitcode.com/ohos-oneconnect/applications_sample_iot_connect_samples/blob/master/ble/liteos/ws63/README_zh.md);

## 五、DEMO使用说明

- 5.1 蓝牙单模设备点对点本地控制
1. 联系OpenHarmony统一互联PMC或在laval社区提单，完成APP白名单配置;

2. 编译通用互联APP源码，生成HAP包并安装至HarmonyOS Next手机。

3. 通过USB数据线连接H3863开发板与电脑，使用[烧录工具](https://www.bearpi.cn/core_board/bearpi/pico/h3863/software/%E4%B8%8B%E8%BD%BD%E7%83%A7%E5%BD%95.html)将编译生成的镜像烧录至开发板。

4. 重启开发板，等待10秒（BLE模块就绪），打开通用互联APP，进入“点对点本地控”页面，自动开始扫描设备。

5. 在扫描结果中选择设备名字为”PD31“设备，拖动到中心手机图标，等待片刻，完成蓝牙配对。

6. 配对成功后，点击设备进入详情页，可控制LED开关状态，查看GPS信息。