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
#ifndef SECURITY_SPEKE_CLIENT_H
#define SECURITY_SPEKE_CLIENT_H

#include "iotc_json.h"
#include "security_speke_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 发起协商请求，生成发送给服务端的消息
 *
 * @param session [IN] 会话
 * @param msg [OUT] 发送给对端的消息
 * @param len [OUT] 消息长度
 * @return 0成功，非0失败
 * @attention 调用方需要释放 msg
 */
int32_t SpekeClientStartReq(const SpekeSession *session, uint8_t **msg, uint32_t *len);

/**
 * @brief 处理服务端发送的协商响应消息，生成回复给对方的消息
 *
 * @param param [IN] 处理请求消息所需参数, 包括会话句柄, 会话Id, 服务端报文securityData中的payload
 * @param msg [OUT] 回复给对端的消息
 * @param len [OUT] 消息长度
 * @return 0成功，非0失败
 * @attention 调用方需要释放 msg
 */
int32_t SpekeClientProcessRsp(SpekeProcessParam param, uint8_t **msg, uint32_t *len);

/**
 * @brief 处理服务端发送的协商结束响应消息，生成回复给对方的消息，若协商成功，生成会话加密使用的密钥
 *
 * @param param [IN] 处理请求消息所需参数, 包括会话句柄, 会话Id, 服务端报文securityData中的payload
 * @param msg [OUT] 回复给对端的消息
 * @param len [OUT] 消息长度
 * @return 0成功，非0失败
 * @attention 调用方需要释放 msg
 */
int32_t SpekeClientProcessCfm(SpekeProcessParam param, uint8_t **msg, uint32_t *len);

#ifdef __cplusplus
}
#endif

#endif /* SECURITY_SPEKE_CLIENT_H */