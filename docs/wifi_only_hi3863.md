# WiFi Only 设备基于Hi3863平台DEMO编译构建

## 一、开发环境准备

### 1.1 硬件要求

| 设备类型 | 配置要求                                                          |
| ---- | ------------------------------------------------------------- |
| 开发板  | BearPi-pico H3863（Hi3863芯片），需配备USB数据线（用于烧录、调试及供电），以下简称为Hi3863 |
| 控制设备 | 可连接WiFi的HarmonyOS Next手机（用于运行通用互联APP，完成WiFi配网及设备控制）           |

### 1.2 软件要求

| 软件/工具         | 版本要求                    | 用途说明                                       |
| ------------- | ----------------------- | ------------------------------------------ |
| OpenHarmony源码 | 5.1.0 release           | 基础系统源码，为Hi3863平台提供编译底座及WiFi驱动支持            |
| IoTConnect组件  | 最新master分支              | 提供WiFi单模通信核心能力，支撑设备WiFi配网、与APP/云侧数据交互      |
| 通用互联APP       | 最新master分支              | 鸿蒙生态控制入口，运行在HarmonyOS手机，用于WiFi配网、设备控制及状态查看 |
| 编译工具链         | arm-none-eabi-gcc 9.3.1 | Hi3863芯片的ARM架构编译工具链，用于生成可执行镜像              |
| hb构建工具        | 0.4.6及以上                | OpenHarmony轻量级设备编译构建工具                     |
| Python        | 3.8~3.9                 | 运行hb工具及编译脚本                                |
| Git           | 2.25及以上                 | 克隆OpenHarmony源码、IoTConnect组件及示例代码仓库        |

## 二、示例代码

