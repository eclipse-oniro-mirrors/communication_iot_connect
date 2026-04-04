## 一. 动态库sdk的下载和编译
-   [L2设备动态库编译](iotc-build-l2.md)

## 二. 本参考Demo的下载和编译
1. [下载源码](https://gitee.com/ohos-oneconnectapplications_sample_iot_connect_samples)
2. 将下载的applications_sample_iot_connect_samples文件夹拷贝到applications/sample文件夹
3. 将applications_sample_iot_connect_samples文件夹改名iot_connect_samples
4. 可参考swanlink_iotc/digital_signage编写自己的demo，并将自己的demo添加到编译子系统中


## 三 添加编译子系统构建APP
**以社区开源产品(rk3568)为例，其余产品可按实际位置自行修改**
1. 修改文件根目录下的BUILD.gn, 将自己想编译的demo目标添加到deps中
``` gn
# 如编译wifi/swanlink_iotc/digital_signage示例
group("iot_connect_samples") {
  if (iot_connect_samples_feature_appPath == "wifi/swanlink_iotc/digital_signage") {
    deps = [ "$app_build_path:swanlink_iotc_app" ]
  }
}
```

2. 如编译wifi/swanlink_iotc/digital_signage的示例，需iot_connect_samples_feature_appPath变量的值
 vendor/hihope/rk3568/config.json 中添加
``` json
    "subsystem": "applications",
      "components": [
        {
          "component": "iot_connect_samples",
          "features": [
            "iot_connect_samples_feature_appPath = wifi/swanlink_iotc/digital_signage"  #编译demo示例可选，可为空
          ]
        }
      ]
```

3. productdefine/common/products/ohos-sdk.json
``` json
      "subsystem": "applications",
      "components": [
        { "component": "iot_connect_samples" },
        ...
      ]
```

4. build/compile_standard_whitelist.json
```json
    "third_deps_bundle_not_add": [
        "//applications/sample/iot_connect_samples:iot_connect_samples",
        ...
    ]
```

## 四.selinux校验处理
启动编译的可执行文件需要selinux权限，调试时可选择临时关闭和临时关闭，或者根据需求进行配置
1. 临时关闭
``` shell
  setenforce 0
```

2. 永久关闭
修改 /etc/selinux/config
``` shell
SELINUX=disabled # 默认：SELINUX=enforcing
```

3. selinux配置相关服务
[参考selinux配置](https://laval.csdn.net/64ee90df4165333c3076badb.html)