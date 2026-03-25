/*
 * Copyright (c) 2024-2024 ShenZhen Kaihong Device Co., Ltd.
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



#ifndef __M2M_CLOUD_RESPONSE_H__
#define __M2M_CLOUD_RESPONSE_H__

#include "utils_list.h"
#include "m2m_cloud_ctx.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    CoapEndpoint *endpoint;
    const CoapPacket *req;
    const SocketAddr *addr;
    const M2mCloudContext *ctx;
    char  *devId;
    char  *msgId;
    ListEntry list;
} CoapResponeNode;

/**
 * 处理M2M云响应消息
 *
 * @param dataArray 包含IoT JSON数据的数组，用于处理云响应消息
 * @return 返回int32_t类型的处理结果，表示消息处理的成功与否或状态
 */
int32_t M2mCloudResponseMessage(const IotcJson *dataArray);

/**
 * 创建COAP协议节点响应
 *
 * 此函数用于在M2M云环境中根据COAP端点和请求包生成一个新的COAP响应节点
 * 它考虑了请求的上下文和设备的网络地址来创建合适的响应
 *
 * @param ep COAP端点结构体指针
 * @param req_pkt 指向接收到的COAP请求包的指针，用于生成响应
 * @param sock_addr 指向套接字地址结构的指针，包含请求发起者的网络地址信息
 * @param cloud_ctx 指向M2M云上下文的指针，包含云交互的必要信息
 * @return 返回新创建的COAP响应节点的指针，用于后续处理或响应
 */
CoapResponeNode* M2mCloudCreateCoapNode(CoapEndpoint *ep, const CoapPacket *req_pkt,const SocketAddr *sock_addr, const M2mCloudContext *cloud_ctx);

/**
 * 根据消息ID查找COAP响应节点
 *
 * 此函数通过消息ID和设备ID在M2M云环境中查找特定的COAP响应节点
 * 它用于在大量响应节点中快速定位特定消息的响应节点
 *
 * @param msgId 消息ID字符串，用于标识特定的消息
 * @param devId 设备ID字符串，表示消息所属的设备
 * @return 返回找到的COAP响应节点的指针，如果没有找到则返回NULL
 */
CoapResponeNode* M2mCloudFindNodeByMsgId( const char *msgId, const char *devId);

/**
 * 移除M2M云中的COAP响应节点
 *
 * 此函数负责从M2M云环境中移除指定的COAP响应节点
 * 它用于清理不再需要的节点，以释放资源或更新节点结构
 *
 * @param node 指向要移除的COAP响应节点的指针，该节点将从环境中移除
 */
void M2mCloudRemoveNode(CoapResponeNode *node);


#ifdef __cplusplus
}
#endif

#endif