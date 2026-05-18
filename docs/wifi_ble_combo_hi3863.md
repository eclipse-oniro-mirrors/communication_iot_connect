# WiFi/BLE Combo 设备基于Hi3863平台DEMO编译构建

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

[WiFi/BLE Combo代码](https://gitcode.com/ohos-oneconnect/applications_sample_iot_connect_samples/blob/OpenHarmony-5.1.0-Release/wifi_ble_combo/hi3863/wifi_ble_combo.c)

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

- 新建工程
  
  创建自己的demo文件(.c文件)

- 配置设备基础信息
  
  在demo文件中配置自己产品的信息，产品需要和云平台注册产品信息进行对应，开发测试阶段用户可以使用统一互联sample仓中demo的产品信息进行配置调试。其中，pid需要与云端注册的pid保持一致，其他根据用户要求以及满足字段定义规则即可。
  
  ```c
  // 设备基础信息配置（必须与APP/云侧一致）
  static const IotcDeviceInfo DEV_INFO = {
      .sn = "hi3863test001",          // 设备序列号，全局唯一
      .prodId = "0001H",              // 产品ID，由平台分配
      .subProdId = "63",              // 子产品ID，区分同系列不同型号
      .model = "Hi3863",              // 设备型号
      .devTypeId = "1005",            // 设备类型ID（如智能开关）
      .devTypeName = "SmartSwitch",   // 设备类型名称
      .manuId = "111",                // 厂商ID
      .manuName = "HiSilicon",        // 厂商名称
      .devName = "SmartSwitch",       // 设备名称
      .fwv = "1.0.0",                 // 固件版本
      .hwv = "1.0.0",                 // 硬件版本
      .swv = "1.0.0",                 // 软件版本
      .protType = IOTC_PROT_TYPE_BLE_AND_WIFI, // 通信协议类型（WiFi+BLE双模）
  };
  // 配网PIN码（APP配网时的鉴权码，需与APP侧一致）
  static const char *PIN_CODE = "01234567";
  
  // 厂商AC KEY（端云通信的加密密钥，需与云侧一致）
  const uint8_t AC_KEY[IOTC_AC_KEY_LEN] = {
      0x49, 0x3F, 0x45, 0x4A, 0x3A, 0x72, 0x38, 0x7B, 
      // 共48字节，由平台分配，不可随意修改
  };
  ```

- 服务定义与处理模块（核心业务）
  
  业务模块种类主要根据用户上传云端的物模型来确定，比如示例代码的prodId对应云端的物模型如下：
  
  ```json
  {
    "prodId":"0001H",
    "deviceModel": "Hi3863",
    "deviceTypeId": "1005",
    "deviceTypeName": "SmartSwitch",
    "deviceTypeNameEn": "SmartSwitch",
    "deviceName": "SmartSwitch",
    "manufacturerId": "111",
    "manufacturerName": "HiSilicon",
    "services": [
      {
        "serviceId": "switch",
        "serviceType": "switch",
        "serviceName": "开关",
        "serviceNameEn": "switch",
        "description": "Smart Switch Service",                  
        "characteristics": [
          {
            "characteristicName": "on",
            "characteristicType": "bool",
            "description": "switch button",
            "method": "RW",
            "permission": "GPR",
            "attrName": "开关",
            "attrNameEn": "On",
            "ecaFlag": "CA",
            "enumList": [
              {
                "descCh": "关闭",
                "descEn": "off",
                "enumVal": 0
              },
              {
                "descCh": "打开",
                "descEn": "on",
                "enumVal": 1
              }
            ]
          }
        ]
      },
      {
        "serviceId": "restart",
        "serviceType": "restart",
        "serviceName": "重启",
        "serviceNameEn": "restart",
        "description": "restart Service",
        "characteristics": [
          {
            "characteristicName": "restart",
            "characteristicType": "bool",
            "description": "restart button",
            "method": "RW",
            "permission": "GPR",
            "attrName": "重启",
            "attrNameEn": "Restart",
            "ecaFlag": "",
            "enumList": [
              {
                "descCh": "重启",
                "descEn": "restart",
                "enumVal": 0
              }
            ]
          }
        ]
      }
    ]
  }
  ```
  
  此时，用户则需要根据物模型的services中的serviceId在服务中添加对应的服务，本例的服务有"restart"、"switch"。但配网完成后，点击配网设备详情页，app会自动下发”allServices“服务来获取目前所有服务的状态进行上报，并且点击设备详情页的下级目录，该页面可以展示设备的产品信息、配网信息及gps信息，也就是可以包含”devNetInfo“、”gps“、”snw“服务；以及在本地控界面，物模型目前是采用固定的模型，该模型下包含”switch“、”gps“服务，因此示例demo一共添加了五个服务来满足这些需求，如下所示（restart服务由于芯片平台不同，重启编写方法不同，本例中暂未添加）
  
  ```c
  // 服务信息配置（需与APP侧服务定义一致）
  static const IotcServiceInfo SVC_INFO[] = {
      {"switch", "switch"},    // 开关服务：ID=switch，类型=switch
      {"snw", "snw"},          // 设备序列号服务：ID=snw，类型=snw
      {"gps", "gps"},          // GPS服务：ID=gps，类型=gps
      {"devNetInfo", "devNetInfo"}, // 网络信息服务：ID=devNetInfo，类型=devNetInfo
      {"allServices", "allServices"}, // 全量服务：ID=allServices，类型=allServices
  };
  
  // 服务处理函数映射表（关联服务与处理函数）
  const struct SvcMap {
      const IotcServiceInfo *svc;          // 服务信息
      int32_t (*putCharState)(const IotcServiceInfo *svc, const char *data, uint32_t len); // 指令接收函数
      int32_t (*getCharState)(const IotcServiceInfo *svc, char **data, uint32_t *len);     // 状态查询函数
  } SVC_MAP[] = {
      {&SVC_INFO[0], SwitchPutCharState, SwitchGetCharState},    // 开关服务
      {&SVC_INFO[1], SnPutCharState, SnGetCharState},            // 序列号服务
      {&SVC_INFO[2], GpsPutCharState, GpsGetCharState},          // GPS服务
      {&SVC_INFO[3], DevNetInfoPutCharState, DevNetInfoGetCharState}, // 网络信息服务
      {&SVC_INFO[4], AllServicesPutCharState, AllServicesGetCharState}, // 全量服务
  };
  ```
  
  该模块定义设备支持的 “服务”（如开关控制、GPS 数据、网络信息），并实现服务的 “指令接收（Put）” 和 “状态查询（Get）” 逻辑，是 Demo 的业务核心。

- 核心服务解析
  
  - 开关服务（switch）
    
    开关服务是 Demo 最核心的业务，本例模拟实现 APP 下发 “开灯 / 关灯” 指令，控制 Hi3863 的 GPIO5 输出，驱动 LED 亮灭。
  
  - GPIO 初始化函数
    
    ```c
    // GPIO初始化（配置GPIO5为输出模式）
    static void GpioInitTask(void)
    {
        IoTGpioInit(LED_TASK_GPIO);                    // 初始化GPIO5
        IoTGpioSetDir(LED_TASK_GPIO, IOT_GPIO_DIR_OUT); // 设置为输出模式
        IoTGpioSetOutputVal(LED_TASK_GPIO, 0);         // 初始值：低电平（LED灭）
    }
    ```
    
    ```c
    // GPIO输出控制函数
    static void SetGpio(int value)
    {
        DEMO_LOG("SetGpio value=%d", value);
        IoTGpioSetOutputVal(LED_TASK_GPIO, value); // 根据指令设置GPIO电平（1=亮，0=灭）
    }
    ```
    
    此处新建两个开发板的LED灯控制函数，分别是GPIO初始化和控制，用于后面的开关服务去调用。这里不同的芯片平台，对应的控制方法不同，请开发者根据自己的开发板进行编写自己的控制函数。
  
  - 开关指令接收函数
    
    处理 APP 下发的开关控制指令，核心逻辑：
    
    - 解析 APP 下发的 JSON 指令（支持数组 / 对象两种格式）；
    - 提取 “on” 字段值（0 = 关，1 = 开）；
    - 更新开关状态并控制 GPIO 输出。
    
    ```c
    static int SwitchPutCharState(const IotcServiceInfo *svc, const char *data, uint32_t len)
    {
        DEMO_LOG("SwitchPutCharState in,data: %s", data);
    
        if (data == NULL || len == 0) {
            DEMO_LOG("param invalid");
            return -1;
        }
        cJSON *json = cJSON_Parse(data); // 解析JSON指令
        if (json == NULL) {
            DEMO_LOG("parse error");
            return -1;
        }
    
        // 兼容APP下发的数组/对象两种格式（核心修复逻辑）
        cJSON *item = NULL;
        cJSON *array_item = NULL;
        cJSON *data_obj = NULL;
        if (cJSON_IsArray(json)) { // 若指令是数组格式（如[{"data":{"on":1}}]）
            array_item = cJSON_GetArrayItem(json, 0); // 取数组第一个元素
            data_obj = cJSON_GetObjectItem(array_item, "data"); // 提取data对象
            item = cJSON_GetObjectItem(data_obj, "on"); // 提取on字段
        } else { // 若指令是对象格式（如{"on":1}）
            item = cJSON_GetObjectItem(json, "on");
        }
    
        if (item == NULL || !cJSON_IsNumber(item)) { // 校验on字段有效性
            cJSON_Delete(json);
            DEMO_LOG("get on error");
            return -1;
        }
    
        int32_t on = cJSON_GetNumberValue(item); // 提取on值（0/1）
        DEMO_LOG("switch on put %d=>%d", g_switch, on);
    
        // 更新开关状态
        if (on == 0) {
            g_switch = false;
        } else if (on == 1) {
            g_switch = true;
        }
    
        SetGpio(on); // 控制GPIO输出
    
        cJSON_Delete(json);
        return 0;
    }
    ```
  
  - 开关状态查询函数
    
    处理 APP 的开关状态查询指令，核心逻辑：
    
    - 读取当前开关状态（g_switch）；
    - 封装为 JSON 格式返回给 APP。
    
    ```c
    static int SwitchGetCharState(const IotcServiceInfo *svc, char **data, uint32_t *len)
    {
        DEMO_LOG("SwitchGetCharState in");
        if (data == NULL || *data != NULL) {
            DEMO_LOG("param invalid");
            return -1;
        }
    
        cJSON *json = cJSON_CreateObject(); // 创建JSON对象
        if (json == NULL) {
            DEMO_LOG("create obj error");
            return -1;
        }
    
        // 添加on字段（当前开关状态）
        if (cJSON_AddNumberToObject(json, "on", g_switch) == NULL) {
            cJSON_Delete(json);
            DEMO_LOG("add num error");
            return -1;
        }
    
        *data = cJSON_PrintUnformatted(json); // 转换为无格式JSON字符串
        cJSON_Delete(json);
        if (*data == NULL) {
            DEMO_LOG("json print error");
            return -1;
        }
        DEMO_LOG("switch get %d", g_switch);
        *len = strlen(*data); // 设置返回数据长度
        return 0;
    }
    ```
  
  - 其他服务简要解析
    
    服务功能如下，具体写法可参考示例代码编写
    
    | 服务名称        | 核心功能                                       | 关键逻辑                                                          |
    | ----------- | ------------------------------------------ | ------------------------------------------------------------- |
    | snw（序列号）    | 上报设备序列号                                    | 读取 DEV_INFO.sn，封装为 JSON 返回；接收指令仅打印，不修改                        |
    | gps（GPS）    | 接收 APP 下发的 GPS 坐标（纬度 / 经度）并存储，查询时返回当前存储的坐标 | 解析 JSON 中的 latitude/longitude 字段，存储到全局变量；查询时返回该变量值            |
    | devNetInfo  | 查询设备当前 WiFi 信息（SSID/IP/RSSI/BSSID）并上报      | 调用 IoTConnect 接口（IotcGetWifiInfo/IotcGetLocalIp 等）获取网络信息，封装返回 |
    | allServices | 批量查询所有服务的状态                                | 遍历 SVC_MAP，调用每个服务的 getCharState 函数，汇总为 JSON 数组返回              |

- IoTConnect 组件对接
  
  - 核心回调函数注册
    
    Demo 向 IoTConnect 组件注册以下关键回调，组件收到 APP 指令后会触发对应回调：
    
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
  
  - IoTConnect 组件收到 APP 指令后，先触发顶层回调（PutCharState/GetCharState），再由该函数根据服务 ID 分发到对应服务的处理函数：
    
    ```c
    static int32_t PutCharState(const IotcCharState state[], uint32_t num)
    {
        DEMO_LOG("PutCharState in");
        if (state == NULL || num == 0) {
            DEMO_LOG("param invalid");
            return -1;
        }
    
        int32_t ret = 0;
        bool found = false;
        // 遍历所有收到的指令
        for (uint32_t i = 0; i < num; ++i) {
            DEMO_LOG("put char sid:%s data:%s", state[i].svcId, state[i].data);
            // 遍历服务映射表，找到对应服务的处理函数
            for (uint32_t j = 0; j < (sizeof(SVC_MAP) / sizeof(SVC_MAP[0])); ++j) {
                if (strcmp(state[i].svcId, SVC_MAP[j].svc->svcId) != 0 || SVC_MAP[j].putCharState == NULL) {
                    continue;
                }
                found = true;
                // 调用对应服务的指令接收函数
                int32_t curRet = SVC_MAP[j].putCharState(SVC_MAP[j].svc, state[i].data, state[i].len);
                if (curRet != 0) {
                    ret = curRet;
                    DEMO_LOG("put char sid:%s error %d", state[i].svcId, ret);
                }
            }
        }
        return ret != 0 ? ret : (found ? 0 : -1);
    }
    ```
  
  - IoTConnect 组件初始化与启动
    
    ```c
    int32_t IotcOhDemoEntry(void)
    {
        DEMO_LOG("IotcOhDemoEntry in");
        int32_t ret = IOTC_OK;
    
        // 1. 初始化设备信息模块
        ret = IotcOhDevInit();
        if (ret != 0) {
            DEMO_LOG("init device error %d", ret);
            return ret;
        }
    
        // 2. 启用BLE/WiFi互联模块
        ret = IotcOhBleEnable(); // 启用BLE
        if (ret != 0) { return ret; }
        ret = IotcOhWifiEnable(); // 启用WiFi
        if (ret != 0) { return ret; }
    
        // 3. 注册回调函数（见2.3.1）
        SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_PUT_CHAR_STATE_CALLBACK, PutCharState);
        // ... 其余回调注册 ...
    
        // 4. 配置设备/服务信息
        SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_DEV_INFO, &DEV_INFO); // 设备信息
        SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_SVC_INFO, SVC_INFO, sizeof(SVC_INFO)/sizeof(SVC_INFO[0])); // 服务信息
    
        // 5. 配置配网参数
        SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_WIFI_NETCFG_MODE, IOTC_NET_CONFIG_MODE_BLE_SUP); // BLE辅助配网
        SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_WIFI_NETCFG_TIMEOUT, (24 * 60 * 60 * 1000)); // 配网超时24小时
    
        // 6. 启动IoTConnect组件主流程
        ret = IotcOhMain();
        if (ret != 0) {
            DEMO_LOG("iotc oh main error %d", ret);
            return ret;
        }
        DEMO_LOG("iotc oh main success");
        return ret;
    }
    ```

- 事件监听与设备管理
  
  - BLE 事件监听
    
    监听 BLE 核心事件（如初始化完成、断开连接），触发 BLE 广播重启，确保设备可被 APP 发现
    
    ```c
    static void DemoBleEventListener(int32_t event)
    {
        DEMO_LOG("DemoBleEventListener in");
        int32_t ret = 0;
        switch (event) {
            case IOTC_CORE_COMM_EVENT_MAIN_INITIALIZED: // 组件初始化完成
            case IOTC_CORE_BLE_EVENT_GATT_DISCONNECT:   // BLE连接断开
                ret = IotcOhBleStartAdv(ADV_TIMEOUT);   // 重启BLE广播（永不超时）
                break;
            default:
                return;
        }
        DEMO_LOG("event[%d] ret:%d", event, ret);
    }
    ```
  
  - 设备重启
    
    目前为空实现，仅做重置，会在手机上删除配对设备之后触发，开发者可以在里面添加自己的重启方法。
    
    ```c
    static int32_t NoticeReboot(IotcRebootReason res)
    {
        DEMO_LOG("notice reboot res %d", res);
        IotcOhReset();       // 重置IoTConnect组件
        IotcRestartWifi();   // 重启WiFi模块
        return 0;
    }
    ```

- 任务启动
  
  基于 CMSIS-OS2 创建任务线程，初始化 GPIO 并启动 IoTConnect 组件
  
  ```c
  int IotTask(void)
  {
      DEMO_LOG("sleep 30 seconds to wait ble ready.");
      IotcOhDemoEntry();   // 启动IoTConnect组件
      GpioInitTask();      // 初始化GPIO
      while (true) {       // 任务循环
          osDelay(1000);   // 延时1秒（LiteOS-M接口）
      }
      return 0;
  }
  
  // 系统启动时自动运行该函数
  static void IOTWifiBleExampleEntry(void)
  {
      osThreadAttr_t attr;
      attr.name = "IotTask";          // 任务名称
      attr.attr_bits = 0U;            // 属性位
      attr.cb_mem = NULL;             // 回调内存
      attr.cb_size = 0U;              // 回调内存大小
      attr.stack_mem = NULL;          // 栈内存
      attr.stack_size = 0x8000;       // 栈大小（32KB）
      attr.priority = 19;             // 任务优先级（数值越小优先级越高）
  
      // 创建并启动任务
      if (osThreadNew((osThreadFunc_t)IotTask, NULL, &attr) == NULL) {
          printf("[BLEExample] Failed to create LedTask!\n");
      }
  }
  
  SYS_RUN(IOTWifiBleExampleEntry); // 标记为系统启动任务，Hi3863开机自动执行
  ```

### 3.3、BUILD.gn配置

- 核心作用
  
  `BUILD.gn`是 OpenHarmony/LiteOS-M 平台的编译配置文件（基于 GN 构建系统），该文件的核心作用：
  
  - 定义 Demo 的编译类型（静态库`static_library`）；
  - 指定 Demo 的源码文件、依赖库、头文件路径；
  - 配置编译选项（如警告等级、宏定义、安全编译规则）；
  - 关联 IoTConnect 核心组件，确保编译时能正确链接依赖。
  
  配置前需确保以下路径 / 组件已存在（与`build.gn`中的路径对应），否则会导致编译失败：
  
  - IoTConnect 组件已部署到`//foundation/communication/iot_connect/`目录；
  
  - Hi3863 SDK 源码已部署到`//device/soc/hisilicon/hi3863v100/sdk_liteos/`目录；
  
  - 第三方依赖（cJSON、bounds_checking_function）已存在于 OpenHarmony 源码对应路径；
  
  - Hi3863 GPIO / 蓝牙 / WiFi 驱动源码已部署到对应路径。

- 配置解析
  
  - 导入依赖模块

```gn
# 导入IoTConnect组件的GN配置（包含编译宏、路径等）
import("//foundation/communication/iot_connect/iotc.gni")
# 导入IoTConnect适配器的GN配置（适配Hi3863平台的宏定义）
import("//foundation/communication/iot_connect/adapter/adapter.gni")
# 导入OpenHarmony轻量级组件编译配置（必须）
import("//build/lite/config/component/lite_component.gni")
# 导入OpenHarmony核心编译配置（可选，视平台而定）
import("//build/ohos.gni")
```

  **作用**：导入其他 GN 配置文件中的变量 / 宏定义，避免重复配置；

- 定义编译目标（静态库）

```gn
# 定义静态库编译目标，名称为"wifi_ble_combo"（可自定义）
static_library("wifi_ble_combo") {
  # 后续所有配置都属于该静态库
}
```

  `static_library`：指定编译产物为静态库（`.a`文件），Hi3863 平台轻量级应用优先使用静态库；

  `"wifi_ble_combo"`：静态库名称，需与产品配置（如`Hi3863.json`）中引用的组件名一致；

- 指定源码文件
  
  ```gn
    sources = [
     "wifi_ble_combo.c",  # Demo核心源码文件
    ]
  ```

- 配置依赖库（deps）
  
  ```gn
    deps = [
      # 安全函数库（如memcpy_s、strncpy_s），Hi3863必须依赖
      "//third_party/bounds_checking_function:libsec_shared",
      # cJSON静态库（JSON解析依赖）
      "//build/lite/config/component/cJSON:cjson_static",
      # IoTConnect核心静态库（必须，Demo依赖其接口）
      "//foundation/communication/iot_connect:iotc_static",
      # 可添加自定义依赖（如新增的传感器驱动库）
    ]
  ```
  
  **作用**：指定编译时需要链接的依赖库，解决 “未定义的引用” 错误；
  
  **关键依赖说明**：
  
  | 依赖库           | 作用                                    |
  | ------------- | ------------------------------------- |
  | libsec_shared | 提供安全内存操作函数（如 memcpy_s）                |
  | cjson_static  | 提供 JSON 解析 / 生成接口                     |
  | iotc_static   | 提供 IoTConnect 核心接口（如 IotcOhBleEnable） |

- 配置头文件路径（include_dirs）
  
  ```gn
    include_dirs = [
      # 通用工具库头文件路径
      "//commonlibrary/utils_lite/include",
      # Hi3863 SDK核心头文件（必须）
      "//device/soc/hisilicon/hi3863v100/sdk_liteos/include",
      # BLE驱动头文件（BLE功能依赖）
      "//device/soc/hisilicon/hi3863v100/sdk_liteos/include/middleware/services/bts/common",
      "//device/soc/hisilicon/hi3863v100/sdk_liteos/include/middleware/services/bts/ble",
      # WiFi驱动头文件（WiFi功能依赖）
      "//device/soc/hisilicon/hi3863v100/sdk_liteos/include/middleware/services/wifi",
      # OS适配层头文件（CMSIS-OS2依赖）
      "//device/soc/hisilicon/hi3863v100/sdk_liteos/kernel/osal/include",
      "//device/soc/hisilicon/hi3863v100/sdk_liteos/kernel/osal/include/memory",    
      # GPIO硬件抽象层头文件（GPIO控制依赖）
      "//base/iothardware/peripheral/interfaces/inner_api",
      # Hi3863应用初始化头文件（SYS_RUN依赖）
      "//device/soc/hisilicon/hi3863v100/sdk_liteos/middleware/utils/app_init",
      # IoTConnect通用接口头文件（必须）
      "//foundation/communication/iot_connect/interfaces/kits/common",
      "//foundation/communication/iot_connect/interfaces/kits/oh_connect",
      # IoTConnect核心工具头文件（必须）
      "//foundation/communication/iot_connect/core/infrastructure/utils/include",
      "//foundation/communication/iot_connect/adapter/include",
      "//foundation/communication/iot_connect/core/device/config/include",
      "//foundation/communication/iot_connect/core/infrastructure/service/include",  
      "//foundation/communication/iot_connect/core/infrastructure/define",
      # LwIP网络协议栈头文件（WiFi通信依赖）
      "//device/soc/hisilicon/hi3863v100/sdk_liteos/open_source/lwip/lwip_v2.1.3/src/include",
      # Hi3863 GPIO驱动头文件（必须）
      "//device/soc/hisilicon/hi3863v100/sdk_liteos/include/driver",
      "//device/soc/hisilicon/hi3863v100/sdk_liteos/drivers/drivers/hal/gpio",
      "//device/soc/hisilicon/hi3863v100/sdk_liteos/drivers/chips/ws63/porting/gpio",
      "//device/soc/hisilicon/hi3863v100/sdk_liteos/drivers/drivers/hal/gpio/v150",
      "//device/soc/hisilicon/hi3863v100/sdk_liteos/drivers/chips/ws63/porting/pinctrl",
      # LittleFS文件系统头文件（可选，视存储需求而定）
      "//device/soc/hisilicon/hi3863v100/sdk_liteos/middleware/chips/ws63/littlefs", 
    ]
  ```
  
  **作用**：指定编译时的头文件搜索路径，解决 “找不到头文件” 错误；

- 配置编译选项（cflags）
  
  ```gn
    cflags = [
      "-ftrapv",                # 检测整数溢出并触发陷阱
      "-Werror",                # 将所有警告视为错误（强制规范代码）
      "-Wextra",                # 启用额外警告（如未使用的参数）
      "-Wshadow",               # 检测变量遮蔽（如局部变量覆盖全局变量）
      "-fstack-protector-all",  # 启用栈保护（防止缓冲区溢出）
      "-D_FORTIFY_SOURCE=2",    # 启用安全函数强化（如strcpy→strcpy_s）
      "-Wformat=2",             # 检测格式化字符串错误
      "-Wfloat-equal",          # 检测浮点数相等比较（避免精度问题）
      "-Wdate-time",            # 检测编译时间相关宏的使用
      "-Wno-error=unused-parameter",  # 忽略“未使用参数”的警告（避免编译失败）
      "-Wall",                  # 启用所有基本警告
      "-fPIC",                  # 生成位置无关代码（静态库必备）
      "-Wunused-parameter",     # 警告未使用的参数（仅提示，不报错）
    ]
  ```
  
  **作用**：配置编译器（arm-none-eabi-gcc）的编译选项，提升代码安全性和规范性；

- 配置宏定义（defines）
  
  ```gn
    defines = iotc_adapter_def
  ```
  
  **作用**：导入 IoTConnect 适配器的宏定义（如`IOTC_CONF_AILIFE_SUPPORT`、`IOTC_PROT_TYPE_BLE_AND_WIFI`等），适配 Hi3863 平台；

## 四、编译配置修改及编译指令

WiFi/BLE Combo（Hi3863平台）的编译配置修改步骤请参考示例代码仓[README.md](https://gitcode.com/ohos-oneconnect/applications_sample_iot_connect_samples/blob/OpenHarmony-5.1.0-Release/wifi_ble_combo/hi3863/README.md);

## 五、DEMO使用说明

- **WiFi/BLE Combo设备端云控制**
1. 联系OpenHarmony统一互联PMC或在laval社区提单，完成APP白名单配置;

2. 编译ohos-connect-hap源码，[安装编译](../通用互联App.md)的hap 至HarmonyOS Next 手机上；

3. 使用[Hi3863开发板烧录](https://www.bearpi.cn/core_board/bearpi/pico/h3863/software/%E4%B8%8B%E8%BD%BD%E7%83%A7%E5%BD%95.html)WiFi/BLE Combo镜像；

4. 打开通用互联APP，在我的页面点击设备库同步将设备库更新到最新版本；

5. 点击通用互联APP底部设备tab，点击设备tab右上角“+”号按钮扫描，可发现对应设备；

6. 在扫描结果中点击对应设备，输入需要配置的WiFi账号密码，点击下一步，即可对设备开始配网；

7. 对应设备连接到WiFi，配网成功，APP自动退回设备页面并显示已配网的设备；

8. 点击设备页面已配网设备，在设备详情页-可控制设备的LED灯状态与设备重启。