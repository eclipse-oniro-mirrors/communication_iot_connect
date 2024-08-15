/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef IOTC_CONFIG_H
#define IOTC_CONFIG_H
#include "iotc_def.h"

/* 根据设备等级不同，部分组件的实现可能会有不同，级别越高性能越好，消耗资源越多 */
#ifndef IOTC_CONF_DEVICE_LEVEL
    #define IOTC_CONF_DEVICE_LEVEL IOTC_DEVICE_LEVEL_MINI
#endif

/* mbedtls版本号，可以为2或3*/
#ifndef IOTC_CONF_ADAPTER_MBEDTLS_MAJOR_VERSION
    #define IOTC_CONF_ADAPTER_MBEDTLS_MAJOR_VERSION 3
#endif

/* socket是否基于lwip，0-否，其他-是 */
#ifndef IOTC_CONF_ADAPTER_SOCKET_LWIP_SUPPORT
    #define IOTC_CONF_ADAPTER_SOCKET_LWIP_SUPPORT 0
#endif

/* mbedtls是否启动debug，0-否，其他-是 */
#ifndef IOTC_CONF_ADAPTER_MBEDTLS_TLS_DEBUG
    #define IOTC_CONF_ADAPTER_MBEDTLS_TLS_DEBUG 0
#endif

/* mbedtls是否支持psk，0-否，其他-是 */
#ifndef IOTC_CONF_ADAPTER_MBEDTLS_TLS_PSK_SUPPORT
    #define IOTC_CONF_ADAPTER_MBEDTLS_TLS_PSK_SUPPORT 1
#endif

#ifndef IOTC_CONF_ADAPTER_MBEDTLS_CCM_SUPPORT
    #define IOTC_CONF_ADAPTER_MBEDTLS_CCM_SUPPORT 1
#endif

/* 支持服务的最大数量 */
#ifndef IOTC_CONF_PROF_MAX_SVC_NUM
#define IOTC_CONF_PROF_MAX_SVC_NUM 128
#endif

/* wifi发送缓冲区常驻内存大小 */
#ifndef IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_SIZE
    #if IOTC_CONF_DEVICE_LEVEL < IOTC_DEVICE_LEVEL_SMALL
        #define IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_SIZE (2 * 1024)
    #else
        #define IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_SIZE (64 * 1024)
    #endif
#endif

/* wifi发送缓冲区动态分配内存最大值 */
#ifndef IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_MAX_SIZE
    #if IOTC_CONF_DEVICE_LEVEL < IOTC_DEVICE_LEVEL_SMALL
        #define IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_MAX_SIZE (8 * 1024)
    #else
        #define IOTC_CONF_WIFI_DEFAULT_SEND_BUFFER_MAX_SIZE (2 * 1024 * 1024)
    #endif
#endif

/* wifi接收缓冲区常驻内存大小 */
#ifndef IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_SIZE
    #if IOTC_CONF_DEVICE_LEVEL < IOTC_DEVICE_LEVEL_SMALL
        #define IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_SIZE (2 * 1024)
    #else
        #define IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_SIZE (64 * 1024)
    #endif
#endif

/* wifi接收缓冲区动态分配内存最大值 */
#ifndef IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_MAX_SIZE
    #if IOTC_CONF_DEVICE_LEVEL < IOTC_DEVICE_LEVEL_SMALL
        #define IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_MAX_SIZE (2 * 1024)
    #else
        #define IOTC_CONF_WIFI_DEFAULT_RECV_BUFFER_MAX_SIZE (2 * 1024 * 1024)
    #endif
#endif

/* wifi缓冲区通过接口配置最大值 */
#ifndef IOTC_CONF_WIFI_BUFFER_MAX_SIZE
    #if IOTC_CONF_DEVICE_LEVEL < IOTC_DEVICE_LEVEL_SMALL
        #define IOTC_CONF_WIFI_BUFFER_MAX_SIZE (64 * 1024)
    #else
        #define IOTC_CONF_WIFI_BUFFER_MAX_SIZE (2 * 1024 * 1024)
    #endif
#endif

/* 适配层通过json打印的字符指针能否通过适配层内存释放接口释放，用于资源申请与释放的匹配 */
#ifndef IOTC_CONF_JSON_FREE_EQUAL_MALLOC_FREE
    #define IOTC_CONF_JSON_FREE_EQUAL_MALLOC_FREE 1
#endif

/* 内存测试，会导致性能较差且可能存在泄漏 */
#ifndef IOTC_CONF_MEM_DEBUG
    #define IOTC_CONF_MEM_DEBUG 0
#endif

/* 内存调试时是否释放内存，不释放内存判断重复释放时更加准确，但会导致内存大量泄漏 */
#ifndef IOTC_CONF_MEM_DEBUG_NO_FREE
    #define IOTC_CONF_MEM_DEBUG_NO_FREE 0
#endif

#ifndef IOTC_CONF_AILIFE_SUPPORT
    #define IOTC_CONF_AILIFE_SUPPORT 0
#endif

#ifndef IOTC_CONF_BROAD_CAST_SUPPORT
    #define IOTC_CONF_BROAD_CAST_SUPPORT 0
#endif

#ifndef IOTC_CONF_BUILD_TYPE
    #define IOTC_CONF_BUILD_TYPE IOTC_BUILD_TYPE_RELEASE
#endif

#ifndef IOTC_CONF_COAP_DEBUG_DUMP_PACKET
    #define IOTC_CONF_COAP_DEBUG_DUMP_PACKET 0
#endif

#ifndef IOTC_CONF_COAP_DEBUG_DUMP_PAYLOAD
    #define IOTC_CONF_COAP_DEBUG_DUMP_PAYLOAD 0
#endif

#ifndef IOTC_CONF_SOFTAP_MAX_PEER_SESS_NUM
    #define IOTC_CONF_SOFTAP_MAX_PEER_SESS_NUM 2
#endif

#ifndef IOTC_CONF_API_WAIT_MAX_TIME
    #define IOTC_CONF_API_WAIT_MAX_TIME (10 * 1000)
#endif

#ifndef IOTC_CONF_LOG_BUILD_LEVEL
    #define IOTC_CONF_LOG_BUILD_LEVEL  IOTC_LOG_LEVEL_MAX
#endif

#ifndef IOT_CONF_LOG_DEFAULT_LEVEL
    #define IOT_CONF_LOG_DEFAULT_LEVEL  IOTC_LOG_LEVEL_INFO
#endif

#endif /* IOTC_CONFIG_H */