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
#ifndef SECURITY_SPEKE_COMMON_H
#define SECURITY_SPEKE_COMMON_H

#include <stdint.h>
#include "iotc_json.h"
#include "security_speke.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    SpekeSession *session;
    int32_t msgType;
    const char *sessionId;
    IotcJson *secDataPayload;
} SpekeProcessParam;

/**
 * @brief 客户端或服务端将本端版本号信息编入目标 JSON 中
 *
 * @param secDataPayload [OUT] 目标 JSON，待发送报文中的 securityData 中的 payload
 * @return 0成功，非0失败
 */
int32_t SpekeCommonAddVerInfoToJson(IotcJson *secDataPayload);

/**
 * @brief 客户端或服务端校验对端 JSON 中的版本号信息
 *
 * @param secDataPayload [IN] 目标 JSON，对端报文中的 securityData 中的 payload
 * @return 0成功，非0失败
 */
int32_t SpekeCommonVerifyVersion(const IotcJson *secDataPayload);

/**
 * @brief 客户端或服务端用于生成发送给对端的消息
 *
 * @param sessionId [IN] 会话Id
 * @param msgType [IN] 发送报文的消息类型
 * @param payload [IN] 要发送的报文中 securityData 中的 payload 的内容
 * @param msg [OUT] 发送给对端的消息
 * @param len [OUT] 消息长度
 * @return 0成功，非0失败
 * @attention 调用方需要释放 msg
 */
int32_t SpekeCommonCreateNegoMsg(const char *sessionId, int32_t msgType, const IotcJson *secDataPayload,
    uint8_t **msg, uint32_t *len);

/**
 * @brief 将输入由十六进制转化为字符串，并添加至目标 JSON 中对应键中作为字符串值
 *
 * @param target [OUT] 目标 JSON
 * @param name [IN] 目标 JSON 的键
 * @param input [IN] 需要转化的输入
 * @param inputLen [IN] 输入长度
 * @return 0成功，非0失败
 */
int32_t SpekeCommonAddDataToJson(IotcJson *target, const char *name, const uint8_t *input, uint32_t inputLen);

/**
 * @brief 获取目标 JSON 中对应键中的字符串值，并转换为十六进制
 *
 * @param src [IN] 目标 JSON
 * @param name [IN] 目标 JSON 的键
 * @param output [OUT] 转换后的十六进制串
 * @param outputLen [OUT] 输出长度
 * @return 0成功，非0失败
 * @attention 调用方需要释放 output
 */
int32_t SpekeCommonParseDataFromJson(const IotcJson *src, const char *name, uint8_t **output, uint32_t *outputLen);

#ifdef __cplusplus
}
#endif

#endif /* SECURITY_SPEKE_COMMON_H */