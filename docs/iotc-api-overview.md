## 接口说明

IoT connect组件独立开放给设备厂商使用的接口，适配OpenHarmony 5.0 release及以上版本。
对外头文件目录为`interfaces/kits/common`与`interfaces/kits/oh_connect`

### 1. 部件运行管理

部件运行管理包括部件运行、复位、停止等接口，接口定义在`iotc_oh_sdk.h`内，部件运行前应先参考2、3、4章节使能对应的能力

#### 1.1 部件业务入口

**函数原型：**  
`/**`  
 ` * @return Returns 0 if successful; otherwise failed`  
 ` * @since 1`  
 ` */`  
`int32_t IotcOhMain(void)`

**说明：**
部件业务入口，调用该接口后部件会拉起自己的任务线程，并且大部分预配置类的接口不再可用

**返回值：**
类型：`int32_t`
值：`0`成功，其它失败，详见`iotc_errcode.h`

#### 1.2 部件复位

**函数原型：**  
`/**`  
 ` * @return Returns 0 if successful; otherwise failed`  
 ` * @since 1`  
 ` */`  
`int32_t IotcOhReset(void)`

**说明：**
部件复位，调用后部件会重置所有组件业务的运行状态，仅在部件运行时有效，该接口为同步接口，返回即表示复位成功/失败

**返回值：**
类型：`int32_t`
值：`0`成功，其它失败，详见`iotc_errcode.h`

#### 1.3 部件停止

**函数原型：**  
`/**`  
 ` * @return Returns 0 if successful; otherwise failed`  
 ` * @since 1`  
 ` */`  
`int32_t IotcOhStop(void)`

**说明：**
停止部件的运行，仅在部件运行时有效，该接口为同步接口，返回即表示停止成功/失败

**返回值：**
类型：`int32_t`
值：`0`成功，其它失败，详见`iotc_errcode.h`

#### 1.4 恢复出厂

**函数原型：**  
`/**`  
 ` * @return Returns 0 if successful; otherwise failed`  
 ` * @since 1`  
 ` */`  
`int32_t IotcOhRestore(void)`

**说明：**
通知所有业务恢复出厂，仅在部件运行时有效，该接口为同步接口，返回即表示恢复出厂成功/失败

**返回值：**
类型：`int32_t`
值：`0`成功，其它失败，详见`iotc_errcode.h`

#### 1.5 配置部件运行参数

**函数原型：**  
`/**`  
 ` * @return Returns 0 if successful; otherwise failed`  
 ` * @since 1`  
 ` */`  
`int32_t IotcOhSetOption(int32_t option, ...);`

**说明：**
配置部件运行时的参数

**返回值：**
类型：`int32_t`
值：`0`成功，其他失败，详见`iotc_errcode.h`

**参数列表：**

| 参数             | 类型  | 说明                   |
| -------------- | --- | -------------------- |
| int32_t option | 输入  | 待配置的参数类型，详见1.6配置选项章节 |

#### 1.7 配置选项

配置选项定义在`IotcOhOptionType`枚举，并通过`IotcOhSetOption`接口使用

