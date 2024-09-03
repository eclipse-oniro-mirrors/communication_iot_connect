## L2设备添加编译子系统
**以社区开源产品(rk3568)为例，其余产品可按实际位置自行修改**

1. vendor/hihope/rk3568/config.json 中添加
```
      "subsystem": "communication",
...
      "components": [
        {
          "component": "iot_connect",
          "features": [
            "iot_connect_feature_ble_support = false",
            "iot_connect_feature_wifi_support = true",
            "iot_connect_feature_ailife_support = false",
          ]
        }
      ]
```

2. productdefine/common/products/ohos-sdk.json
```
      "subsystem": "communication",
      "components": [
        ...
        { "component": "iot_connect" },
      ]
```

3. build/compile_standard_whitelist.json
```
    "third_deps_bundle_not_add": [
        ...
        "//foundation/communication/iot_connect:iotc_shared",
    ]
```
