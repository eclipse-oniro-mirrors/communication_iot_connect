# IoT Connect SDK组件

## 一、内容介绍

IoT Connect SDK（iot_connect）是专为OpenHarmony资源受限的瘦设备（mini/small系统级别）构建的极简、高性能连接控制核心组件，提供设备发现配网、端云连接、设备控制等能力。

**核心功能：** BLE/WiFi/Combo/SLE发现配网、点到点/局域网/云端控制、端云连接

**适配系统：** mini/small/standard系统

**详细说明请参考：** [IoT-Connect组件介绍](https://gitcode.com/ohos-oneconnect/docs/blob/master/zh-cn/设备联动与控制/IoT-Connect组件/README.md)

## 二、架构图

![image.png](https://raw.gitcode.com/user-images/assets/9584680/0361da33-c9d6-4f82-bf5f-e57324f04db6/image.png 'image.png')

**架构详细说明：** [联动与控制概述](https://gitcode.com/ohos-oneconnect/docs/blob/master/zh-cn/设备联动与控制/联动与控制概述.md)

### 目录结构

```
//foundation/communication/iot_connect
├── adapter                 # 适配层代码（OS/蓝牙/WiFi）
├── core                    # 核心代码
│   ├── ble                 # BLE发现配网代码
│   ├── wifi                # Wi-Fi发现配网及端云连接代码
│   ├── device              # 设备控制及注册信息管理代码
│   └── infrastructure      # 核心基础设施代码（安全/日志/事件等）
├── interfaces              # 对外接口代码
│   └ kits/common          # 通用接口定义（错误码/事件/配置等）
│   └ kits/oh_connect      # OpenHarmony对接接口
├── sdk                     # 解决方案业务入口代码
├── test                    # 测试代码
├── tools                   # 工具代码
└── docs                    # 文档目录
```

## 三、编译构建方法

### 3.1 控制端开发

| 项目                   | 说明                             | 仓库链接                                                                                              |
| -------------------- | ------------------------------ | ----------------------------------------------------------------------------------------------- |
| 通用互联APP              | 控制核心入口，运行在HOS手机上 | [通用互联APP](https://gitcode.com/ohos-oneconnect/docs/blob/master/zh-cn/设备联动与控制/通用互联APP/README.md) |
| IoTManagementService | OpenHarmony设备上的设备管理服务组件        | [IoTManagementService](https://gitcode.com/ohos-oneconnect/IoTManagementService)                |
| 云平台                  | 互联互通云端服务，提供账号、设备、场景管理          | [云平台](https://gitcode.com/ohos-oneconnect/docs/blob/master/zh-cn/设备联动与控制/云平台/README.md)         |
### 3.2 被控端开发

| 开发内容        | 文档链接                                                                                                                        |
| ----------- | --------------------------------------------------------------------------------------------------------------------------- |
| API概述       | [iotc-api-overview.md](https://gitcode.com/ohos-oneconnect/communication_iot_connect/blob/master/docs/iotc-api-overview.md) |
| WiFi设备DEMO  | [iotc-demo-wifi.md](https://gitcode.com/ohos-oneconnect/communication_iot_connect/blob/master/docs/iotc-demo-wifi.md)       |
| BLE设备DEMO   | [iotc-demo-ble.md](https://gitcode.com/ohos-oneconnect/communication_iot_connect/blob/master/docs/iotc-demo-ble.md)         |
| Combo设备DEMO | [iotc-demo-combo.md](https://gitcode.com/ohos-oneconnect/communication_iot_connect/blob/master/docs/iotc-demo-combo.md)     |
| 示例代码仓库      | [applications_sample_iot_connect_samples](https://gitcode.com/ohos-oneconnect/applications_sample_iot_connect_samples)      |

### 3.3 各平台开发指南

| 平台     | 套餐类型           | 开发指南                                                                                                                           |
| ------ | -------------- | ------------------------------------------------------------------------------------------------------------------------------ |
| RK3568 | WiFi only      | [wifi_only_rk3568.md](https://gitcode.com/ohos-oneconnect/docs/blob/master/zh-cn/设备联动与控制/demo构建/wifi_only_rk3568.md)           |
| RK3568 | BLE only       | [ble_only_rk3568.md](https://gitcode.com/ohos-oneconnect/docs/blob/master/zh-cn/设备联动与控制/demo构建/ble_only_rk3568.md)             |
| RK3568 | WiFi/BLE Combo | [wifi_ble_combo_rk3568.md](https://gitcode.com/ohos-oneconnect/docs/blob/master/zh-cn/设备联动与控制/demo构建/wifi_ble_combo_rk3568.md) |
| Hi3863 | WiFi only      | [wifi_only_hi3863.md](https://gitcode.com/ohos-oneconnect/docs/blob/master/zh-cn/设备联动与控制/demo构建/wifi_only_hi3863.md)           |
| Hi3863 | BLE only       | [ble_only_hi3863.md](https://gitcode.com/ohos-oneconnect/docs/blob/master/zh-cn/设备联动与控制/demo构建/ble_only_hi3863.md)             |
| Hi3863 | WiFi/BLE Combo | [wifi_ble_combo_hi3863.md](https://gitcode.com/ohos-oneconnect/docs/blob/master/zh-cn/设备联动与控制/demo构建/wifi_ble_combo_hi3863.md) |
| Hi3863 | BLE/SLE Combo  | [ble_sle_combo_hi3863.md](https://gitcode.com/ohos-oneconnect/docs/blob/master/zh-cn/设备联动与控制/demo构建/ble_sle_combo_hi3863.md)   |

## 四、配套资料

### 4.1 技术标准

| 标准          | 
| ----------- | 
| [《物模型技术标准》](https://www.giiconsortium.org/2025/post/2502/)| 
| [《接入与控制接口技术标准》](https://www.giiconsortium.org/2025/post/2506/)| 

### 4.2 Release版本说明

| 版本                        | 链接                                                                                                                                                           |
| ------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| OneConnect v2.0.0 Release | [OpenHarmony OneConnect-v2.0.0-release](https://gitcode.com/ohos-oneconnect/docs/blob/master/zh-cn/release-notes/OpenHarmony%20OneConnect-v2.0.0-release.md) |

## 五、案例

### 5.1 WiFi/BLE Combo设备端云控制（RK3568）

**场景：** DAYU200开发板模拟WiFi/BLE Combo设备，实现屏幕控制、设备重启等功能

**开发指南：** [wifi_ble_combo_rk3568.md](https://gitcode.com/ohos-oneconnect/docs/blob/master/zh-cn/设备联动与控制/demo构建/wifi_ble_combo_rk3568.md)

**示例代码：** [iotc_oh_demo_wifi_ble_combo.c](https://gitcode.com/ohos-oneconnect/applications_sample_iot_connect_samples/blob/OpenHarmony-5.0.0-Release/wifi_ble_combo/yarward_iotc/digital_signage/src/iotc_oh_demo_wifi_ble_combo.c)

### 5.2 BLE设备点到点本地控制（Hi3863）

**场景：** Hi3863作为BLE瘦设备，实现与APP的点对点本地控制

**开发指南：** [ble_only_hi3863.md](https://gitcode.com/ohos-oneconnect/docs/blob/master/zh-cn/设备联动与控制/demo构建/ble_only_hi3863.md)

---

**相关链接：**

- [OpenHarmony统一互联文档仓](https://gitcode.com/ohos-oneconnect/docs)
- [设备联动与控制](https://gitcode.com/ohos-oneconnect/docs/blob/master/zh-cn/设备联动与控制/README.md)
- [Laval社区- 问题交流与反馈](https://laval.csdn.net) 