# WiFi 局域网本地控

ws63 局域网本地控demo

## 配置说明

### OpenHarmony 代码集成

在linux 环境下载OpenHarmony代码，可以全量下载，可以只选择小系统代码。本demo OpenHarmony 环境为[5.1.0 release](https://gitcode.com/openharmony/docs/blob/master/zh-cn/release-notes/OpenHarmony-v5.1.0-release.md)

### iot connect 组件集成与适配

1、使用git命令克隆[communication_iot_connect](https://gitcode.com/ohos-oneconnect/communication_iot_connect)复制到foundation/communication目录下，将文件夹名称修改为iot_connect。

2、在foundation/communication/iot_connect/iotc.gni修改如下配置

```
declare_args() {
  iot_connect_wifi_support = true
  其他设置为false
}

if (defined(ohos_lite)) {
    iot_connect_ble_support = false
    iot_connect_sle_support = false
}
```

3、在foundation/communication/iot_connect/adapter/adapter.gni中修改mbedtls库路径

，将gni文件内的所有的device/soc/hisilicon/hi3863v100/sdk_liteos/open_source/mbedtls/mbedtls_v3.1.0/include路径修改为 device/soc/hisilicon/ws63v100/sdk/open_source/mbedtls/mbedtls_v3.1.0/include

4、修改iotc_os.c
foundation/communication/iot_connect/adapter/os/cmsis2/iotc_os.c MS_PER_SECOND 常量修改为100

5、lwip协议栈适配

foundation/communication/iot_connect/adapter/socket/iotc_socket.c路径下将原头文件替换成以下头文件

```c
#include "lwip/inet.h"
#include "lwip/netifapi.h"
#include "lwip/sockets.h"
#include "iotc_socket.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include "securec.h"
#include "iotc_errcode.h"
#include "iotc_network.h"
#include "iotc_mem.h"
#include "iotc_log.h"
#include <sys/time.h>
// #include <arpa/inet.h>
// #include <unistd.h>
// #include <fcntl.h>
```

### demo 集成

将本demo代码ble/litos/ws63文件夹复制到applications/sample/wifi-iot/app 目录下并改名为iotc_wifi_demo

### 编译配置

1、//vendor/hihope/nearlink_dk_3863目录下的config.json

```json
    {
      "subsystem": "communication",
      "components": [
        { "component": "iot_connect", "features":[] }
      ]
    }
```

2、device/soc/hisilicon/ws63v100/sdk/build/config/target_config/ws63/config.py

添加如下配置，并删除'ble_lite'、'sle_lite'

```
{
    'ws63-liteos-app': {
        ...
        'ram_component': {
            ...
            'iotc','iotc_wifi_demo',
        }
    }
}
```

3、device/soc/hisilicon/ws63v100/sdk/libs_url/ws63/cmake/ohos.cmake

添加如下配置，并删除'ble_lite'、'sle_lite'

```
(${TARGET_COMMAND} MATCHES "ws63-liteos-app")
set(COMPONENT_LIST
    ... "iotc" "iotc_wifi_demo")
endif()
```

4、applications/sample/wifi-iot/app/BUILD.gn添加如下配置

```
lite_component("app") {
    features = [ "iotc_wifi_demo" ]
}
```

5、build/lite/components/communication.json添加如下配置

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

6、device/soc/hisilicon/ws63v100/sdk/BUILD.gn

注释ble/sle依赖

```c
lite_component("sdk") {
  features = []

  deps = [
    "//build/lite/config/component/cJSON:cjson_static",
#    "//device/soc/hisilicon/ws63v100/adapter/hals/communication/ble_lite",
#    "//device/soc/hisilicon/ws63v100/adapter/hals/communication/sle_lite",
    "//device/soc/hisilicon/ws63v100/adapter/hals/communication/wifi_lite/wifiservice",

    #   "//device/soc/hisilicon/ws63v100/adapter/kal",
  ]
}
```

7、device/soc/hisilicon/ws63v100/adapter/hals/communication/wifi_lite/wifiservice/source/wifi_device.c

这里的sdk的DispatchScanStateChangeEvent没有定义，注释掉DispatchScanStateChangeEvent函数调用

```c
    for (int i = 0; i < WIFI_MAX_EVENT_SIZE; i++) {
        if (g_wifiEvents[i] == NULL) {
            continue;
        }
        // DispatchScanStateChangeEvent(NULL, g_wifiEvents[i], WIFI_STATE_NOT_AVAILABLE);
    }
```

8、工具链添加到环境变量

```bash
vim ~/.bashrc               # 编辑环境变量
export PATH=~/device/soc/hisilicon/ws63v100/sdk/tools/bin/compiler/riscv/cc_riscv32_musl_100/cc_riscv32_musl/bin     # 在环境变量的最后添加一行编译工具链路径信息
source ~/.bashrc            # 应用环境变量
```

### 编译和烧录

### 编译步骤

```
hb set
选择mini/hihope/nearlink_dk_3863
hb build -f
```

### 烧录参考
[ws63 官方文档](https://www.bearpi.cn/core_board/bearpi/pico/h3863/software/%E7%8E%AF%E5%A2%83%E6%90%AD%E5%BB%BAubuntu.html)