| 配置项     | 枚举                                       | 参数                  | 使用样例                                                                                                                    | 备注                                               |
| ------- | ---------------------------------------- | ------------------- | ----------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------ |
| 日志等级    | `IOTC_OH_OPTION_部件_LOG_LEVEL`            | uint32_t            | uint32 logLevel = 7;<br>IotcOhSetOption(IOTC_OH_OPTION_部件_LOG_LEVEL, logLevel);                                         | 部件日志输出除了受该运行时日志等级约束以外，还受到编译时等级约束，不会输出高于日志编译等级的日志 |
| 主线程栈大小  | `IOTC_OH_OPTION_部件_MAIN_TASK_SIZE`       | uint32_t            | uint32 mainTaskSize= 0x4000;<br />IotcOhSetOption(IOTC_OH_OPTION_部件_MAIN_TASK_SIZE, mainTaskSize);                      |                                                  |
| 监控线程栈大小 | `IOTC_OH_OPTION_部件_MONITOR_TASK_SIZE`    | uint32_t            | uint32 monitorTaskSize= 0x1000;<br />IotcOhSetOption(IOTC_OH_OPTION_部件_MONITOR_TASK_SIZE, monitorTaskSize);             |                                                  |
| 配置文件路径  | `IOTC_OH_OPTION_部件_CONFIG_PATH`          | const char *        | const char *path = "/config/iotc";<br />IotcOhSetOption(IOTC_OH_OPTION_部件_CONFIG_PATH, path );                          |                                                  |
| 事件监听注册  | `IOTC_OH_OPTION_部件_REG_EVENT_LISTENER`   | IotcOhEventCallback | void YourEventListener(int32_t event);<br />IotcOhSetOption(IOTC_OH_OPTION_部件_REG_EVENT_LISTENER, YourEventListener);   |                                                  |
| 事件监听去注册 | `IOTC_OH_OPTION_部件_UNREG_EVENT_LISTENER` | IotcOhEventCallback | void YourEventListener(int32_t event);<br />IotcOhSetOption(IOTC_OH_OPTION_部件_UNREG_EVENT_LISTENER, YourEventListener); |                                                  |

### 2. BLE Connect能力

#### 2.1 BLE Connect能力使能

**函数原型：**  
`/**`  
 ` * @return Returns 0 if successful; otherwise failed`  
 ` * @since 1`  
 ` */`  
`int32_t IotcOhBleEnable(void)`

**说明：**
使能BLE发现、连接、控制的能力，该接口仅做使能，调用不会触发任何业务流程，该接口应在iot connect运行前调用

**返回值：**
类型：`int32_t`
值：`0`成功，其它失败，详见`iotc_errcode.h`

#### 2.2 BLE Connect能力关闭

**函数原型：**  
`/**`  
 ` * @return Returns 0 if successful; otherwise failed`  
 ` * @since 1`  
 ` */`  
`int32_t IotcOhBleDisable(void)`

**说明：**
关闭iot connect的BLE发现、连接、控制的能力，用于释放调用`IotcOhBleEnable`时申请的资源，该接口无法在iot connect运行时调用。

**返回值：**
类型：`int32_t`
值：`0`成功，其它失败，详见`iotc_errcode.h`

#### 2.3 发送BLE发现广播

**函数原型：**  
`/**`  
 ` * @param ms Indicate advertising duration in milliseconds.`  
 ` * @return Returns 0 if successful; otherwise failed`  
 ` * @since 1`  
 ` */`  
`int32_t IotcOhBleStartAdv(uint32_t ms);`

**说明：**
启动BLE广播发现，根据初始化参数的不同，iot connect启动后会自动发送一段时间广播，后续需要调用该接口激活广播，用于配合外部按键，实现按键触发的场景

**参数列表：**

| 参数          | 类型  | 说明                          |
| ----------- | --- | --------------------------- |
| uint32_t ms | 输入  | 广播时长，单位ms，gatt的连接不会刷新、延长该时间 |

**返回值：**
类型：`int32_t`
值：`0`成功，其它失败，详见`iotc_errcode.h`

#### 2.4 停止BLE发现广播

**函数原型：**  
`/**`  
 ` * @return Returns 0 if successful; otherwise failed`  
 ` * @since 1`  
 ` */`  
`int32_t IotcOhBleStopAdv(void)`

**说明：**
在BLE广播发现期间，调用该接口可以停止广播

**返回值：**
类型：`int32_t`
值：`0`成功，其它失败，详见`iotc_errcode.h`

#### 2.5 发送`customSecData`数据

**函数原型：**  
`/**`  
 ` * @param data Indicate pointer to buffer of data to be sent.`  
 ` * @param len Indicate data length in bytes.`  
 ` * @return Returns 0 if successful; otherwise failed`  
 ` * @since 1`  
 ` */`  
`int32_t IotcOhBleSendCustomSecData(const uint8_t *data, uint32_t len);`

**说明：**
部分场景下，需要使用`customSecData`服务通道，可以调用该接口，sdk完成数据加密后发送给对端

