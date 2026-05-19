# IoT Connect SDK组件

## 简介

IoT Connect SDK 是专为OpenHarmony资源受限的瘦设备（mini/small系统级别）所构建的一款极简、高性能的连接控制核心组件。核心在于为这些计算、存储和功耗都极为敏感的设备，提供稳定、安全且低功耗的设备接入、网络通信与远程控制能力。通过抽象底层复杂的网络差异，极大地简化了物联网设备的开发流程，为构建轻量级智能硬件与实现万物互联的泛在化接入提供了关键的技术基础支撑，是实现设备间智能协同与无缝连接的重要桥梁。

**核心功能：** 面向 OpenHarmony mini/small 级资源受限瘦设备，提供**BLE‑Only、WiFi‑Only、BLE/WiFi Combo 多形态设备的邻近发现、安全配网与网络接入**能力；支持**点对点直连、局域网本地控制、端云控制**多种方式实现设备间端侧协同。

## 系统架构

![imagepng](https://raw.gitcode.com/user-images/assets/9584680/1c2c0fde-b28b-4d5f-a107-c97a51c1606d/image.png "image.png")

图1 统一互联联动与控制架构图

![](https://foruda.gitee.com/images/1779111176849795024/17a487c8_8030656.png)

图2 IoT-Connect 组件架构图

### 模块功能说明

整体架构划分为接口层、业务层、基础能力层、适配层，同时依赖OHOS蓝牙子系统、WiFi子系统以及依赖mbedtls、cJSON等三方库。

### 接口层

用于向厂商业务/应用层提供标准化调用接口，包含部件运行、BLE Connect、WiFi Connect、设备管理接口能力。

### 业务层

业务层实现物联网核心业务逻辑，分为**设备服务、BLE 服务、WiFi 服务**3 个子模块。

**设备服务**

用于设备基础参数、唯一 ID、密钥配置管理并解析 / 存储鸿蒙标准物模型，定义设备能力、属性、指令，对接云端物模型协议以及下发设备控制指令、接收设备状态上报，实现局域网 / 远程设备控制。

**BLE服务**

用于BLE 协议栈、广播 / 扫描初始化、BLE 广播扫描、周边设备发现、BLE 辅助配网、BLE 直连设备点对点本地控。

**WiFi服务**

用于WiFi 协议栈、STA/AP 模式初始化、局域网 WiFi 设备扫描发现、WiFi 配网、局域网内 WiFi 直连设备控制以及通过 WiFi 对接云平台，实现**远程控制、设备上云**等。

### 基础能力层

为上层业务提供通用底层能力。

**安全**

包括建立 TLS/DTLS 安全通信会话、设备接入身份校验、密钥绑定，防止非法设备接入、通信报文 AES 加密，保障 BLE/Wi‑Fi 通信安全等。

**事件总线 & 调度**

用于模块间解耦通信，BLE/Wi‑Fi 事件、设备状态统一分发、处理定时任务、心跳保活、超时重传管理以及异步任务调度，避免阻塞业务流程。

**DFX**

负责日志采集、设备自恢复等。

### 适配层

适配层是**跨平台、跨硬件的抽象隔离层**，屏蔽鸿蒙系统、硬件、三方库差异。包括了BLE 协议适配，WiFi 协议适配，KV键值对持久化存储，保存设备配置、密钥、物模型，适配系统线程、内存、信号量接口、JSON 报文解析 / 序列化、安全算法加解密以及其他拓展接口。

### 依赖

依赖OHOS蓝牙子系统支撑 BLE 服务硬件协议栈、WiFi 子系统支撑 Wi‑Fi 服务硬件协议栈以及mbedtls、cJSON、bounds_checking_function三方库依赖。

### 关键交互流程

为了更清晰地展示各模块如何协同工作，以下详解三大核心流程

#### 设备发现连接

设备发现与连接流程根据配网方式的不同，涉及不同的通信链路交互。

- BLE 设备发现流程
  

1. **初始化模块**:
  
  - 当业务层调用 `IotcOhDevInit` 时，SDK初始化设备信息模块，配置设备基本信息（DEV_INFO）和服务信息（SVC_INFO）。
  - 业务层调用 `IotcOhBleEnable`，使能 BLE 发现、连接、控制能力，`BLE模块` 初始化广播参数和 GATT 服务。
2. **启动广播**:
  
  - 业务层调用 `IotcOhBleStartAdv`，`BLE模块` 构造广播数据包，包含设备类型、厂商名称等信息。
  - 广播数据通过 BLE Stack 发送，APP/手机可扫描发现设备。
3. **建立连接**:
  
  - APP 发起BLE连接请求，`BLE模块` 建立连接。
  - 建立安全会话：双方通过获取到的PIN码，进行SPEKE 会话密钥协商建立安全会话，后续数据交互通过协商的密钥进行加密安全传输。

- WiFi SoftAP 发现连接流程
  

1. **启动热点**:
  
  - 业务层调用 `IotcOhWifiEnable` 并配置 `IOTC_NET_CONFIG_MODE_SOFTAP` 模式。
  - `WiFi模块` 开启 SoftAP 热点，生成包含设备信息的 SSID 并广播。
2. **连接发现**:
  
  - APP 连接 SoftAP 热点，`SoftAP服务` 收到 STA 连接通知。
  - APP 通过 CoAP 协议发送设备发现请求，`CoAP服务` 响应并返回设备信息。
3. **协商认证**:
  
  - 与 APP 进行 SPEKE 协商，获取 PIN 码建立会话密钥。
4. **配网执行**:
  
  - APP 发送 WiFi 配网信息（SSID/密码），经 CoAP 传输并由 `Session管理` 解密。
  - 业务层收到配网回调，控制 `WiFi模块` 连接目标 WiFi 网络。
  - 连接成功。

#### 端云连接

端云连接流程实现设备与云端服务的安全通信，包括 TLS 建链、设备注册、登录认证及心跳保活。

- **网络就绪检查**:
  
  - `WiFi模块` 完成 WiFi 连接后，通过 EventBus 发布 `IOTC_CORE_WIFI_EVENT_CONNECTED` 事件。
  - `Cloud FSM` 收到事件后，状态从 `INIT` 切换至 `CONNECTING`，准备建立端云连接。
- **TLS 连接建立**:
  
  - `Cloud FSM` 调用 `Cloud Link` 模块建立云端连接。
  - `Cloud Link` 通过 `TLS连接` 模块与云端服务进行 TLS 握手，完成证书验证后建立安全通道。
  - 连接成功后，`Cloud FSM` 状态切换至 `REGISTER`。
- **设备注册**:
  
  - `Cloud FSM` 通过 `设备服务` 获取注册信息，调用业务层的 `GetAcKey` 回调获取厂商 AC_KEY。
  - 使用 AC_KEY 向云端发送注册请求，云端 `认证模块` 验证后返回设备 ID 和 Token。
  - 注册成功后，`Cloud FSM` 状态切换至 `LOGIN`。
- **设备登录**:
  
  - `Cloud FSM` 使用 Token 向云端发送登录请求。
  - 云端验证 Token 后建立 Session，`Cloud FSM` 状态切换至 `ONLINE`。
  - 触发上线事件，业务层调用 `ReportAll` 上报全量服务状态。
- **心跳保活**:
  
  - 进入 `ONLINE` 状态后，`Cloud FSM` 定期向云端发送 Heartbeat 请求，维持连接活跃。
  - 心跳失败时触发重连机制，状态切换至 `OFFLINE`。
- **云端控制**:
  
  - 云端下发控制指令，经 TLS 通道传输至 `Cloud FSM`。
  - `Cloud FSM` 解析指令后调用 `设备服务`，触发业务层的 `PutCharState` 回调。
  - 业务层执行设备控制后，通过 `设备服务` 上报状态变化。

#### 设备控制

设备控制流程支持多通道控制能力，包括云端远程控制、局域网本地控制及 BLE 点对点控制，提供统一的设备服务状态管理。

- 控制指令处理流程
  

1. **指令接收**:
  
  - **控制源**（云端/局域网/BLE）发送控制指令，包含服务 ID（SvcId）和控制数据（JSON格式）。
  - `传输层`（TLS/CoAP/GATT）接收并解密数据，交给 `控制模块` 解析。
2. **指令分发**:
  
  - `控制模块` 根据 SvcId 调用 `设备服务` 的对应处理逻辑。
  - `设备服务` 调用 `CharState Model` 的 `PutCharState` 方法。
3. **业务执行**:
  
  - `CharState Model` 调用业务层注册的 `PutCharState` 回调函数。
  - 业务层执行实际设备控制操作（如开关灯、调节亮度等），并更新 `设备状态`。
4. **状态上报**:
  
  - 业务层可通过 `IotcOhDevReportCharState` 主动上报状态变化。
  - `CharState Model` 更新服务状态，触发状态上报流程。
  - 状态数据经加密后通过 `传输层` 发送至控制源。

- 查询指令处理流程
  

1. **查询接收**:
  
  - 控制源发送查询指令，指定服务 ID。
  - `传输层` 和 `控制模块` 解析查询请求。
2. **状态获取**:
  
  - `设备服务` 调用 `CharState Model` 的 `GetCharState` 方法。
  - `CharState Model` 调用业务层的 `GetCharState` 回调，获取当前设备状态。
3. **响应返回**:
  
  - 业务层返回 JSON 格式的状态数据（如 `{"on":1, "brightness":80}`）。
  - 数据经加密后通过 `传输层` 返回至控制源。

- 局域网控制详细流程
  

1. **服务启动**:
  
  - WiFi 连接成功后，`Local Control` 服务启动 CoAP 服务端，监听 UDP 端口。
  - 通过 EventBus 订阅设备上线事件。
2. **设备发现**:
  
  - APP 通过 CoAP 协议发送发现请求，`Local Control` 响应并返回设备信息。
  - 建立 Session 并通过 SPEKE 协商会话密钥。
3. **控制交互**:
  
  - APP 发送加密控制指令，`Session管理` 解密后交给 `设备服务` 处理。
  - 控制完成后，`Local Control` 广播状态变化通知所有订阅的客户端。

## 目录

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

## 编译构建

根据不同的目标平台，使用以下命令进行编译：

```bash
bash build/prebuilts_download.sh #预编译
hb set #选择dk3863
hb build -f #编译
```

## 使用说明

### 接口说明

#### 部件运行管理模块

| 函数原型 | 核心功能说明 |
| --- | --- |
| int32_t IotcOhMain(void) | 部件业务入口，调用后拉起自身任务线程，大部分预配置类接口不再可用 |
| int32_t IotcOhReset(void) | 部件复位，调用后部件会重置所有组件业务的运行状态，仅在部件运行时有效，该接口为同步接口，返回即表示复位成功/失败 |
| int32_t IotcOhStop(void) | 停止部件的运行，仅在部件运行时有效，该接口为同步接口，返回即表示停止成功/失败 |
| int32_t IotcOhRestore(void) | 通知所有业务恢复出厂，仅在部件运行时有效，该接口为同步接口，返回即表示恢复出厂成功/失败 |
| int32_t IotcOhSetOption(int32_t option, ...); | 配置部件运行时的参数，option为待配置的参数类型，需结合对应配置选项使用 |

#### BLE Connect能力模块

| 函数原型 | 核心功能说明 |
| --- | --- |
| int32_t IotcOhBleEnable(void) | 使能BLE发现、连接、控制的能力，该接口仅做使能，调用不会触发任何业务流程，应在iot connect运行前调用 |
| int32_t IotcOhBleDisable(void) | 关闭iot connect的BLE发现、连接、控制的能力，用于释放调用IotcOhBleEnable时申请的资源，无法在iot connect运行时调用 |
| int32_t IotcOhBleStartAdv(uint32_t ms); | 启动BLE广播发现，根据初始化参数的不同，iot connect启动后会自动发送一段时间广播，后续需调用该接口激活广播，用于配合外部按键实现按键触发场景；ms为广播时长（单位ms），gatt的连接不会刷新、延长该时间 |
| int32_t IotcOhBleStopAdv(void) | 在BLE广播发现期间，调用该接口可以停止广播 |
| int32_t IotcOhBleSendCustomSecData(const uint8_t *data, uint32_t len); | 部分场景下，用于通过customSecData服务通道发送数据，sdk完成数据加密后发送给对端；data为待发送数据，len为数据长度 |
| int32_t IotcOhBleSendIndicateData(const char *svcUuid, const char *charUuid, const uint8_t *value, uint32_t valueLen) | 当使用部件管理GATT服务时，用于发送BLE Indicate数据；svcUuid为Ble gatt svc uuid，charUuid为Ble gatt svc character UUID，value为待发送数据，valueLen为数据长度 |
| int32_t IotcOhBleRelease(void) | BLE资源释放，用于释放调用IotcOhBleEnable时申请的资源，该接口在iot connect运行时调用 |

#### Wi-Fi Connect能力模块

| 函数原型 | 核心功能说明 |
| --- | --- |
| int32_t IotcOhWifiEnable(void) | 使能基于Wi-Fi/LAN的发现、连接、控制的能力，该接口仅做使能，调用不会触发任何业务流程，应在iot connect运行前调用 |
| int32_t IotcOhWifiDisable(void) | 关闭iot connect的Wi-Fi发现、连接、控制的能力，用于释放调用IotcOhWifiEnable时申请的资源，无法在iot connect运行时调用 |

#### 设备信息/服务/控制能力模块

| 函数原型 | 核心功能说明 |
| --- | --- |
| int32_t IotcOhDevInit(void) | 配置设备信息，并注册设备服务/控制等相关业务回调，该接口应在iot connect运行前调用 |
| int32_t IotcOhDevDeinit(void) | 释放调用IotcOhDevInit时申请的资源，该接口无法在iot connect运行时调用 |
| int32_t IotcOhDevReportCharState(const IotcCharState state[], uint32_t num) | 上报设备的服务信息，应在设备的服务信息发生变化/事件服务发生时调用；state为上报服务列表（应为常量指针，详见IotcCharState结构体定义），num为上报服务数量 |

### 开发步骤

**说明：** 以下演示介绍开发 IoT Connect 的完整流程，包含控制端APP安装部署，云平台部署，被控能力使能、配置参数、设备发现、数据交互及资源释放。

#### 控制端

| 项目  | 说明  | 链接  |
| --- | --- | --- |
| 通用互联APP | 控制核心入口，运行在HOS手机上 | [通用互联APP](https://gitcode.com/ohos-oneconnect/docs/blob/master/zh-cn/设备联动与控制/通用互联APP/README.md) |
| IoTManagementService | OpenHarmony设备上的设备管理服务组件 | [IoTManagementService](https://gitcode.com/ohos-oneconnect/IoTManagementService) |

#### 云平台

| 项目  | 说明  | 链接  |
| --- | --- | --- |
| 云平台 | 互联互通云端服务，提供账号、设备、场景管理 | [云平台](https://gitcode.com/ohos-oneconnect/docs/blob/master/zh-cn/设备联动与控制/云平台/README.md) |

#### 被控端

| 平台  | 套餐类型 | 开发指南 |
| --- | --- | --- |
| Hi3863 | WiFi only | [wifi_only_hi3863.md](./docs/wifi_only_hi3863.md) |
| Hi3863 | BLE only | [ble_only_hi3863.md](./docs/ble_only_hi3863.md) |
| Hi3863 | WiFi/BLE Combo | [wifi_ble_combo_hi3863.md](./docs/wifi_ble_combo_hi3863.md) |
| Hi3863 | BLE/SLE Combo | [ble_sle_combo_hi3863.md](./docs/ble_sle_combo_hi3863.md) |

#### 注意事项

- **模块调用顺序**：必须先调用各能力模块的使能接口（IotcOhBleEnable、IotcOhWifiEnable、IotcOhDevInit），再调用IotcOhMain启动部件业务；释放资源时，需先调用IotcOhStop停止部件，再依次释放各模块资源。
  
- **配置项要求**：设备管理模块的配置项为必需配置，未配置会导致部件启动失败；配置时需确保设备信息（如prodId、devTypeId）与云测定义严格一致。
  
- **广播与配网**：BLE广播时长可通过IOTC_OH_OPTION_BLE_START_UP_ADV_TIMEOUT配置（单位ms），GATT连接不会延长广播时间；Wi-Fi配网模式需根据设备能力选择，BLE辅助配网需确保BLE能力已使能。
  
- **数据交互限制**：IotcOhBleSendCustomSecData接口会对数据进行加密后发送，需确保待发送数据指针有效、长度合法；发送Indicate数据时，需正确传入GATT服务UUID和特征UUID。
  
- **回调线程限制**：所有注册的回调函数（如配网回调、数据接收回调）运行在非UI线程，请勿在回调中执行耗时操作，避免阻塞业务流程。
  
- **资源释放规范**：调用IotcOhBleRelease释放BLE资源时，需确保在部件运行时调用；IotcOhDevDeinit、IotcOhWifiDisable需在部件停止后调用，否则会返回失败。
  

## 配套资料

### 技术标准

| 标准  |
| --- |
| [《物模型技术标准》](https://www.giiconsortium.org/2025/post/2502/) |
| [《接入与控制接口技术标准》](https://www.giiconsortium.org/2025/post/2506/) |

### Release版本说明

| 版本  | 链接  |
| --- | --- |
| OneConnect v2.0.0 Release | [OpenHarmony OneConnect-v2.0.0-release](https://gitcode.com/ohos-oneconnect/docs/blob/master/zh-cn/release-notes/OpenHarmony%20OneConnect-v2.0.0-release.md) |

## 相关仓

[communication_wifi_lite](https://gitcode.com/openharmony/communication_wifi_lite)

[communication_bluetooth](https://gitcode.com/openharmony/communication_bluetooth)

[communication_bluetooth_service](https://gitcode.com/openharmony/communication_bluetooth_service)