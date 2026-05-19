# BLE/SLE双模设备基于Hi3863平台DEMO编译构建

## 一、开发环境准备

### 1.1 硬件要求

| 设备类型 | 配置要求                                                          |
| ---- | ------------------------------------------------------------- |
| 开发板  | BearPi-pico H3863（Hi3863 芯片），需配备 USB 数据线（用于烧录和调试），后续简称为Hi3863 |

### 1.2 软件要求

| 软件/工具         | 版本要求                    | 用途说明                       |
| ------------- | ----------------------- | -------------------------- |
| OpenHarmony源码 | 5.1.0 release           | 基础系统源码，为 Hi3863 平台提供编译底座   |
| IoTConnect 组件 | 最新 master 分支            | 设备互联核心能力依赖                 |
| 通用互联APP       | 最新 master 分支            | 鸿蒙生态核心控制入口，运行在HarmonyOS手机上 |
| 编译工具链         | arm-none-eabi-gcc 9.3.1 | Hi3863 芯片的 ARM 架构编译工具链     |
| hb 构建工具       | 0.4.6 及以上               | OpenHarmony 轻量级设备编译构建工具    |
| Python        | 3.8~3.9                 | 运行 hb 工具及编译脚本              |
| Git           | 2.25 及以上                | 克隆仓库代码                     |

## 二、示例代码