**参数列表：**

| 参数                  | 类型  | 说明     |
| ------------------- | --- | ------ |
| const uint8_t *data | 输入  | 待发送的数据 |
| uint32_t len        | 输入  | 数据长度   |

**返回值：**
类型：`int32_t`
值：`0`成功，其它失败，详见`iotc_errcode.h`

#### 2.6 发送BLE Indicate数据

**函数原型：**  
`/**`  
 ` * @param svcUuid Indicate Ble gatt svc uuid.`  
 ` * @param svcUuid Indicate Ble gatt svc character UUID.`  
 ` * @param data Indicate pointer to buffer of data to be sent.`  
 ` * @param len Indicate data length in bytes.`  
 ` * @return Returns 0 if successful; otherwise failed`  
 ` * @since 1`  
 ` */`  
`int32_t IotcOhBleSendIndicateData(const char *svcUuid, const char *c, const uint8_t *value, uint32_t valueLen)`

**说明：**
当使用部件管理GATT服务时，可以使用该接口发送BLE Indicate数据

**参数列表：**

| 参数                   | 类型  | 说明                          |
| -------------------- | --- | --------------------------- |
| const char *svcUuid  | 输入  | Ble gatt svc uuid           |
| const char *charUuid | 输入  | Ble gatt svc character UUID |
| const uint8_t *value | 输入  | 待发送的数据                      |
| uint32_t valueLen    | 输入  | 数据长度                        |

**返回值：**
类型：`int32_t`
值：`0`成功，其它失败，详见`iotc_errcode.h`

#### 2.7 配置选项

配置选项定义在`IotcOhOptionType`枚举，并通过`IotcOhSetOption`接口使用

| 配置项                       | 枚举                                             | 参数                            | 使用样例                                                                                                                                                                         | 备注                     |
| ------------------------- | ---------------------------------------------- | ----------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------- |
| 配置BLE Connect能力在配网完成后退出   | `IOTC_OH_OPTION_BLE_EXIT_AFTER_NETCFG`         | 无                             | IotcOhSetOption(IOTC_OH_OPTION_BLE_EXIT_AFTER_NETCFG);                                                                                                                       |                        |
| 注册接收配网信息的业务回调             | `IOTC_OH_OPTION_BLE_RECV_NETCFG_CALLBACK`      | IotcRecvNetCfgInfoCallback    | int32_t YourNetCfgInfoCallback(const char *netInfo, uint32_t len);<br />IotcOhSetOption(IOTC_OH_OPTION_BLE_RECV_NETCFG_CALLBACK, YourNetCfgInfoCallback);                    |                        |
| 注册`customSecData`服务数据业务回调 | `IOTC_OH_OPTION_BLE_RECV_CUSTOM_DATA_CALLBACK` | IotcRecvCustomSecDataCallback | int32_t YourRecvCustomSecDataCallback(const uint8_t *data, uint32_t len);<br />IotcOhSetOption(IOTC_OH_OPTION_BLE_RECV_CUSTOM_DATA_CALLBACK, YourRecvCustomSecDataCallback); |                        |
| 配置启动后BLE广播时长              | `IOTC_OH_OPTION_BLE_START_UP_ADV_TIMEOUT`      | uint32_t                      | uint32_t advTimeout = 10 * 60 * 1000;<br />IotcOhSetOption(IOTC_OH_OPTION_BLE_START_UP_ADV_TIMEOUT, advTimeout );                                                            | 单位ms                   |
| 注册GATT服务列表                | `IOTC_OH_OPTION_BLE_GATT_PROFILE_SVC_LIST`     | IotcBleGattProfileSvcList     | static const IotcBleGattProfileSvcList *YOUR_GATT_LIST = XXX;<br />IotcOhSetOption(IOTC_OH_OPTION_BLE_GATT_PROFILE_SVC_LIST, YOUR_GATT_LIST );                               | GATT服务列表的指针及内部指针应为静态常量 |

#### 2.8 结构体/枚举定义

**IotcBleGattProfileSvcList：**

