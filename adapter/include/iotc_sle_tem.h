/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
 * Licensed u   nder the Apache License, Version 2.0 (the "License");
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
#ifndef IOTC_ADPT_SLE_TEM_H
#define IOTC_ADPT_SLE_TEM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "sle_ssap_server.h"
#include "sle_device_discovery.h"
#include "sle_errcode.h"
#include "sle_ssap_stru.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct SleUuidT IotcAdptUuidAddr;
// typedef struct ssaps_add_service_callback SsapsAddServiceCallback;
// typedef struct ssaps_add_property_callback SsapsAddPropertyCallback;
// typedef struct ssaps_add_descriptor_callback SsapsAddDescriptorCallback;
// typedef struct ssaps_start_service_callback SsapsStartServiceCallback;
// typedef struct ssaps_delete_all_service_callback SsapsDeleteAllServiceCallback;
// typedef struct ssaps_read_request_callback SsapsReadRequestCallback;
// typedef struct ssaps_write_request_callback SsapsWriteRequestCallback;
// typedef struct ssaps_mtu_changed_callback SsapsMtuChangedCallback;

typedef struct {
    ssaps_add_service_callback add_service_cb;               /*!< @if Eng Service added callback.
                                                                  @else   添加服务回调函数。 @endif */
    ssaps_add_property_callback add_property_cb;             /*!< @if Eng Characteristc added callback.
                                                                  @else   添加特征回调函数。 @endif */
    ssaps_add_descriptor_callback add_descriptor_cb;         /*!< @if Eng Descriptor added callback.
                                                                  @else   添加描述符回调函数。 @endif */
    ssaps_start_service_callback start_service_cb;           /*!< @if Eng Service started callback.
                                                                  @else   启动服务回调函数。 @endif */
    ssaps_delete_all_service_callback delete_all_service_cb; /*!< @if Eng Service deleted callback.
                                                                  @else   删除服务回调函数。 @endif */
    ssaps_read_request_callback read_request_cb;             /*!< @if Eng Read request received callback.
                                                                  @else   收到远端读请求回调函数。 @endif */
    ssaps_write_request_callback write_request_cb;           /*!< @if Eng Write request received callback.
                                                                  @else   收到远端写请求回调函数。 @endif */
    ssaps_mtu_changed_callback mtu_changed_cb;               /*!< @if Eng Mtu changed callback.
                                                                  @else   mtu 大小更新回调函数。 @endif */
} SsapsCallbacks;

// typedef struct ssaps_callbacks_t SsapsCallbacks;
typedef struct {
    uint16_t announceDataLen; /*!< @if Eng announce data length
                                      @else   设备公开数据长度 @endif */
    uint16_t seekRspDataLen;  /*!< @if Eng scan response data length
                                      @else   扫描响应数据长度 @endif */
    uint8_t  *announceData;    /*!< @if Eng announce data
                                      @else   设备公开数据 @endif */
    uint8_t  *seekRspData;     /*!< @if Eng scan response data
                                      @else   扫描响应数据 @endif */
} SleAnnounceData;

// typedef struct sle_announce_data_t SleAnnounceData;
typedef struct {
    uint8_t type;                         /*!< @if Eng SLE device address type { @ref sle_addr_type_t }
                                               @else   SLE设备地址类型 { @ref sle_addr_type_t } @endif */
    unsigned char addr[SLE_ADDR_LEN];     /*!< @if Eng SLE device address
                                               @else   SLE设备地址 @endif */
} SleAddr;
// typedef struct sle_addr_t SleAddr;

typedef struct {
    uint8_t  announceHandle;               /*!< @if Eng announce handle
                                                 @else   设备公开句柄，取值范围[0, 0xFF] @endif */
    uint8_t  announceMode;                 /*!< @if Eng announce mode { @ref sle_announce_mode_t }
                                                 @else   设备公开类型， { @ref sle_announce_mode_t } @endif */
    uint8_t  announceGtRole;              /*!< @if Eng G/T role negotiation indication
                                                         { @ref sle_announce_gt_role_t }
                                                 @else   G/T 角色协商指示，
                                                         { @ref sle_announce_gt_role_t } @endif */
    uint8_t  announceLevel;                /*!< @if Eng announce level
                                                         { @ref sle_announce_level_t }
                                                 @else   发现等级，
                                                         { @ref sle_announce_level_t } @endif */
    uint32_t announceIntervalMin;         /*!< @if Eng minimum of announce interval
                                                 @else   最小设备公开周期, 0x000020~0xffffff, 单位125us @endif */
    uint32_t announceIntervalMax;         /*!< @if Eng maximum of announce interval
                                                 @else   最大设备公开周期, 0x000020~0xffffff, 单位125us @endif */
    uint8_t  announceChannelMap;          /*!< @if Eng announce channel map
                                                 @else   设备公开信道, 0:76, 1:77, 2:78 @endif */
    int8_t   announceTxPower;             /*!< @if Eng adv transmit power
                                                 @else   广播发射功率，单位dbm, 取值范围[-127, 20],
                                                         0x7F：不设置特定发送功率 @endif */
    SleAddr own_addr;                    /*!< @if Eng own address
                                                 @else   本端地址 @endif */
    SleAddr peer_addr;                   /*!< @if Eng peer address
                                                 @else   对端地址 @endif */
    uint16_t connIntervalMin;             /*!< @if Eng minimum of connection interval
                                                 @else   连接间隔最小取值，取值范围[0x001E,0x3E80]，
                                                         announce_gt_role 为 SLE_ANNOUNCE_ROLE_T_NO_NEGO
                                                         时无需配置 @endif */
    uint16_t connIntervalMax;             /*!< @if Eng maximum of connection interval
                                                 @else   连接间隔最大取值，取值范围[0x001E,0x3E80]，
                                                         announce_gt_role 为 SLE_ANNOUNCE_ROLE_T_NO_NEGO
                                                         无需配置 @endif */
    uint16_t connMaxLatency;              /*!< @if Eng max connection latency
                                                 @else   最大休眠连接间隔，取值范围[0x0000,0x01F3]，
                                                         announce_gt_role 为 SLE_ANNOUNCE_ROLE_T_NO_NEGO
                                                         无需配置 @endif */
    uint16_t connSupervisionTimeout;      /*!< @if Eng connect supervision timeout
                                                 @else   最大超时时间，取值范围[0x000A,0x0C80]，
                                                         announce_gt_role 为 SLE_ANNOUNCE_ROLE_T_NO_NEGO
                                                         无需配置 @endif */
    void *extParam;                        /*!< @if Eng extend parameter, default value is NULL
                                                 @else   扩展设备公开参数, 缺省时置空 @endif */
} SleAnnounceParam;
// typedef struct sle_announce_param_t SleAnnounceParam;

typedef struct ErrcodeSleT  ErrCodeType;

typedef struct {
    uint16_t handle;      /*!< @if Eng Properity handle.
                               @else   属性句柄。 @endif */
    uint8_t type;         /*!< @if Eng property type { @ref ssap_property_type_t }.
                               @else   属性类型 { @ref ssap_property_type_t }。 @endif */
    uint16_t valueLen;   /*!< @if Eng Length of notification/indication data.
                               @else   通知/指示数据长度。 @endif */
    uint8_t *value;       /*!< @if Eng Notification/indication data.
                               @else   发送的通知/指示数据。 @endif */
} SsapsNtfInd;

#ifdef __cplusplus
}
#endif

#endif /* IOTC_ADPT_SLE_TEM_H */