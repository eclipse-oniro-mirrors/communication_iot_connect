## 小型系统添加编译子系统
**以社区开源产品(Hi3863)为例，其余产品可按实际位置自行修改**
1. 使用git命令克隆[communication_iot_connect](https://gitcode.com/ohos-oneconnect/communication_iot_connect)复制到foundation/communication目录下，将文件夹名称修改为iot_connect。
2. 在vendor/hihope/nearlink_dk_3863/config.json路径下添加如下配置
```json
    {
      "subsystem": "communication",
      "components": [
        { "component": "iot_connect", "features":[] }
      ]
    }
```

3. device/soc/hisilicon/ws63v100/sdk/build/config/target_config/ws63/config.py中添加如下配置
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

4. device/soc/hisilicon/ws63v100/sdk/libs_url/ws63/cmake/ohos.cmake添加如下配置
```
(${TARGET_COMMAND} MATCHES "ws63-liteos-app")
set(COMPONENT_LIST
    ... "iotc")
endif()
```

5. build/lite/components/communication.json添加如下配置
```json
    {
      "component": "iot_connect",
      "description": "iot_connect based on liteos-m and liteos-a.",
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
      "adapted_kernel": [ "liteos_m", "liteos_a" ],
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