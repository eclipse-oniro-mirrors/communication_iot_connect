# 自动化测试搭建

## 文档目录结构

下载工程软件

搭建测试环境

自动化脚本

## 下载工程软件

一、python安装：需要安装3.10.5~3.10.10之间的版本，其他版本可能出现兼容性问题。



二、pip源配置：

​      1、在用户目录下的pip目录创建pip.ini:

![image-20260509111655003](C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20260509111655003.png)

2、pip.ini内容如下：

[global]

index-url = https://mirrors.tools.huawei.com/pypi/simple

trusted-host = mirrors.tools.huawei.com

timeout = 120



三、IDE安装：推荐从pycharm官网获取2022.3以后的社区版本。



四、HDC安装：下载任意版本的HDC即可，下载后解压后，添加环境变量后重启电脑，即可使用hdc工具。



五、DevEco Testing Hypium安装包解压

​      执行pip install命令安装hypium

​      pip install --extra-index-url http://hypium.rnd.huawei.com/simple hypium --trusted-host hypium.rnd.huawei.com -i http://mirrors.tools.huawei.com/pypi/simple --trusted-host mirrors.tools.huawei.com -U



六、DevEco Testing Hypium插件安装及使用方法

​      在插件中搜索DevEco Testing-Hypium进行安装





## 搭建测试环境

### WiFi/BLE Combo设备配网云端交互详细指导

#### WiFi连接与云端激活

#### 设备连接WiFi网络

设备根据BLE下发的WiFi凭证，连接到家庭路由器，获取IP地址。

#### PSK密钥协商（/.sys/psk）

**协议**：IF2.1 (CoAP over TCP/UDP + 应用层加密)

**接口**：POST /.sys/psk

**目的**：设备与云端协商会话密钥，用于后续通信加密

**实际日志对照**：

```
日志时间: 2026-01-20 20:04:51.636
设备 -> 云端:
POST /.sys/psk
{
  "devId": "0282bb7d-2458-491e-a90a-c77174a3102c",
  "sn1": "7ddfd57bcd74c4f4",  // 设备随机数
  "seq": 169
}
CoAP Options:
{
  "REQ_ID": "45577aedb2f7c636dff074ffb713e53e36f0b714",
  "SEQ_NUM_ID": "21695101",
  "DEV_ID": "0282bb7d-2458-491e-a90a-c77174a3102c"
}

云端 -> 设备:
{
  "sn2": "bfe392c148262938",  // 云端随机数
  "encryptMode": "plaintext",  // 加密模式
  "seq": 170,
  "errcode": 0
}
CoAP Options:
{
  "SEQ_NUM_ID": "21695101"
}
```

**密钥派生过程**：

```
输入：
- psk: 配网时云端分配的预共享密钥
- sn1: 设备生成的随机数（8字节）
- sn2: 云端生成的随机数（8字节）

计算：
salt = sn1(hex→binary 8B) + sn2(hex→binary 8B) = 16B
Key和IV通过PBKDF2算法生成：

digest = PBKDF2(
  pass = psk,
  salt = sn1 || sn2,
  iterations = 1,
  digest = SHA-256,
  keylen = 32B
)

AES_Key = digest[0:16]  // 前16字节作为AES密钥
AES_IV = digest[16:32]  // 后16字节作为IV
HMAC_Secret = PBKDF2(
  pass = AES_Key,
  salt = sn1 || sn2,
  iterations = 1,
  digest = SHA-256,
  keylen = 32B
)
```

**加密方式**：

- 加密算法：AES-128-CBC-PKCS5Padding
- 完整性保护：HMAC-SHA256（32字节MAC）
- MAC追加在payload末尾

#### 设备激活（/.sys/activate）

**协议**：IF2.1 (CoAP over TCP)

**接口**：POST /.sys/activate

**目的**：设备完成云端注册激活，建立设备与用户账号的绑定关系

**实际日志对照**：

```
日志时间: 2026-01-20 20:04:53.278
设备 -> 云端:
POST /.sys/activate
{
  "code": "56473040",  // 注册码（BLE下发）
  "devId": "0282bb7d-2458-491e-a90a-c77174a3102c",
  "devInfo": {
    "sn": "hi3863test001",
    "model": "Hi3863",
    "devType": "1005",
    "manu": "111",
    "prodId": "0001H",
    "devName": "SmartSwitch",
    "fwv": "1.0.0",
    "hwv": "1.0.0",
    "swv": "1.0.0",
    "subProdId": "63",
    "protType": 12,  // 12=BLE+WiFi Combo
    "mac": "4E:D0:E0:8A:E8:A3"
  }
}
CoAP Options:
{
  "REQ_ID": "ff289299c034e6bf9feb6b3246903860d0c95121",
  "SEQ_NUM_ID": "21695102",
  "DEV_ID": "0282bb7d-2458-491e-a90a-c77174a3102c"
}

云端处理过程：
1. 验证注册码有效性
2. 检查设备证书认证结果
3. 创建设备记录（数据库操作）
   SQL插入参数：
   - devId: 0282bb7d-2458-491e-a90a-c77174a3102c
   - prodId: 0001H
   - devName: SmartSwitch
   - sn: hi3863test001
   - secret: 2lo4q0sv6sb9ta46v9kbd6nso2h6fwtl
   - projectId: 07912b92-e9bc-4ed9-b566-d8ac51b4e222
   - uid: 71815794283328698618827311787778
   - 其他设备信息...

4. 分配projectId
   - projectId: 2013583506982281218
   - familyId: 1959846095798644737

5. 返回激活结果

数据库查询结果示例：
<== Row: 2013583506982281218, 1959846095798644737, 
       0282bb7d-2458-491e-a90a-c77174a3102c, 
       99, NOT_DELETE, 2026-01-20 20:04:53, -1
```

**云端返回**：

```
{
  "errcode": 0,
  "devId": "0282bb7d-2458-491e-a90a-c77174a3102c",
  "projectId": "1959846095798644737"
}
```

#### 设备登录（/.sys/login）

**协议**：IF2.1 (CoAP over TCP)

**接口**：POST /.sys/login

**目的**：设备登录云端，建立长连接会话

**请求格式**：

```
POST /.sys/login
{
  "devId": "0282bb7d-2458-491e-a90a-c77174a3102c",
  "seq": 171
}
```

**响应**：

```
{
  "errcode": 0,
  "status": "online"
}
```







## 自动化脚本

一、新建Hypium项目

1、点击文件 --->新建项目

2、选择DevEco Testing Hypium

（1）设置项目位置和名称

（2）选择python解释器

（3）设置完成后点击创建



二、编写测试用例

（1）点击testcase文件夹

（2）选择新建Hypium Testcase

（3）设置用例名称，即可生成同名用例·py文件与json配置文件



三、工程目录文件介绍

```
hypiumProjectTemplate
|		|---aw                            //工程中自定义模块文件夹
|       |	|---Utils.py				  //示例模块文件
|		|---config						  //测试工程配置文件夹
|		|	|---user_config.xml			  //测试工程配置文件，主要是测试框架的任务配置
|		|---resource					  //测试资源文件夹，测试过程中用到的资源文件默认优先会从当前文件夹进行查找
|       |---testcases					  //测试用例文件夹，测试过程中的测试用例文件优先会从当前文件夹进行查找
|		|	---Example.json				  //Example测试用例配置文件，配置用例设备信息等
|		|   ---Example.py				  //Example测试用例文件，实际的测试逻辑代码
|		|---main.py						  //测试用例执行入口
```