| 成员                               | 描述            |
| -------------------------------- | ------------- |
| const IotcBleGattProfileSvc *svc | ble gatt服务    |
| uint32_t svcNum                  | ble gatt 服务数量 |

**IotcBleGattProfileSvc：**

| 成员                                      | 描述               |
| --------------------------------------- | ---------------- |
| const char *uuid                        | ble gatt 服务 uuid |
| const IotcBleGattProfileChar *character | ble gatt 属性      |
| uint32_t charNum                        | ble gatt 属性数量    |

**IotcBleGattProfileChar：**

| 成员                                 | 描述               |
| ---------------------------------- | ---------------- |
| const char *uuid                   | ble gatt 属性 uuid |
| uint32_t permission                | ble gatt 读写权限    |
| uint32_t property                  | ble gatt 特征属性    |
| readFunc                           | ble gatt 读回调     |
| writeFunc                          | ble gatt 写回调     |
| indicateFunc                       | ble gatt 指示回调    |
| const IotcBleGattProfileDesc *desc | 描述特征列表           |
| uint32_t descNum                   | 描述特征列表数量         |

**IotcBleGattProfileDesc：**

| 成员                  | 描述               |
| ------------------- | ---------------- |
| const char *uuid    | ble gatt 属性 uuid |
| uint32_t permission | ble gatt 读写权限    |
| readFunc            | ble gatt 读回调     |
| writeFunc           | ble gatt 写回调     |

**IotcBleGattProperties：**

| 成员                                                     | 描述      |
| ------------------------------------------------------ | ------- |
| IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_BROADCAST         | 可广播     |
| IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_READ              | 可读      |
| IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_WRITE_NO_RSP      | 可不响应写入  |
| IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_WRITE             | 可写入     |
| IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_NOTIFY            | 支持通知    |
| IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_INDICATE          | 支持指示    |
| IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_SIGNED_WRITE      | 支持带签名写入 |
| IOTC_BLE_GATT_CHARACTER_PROPERTY_BIT_EXTENDED_PROPERTY | 具有拓展属性  |

**IotcBleGattPermission：**

| 成员                                            | 描述        |
| --------------------------------------------- | --------- |
| IOTC_BLE_GATT_PERMISSION_READ                 | 可读        |
| IOTC_BLE_GATT_PERMISSION_READ_ENCRYPTED       | 加密可读      |
| IOTC_BLE_GATT_PERMISSION_READ_ENCRYPTED_MITM  | 中间人保护可读   |
| IOTC_BLE_GATT_PERMISSION_WRITE                | 可写        |
| IOTC_BLE_GATT_PERMISSION_WRITE_ENCRYPTED      | 加密可写      |
| IOTC_BLE_GATT_PERMISSION_WRITE_ENCRYPTED_MITM | 中间人保护可写   |
| IOTC_BLE_GATT_PERMISSION_WRITE_SIGNED         | 签名可写      |
| IOTC_BLE_GATT_PERMISSION_WRITE_SIGNED_MITM    | 中间人保护签名可写 |

#### 2.9 BLE Connect资源释放

**函数原型：**  
`/**`  
 ` * @return Returns 0 if successful; otherwise failed`  
 ` * @since 1`  
 ` */`  
`int32_t IotcOhBleRelease(void)`

**说明：**
BLE 资源释放，用于释放调用`IotcOhBleEnable`时申请的资源，该接口在iot connect运行时调用。

**返回值：**
类型：`int32_t`
值：`0`成功，其他失败，详见`iotc_errcode.h`

### 3. Wi-Fi Connect 能力

#### 3.1 Wi-Fi Connect能力使能

**函数原型：**  
`/**`  
 ` * @return Returns 0 if successful; otherwise failed`  
 ` * @since 1`  
 ` */`  
`int32_t IotcOhWifiEnable(void)`

**说明：**
使能基于Wi-Fi/LAN的发现、连接、控制的能力，该接口仅做使能，调用不会触发任何业务流程，该接口应在iot connect运行前调用

**返回值：**
类型：`int32_t`
值：`0`成功，其它失败，详见`iotc_errcode.h`

