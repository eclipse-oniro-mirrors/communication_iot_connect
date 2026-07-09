## 轻量系统IoT connect组件编译和特性配置指导
**以海思3863芯片为例，其余产品可按实际位置自行修改**
1. 在vendor/hihope/nearlink_dk_3863/config.json中添加如下配置
```json
    {
      "subsystem": "communication",
      "components": [
        { 
          "component": "iot_connect",
          "description": "BLE only, no provisioning and no watchdog",
          "features":[
            "iot_connect_ble_support = true",
            "iot_connect_wifi_support = false",
            "iotc_connect_wifi_cloud_support = false",
            "iotc_connect_ble_net_cfg_support = false",
            "iotc_connect_device_watch_dog = false"
          ] 
        }
      ]
    }
```

2. 在device/soc/hisilicon/ws63v100/sdk/build/config/target_config/ws63/config.py中添加如下配置
```
{
    'ws63-liteos-app': {
        ...
        'ram_component': {
            ...
            'iotc'
        }
    }
}
```

3. 在device/soc/hisilicon/ws63v100/sdk/libs_url/ws63/cmake/ohos.cmake中添加如下配置
```
(${TARGET_COMMAND} MATCHES "ws63-liteos-app")
set(COMPONENT_LIST
    ... "iotc")
endif()
```

4. 在build/lite/components/communication.json中添加如下配置
```json
    {
      "component": "iot_connect",
      "description": "iot_connect based on liteos-a.",
      "optional": "true",
      "dirs": [
        "foundation/communication/iot_connect"
      ],
      "targets": [
        "//foundation/communication/iot_connect:iotc_static"
      ],
      "rom": "",
      "ram": "",
      "output": [ ],
      "adapted_kernel": [ "liteos_a" ],
      "features": [],
      "deps": {
        "third_party": [
        ],
        "kernel_special": {},
        "board_special": {},
        "components": [
        ]
      }
    }
```