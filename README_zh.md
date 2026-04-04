# IoT 互联

## 说明

响应OpenHarmony生态共建伙伴提出智慧病房、智慧隧道、会议办公等场景的富设备对瘦设备控制的需求。在生态互联互通共建项目中，构建瘦设备上运行的极简连接控制组件：iot_connect，实现在mini/small级别OH设备上的控制和连接。主要功能如下：
-   发现配网：提供基于Wi-Fi、BLE等通信方式的设备发现配网能力。
-   端云连接：提供连接互联互通云端的能力，为已配网设备提供远程控制、管理的通道。

## 目录
```text
//foundation/communication/iot_connect
├── adapter                 # 适配层代码
├── core                    # 核心代码
│   ├── ble                 # ble发现配网代码
│   ├── device              # 设备控制及注册信息管理代码
│   ├── infrastructure      # 核心基础设施代码
│   └── wifi                # Wi-Fi发现配网及段云连接代码
├── interfaces              # 对外接口代码
├── sdk                     # 解决方案业务入口代码
├── test                    # 测试代码
└── tools                   # 工具代码
```

## 使用说明

-   [API概述](docs/iotc-api-overview.md)
-   [Wi-Fi设备DEMO](docs/iotc-demo-wifi.md)
-   [BLE设备DEMO](docs/iotc-demo-ble.md)
-   [Combo设备DEMO](docs/iotc-demo-combo.md)

## 添加编译子系统构建动态库

- [L0设备库编译](docs/iotc-build-l0.md)
- [L2设备库编译](docs/iotc-build-l2.md)