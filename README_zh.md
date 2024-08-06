# IOT_CONNECT

## 说明

响应OpenHarmony生态共建伙伴提出智慧病房、智慧隧道、会议办公等场景的富设备对瘦设备控制的需求。在生态互联互通共建项目中，构建瘦设备上运行的极简连接控制组件：iot_connect，实现在mini/small级别OH设备上的控制和连接。

### 使用说明
#### 使能wifi_connect
1. 设备具有wifi能力时，调用接口使能wifi相关的能力
```
/* 使能wifi connect能力 */ 
int32_t IotcOhWifiEnable(void);
```

2. 当iot_connect退出后，可以通过接口去使能wifi相关的能力
```
/* 去使能wifi connect能力 */ 
int32_t IotcOhWifiDisable(void);
```
#### 使能ble_connect
1. 设备具有ble能力时，调用接口使能ble相关的能力
```
/* ble connect能力 */ 
int32_t IotcOhBleEnable(IotcOhBleInitParam *init);
```

2. 当iot_connect退出后，可以通过接口去使能ble相关的能力
```
/* 去使能ble connect能力 */ 
int32_t IotcOhBleDisable(void);
```
#### 使能设备信息及控制功能
1. 设备运行前，注册一些回调用于iot_connect查询设备信息及下发控制指令
```
/* 注册设备信息及控制回调 */ 
int32_t IotcOhDevInit(IotcOhDevInitParam *param);
```
2. 当iot_connect退出后，可以通过接口去使能设备信息及控制指令回调
```
/* 去注册设备信息及控制回调 */ 
int32_t IotcOhDevDeinit(void);
```

### DEMO
#### BLE设备DEMO
详细参考`interfaces/demo/oh_connect/iotc_oh_demo_ble.c`
```
static int32_t IotcOhDemoEntry(void)
{
    IotcOhProfCallback profCbs = {
        .putChar = PutCharState,
        .getChar = GetCharState,
        .reportAll = ReportAll,
        .getPincode = GetPincode,
        .getAcKey = GetAcKey,
        .free = cJSON_free,
    };
    /* 注册设备信息以及控制回调 */
    int32_t ret = IotcOhDevInit(&DEV_INFO, SVC_INFO, sizeof(SVC_INFO) / sizeof(SVC_INFO[0]), &profCbs);
    if (ret != 0) {
        DEMO_LOG("init device error %d", ret);
        return ret;
    }

    /* 使能BLE Connect能力 */
    ret = IotcOhBleEnable(NULL);
    if (ret != 0) {
        DEMO_LOG("enable ble connect error %d", ret);
        return ret;
    }

    /* 启动互联互通服务 */
    ret = IotcOhMain();
    if (ret != 0) {
        DEMO_LOG("iotc oh main error %d", ret);
        return ret;
    }
    DEMO_LOG("iotc oh main success");
    return ret;
}
```

互联互通服务启动后，所有的业务交互都通过注册的回调进行，服务的主动上报使用接口`IotcOhDevReportCharState`