[BLE/SLE代码](https://gitcode.com/ohos-oneconnect/communication_nearlink/tree/master/openharmony/vendor/hihope/nearlink_dk3863e/app)

## 三、代码开发详解

### 3.1、云平台注册

- 登录OpenHarmony统一互联云平台，注册产品信息，获取产品ID（prodId）、厂商ID（manuId）及AC密钥，需与Demo中设备信息配置严格一致。

- 创建物模型，添加所需服务，定义服务ID、类型及数据格式，与Demo服务配置匹配。

### 3.2、OpenHarmony底座及IoT Connect SDK导入

开发者可以使用下列命令拉取OH 5.1小型系统底座及IoT Connect SDK，并通过执行bash copy.sh命令一键修改基本配置。

```bash
repo init -u git@gitcode.com:ohos-oneconnect/manifest.git -b OpenHarmony-5.1.0-Release -m nearlink.xml -g ohos:mini --no-repo-verify
repo sync -c
repo forall -c 'git lfs pull'
bash build/prebuilts_download.sh  (如执行不成功，可加上sudo权限）

通过cd切换到foundation/communication/nearlink/目录,
执行bash copy.sh  //使得nearlink仓目录下的openharmony目录覆盖到代码根目录
```

### 3.3、DEMO编写

#### 3.3.1 工程目录结构

```
nearlink_dk3863e/
├── app/                      # 应用层代码
│   ├── gas_control_valves/   # 燃气控制阀Demo
│   │   ├── BUILD.gn          # 构建配置
│   │   ├── gas_control_valves.c  # 主业务逻辑
│   │   ├── inc/              # 头文件目录
│   │   │   ├── app_iotc.h    # IoTConnect配置定义
│   │   │   ├── app_log.h    # 日志接口定义
│   │   │   └── hi3863_led_btn.h  # LED/按键硬件接口
│   │   └── src/              # 源文件目录
│   │       ├── app_iotc_ble.c   # BLE业务流程实现
│   │       ├── app_iotc_sle.c   # SLE业务流程实现
│   │       ├── hi3863_led.c     # LED驱动实现
│   │       └── hi3863_button.c  # 按键驱动实现
│   └── gas_leak_detector/    # 燃气泄漏探测器Demo
├── hals/                     # 硬件抽象层
│   ├── utils/
│   │   ├── sys_param/        # 系统参数管理
│   │   └── token/           # Token管理
├── BUILD.gn                  # 根目录构建配置
└── config.json               # 产品配置
```

#### 3.3.2 设备基础信息配置

在demo文件中配置自己产品的信息，产品需要和云平台注册产品信息进行对应，开发测试阶段用户可以使用统一互联sample仓中demo的产品信息进行配置调试。其中，pid需要与云端注册的pid保持一致，其他根据用户要求以及满足字段定义规则即可。

```c
// 设备基础信息配置（必须与APP/云侧一致）
static IotcDeviceInfo DEV_INFO = {
    .sn = "GC0001",                      // 设备序列号，全局唯一
    .prodId = "00194",                    // 产品ID，由平台分配
    .subProdId = "00",                    // 子产品ID，区分同系列不同型号
    .model = "GAS-CONTROL-VALVES-01",    // 设备型号
    .devTypeId = "2002",                  // 设备类型ID
    .devTypeName = "GasControlValves",    // 设备类型名称
    .manuId = "100",                      // 厂商ID
    .manuName = "SwanLink",               // 厂商名称
    .devName = "GAS-CONTROL-VALVES-01",   // 设备名称
    .fwv = "1.0.0",                       // 固件版本
    .hwv = "1.0.0",                       // 硬件版本
    .swv = "1.0.0",                       // 软件版本
    .protType = IOTC_PROT_TYPE_BLE,       // 通信协议类型（BLE配网模式）
};

// 配网PIN码（APP配网时的鉴权码，需与APP侧一致）
static const char *PIN_CODE = "01234567";

// 厂商AC KEY（端云通信的加密密钥，需与云侧一致）
static const uint8_t AC_KEY[IOTC_AC_KEY_LEN] = {
    0x49, 0x3F, 0x45, 0x4A, 0x3A, 0x72, 0x38, 0x7B, 0x36, 0x32, 0x50, 0x3C, 
    0x49, 0x39, 0x62, 0x38, 0x72, 0xCB, 0x6D, 0xC5, 0xAE, 0xE5, 0x4A, 0x82, 
    0xD3, 0xE5, 0x6D, 0xF5, 0x36, 0x82, 0x62, 0xEB, 0x89, 0x30, 0x6C, 0x88, 
    0x32, 0x56, 0x23, 0xFD, 0xB8, 0x67, 0x90, 0xA7, 0x7B, 0x61, 0x1E, 0xAE
};
```

#### 3.3.3 服务定义与处理模块

业务模块种类主要根据用户上传云端的物模型来确定，本Demo的BLE和SLE服务定义如下：

**BLE模式服务定义：**

```c
static const IotcServiceInfo SVC_INFO[] = {
    {"snw", "snw"},           // 设备序列号服务
    {"allservices", "allservices"}, // 全量服务
    {"devInfo", "devInfo"},   // 设备信息
    {"controlValves", "controlValves"} // 阀门控制
};
```

**BLE服务处理函数映射表：**

```c
const struct SvcMap {
    const IotcServiceInfo *svc;
    int32_t (*putCharState)(const IotcServiceInfo *svc, const char *data, uint32_t len);
    int32_t (*getCharState)(const IotcServiceInfo *svc, char **data, uint32_t *len);
} SVC_MAP[] = {
    {&SVC_INFO[0], SnPutCharState, SnGetCharState},      // snw服务
    {&SVC_INFO[1], SwitchPutCharState, SwitchGetCharState}, // 开关服务
};
```

**SLE服务处理函数映射表：**

```c
const struct SvcMap {
    const IotcServiceInfo *svc;
    int32_t (*putCharState)(const IotcServiceInfo *svc, const char *data, uint32_t len);
    int32_t (*getCharState)(const IotcServiceInfo *svc, char **data, uint32_t *len);
} SLE_SVC_MAP[] = {
    {&SVC_INFO[2], devInfoPutCharState, devInfoGetCharState},    // devInfo服务
    {&SVC_INFO[3], ControlValvesPutCharState, ControlValvesGetCharState}, // controlValves服务
};
```

该模块定义设备支持的 "服务"，并实现服务的 "指令接收（Put）" 和 "状态查询（Get）" 逻辑，是 Demo 的业务核心。

#### 3.3.4 核心服务解析

##### 3.3.4.1 BLE模式服务

**开关服务（switch）**

开关服务处理APP下发的开关控制指令：

```c
static int32_t SwitchPutCharState(const IotcServiceInfo *svc, const char *data, uint32_t len)
{
    DEMO_LOG("SwitchPutCharState call in ");
    if (data == NULL || len == 0) {
        return -1;
    }
    cJSON *json = cJSON_Parse(data);
    if (json == NULL) {
        return -1;
    }

    cJSON *item = cJSON_GetObjectItem(json, "on");
    if (item == NULL || !cJSON_IsNumber(item)) {
        cJSON_Delete(json);
        return -1;
    }

    int32_t on = cJSON_GetNumberValue(item);
    DEMO_LOG("switch on put %d=>%d", g_switch, on);
    g_switch = on == 0 ? false : true;

    cJSON_Delete(json);
    return 0;
}
```

**开关状态查询函数：**

```c
static int32_t SwitchGetCharState(const IotcServiceInfo *svc, char **data, uint32_t *len)
{
    if (data == NULL || *data != NULL) {
        return -1;
    }

    cJSON *json = cJSON_CreateObject();
    if (json == NULL) {
        return -1;
    }

    bool gas_status = false;
    if (cJSON_AddNumberToObject(json, "on", gas_status) == NULL) {
        cJSON_Delete(json);
        return -1;
    }

    *data = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    if (*data == NULL) {
        return -1;
    }
    *len = strlen(*data);
    return 0;
}
```

**SNW服务（设备信息查询）：**

```c
static int32_t SnGetCharState(const IotcServiceInfo *svc, char **data, uint32_t *len)
{
    cJSON *json = cJSON_CreateObject();
    if (json == NULL) {
        return -1;
    }
    IotcDeviceInfo dev = app_get_dev_info();
    // 获取sn
    if (cJSON_AddStringToObject(json, "sn", dev.sn) == NULL) { ... }
    if (cJSON_AddStringToObject(json, "devTypeId", dev.devTypeId) == NULL) { ... }
    if (cJSON_AddStringToObject(json, "devTypeName", dev.devTypeName) == NULL) { ... }
    if (cJSON_AddStringToObject(json, "prodId", dev.prodId) == NULL) { ... }
    if (cJSON_AddStringToObject(json, "model", dev.model) == NULL) { ... }

    *data = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    *len = strlen(*data);
    return 0;
}
```

##### 3.3.4.2 SLE模式服务

**devInfo服务（设备信息上报）：**

```c
static int32_t devInfoGetCharState(const IotcServiceInfo *svc, char **data, uint32_t *len)
{
    cJSON *json = cJSON_CreateObject();
    if (json == NULL) {
        return -1;
    }

    if (cJSON_AddStringToObject(json, "brand", BRAND) == NULL) { ... }
    if (cJSON_AddStringToObject(json, "serialNumber", DEV_INFO.sn) == NULL) { ... }
    if (cJSON_AddStringToObject(json, "deviceID", DEV_INFO.prodId) == NULL) { ... }
    if (cJSON_AddNumberToObject(json, "controlValves", g_control_valves) == NULL) { ... }

    *data = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    *len = strlen(*data);
    return 0;
}
```

**controlValves服务（阀门控制）：**

```c
static int32_t ControlValvesPutCharState(const IotcServiceInfo *svc, const char *data, uint32_t len)
{
    cJSON *json = cJSON_Parse(data);
    if (json == NULL) {
        return -1;
    }

    cJSON *item = cJSON_GetObjectItem(json, "controlValves");
    if (item == NULL || !cJSON_IsNumber(item)) {
        cJSON_Delete(json);
        return -1;
    }

    int32_t value = cJSON_GetNumberValue(item);
    DEMO_LOG("control Valves put %d=>%d", g_control_valves, value);
    g_control_valves = value;

    cJSON_Delete(json);
    return 0;
}
```

#### 3.3.5 BLE/SLE模式切换连接桥设备逻辑

根据设备是否已完成配网选择是否开启sle，开启sle后可以被桥设备扫描到

```c
static void gas_control_valves_print(const char *arg)
{
    fs_adapt_mount();
    init_led();
    button_init();

    // 检查是否存在认证文件
    if (check_iotc_file(IOTC_AUTHCODE_PATH)) {
        // 已配网，直接启动SLE连接
        app_iotc_sle_start();
        g_connect_status = 5;
    } else {
        // 未配网，启动BLE配网
        app_iotc_ble_init();
        g_connect_status = 1;
    }

    while (1) {
        if (g_connect_status <= 2) {
            // 定期检查认证文件
            if (runCounter % 5 == 0) {
                ret = check_iotc_file(IOTC_AUTHCODE_PATH);
            }
            if (ret) {
                change_connect_sle();  // 触发切换到SLE
            }
        } else if (g_connect_status == 3) {
            // 关闭BLE
            app_iotc_ble_exit();
        } else if (g_connect_status == 4) {
            // 启动SLE
            app_iotc_sle_start();
        }
        osal_msleep(1000);
    }
}
```

**关键点说明：**

- `IOTC_AUTHCODE_PATH`: 认证文件路径，配网成功后会在文件系统保存
- `check_iotc_file()`: 检查认证文件是否存在
- 配网成功后会从BLE切换到SLE连接

#### 3.3.6 IoTConnect 组件对接

**BLE初始化与回调注册：**

```c
void app_iotc_ble_init(void)
{
    // 初始化设备信息模块
    int32_t ret = IotcOhDevInit();
    if (ret != 0) {
        DEMO_LOG("init device error %d", ret);
    }

    // 启用BLE模块
    ret = IotcOhBleEnable();
    if (ret != 0) {
        DEMO_LOG("enable ble connect error %d", ret);
    }

    // 注册回调函数
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_PUT_CHAR_STATE_CALLBACK, PutCharState);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_GET_CHAR_STATE_CALLBACK, GetCharState);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_REPORT_ALL_CALLBACK, ReportAll);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_GET_PINCODE_CALLBACK, GetPincode);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_GET_AC_KEY_CALLBACK, GetAcKey);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_DATA_FREE_CALLBACK, cJSON_free);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_REBOOT_CALLBACK, NoticeReboot);

    // 配置设备信息
    IotcDeviceInfo dev = app_get_dev_info();
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_DEV_INFO, &dev);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_SVC_INFO, SVC_INFO, sizeof(SVC_INFO) / sizeof(SVC_INFO[0]));

    // 启动后的广播超时时间（10分钟）
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_BLE_START_UP_ADV_TIMEOUT, (10 * 60 * 1000));

    // 配置BLE Connect能力在配网完成后退出
    IotcOhSetOption(IOTC_OH_OPTION_BLE_EXIT_AFTER_NETCFG);

    // 启动IoTConnect主流程
    ret = IotcOhMain();
    if (ret != 0) {
        DEMO_LOG("iotc oh main error %d", ret);
        return ret;
    }
}
```

**SLE初始化与回调注册：**

```c
void app_iotc_sle_start(void)
{
    // 初始化设备信息模块
    int32_t ret = IotcOhDevInit();

    // 启用SLE模块
    ret = IotcOhSleEnable();
    if (ret != 0) {
        DEMO_LOG("enable sle connect error %d", ret);
        return;
    }

    // 注册回调函数
    SLE_SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_PUT_CHAR_STATE_CALLBACK, SlePutCharState);
    SLE_SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_GET_CHAR_STATE_CALLBACK, SleGetCharState);
    SLE_SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_REPORT_ALL_CALLBACK, SleReportAll);
    SLE_SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_GET_PINCODE_CALLBACK, SleGetPincode);
    SLE_SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_GET_AC_KEY_CALLBACK, SleGetAcKey);
    SLE_SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_DATA_FREE_CALLBACK, cJSON_free);
    SLE_SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_REBOOT_CALLBACK, SleNoticeReboot);

    // 配置设备信息
    IotcDeviceInfo dev = app_get_dev_info();
    SLE_SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_DEV_INFO, &dev);
    SLE_SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_SVC_INFO, SVC_INFO, sizeof(SVC_INFO) / sizeof(SVC_INFO[0]));

    // 设置SLE广播超时（10分钟）
    SLE_SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_SLE_START_UP_ADV_TIMEOUT, (10 * 60 * 1000));

    // 注册自定义数据接收回调
    SLE_SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_SLE_RECV_CUSTOM_DATA_CALLBACK, SleRecvCustomData);

    // 启动IoTConnect主流程
    ret = IotcOhMain();
}
```

#### 3.3.7 SLE自定义数据收发

SLE模式下支持自定义业务数据的收发：

**发送自定义数据：**

```c
void sendCustomData(char *dev_id)
{
    cJSON *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "controlValves", g_control_valves);

    cJSON *vendorItem = cJSON_CreateObject();
    cJSON_AddStringToObject(vendorItem, "sid", "controlValves");
    cJSON_AddItemToObject(vendorItem, "data", json);

    cJSON *vendor = cJSON_CreateArray();
    cJSON_AddItemToArray(vendor, vendorItem);

    cJSON *jsonData = cJSON_CreateObject();
    cJSON_AddItemToObject(jsonData, "vendor", vendor);
    cJSON_AddNumberToObject(jsonData, "seq", g_seq);

    char *data = cJSON_PrintUnformatted(jsonData);
    uint32_t len = strlen(data);

    // 调用SLE发送接口
    int32_t ret = IotcOhSleSendCustomSecData(dev_id, data, len);
    if (ret == EOK) {
        g_seq++;
    }

    // 释放内存
    cJSON_free(data);
    cJSON_Delete(jsonData);
}
```

**接收自定义数据：**

```c
int32_t SleRecvCustomData(const char *data, uint32_t len)
{
    cJSON *json = cJSON_Parse(data);
    if (!cJSON_IsArray(json)) {
        return -1;
    }

    for (uint8_t i = 0; i < cJSON_GetArraySize(json); ++i) {
        cJSON *serviceItem = cJSON_GetArrayItem(json, i);
        cJSON *dataJson = cJSON_GetObjectItem(serviceItem, "data");

        // 解析controlValves指令
        cJSON *controlValvesJson = cJSON_GetObjectItem(dataJson, "controlValves");
        if (controlValvesJson != NULL) {
            app_receive_valve_state(controlValvesJson->valueint);
        }

        // 解析restart指令
        cJSON *actionRestart = cJSON_GetObjectItem(dataJson, "restart");
        if (actionRestart != NULL) {
            if (actionRestart->valueint == IOTC_REBOOT_RESTORE) {
                execute_reset_chip();  // 恢复出厂
            } else {
                execute_reboot_chip();  // 重启设备
            }
        }
    }
    cJSON_Delete(json);
    return 0;
}
```

#### 3.3.8 核心回调函数说明

| 回调函数              | 说明                    |
| ----------------- | --------------------- |
| PutCharState      | 控制指令接收回调，APP下发控制指令时触发 |
| GetCharState      | 状态查询回调，APP查询设备状态时触发   |
| ReportAll         | 全量服务上报回调，设备上线时触发      |
| GetPincode        | PIN码获取回调，配网鉴权时使用      |
| GetAcKey          | AC KEY获取回调，端云加密时使用    |
| NoticeReboot      | 重启回调，收到云端重启指令时触发      |
| NetCfgCallback    | 配网信息接收回调（BLE模式）       |
| SleRecvCustomData | 自定义数据接收回调（SLE模式）      |

### 3.4、BUILD.gn配置

- 核心作用
  
  `BUILD.gn`是 OpenHarmony/LiteOS-M 平台的编译配置文件（基于 GN 构建系统），该文件的核心作用：
  
  - 定义 Demo 的编译类型（静态库`static_library`）；
  - 指定 Demo 的源码文件、依赖库、头文件路径；
  - 配置编译选项（如警告等级、宏定义、安全编译规则）；
  - 关联 IoTConnect 核心组件，确保编译时能正确链接依赖。

- 配置解析
  
  **导入依赖模块：**
  
  ```gn
  import("//build/lite/config/component/lite_component.gni")
  import("//build/ohos.gni")
  ```
  
  **定义编译目标：**
  
  ```gn
  ohos_static_library("gas_control_valves") {
      subsystem_name = "communication"
      part_name = "iot_connect"
      # ...
  }
  ```
  
  **指定源码文件：**
  
  ```gn
  sources = [
      "gas_control_valves.c",
      "src/app_iotc_ble.c",
      "src/app_iotc_sle.c",
      "src/hi3863_led.c",
      "src/hi3863_button.c",
      "src/ws2812/ws2812.c"
  ]
  ```
  
  **配置依赖库：**
  
  ```gn
  deps = [
      "//foundation/communication/iot_connect:iotc_static",
  ]
  ```
  
  **配置头文件路径：**
  
  ```gn
  include_dirs = [
      "//commonlibrary/utils_lite/include",
      "//device/soc/hisilicon/hi3863v100/sdk_liteos/include",
      "//device/soc/hisilicon/hi3863v100/sdk_liteos/include/middleware/services/bts/common",
      "//device/soc/hisilicon/hi3863v100/sdk_liteos/include/middleware/services/bts/ble",
      "//device/soc/hisilicon/hi3863v100/sdk_liteos/kernel/osal/include",
      "//device/soc/hisilicon/hi3863v100/sdk_liteos/drivers/drivers/hal/gpio",
      "//device/soc/hisilicon/hi3863v100/sdk_liteos/middleware/chips/ws63/littlefs",
      "//foundation/communication/iot_connect/interfaces/kits/common",
      "//foundation/communication/iot_connect/interfaces/kits/oh_connect",
      # ...
  ]
  ```
  
  **配置编译选项：**
  
  ```gn
  defines = [
      "Hi3863Led = 1",
      "Hi3863Btn = 1",
  ]
  ```

## 四、编译配置修改及编译指令

BLE/SLE Combo（Hi3863平台）的编译配置修改步骤请参考示例代码仓[README.md](https://gitcode.com/ohos-oneconnect/communication_nearlink/blob/master/openharmony/vendor/hihope/nearlink_dk3863e/README.md);

## 五、DEMO使用说明

- **BLE/SLE双模设备端云控制**
1. 联系OpenHarmony统一互联PMC或在laval社区提单，完成APP白名单配置;

2. 编译ohos-connect-hap源码，[安装编译](../通用互联App.md)的hap 至HarmonyOS Next 手机上；

3. 使用[Hi3863开发板烧录](https://www.bearpi.cn/core_board/bearpi/pico/h3863/software/%E4%B8%8B%E8%BD%BD%E7%83%A7%E5%BD%95.html)BLE/SLE双模Combo镜像；

4. 打开通用互联APP，在我的页面点击设备库同步将设备库更新到最新版本；

5. 点击通用互联APP底部设备tab，点击设备tab右上角“+”号按钮扫描，可发现对应设备；

6. 在扫描结果中点击对应设备，点击下一步，即可对设备开始配网（BLE辅助配网模式）；

7. 配网成功后，设备自动从BLE切换到SLE连接，桥设备可以通过SLE发现生态设备，连接后该生态设备会下挂到桥设备下，完成后APP自动退回设备页面并显示已配网的设备；

8. 点击设备页面已配网设备，在设备详情页可控制设备的阀门状态。