#### 3.2 Wi-Fi Connect能力关闭

**函数原型：**  
`/**`  
 ` * @return Returns 0 if successful; otherwise failed`  
 ` * @since 1`  
 ` */`  
`int32_t IotcOhWifiDisable(void)`

**说明：**
关闭iot connect的Wi-Fi发现、连接、控制的能力，用于释放调用`IotcOhWifiEnable`时申请的资源，该接口无法在iot connect运行时调用。

**返回值：**
类型：`int32_t`
值：`0`成功，其它失败，详见`iotc_errcode.h`

#### 3.3 配置选项

配置选项定义在`IotcOhOptionType`枚举，并通过`IotcOhSetOption`接口使用

| 配置项              | 枚举                                      | 参数                                                       | 使用样例                                                                                                                                                                               | 备注   |
| ---------------- | --------------------------------------- | -------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---- |
| 配置发送缓冲区大小        | `IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE`  | 个数：2<br />参数1：uint32_t 常驻缓冲区大小<br />参数2：uint32_t 缓冲区大小上限 | uint32_t resSendBufferSize = 0x4000;<br />uint32_t maxSendBufferSize = 0x10000;<br />IotcOhSetOption(IOTC_OH_OPTION_WIFI_SEND_BUFFER_SIZE, resSendBufferSize , maxSendBufferSize); |      |
| 配置接收缓冲区大小        | `IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE`  | 个数：2<br />参数1：uint32_t 常驻缓冲区大小<br />参数2：uint32_t 缓冲区大小上限 | uint32_t resSendBufferSize = 0x4000;<br />uint32_t maxSendBufferSize = 0x10000;<br />IotcOhSetOption(IOTC_OH_OPTION_WIFI_RECV_BUFFER_SIZE, resSendBufferSize , maxSendBufferSize); |      |
| 配置配网模式           | `IOTC_OH_OPTION_WIFI_NETCFG_MODE`       | int32_t                                                  | int32_t mode = IOTC_NET_CONFIG_MODE_SOFTAP;<br />IotcOhSetOption(IOTC_OH_OPTION_WIFI_NETCFG_MODE, mode);                                                                           |      |
| 配置配网超时时长         | `IOTC_OH_OPTION_WIFI_NETCFG_TIMEOUT`    | uint32_t                                                 | uint32_t netCfgTimeout = 10 * 60 * 1000;<br />IotcOhSetOption(IOTC_OH_OPTION_WIFI_NETCFG_TIMEOUT, netCfgTimeout);                                                                  | 单位ms |
| 注册端云根CA证书获取的业务回调 | `IOTC_OH_OPTION_WIFI_GET_CERT_CALLBACK` | IotcOhWifiGetCertCallback                                | int32_t YourWifiGetCertCallback(const char **ca[], uint32_t *num);<br />IotcOhSetOption(IOTC_OH_OPTION_WIFI_GET_CERT_CALLBACK, YourWifiGetCertCallback);                           |      |

#### 3.3 结构体/枚举定义

**IotcNetConfigMode：**

| 成员                           | 描述                   |
| ---------------------------- | -------------------- |
| IOTC_NET_CONFIG_MODE_NONE    | 无需配网，自身有配网/联网能力的设备选择 |
| IOTC_NET_CONFIG_MODE_SOFTAP  | 使用SoftAP配网           |
| IOTC_NET_CONFIG_MODE_BLE_SUP | 使用BLE辅助配网            |
| IOTC_NET_CONFIG_MODE_BLE_AGT | 使用BLE辅助配网并代理注册设备     |

### 4. 设备信息/服务/控制能力

#### 4.1 设备管理模块使能

**函数原型：**  
`/**`  
 ` * @return Returns 0 if successful; otherwise failed`  
 ` * @since 1`  
 ` */`  
`int32_t IotcOhDevInit(void)`

**说明：**
配置设备信息，并注册设备服务/控制等相关业务回调，该接口应在iot connect运行前调用

**返回值：**
类型：`int32_t`
值：`0`成功，其它失败，详见`iotc_errcode.h`