[WiFi Only示例代码](https://gitcode.com/ohos-oneconnect/applications_sample_iot_connect_samples/blob/master/wifi/liteos/ws63/iotc_oh_demo_wifi.c)

## 三、代码开发详解

### 3.1 云平台注册

- 登录OpenHarmony统一互联云平台，注册产品信息，获取产品ID（prodId）、厂商ID（manuId）及AC密钥，需与Demo中`DEV_INFO`、`AC_KEY`配置严格一致。

- 创建物模型，添加“switch”（开关）和“gps”（定位）服务，定义服务ID、类型及数据格式，确保与Demo中`SVC_INFO`数组配置匹配。

### 3.2 DEMO编写

创建自己的demo文件(.c文件)。

#### 3.2.1 配置设备基础信息

配置设备身份信息、配网PIN码及加密AC密钥，需与云平台注册信息完全一致，开发测试阶段可复用示例配置，量产时替换为实际信息，WiFi单模协议指定。

```c
// 设备基础信息配置（WiFi单模核心：protType设为IOTC_PROT_TYPE_WIFI）
static const IotcDeviceInfo DEV_INFO = {
    .sn = "12345678",          // 设备唯一序列号，量产建议拼接MAC地址
    .prodId = "00414",         // 云平台注册产品ID
    .subProdId = "",           // 无子产品时留空
    .model = "DISP-01",        // 设备型号，对应实际产品型号
    .devTypeId = "1414",       // 云平台设备类型ID
    .devTypeName = "Disp",     // 设备类型名称，APP显示用
    .manuId = "116",           // 云平台注册厂商ID
    .manuName = "SwanLink",    // 厂商名称，APP显示用
    .devName = "OneConnectName",// 设备名称，APP端显示
    .fwv = "1.0.0",            // 固件版本
    .hwv = "1.0.0",            // 硬件版本
    .swv = "1.0.0",            // 软件版本
    .protType = IOTC_PROT_TYPE_WIFI, // 关键配置：指定为WiFi单模
};

// 配网PIN码：8位数字，与APP配网时输入一致，用于身份校验
const char *PIN_CODE = "01234567";

// 端云通信加密AC密钥：32字节（需与云平台配置完全一致，示例为48字节，实际需按IOTC_AC_KEY_LEN修正）
const uint8_t AC_KEY[IOTC_AC_KEY_LEN] = {
    0x49, 0x3F, 0x45, 0x4A, 0x3A, 0x72, 0x38, 0x7B, 
    // 省略部分字节，实际需完整复制云平台获取的密钥
};
```

#### 3.2.2 服务定义与处理模块（核心业务）

根据云平台物模型定义服务列表及处理函数映射表，本Demo支持开关（双向交互）和GPS（双向交互，示例Put为空实现）服务，适配WiFi单模数据传输逻辑。

```c
// 服务列表：与云平台物模型服务ID、类型严格对齐
static const IotcServiceInfo SVC_INFO[] = {
    {"switch", "switch"},    // 开关服务：接收APP指令并返回状态
    {"gps", "gps"},          // GPS服务：上报定位数据，可扩展接收配置指令
};

// 服务处理函数映射表：关联服务与对应的Put（指令接收）、Get（状态查询）回调
const struct SvcMap
{
    const IotcServiceInfo *svc;
    int32_t (*putCharState)(const IotcServiceInfo *svc, const char *data, uint32_t len);
    int32_t (*getCharState)(const IotcServiceInfo *svc, char **data, uint32_t *len);
} SVC_MAP[] = {
    {&SVC_INFO[0], SwitchPutCharState, SwitchGetCharState}, // 开关服务（完整双向交互）
    {&SVC_INFO[1], GpsPutCharState, GpsGetCharState},       // GPS服务（可扩展双向交互）
};
```

#### 3.2.3 核心服务解析

##### 3.2.3.1 开关服务（switch）

实现APP指令接收（Put）和状态查询（Get）逻辑，核心为JSON数据解析与封装，无硬件依赖，适配WiFi单模数据传输格式。

- **Put回调**：解析APP下发的JSON指令，提取“on”字段值（0=关，1=开），更新全局开关状态变量。

- **Get回调**：创建JSON对象，封装当前开关状态，转换为字符串返回给APP，支撑状态同步。

```c
/**
 * @brief  解析JSON数据并设置开关状态（PUT操作）
 * @note   接收JSON格式的开关状态指令，解析出"on"字段的值并更新全局开关状态
 * @param  svc: 物联网服务信息结构体指针（当前函数未使用，保留为接口兼容）
 * @param  data: 待解析的JSON格式字符串
 * @param  len: JSON字符串的长度
 * @retval 0: 成功, -1: 失败（参数无效/JSON解析失败/字段获取失败）
 */
static int SwitchPutCharState(const IotcServiceInfo *svc, const char *data, uint32_t len)
{
    // 1. 参数合法性校验：数据指针为空 或 数据长度为0，直接返回错误
    if (data == NULL || len == 0)
    {
        DEMO_LOG("param invalid");  // 打印参数无效日志
        return -1;
    }

    // 2. 解析JSON字符串为cJSON对象
    cJSON *json = cJSON_Parse(data);
    if (json == NULL)  // JSON解析失败（格式错误/内存不足等）
    {
        DEMO_LOG("parse error");   // 打印解析失败日志
        return -1;
    }

    // 3. 获取JSON对象中"on"字段，并校验字段类型（必须是数字类型）
    cJSON *item = cJSON_GetObjectItem(json, "on");
    if (item == NULL || !cJSON_IsNumber(item))
    {
        cJSON_Delete(json);  // 解析失败时必须释放cJSON对象，避免内存泄漏
        DEMO_LOG("get on error");  // 打印获取"on"字段失败日志
        return -1;
    }

    // 4. 提取"on"字段的数值，并更新全局开关状态
    int32_t on = cJSON_GetNumberValue(item);
    DEMO_LOG("switch on put %d=>%d", g_switch, on);  // 打印开关状态变更日志（旧值=>新值）

    // 根据数值设置开关状态：0=关闭(false)，1=打开(true)
    if (on == 0)
    {
        g_switch = false;
    }
    else if (on == 1)
    {
        g_switch = true;
    }

    // 5. 释放cJSON对象内存（核心：使用完必须释放，避免内存泄漏）
    cJSON_Delete(json);
    return 0;  // 状态设置成功
}

/**
 * @brief  构建JSON数据并返回当前开关状态（GET操作）
 * @note   将全局开关状态封装为JSON格式字符串，供外部获取当前设备状态
 * @param  svc: 物联网服务信息结构体指针（当前函数未使用，保留为接口兼容）
 * @param  data: 输出参数，指向存储JSON字符串的指针（需外部释放内存）
 * @param  len: 输出参数，JSON字符串的长度
 * @retval 0: 成功, -1: 失败（参数无效/JSON创建失败/内存分配失败）
 */
static int SwitchGetCharState(const IotcServiceInfo *svc, char **data, uint32_t *len)
{
    // 1. 参数合法性校验：输出指针为空 或 指针已指向非空内存（避免内存覆盖/泄漏）
    if (data == NULL || *data != NULL)
    {
        DEMO_LOG("param invalid");  // 打印参数无效日志
        return -1;
    }

    // 2. 创建空的cJSON对象（用于构建返回的JSON数据）
    cJSON *json = cJSON_CreateObject();
    if (json == NULL)  // cJSON对象创建失败（内存不足等）
    {
        DEMO_LOG("create obj error");  // 打印创建对象失败日志
        return -1;
    }

    // 3. 向JSON对象中添加"on"字段，值为当前全局开关状态（bool转number）
    if (cJSON_AddNumberToObject(json, "on", g_switch) == NULL)
    {
        cJSON_Delete(json);  // 添加字段失败时释放cJSON对象
        DEMO_LOG("add num error");  // 打印添加字段失败日志
        return -1;
    }

    // 4. 将cJSON对象转换为无格式的JSON字符串（节省空间，无换行/空格）
    *data = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);  // 转换完成后释放cJSON对象（核心：避免内存泄漏）

    // 5. 校验JSON字符串生成结果
    if (*data == NULL)
    {
        DEMO_LOG("json print error");  // 打印字符串生成失败日志
        return -1;
    }

    // 6. 记录当前开关状态，并设置输出参数（JSON字符串长度）
    DEMO_LOG("switch get %d", g_switch);  // 打印当前开关状态日志
    *len = strlen(*data);  // 设置JSON字符串长度（strlen不包含末尾的'\0'）

    return 0;  // 状态获取&JSON构建成功
}
```

##### 3.2.3.2 GPS服务（gps）

示例用固定经纬度模拟GPS数据，实际场景需对接GPS模块实时读取。支持状态上报（Get）和指令接收（Put，空实现可扩展）。

```c
/**
 * @brief  构建包含GPS经纬度的JSON数据并返回（GET操作）
 * @note   该函数用于响应GPS状态获取请求，返回固定的经纬度信息（纬度30.496039，经度114.546093）
 * @param  svc: 物联网服务信息结构体指针（当前函数未使用，保留为接口兼容）
 * @param  data: 输出参数，指向存储JSON字符串的指针（内存由cJSON分配，需外部释放）
 * @param  len: 输出参数，返回JSON字符串的长度（不含末尾'\0'）
 * @retval 0: 成功, -1: 失败（参数无效/JSON创建失败/字段添加失败/字符串生成失败）
 */
static int GpsGetCharState(const IotcServiceInfo *svc, char **data, uint32_t *len)
{
    // 1. 参数合法性校验：输出指针为空 或 指针已指向非空内存（避免内存覆盖/泄漏）
    if (data == NULL || *data != NULL)
    {
        DEMO_LOG("param invalid");  // 打印参数无效日志
        return -1;
    }

    // 2. 创建空的cJSON对象（用于构建返回的JSON数据）
    cJSON *json = cJSON_CreateObject();
    if (json == NULL)  // cJSON对象创建失败（内存不足等原因）
    {
        DEMO_LOG("create obj error");  // 打印JSON对象创建失败日志
        return -1;
    }

    // 3. 向JSON对象添加经纬度字段（均为字符串类型）
    // 注：原日志提示"add num error"为笔误，实际是添加字符串字段，注释已标注
    if ((cJSON_AddStringToObject(json, "latitude", "30.496039") == NULL) || 
        (cJSON_AddStringToObject(json, "longitude", "114.546093") == NULL))
    {
        cJSON_Delete(json);  // 添加字段失败时释放cJSON对象，避免内存泄漏
        DEMO_LOG("add string error");  // 修正原日志提示（原"add num error"为笔误）
        return -1;
    }

    // 4. 将cJSON对象转换为无格式JSON字符串（无换行/空格，节省传输空间）
    *data = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);  // 转换完成后立即释放cJSON对象，避免内存泄漏

    // 5. 校验JSON字符串生成结果
    if (*data == NULL)
    {
        DEMO_LOG("json print error");  // 打印JSON字符串生成失败日志
        return -1;
    }

    // 6. 打印GPS经纬度日志，并设置输出参数（JSON字符串长度）
    DEMO_LOG("gps %s", "30.496039, 114.546093");  // 打印当前返回的经纬度信息
    *len = strlen(*data);  // 计算JSON字符串长度（strlen不包含末尾的'\0'）

    return 0;  // GPS状态获取&JSON构建成功
}

/**
 * @brief  处理GPS状态的PUT请求（仅参数校验和日志打印，无实际业务逻辑）
 * @note   当前版本仅做参数合法性校验和日志打印，未实现解析/更新GPS状态的逻辑
 * @param  svc: 物联网服务信息结构体指针（当前函数未使用，保留为接口兼容）
 * @param  data: 传入的JSON格式字符串（理论上包含GPS状态指令，当前未解析）
 * @param  len: 传入的JSON字符串长度
 * @retval 0: 成功（参数合法）, -1: 失败（参数无效）
 */
static int GpsPutCharState(const IotcServiceInfo *svc, const char *data, uint32_t len)
{
    // 1. 参数合法性校验：数据指针为空 或 数据长度为0
    if (data == NULL || len == 0)
    {
        DEMO_LOG("param invalid");  // 打印参数无效日志
        return -1;
    }

    // 2. 仅打印日志，未实现解析data更新GPS状态的业务逻辑
    DEMO_LOG("gps put %s", "30.496039, 114.546093");
    return 0;  // 参数合法则返回成功
}
```

##### 3.2.3.3 服务分发与全量上报

实现指令分发与状态采集逻辑，支撑批量指令处理和全量状态上报，适配IoTConnect组件WiFi单模数据交互规范。

- **指令分发**：`PutCharState`和`GetCharState`函数遍历服务映射表，根据服务ID将APP指令/查询请求分发至对应回调函数。
  
  ```c
  /**
   * @brief  通用的字符状态PUT操作调度函数
   * @note   遍历状态数组和服务映射表，根据svcId匹配对应的服务，并调用该服务的putCharState函数更新状态
   *         支持批量处理多个服务的状态更新，匹配失败/函数指针为空时跳过，记录首个（最后一个）错误码
   * @param  state: 待更新的状态数组（包含svcId、data、len等信息）
   * @param  num: 状态数组的元素个数
   * @retval 0: 所有服务PUT操作成功, -1: 参数无效, 其他值: 对应服务PUT操作返回的错误码
   */
  static int32_t PutCharState(const IotcCharState state[], uint32_t num)
  {
      // 1. 参数合法性校验：状态数组为空 或 数组长度为0，直接返回错误
      if (state == NULL || num == 0)
      {
          DEMO_LOG("param invalid");  // 打印参数无效日志
          return -1;
      }
  
      int32_t ret = 0;  // 全局返回值，默认成功（0）
      // 2. 外层循环：遍历所有待更新的状态项（支持批量处理多个服务）
      for (uint32_t i = 0; i < num; ++i)
      {
          // 3. 内层循环：遍历服务映射表（SVC_MAP），匹配对应的服务
          //    sizeof(SVC_MAP)/sizeof(SVC_MAP[0])：计算服务映射表的元素总数
          for (uint32_t j = 0; j < (sizeof(SVC_MAP) / sizeof(SVC_MAP[0])); ++j)
          {
              // 打印当前处理的服务ID和数据（调试用）
              DEMO_LOG("put char sid:%s data:%s", state[i].svcId, state[i].data);
  
              // 匹配规则：svcId不一致 或 该服务的putCharState函数指针为空 → 跳过当前服务
              if (strcmp(state[i].svcId, SVC_MAP[j].svc->svcId) != 0 || SVC_MAP[j].putCharState == NULL)
              {
                  continue;
              }
  
              // 4. 调用匹配服务的putCharState函数，执行具体的状态更新逻辑
              int32_t curRet = SVC_MAP[j].putCharState(SVC_MAP[j].svc, state[i].data, state[i].len);
              // 5. 错误处理：单个服务更新失败时，记录错误码（覆盖原有ret，保留最后一个错误）
              if (curRet != 0)
              {
                  ret = curRet;
                  DEMO_LOG("put char sid:%s error %d", state[i].svcId, ret);  // 打印具体服务的错误日志
              }
          }
      }
      return ret;  // 返回最终结果（0=全部成功，非0=最后一个失败服务的错误码）
  }
  
  /**
   * @brief  通用的字符状态GET操作调度函数
   * @note   遍历状态数组和服务映射表，根据svcId匹配对应的服务，调用getCharState函数获取状态
   *         结果通过out和len输出参数返回，匹配失败/函数指针为空时跳过，记录首个（最后一个）错误码
   * @param  state: 待获取状态的数组（仅使用svcId字段匹配服务）
   * @param  out: 输出参数数组，存储各服务返回的JSON字符串（内存由对应服务分配，需外部释放）
   * @param  len: 输出参数数组，存储各服务返回字符串的长度
   * @param  num: 状态数组/输出数组的元素个数（三者长度需一致）
   * @retval 0: 所有服务GET操作成功, -1: 参数无效, 其他值: 对应服务GET操作返回的错误码
   */
  static int32_t GetCharState(const IotcCharState state[], char *out[], uint32_t len[], uint32_t num)
  {
      // 1. 参数合法性校验：状态数组/输出数组/长度数组为空 或 数组长度为0 → 返回错误
      if (state == NULL || num == 0 || out == NULL || len == NULL)
      {
          DEMO_LOG("param invalid");  // 打印参数无效日志
          return -1;
      }
  
      int32_t ret = 0;  // 全局返回值，默认成功（0）
      // 2. 外层循环：遍历所有待获取状态的服务项（支持批量处理多个服务）
      for (uint32_t i = 0; i < num; ++i)
      {
          // 3. 内层循环：遍历服务映射表（SVC_MAP），匹配对应的服务
          for (uint32_t j = 0; j < (sizeof(SVC_MAP) / sizeof(SVC_MAP[0])); ++j)
          {
              // 打印当前处理的服务ID（调试用）
              DEMO_LOG("get char sid:%s", state[i].svcId);
  
              // 匹配规则：svcId不一致 或 该服务的getCharState函数指针为空 → 跳过当前服务
              if (strcmp(state[i].svcId, SVC_MAP[j].svc->svcId) != 0 || SVC_MAP[j].getCharState == NULL)
              {
                  continue;
              }
  
              // 4. 调用匹配服务的getCharState函数，获取状态并写入输出参数
              //    &out[i]/&len[i]：传递输出参数的地址，由对应服务填充结果
              int32_t curRet = SVC_MAP[j].getCharState(SVC_MAP[j].svc, &out[i], &len[i]);
              // 5. 错误处理：单个服务获取失败时，记录错误码（覆盖原有ret，保留最后一个错误）
              if (curRet != 0)
              {
                  ret = curRet;
                  DEMO_LOG("get char sid:%s error %d", state[i].svcId, ret);  // 打印具体服务的错误日志
              }
          }
      }
  
      return ret;  // 返回最终结果（0=全部成功，非0=最后一个失败服务的错误码）
  }
  ```

- **全量上报**：`ReportAll`函数批量采集所有服务状态，调用组件API上报至APP/云侧，上报后释放JSON内存避免泄漏。
  
  ```c
  /**
   * @brief  批量获取所有服务的状态并上报到物联网平台
   * @note   核心流程：1. 遍历SVC_MAP获取所有服务的状态；2. 若全部获取成功则调用上报接口；3. 无论上报结果如何，释放所有已分配的内存
   *         特点：单个服务状态获取失败时，立即终止获取流程且不上报；最终必做内存释放，避免泄漏
   * @retval 0: 所有服务状态获取成功且上报成功, 非0: 状态获取失败码 或 上报接口返回的错误码
   */
  static int32_t ReportAll(void)
  {
      // 1. 定义状态上报数组，长度与服务映射表SVC_MAP一致，初始化为0（避免野指针）
      //    sizeof(SVC_MAP)/sizeof(SVC_MAP[0])：动态计算SVC_MAP的元素总数，适配服务扩展
      IotcCharState reportInfo[sizeof(SVC_MAP) / sizeof(SVC_MAP[0])] = {0};
      int32_t ret;  // 全局返回值，存储状态获取/上报的结果
  
      // 2. 遍历所有服务，逐个获取状态并填充到上报数组
      for (uint32_t i = 0; i < (sizeof(SVC_MAP) / sizeof(SVC_MAP[0])); ++i)
      {
          // 2.1 填充当前服务的ID（用于上报时标识服务）
          reportInfo[i].svcId = SVC_MAP[i].svc->svcId;
  
          // 2.2 调用该服务的getCharState函数，获取状态数据和长度
          //    (char **)&reportInfo[i].data：类型转换，将data指针地址传递给输出参数
          //    &reportInfo[i].len：输出参数，接收状态数据的长度
          ret = SVC_MAP[i].getCharState(SVC_MAP[i].svc, (char **)&reportInfo[i].data, &reportInfo[i].len);
  
          // 2.3 错误处理：单个服务状态获取失败
          if (ret != 0)
          {
              DEMO_LOG("get char sid:%s error %d", reportInfo[i].svcId, ret);  // 打印失败服务的ID和错误码
              break;  // 终止后续服务的状态获取（失败则不上报）
          }
      }
  
      // 3. 若所有服务状态获取成功（ret==0），调用平台接口上报状态
      if (ret == 0)
      {
          ret = IotcOhDevReportCharState(reportInfo, sizeof(reportInfo) / sizeof(reportInfo[0]));
      }
  
      // 4. 内存释放：无论上报成功/失败，释放所有已分配的状态数据内存（核心：避免内存泄漏）
      for (uint32_t i = 0; i < (sizeof(SVC_MAP) / sizeof(SVC_MAP[0])); ++i)
      {
          // 仅释放非空的data指针（未获取成功的服务data为NULL，无需释放）
          if (reportInfo[i].data != NULL)
          {
              cJSON_free((char *)reportInfo[i].data);  // 释放cJSON分配的字符串内存
              reportInfo[i].data = NULL;  // 置空指针，避免野指针问题
          }
      }
  
      // 5. 返回最终结果（0=全部成功，非0=获取/上报失败码）
      return ret;
  }
  ```

#### 3.2.4 IoTConnect组件对接

##### 3.2.4.1 核心回调注册

通过宏定义简化组件参数设置，向IoTConnect组件注册指令处理、安全认证、全量上报等回调函数，组件收到WiFi数据或触发事件时自动调用。

```c
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_PUT_CHAR_STATE_CALLBACK, PutCharState);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_GET_CHAR_STATE_CALLBACK, GetCharState);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_REPORT_ALL_CALLBACK, ReportAll);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_GET_PINCODE_CALLBACK, GetPincode);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_GET_AC_KEY_CALLBACK, GetAcKey);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_DATA_FREE_CALLBACK, cJSON_free);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_REBOOT_CALLBACK, NoticeReboot);
```

##### 3.2.4.2 WiFi模块初始化与配置

启用WiFi模块，配置WiFi配网参数（SoftAP模式、配网超时），注册WiFi相关回调（证书获取），启动组件核心流程，适配WiFi单模场景。

```c
int32_t IotcOhDemoEntry(void)
{
    int32_t ret = IotcOhDevInit(); // 初始化设备管理模块
    if (ret != 0) { return ret; }

    ret = IotcOhWifiEnable(); // 关键操作：启用WiFi模块（WiFi单模核心）
    if (ret != 0) { return ret; }

    // 注册核心回调（指令处理、安全认证、状态上报等）
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_PUT_CHAR_STATE_CALLBACK, PutCharState);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_GET_CHAR_STATE_CALLBACK, GetCharState);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_REPORT_ALL_CALLBACK, ReportAll);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_GET_PINCODE_CALLBACK, GetPincode);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_DEVICE_GET_AC_KEY_CALLBACK, GetAcKey);

    // 配置WiFi配网参数：SoftAP模式（设备开启热点供APP连接配网），超时24小时
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_WIFI_NETCFG_MODE, IOTC_NET_CONFIG_MODE_SOFTAP);
    SET_OH_SDK_OPTION(ret, IOTC_OH_OPTION_WIFI_NETCFG_TIMEOUT, (24 * 60 * 60 * 1000));

    ret = IotcOhMain(); // 启动组件核心线程，开始WiFi通信
    return ret;
}
```

#### 3.2.5 任务启动与系统集成

基于CMSIS-OS2创建业务线程，延时等待WiFi模块就绪后启动组件，通过`SYS_RUN`注册为系统启动任务，Hi3863开机后自动执行WiFi单模逻辑。

```c
// 业务任务：等待WiFi模块就绪（10秒延时），启动组件
int IotTask(void)
{
    DEMO_LOG("sleep 10 seconds to wait wifi ready.");
    sleep(10); // 适配WiFi模块启动时序，避免初始化失败
    IotcOhDemoEntry();
    while (true)
    {
        osDelay(1000); // 循环延时，降低CPU占用
    }
    return 0;
}

// 系统启动入口：创建线程，注册为系统任务
static void IOTWiFiExampleEntry(void)
{
    osThreadAttr_t attr;
    attr.name = "IotTask";
    attr.stack_size = 0x4000; // 16KB栈空间，适配WiFi单模内存需求
    attr.priority = 19;       // 中等优先级，平衡响应速度与系统资源

    if (osThreadNew((osThreadFunc_t)IotTask, NULL, &attr) == NULL) {
        printf("[LedExample] Failed to create LedTask!\n");
    }
}

SYS_RUN(IOTWiFiExampleEntry); // 系统启动时自动执行
```

### 3.3 BUILD.gn配置

#### 3.3.1 核心作用

`BUILD.gn`是OpenHarmony轻量级设备的编译配置文件（基于GN构建系统），核心作用为定义编译目标、指定依赖库、配置头文件路径及编译选项，使Demo能正确链接IoTConnect组件及Hi3863 WiFi驱动。

配置前需确认以下路径有效：

- IoTConnect组件路径：`//foundation/communication/iot_connect/`

- Hi3863 SDK路径：`//device/soc/hisilicon/ws63v100/sdk/`

- 第三方依赖（cJSON、bounds_checking_function）已存在于对应路径

#### 3.3.2 配置解析

```gn
# 导入IoTConnect组件核心配置文件，加载组件编译依赖及宏定义
import("//foundation/communication/iot_connect/iotc.gni")

# 定义静态库编译目标，名称为"iotc_wifi_demo"，生成WiFi单模Demo核心库
static_library("iotc_wifi_demo") {
  # 指定Demo核心源码文件，仅包含WiFi单模Demo主文件
  sources = [
    "iotc_oh_demo_wifi.c",
  ]

  # 配置依赖库，确保编译时链接所需第三方库及IoTConnect核心组件
  deps = [
    // 安全函数库，提供memcpy_s等安全内存操作接口
    "//third_party/bounds_checking_function:libsec_shared",
    // cJSON静态库，用于Demo中JSON数据解析与生成（服务数据处理）
    "//build/lite/config/component/cJSON:cjson_static",
    // IoTConnect核心静态库，提供WiFi通信、配网等核心能力
    "//foundation/communication/iot_connect:iotc_static",
  ]

  # 配置头文件搜索路径，确保编译时找到依赖组件头文件
  include_dirs = [
    // IoTConnect组件通用接口头文件
    "//foundation/communication/iot_connect/interfaces/kits/common",
    // IoTConnect OH连接模式接口头文件
    "//foundation/communication/iot_connect/interfaces/kits/oh_connect",
    // IoTConnect适配器层头文件，适配Hi3863硬件与系统
    "//foundation/communication/iot_connect/adapter/include",
    // LittleFS文件系统头文件，用于组件配置文件存储
    "//device/soc/hisilicon/ws63v100/sdk/open_source/littlefs/v2.5.0",
  ]

  # C语言编译选项，强化编译检查与安全防护
  cflags = [
    "-ftrapv",                  // 检测整数溢出并触发陷阱
    "-Werror",                  // 将警告视为错误，强制修正潜在问题
    "-Wextra",                  // 开启额外警告检查，提升代码规范性
    "-Wshadow",                 // 检测变量隐藏问题（局部覆盖全局）
    "-fstack-protector-all",    // 开启全栈保护，防止栈溢出
    "-D_FORTIFY_SOURCE=2",      // 增强内存操作函数安全性
    "-Wformat=2",               // 强化格式化字符串检查，防注入漏洞
    "-Wfloat-equal",            // 警告浮点数直接相等比较，避免精度问题
    "-Wdate-time",              // 警告代码中包含日期时间，确保版本一致
    "-Wno-error=unused-parameter", // 不将未使用参数视为错误（适配回调接口）
    "-Wall",                    // 开启所有基础警告
    "-fPIC",                    // 生成位置无关代码，适配动态链接
    "-Wunused-parameter",       // 警告未使用参数，提醒优化代码
  ]
}
```

## 四、DEMO编译

WiFi Only（Hi3863平台）的编译方法以及步骤请参考示例代码仓的[README.md](https://gitcode.com/ohos-oneconnect/applications_sample_iot_connect_samples/blob/master/wifi/liteos/ws63/README_zh.md);

## 五、DEMO使用说明

### 5.1 WiFi单模设备点对点本地控制流程

1. 联系OpenHarmony统一互联PMC或在laval社区提单，完成APP白名单配置;

2. 编译通用互联APP源码，生成HAP包并安装至HarmonyOS Next手机。

3. 通过USB数据线连接Hi3863开发板与电脑，使用烧录工具将编译生成的镜像烧录至开发板。

4. 重启开发板，等待10秒（WiFi模块就绪，进入SoftAP配网模式）。

5. 打开通用互联APP，进入“点对点本地控”页面，APP自动扫描附近设备，显示设备名称“OneConnectName”，拖动设备到界面中心手机图标范围内，等待完成WiFi直连配对。

6. 配对成功后，可在详情页下发开关控制指令（更新开关状态），查看GPS模拟定位数据，实现WiFi单模本地控制。