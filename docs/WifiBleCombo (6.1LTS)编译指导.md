# Hi3863 WiFi/BLE Combo 编译构建

### 1、获取OH master系统底座

```bash
repo init -u git@gitcode.com:openharmony/manifest.git -b master --no-repo-verify
repo sync -c
repo forall -c 'git lfs pull'
bash build/prebuilts_download.sh  (如执行不成功，可加上sudo权限)
```



### 2、iot connect 组件集成

1、使用git命令克隆[communication_iot_connect](https://gitcode.com/openharmony/communication_iot_connect)复制到foundation/communication目录下，将文件夹名称修改为iot_connect。

2、在foundation/communication/iot_connect/iotc.gni修改如下配置

```text
declare_args() {
  iot_connect_ble_support = true
  iot_connect_wifi_support = true
  iot_connect_kv_support = true
  iotc_connect_ble_net_cfg_support = true
  iotc_connect_wifi_cloud_support = true
  其他设置为false
  如
  iot_connect_ailife_support = false
  iot_connect_mbedtls_v2_support = false
  iot_connect_mbedtls_psk_support = false
  iot_connect_mbedtls_ccm_support = false
}

if (defined(ohos_lite)) {
    iot_connect_ble_support = true
    iot_connect_sle_support = false
}
```

3、在foundation/communication/iot_connect/adapter/adapter.gni中修改mbedtls库路径，

将gni文件内的所有的device/soc/hisilicon/hi3863v100/sdk_liteos/open_source/mbedtls/mbedtls_v3.1.0/include路径修改为 device/soc/hisilicon/ws63v100/sdk/open_source/mbedtls/mbedtls_v3.1.0/include

4、修改foundation/communication/iot_connect/adapter/os/cmsis2/iotc_os.c文件中，MS_PER_SECOND 常量修改为100（此tick频率是100Hz而非常见的1000Hz）

```
#ifndef MS_PER_SECOND
#define MS_PER_SECOND   100
```

5、进入`foundation/communication/iot_connect/adapter/kv/oh_lite/iotc_kv.c`进行如下修改，避免创建文件报错（hi3863不支持多级目录直接创建，需要手动创建目录）

    uint32_t tagLen = strlen(tag);
    if (tagLen > MAX_TAG_LEN) {
        IOTC_LOGE("tagLen[%u] out of range", tagLen);
        return IOTC_ERR_PARAM_INVALID;
    }
    
    int ret = fs_adapt_mkdir("/data");  //添加此处
    ret = fs_adapt_mkdir("/data/app");  //添加此处
    ret = fs_adapt_mkdir("/data/app/iotc");  //添加此处
    ret = fs_adapt_mkdir(tag);  //添加此处

6、进入`foundation/communication/iot_connect/core/infrastructure/utils/utils_mutex_ex.c`,注释互斥锁创建，直接return（ws63v100锁的资源有限，若创建锁的话会导致编译失败，此处return）

```
bool UtilsExMutexLockInner(UtilsExMutex *mutex, uint32_t timeout, const char *func)
{
    return true; //添加此处
    if (mutex == NULL || func == NULL || mutex->mutexId == NULL) {
        IOTC_LOGW("param invalid");
        return false;
    } 
    …………
}

void UtilsExMutexUnlockInner(UtilsExMutex *mutex, const char *func)
{
    return;//添加此处
    if (mutex == NULL || func == NULL || mutex->mutexId == NULL) {
        IOTC_LOGW("param invalid");
        return;
    } 
    …………
}

```

7、进入`foundation/communication/iot_connect/core/wifi/infrastructure/transport/coap/endpoint/coap_endpoint.c`注释掉如下代码。（ws63v100锁的资源有限，若创建锁的话会导致编译失败，此处return）

```
/*
        endpoint->mutex = UtilsCreateExMutex();
        if (endpoint->mutex == NULL) {
            IOTC_LOGW("mutex create error");
            break;
        }
*/
```

8、进入`foundation/communication/iot_connect/core/wifi/application/cloud/biz/m2m_cloud_ctl.c`可以参考如下改法（云端下发的控制报文解析失败问题修改）：

```
static int32_t BuildCloudCtlRespMsg(CoapEndpoint *endpoint, const CoapPacket *req, const SocketAddr *addr,
                                    const M2mCloudContext *ctx, IotcJson *respJson)
{
    int32_t ret = IOTC_OK;
    do {
        uint32_t seg = 0;
        const CoapOption *reqIdOpt = CoapUtilsFindOption(req,COAP_OPTION_TYPE_REQ_ID, &seg);
        if(reqIdOpt == NULL || seg != 1 || reqIdOpt -> value.data == NULL || reqIdOpt -> value.len == 0){
            ret = IOTC_CORE_WIFI_M2M_ERR_CLOUD_GET_OPT_REQ_ID;
            break;
        }
        const CoapOption *devIdOpt = CoapUtilsFindOption(req,COAP_OPTION_TYPE_DEV_ID, &seg);
        if(devIdOpt == NULL || seg != 1 || devIdOpt -> value.data == NULL || devIdOpt -> value.len == 0){
            ret = IOTC_CORE_WIFI_M2M_ERR_CLOUD_GET_OPT_DEV_ID;
            break;
        }
        const CoapOption *userIdOpt = CoapUtilsFindOption(req,COAP_OPTION_TYPE_USER_ID, &seg);
        if(userIdOpt == NULL || seg != 1 || userIdOpt -> value.data == NULL || userIdOpt -> value.len == 0){
            ret = IOTC_CORE_WIFI_M2M_ERR_CLOUD_GET_OPT_USER_ID;
            break;
        }
        const CoapOption *seqIdOpt = CoapUtilsFindOption(req,COAP_OPTION_TYPE_SEQ_NUM_ID, &seg);
        if(seqIdOpt == NULL || seg != 1 || seqIdOpt -> value.data == NULL || seqIdOpt -> value.len == 0){
            ret = IOTC_CORE_WIFI_M2M_ERR_CLOUD_GET_OPT_SEQ_NUM_ID;
            break;
        }

        const CoapOption options[] = {
            {COAP_OPTION_TYPE_ACCESS_TOKEN_ID, {(const uint8_t *)ctx->tokenInfo.access, strlen(ctx->tokenInfo.access)}},
            {COAP_OPTION_TYPE_REQ_ID, {(const uint8_t *)reqIdOpt->value.data, reqIdOpt->value.len}},
            {COAP_OPTION_TYPE_DEV_ID, {(const uint8_t *)devIdOpt->value.data, devIdOpt->value.len}},
            {COAP_OPTION_TYPE_USER_ID, {(const uint8_t *)userIdOpt->value.data, userIdOpt->value.len}},
            {COAP_OPTION_TYPE_SEQ_NUM_ID, {(const uint8_t *)seqIdOpt->value.data, seqIdOpt->value.len}},
        };

        CoapServerRespParam respParam = {
            .req = req,
            .type = COAP_MSG_TYPE_NCON,
            .code = COAP_RESPONSE_CODE_CONTENT,
            .opNum = ARRAY_SIZE(options),
            .options = options,
            .payload = NULL,
            .payloadBuilder = CoapUtilsBuildJsonPayloadFunc,
            .payloadUserData = respJson,
            .preSize = 0,
        };
        CoapPacket packet;
        ret = CoapServerSendResp(endpoint, &respParam, addr, &packet);
        if (ret != IOTC_OK) {
            IOTC_LOGW("send e2e ctrl resp msg error %d", ret);
        }
    } while (false);
    return ret;
}

```

```
/*    
    CoapResponeNode *respInfo = M2mCloudCreateCoapNode(endpoint, req, addr, ctx);
    if (respInfo == NULL) {
        IOTC_LOGW("create cloud resp node error");
        return IOTC_ERROR;
    }
*/
```

9、进入`foundation/communication/iot_connect/core/wifi/infrastructure/transport/socket/trans_socket_udp.c`（加入套接字会失败，且配网走的是tcp），注释掉以下内容：

```
/*
    ret = JoinMulticastGroup(fd, param);
    if (ret != IOTC_OK) {
        return ret;
    }
*/
```

10、进入foundation/communication/iot_connect/adapter/log/hilog_m/iotc_log.c（添加日志换行，日志才会规则显示）

```
void IotcLogOutputImpl(uint8_t level, const char *fileName,
    const char *funcName, uint32_t line, const char *fmt, ...)
{
    const char *tag[6] = {"IC_FATAL", "IC_ERROR", "IC_WARN", "IC_NOTICE", "IC_INFO", "IC_DEBUG"};
    (void)fmt;
    if (funcName != NULL) {
        printf("%s:%s:%u, ", tag[level - 1], funcName, line);
    } else {
        printf("%s:%s:%u, ", tag[level - 1], fileName != NULL ? fileName : "NULL", line);
    }
    printf("\r\n");		//添加换行，方便后面查看日志
}
```



### 3、ble 接口适配

1、clone [communication_nearlink](https://gitcode.com/ohos-oneconnect/communication_nearlink),将communication_nearlink中的openharmony/device/soc/hisilicon/hi3863v100/hi3863_adapter/hals/communication/bluetooth文件夹复制到OpenHarmony源码的device/soc/hisilicon/ws63v100/adapter/hals/communication目录下。

2、在\\device\soc\hisilicon\ws63v100\sdk\BUILD.gn中更改为如下配置

```
lite_component("sdk") {
  features = []

  deps = [
    "//build/lite/config/component/cJSON:cjson_static",
    "//device/soc/hisilicon/ws63v100/adapter/hals/communication/bluetooth/services:btservice",
    "//device/soc/hisilicon/ws63v100/adapter/hals/communication/sle_lite",
    "//device/soc/hisilicon/ws63v100/adapter/hals/communication/wifi_lite/wifiservice",

    #   "//device/soc/hisilicon/ws63v100/adapter/kal",
  ]
}
```



3、在\\device\soc\hisilicon\ws63v100\adapter\hals\communication\bluetooth\services\BUILD.gn中，将文件内所有的//device/soc/hisilicon/hi3863v100路径改为//device/soc/hisilicon/ws63v100，例如将//device/soc/hisilicon/hi3863v100/sdk_liteos/include/middleware/services/bts/ble改为//device/soc/hisilicon/ws63v100/sdk/include/middleware/services/bts/ble"，文件路径都要对应起来。



### 4、lwip协议栈适配

1、foundation/communication/iot_connect/adapter/socket/iotc_socket.c路径下将原头文件替换成以下头文件

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

如果遇到fcntl或close接口未定义，把fcntl改为lwip_fcntl，close改为lwip_close

2、foundation/communication/iot_connect/adapter/socket/iotc_socket.c文件中的**IotcSelect**方法中，修改**select**为**lwip_select** （改为使用ws63v100 sdk实现的lwip_select去监听套接字，而不是直接使用系统的Select）

```c
int32_t ret = select(maxFd + 1, (readSet == NULL) ? NULL : &read,
        (writeSet == NULL) ? NULL : &write,
        (exceptSet == NULL) ? NULL : &except, &timeout);
        
        修改为
        
int32_t ret = lwip_select(maxFd + 1, (readSet == NULL) ? NULL : &read,
        (writeSet == NULL) ? NULL : &write,
        (exceptSet == NULL) ? NULL : &except, &timeout);
```



### 5、ws63 sdk适配

1、进入`device/soc/hisilicon/ws63v100/sdk/build/config/target_config/ws63/menuconfig/acore/ws63_liteos_app.config`（hi3863不支持多级目录直接创建，需要手动创建目录，需要修改CONFIG_LFS_PARTITION_ID为0x21，手动创建才能生效）

```
CONFIG_LFS_PARTITION_ID=0x21
```

2、进入`device/soc/hisilicon/ws63v100/sdk/open_source/lwip/lwip_adapter/liteos_207/src/arch/sys_arch.c`注释掉加锁的地方（ws63v100锁的资源不够，创建锁会失败），如下所示

```c
  mbox->first = 0;
  mbox->last = 0;
  mbox->is_full = 0;
  mbox->is_empty = 1;
  mbox->is_autoexpand = is_auto_expand;

//  ret = osal_mutex_init(&(mbox->mutex)); //注释掉这行
  ret = 0;		//添加ret = 0

```

3、进入`device/soc/hisilicon/ws63v100/sdk/open_source/lwip/lwip_v2.1.3/src/api/tcpip.c`为保证有足够时间清理tcpip资源，需要添加休眠时间，进行如下修改：

```c
static void
tcpip_timeouts_mbox_fetch(sys_mbox_t *tbox, void **msg)
{
  u32_t sleeptime, res;

again:
  LWIP_ASSERT_CORE_LOCKED();

  sleeptime = sys_timeouts_sleeptime();
  if (sleeptime == SYS_TIMEOUTS_SLEEPTIME_INFINITE) {
    UNLOCK_TCPIP_CORE();
    sys_arch_mbox_fetch(tbox, msg, 0);
    LOCK_TCPIP_CORE();
    return;
  } else if (sleeptime == 0) {
    sys_check_timeouts();
    /* We try again to fetch a message from the mbox. */
    UNLOCK_TCPIP_CORE();   //新增
    sys_msleep(100);   //新增
    LOCK_TCPIP_CORE();  //新增
    goto again;
  }

  UNLOCK_TCPIP_CORE();
  res = sys_arch_mbox_fetch(tbox, msg, sleeptime);
  LOCK_TCPIP_CORE();
  if (res == SYS_ARCH_TIMEOUT) {
    /* If a SYS_ARCH_TIMEOUT value is returned, a timeout occurred
       before a message could be fetched. */
    sys_check_timeouts();
    /* We try again to fetch a message from the mbox. */
    goto again;
  }
}
#endif /* !LWIP_TIMERS */
```

```
static void
tcpip_thread(void *arg)
{
  struct tcpip_msg *msg;
  LWIP_UNUSED_ARG(arg);

  LWIP_MARK_TCPIP_THREAD();

  LOCK_TCPIP_CORE();
  if (tcpip_init_done != NULL) {
    tcpip_init_done(tcpip_init_done_arg);
  }

  tcpip_init_finish = 1;   //@@@add  新增

  while (1) {                          /* MAIN Loop */
#if defined(LWIP_HI3861_THREAD_SLEEP) && LWIP_HI3861_THREAD_SLEEP
    UNLOCK_TCPIP_CORE();
    hi_cpup_load_check_proc(hi_task_get_current_id(), LOAD_SLEEP_TIME_DEFAULT);
    LOCK_TCPIP_CORE();
#endif
    LWIP_TCPIP_THREAD_ALIVE();
    /* wait for a message, timeouts are processed while waiting */
    TCPIP_MBOX_FETCH(MBOXPTR, (void **)&msg);
    if (msg == NULL) {
      LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_thread: invalid message: NULL\n"));
      LWIP_ASSERT("tcpip_thread: invalid message", 0);
      UNLOCK_TCPIP_CORE(); //新增
      sys_msleep(100);    //新增
      LOCK_TCPIP_CORE(); //新增
      continue;
    }
    tcpip_thread_handle_msg(msg);
  }
}
```

4、进入`device/soc/hisilicon/ws63v100/adapter/hals/communication/wifi_lite/wifiservice/source/wifi_device.c`文件。（修改扫描信道和按wifi名扫描，才能扫描到wifi）

AdvanceScan修改如下：

```c
	params->freqs = 2 ;		//新增
    sp.ssid_len = params->ssidLen;
    sp.scan_type = params->scanType;
    sp.channel_num = params->freqs;
    sp.scan_type = 2;		//新增
    
    hiRet = wifi_sta_scan_advance(&sp);

//添加这个修改才能扫描到wifi
```

ConnectTo函数修改如下（增加启动dhcp，获取ip地址）：

```
    if (StaConnect(0, &assocReq, g_wifiConfigs[networkId].wapiPskType) != WIFI_SUCCESS) {
        if (UnlockWifiEventLock() != WIFI_SUCCESS) {
            return ERROR_WIFI_UNKNOWN;
        }
        return ERROR_WIFI_UNKNOWN;
    }
    if (UnlockWifiEventLock() != WIFI_SUCCESS) {
        return ERROR_WIFI_UNKNOWN;
    }

    StaSetWifiNetConfig(WIFI_CONNECTED);	//添加这行，才能启动dhcp，成功获取ip地址

    return WIFI_SUCCESS;
```

5、进入device/soc/hisilicon/ws63v100/adapter/hals/communication/wifi_lite/wifiservice/source/wifi_device.c文件

这里的sdk的DispatchScanStateChangeEvent没有定义，注释掉DispatchScanStateChangeEvent函数调用

```
    for (int i = 0; i < WIFI_MAX_EVENT_SIZE; i++) {
        if (g_wifiEvents[i] == NULL) {
            continue;
        }
        // DispatchScanStateChangeEvent(NULL, g_wifiEvents[i], WIFI_STATE_NOT_AVAILABLE);
    }
```

6、进入device/soc/hisilicon/ws63v100/sdk/open_source/lwip/lwip_v2.1.3/src/include/lwip/sockets.h文件，ws63v100中将lwip_select重定向到了系统select。这样调用的就是系统select而不是自身实现的lwip_select了，需要将重定向注释掉。

```
// #define lwip_select       select
将这行注释掉，否则lwip_select调用的是系统select而不是ws63 SDk 自身实现的lwip_select
```



### 6、mbedtls修改

在third_party/mbedtls/port/config/config_liteos_m.h这个头文件中，打开MBEDTLS_NO_DEFAULT_ENTROPY_SOURCES宏

```
#define MBEDTLS_NO_DEFAULT_ENTROPY_SOURCES
```



### 7、集成WifiBleCombo的demo

获取hi3863平台 [WiFi/BLE Combo demo](https://gitcode.com/ohos-oneconnect/applications_sample_iot_connect_samples/tree/OpenHarmony-5.1.0-Release/wifi_ble_combo)，将wifi_ble_combo/hi3863文件夹复制到applications/sample/wifi-iot/app 目录下并改名为wifi_ble_demo



### 8、编译配置

1、\\vendor/hihope/nearlink_dk_3863目录下的config.json中添加iot_connect模块

```
    {
      "subsystem": "communication",
      "components": [
        { "component": "iot_connect", "features":[] }
      ]
    }
```

2、\\device/soc/hisilicon/ws63v100/sdk/build/config/target_config/ws63/config.py

添加如下配置，并删除’ble_lite‘  、 'sle_lite'

```
{
    'ws63-liteos-app': {
        ...
        'ram_component': {
            ...
            'iotc','wifi_ble_combo','btservice',
        }
    }
}
```

3、\\device/soc/hisilicon/ws63v100/sdk/libs_url/ws63/cmake/ohos.cmake

添加如下配置，并删除’ble_lite‘ 、'sle_lite'

```
(${TARGET_COMMAND} MATCHES "ws63-liteos-app")
set(COMPONENT_LIST
    ... "btservice"  "iotc" "wifi_ble_combo" )
endif()

```

4、\\applications/sample/wifi-iot/app/BUILD.gn添加如下配置

```
lite_component("app") {
  features = [ "startup" ]
  features += [ "./wifi_ble_demo:wifi_ble_combo" ]   //添加这行
}
```

5、\\build\lite\components\communication.json添加如下配置

```
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

6、工具链添加到环境变量

```
vim ~/.bashrc               # 编辑环境变量
export PATH=~/device/soc/hisilicon/ws63v100/sdk/tools/bin/compiler/riscv/cc_riscv32_musl_100/cc_riscv32_musl/bin     # 在环境变量的最后添加一行编译工具链路径信息
source ~/.bashrc            # 应用环境变量
```



### 9、编译和烧录

编译步骤

```
hb set
选择mini/hihope/nearlink_dk_3863
hb build -f
```

烧录参考[ws63 官方文档]([Ubuntu环境下开发环境搭建 | BearPi-Pico H3863 | 小熊派BearPi](https://www.bearpi.cn/core_board/bearpi/pico/h3863/software/环境搭建ubuntu.html))