#### 4.2 设备管理模块去使能

**函数原型：**  
`/**`  
 ` * @return Returns 0 if successful; otherwise failed`  
 ` * @since 1`  
 ` */`  
`int32_t IotcOhDevDeinit(void)`

**说明：**
释放调用`IotcOhDevInit`时申请的资源，该接口无法在iot connect运行时调用

**返回值：**
类型：`int32_t`
值：`0`成功，其它失败，详见`iotc_errcode.h`

#### 4.3 服务主动上报

**函数原型：**  
`/**`  
 ` * @param state Indicate array of reported characteristic status.`  
 ` * @param num Indicate count of reported characteristic status entries.`  
 ` * @return Returns 0 if successful; otherwise failed`  
 ` * @since 1`  
 ` */`  
`int32_t IotcOhDevReportCharState(const IotcCharState state[], uint32_t num)`

**说明：**
上报设备的服务信息，应在设备的服务信息发生变化/事件服务发生时调用

**参数列表：**

| 参数                          | 类型  | 说明                                   |
| --------------------------- | --- | ------------------------------------ |
| const IotcCharState state[] | 输入  | 上报服务列表，详见`IotcCharState`结构体定义，应为常量指针 |
| uint32_t num                | 输入  | 上报服务数量                               |

**返回值：**
类型：`int32_t`
值：`0`成功，其它失败，详见`iotc_errcode.h`

#### 4.3 配置选项

配置选项定义在`IotcOhOptionType`枚举，并通过`IotcOhSetOption`接口使用

| 配置项              | 枚举                                              | 参数                                                           | 使用样例                                                                                                                                                                                     | 备注        |
| ---------------- | ----------------------------------------------- | ------------------------------------------------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------- |
| 配置服务信息修改业务回调     | `IOTC_OH_OPTION_DEVICE_PUT_CHAR_STATE_CALLBACK` | IotcDevProfPutCharState                                      | int32_t YourPutCharState(const IotcCharState state[], uint32_t num);<br />IotcOhSetOption(IOTC_OH_OPTION_DEVICE_PUT_CHAR_STATE_CALLBACK, YourPutCharState);                              | 必须配置      |
| 配置服务信息查询业务回调     | `IOTC_OH_OPTION_DEVICE_GET_CHAR_STATE_CALLBACK` | IotcDevProfGetCharState                                      | int32_t YourGetCharState(const IotcCharState state[], char *out[], uint32_t len[], uint32_t num);<br />IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_CHAR_STATE_CALLBACK, YourGetCharState); | 必须配置      |
| 配置全量可上报服务上报业务回调  | `IOTC_OH_OPTION_DEVICE_REPORT_ALL_CALLBACK`     | IotcDevProfReportAll                                         | int32_t YourReportAll(void);<br />IotcOhSetOption(IOTC_OH_OPTION_DEVICE_REPORT_ALL_CALLBACK, YourReportAll);                                                                             | 必须配置      |
| 配置获取PIN码回调       | `IOTC_OH_OPTION_DEVICE_GET_PINCODE_CALLBACK`    | IotcDevProfGetPincode                                        | int32_t YourGetPincode(uint8_t *buf, uint32_t bufLen);<br />IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_PINCODE_CALLBACK, YourGetPincode);                                                 | 单位ms，必须配置 |
| 配置获取厂商AC KEY业务回调 | `IOTC_OH_OPTION_DEVICE_GET_AC_KEY_CALLBACK`     | IotcDevProfGetAcKey                                          | int32_t YourGetAcKey(uint8_t *buf, uint32_t bufLen);<br />IotcOhSetOption(IOTC_OH_OPTION_DEVICE_GET_AC_KEY_CALLBACK, YourGetAcKey);                                                      | 必须配置      |
| 配置服务信息资源释放回调     | `IOTC_OH_OPTION_DEVICE_DATA_FREE_CALLBACK`      | IotcDevProfFree                                              | int32_t YourProfDataFree(void *ptr);<br />IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DATA_FREE_CALLBACK, YourProfDataFree);                                                                   | 必须配置      |
| 配置设备重启回调         | `IOTC_OH_OPTION_DEVICE_REBOOT_CALLBACK`         | IotcDevReboot                                                | int32_t YourDevReboot(int32_t res);<br />IotcOhSetOption(IOTC_OH_OPTION_DEVICE_REBOOT_CALLBACK, YourDevReboot);                                                                          | 必须配置      |
| 配置硬件随机数回调        | `IOTC_OH_OPTION_DEVICE_TRNG_CALLBACK`           | IotcDevTrng                                                  | int32_t YourDevTrng(uint8_t *buf, uint32_t len);<br />IotcOhSetOption(IOTC_OH_OPTION_DEVICE_TRNG_CALLBACK, YourDevTrng);                                                                 |           |
| 配置设备信息           | `IOTC_OH_OPTION_DEVICE_DEV_INFO`                | const IotcDeviceInfo *                                       | static const IotcDeviceInfo DEV_INFO = {...};<br />IotcOhSetOption(IOTC_OH_OPTION_DEVICE_DEV_INFO, &DEV_INFO );                                                                          | 必须配置      |
| 配置服务信息           | `IOTC_OH_OPTION_DEVICE_SVC_INFO`                | 个数：2<br />参数1：const IotcServiceInfo *<br />参数2：uint32_t 服务个数 | static const IotcDeviceInfo SVC_INFO= {...};<br />uint32_t svcNum = sizeof(SVC_INFO) / sizeof(SVC_INFO[0])<br />IotcOhSetOption(IOTC_OH_OPTION_DEVICE_SVC_INFO, DEV_INFO, svcNum );      | 必须配置      |

#### 4.4 结构体/枚举定义

**IotcCharState：**

| 成员    | 描述                                                 |
| ----- | -------------------------------------------------- |
| svcId | 服务id，应为以`\0`结束的字符串                                 |
| data  | 服务属性值，应为以`\0`结束的json字符串，格式为`{"属性1":xxx,"属性2":xxx}` |
| len   | 服务属性值字符串长度                                         |

**IotcDeviceInfo:**

| 成员                      | 描述                                   |
| ----------------------- | ------------------------------------ |
| const char *sn          | 设备SN，字符串长度(0,40]                     |
| const char *prodId      | 设备产品ID，字符串长度5，需严格与云测定义一致             |
| const char *subProdId   | 设备子产品ID，字符串长度2，可以为`NULL`             |
| const char *model       | 设备型号，字符串长度(0,32]，需严格与云测定义一致          |
| const char *devTypeId   | 设备类型ID，字符串长度4，需严格与云测定义一致             |
| const char *devTypeName | 设备类型名称，字符串长度(0,32]                   |
| const char *manuId      | 厂商ID，字符串长度3，需严格与云测定义一致               |
| const char *manuName    | 厂商名，字符串长度(0,32]                      |
| const char *fwv         | 固件版本号，字符串长度(0,64]                    |
| const char *hwv         | 硬件版本号，字符串长度(0,64]                    |
| const char *swv         | 软件版本号，字符串长度(0,64]                    |
| int8_t protType         | 设备协议类型，详见`IotcProtType`定义，需严格与云测定义一致 |

*注意：`manuName `与`devTypeName`用于BLE广播与SoftAp的SSID拼接，过长可能导致截断，其中BLE广播要求两个字段长度和小于10，SoftAP SSID要求两个字段和小于14字节*

**IotcServiceInfo:**

| 成员                  | 描述                          |
| ------------------- | --------------------------- |
| const char *svcType | 服务类型，以`\0`结束的字符串，需严格与云测定义一致 |
| const char *svcId   | 服务id，以`\0`结束的字符串，需严格与云测定义一致 |

**IotcRebootReason:**

| 成员                            | 描述                    |
| ----------------------------- | --------------------- |
| IOTC_REBOOT_WATCH_DOG_TIMEOUT | 软狗超时，部件内部的开门狗超时，业务不可用 |
| IOTC_REBOOT_RESTORE           | 设备恢复出厂                |