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
#ifndef SECURITY_SPEKE_H
#define SECURITY_SPEKE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** speke会话句柄 */
typedef struct TagSpekeSession SpekeSession;

/** speke会话类型 */
typedef enum {
    SPEKE_TYPE_UNKNOWN = 0,
    SPEKE_TYPE_CLIENT, /**< 作为client端 */
    SPEKE_TYPE_SERVER  /**< 作为server端 */
} SpekeType;

typedef struct {
    /**
     * @brief 获取PIN码
     *
     * @param session [IN] 会话句柄
     * @param user [IN] 用户数据
     * @param pinCode [OUT] PIN码输出缓冲区
     * @param len [IN/OUT] 输入为缓冲区长度，输出为PIN码长度
     * @return 0成功非0失败
     */
    int32_t (*getPinCode)(SpekeSession *session, void *user, uint8_t *pinCode, uint32_t *len);
    /**
     * @brief speke协商完成通知回调函数
     *
     * @param session [IN] 会话句柄
     * @param user [IN] 用户数据
     * @param errorCode [IN] 协商结果，0成功，其他失败
     * @return 0成功非0失败
     */
    int32_t (*notifySpekeFinished)(SpekeSession *session, void *user, int32_t errorCode);
} SpekeCallback;

/**
 * @brief 初始化speke会话
 *
 * @param spekeType [IN] 会话类型
 * @param cb [IN] 回调函数
 * @param user [IN] 用户数据
 * @return 会话句柄，NULL失败
 */
SpekeSession *SpekeInitSession(SpekeType spekeType, const SpekeCallback *cb, void *user);

/**
 * @brief 客户端启动speke协商
 *
 * @param session [IN] 会话
 * @param msg [OUT] 发送对端的请求数据
 * @param len [OUT] 数据长度
 * @return 0成功，非0失败
 * @attention 调用方需要释放msg
 */
int32_t SpekeStartSession(const SpekeSession *session, uint8_t **msg, uint32_t *len);

/**
 * @brief SPEKE协商过程协议报文消息处理接口
 *
 * @param session [IN] 会话
 * @param requestPayload [IN] 对端发送消息字符串
 * @param msg [OUT] 回复对端的数据
 * @param len [OUT] 数据长度
 * @return 0成功，非0失败
 * @attention 调用方需要释放msg, 若无需要回复的数据, 则 *msg 为 NULL
 */
int32_t SpekeProcessPacket(SpekeSession *session, const char *requestPayload, uint8_t **msg, uint32_t *len);

/**
 * @brief 释放SPEKE协商会话
 *
 * @param session [IN] 会话
 */
void SpekeFreeSession(SpekeSession *session);

/**
 * @brief 解密收到的业务数据
 *
 * @param session [IN] 会话
 * @param data [IN] 业务数据
 * @param dataLen [IN] 数据长度
 * @param decData [OUT] 解密数据
 * @param decDataLen [OUT] 解密数据长度
 * @return 0成功，非0失败
 * @attention 调用方需要释放decData
 */
int32_t SpekeDecryptData(SpekeSession *session, const uint8_t *data, uint32_t dataLen,
    uint8_t **decData, uint32_t *decDataLen);

/**
 * @brief 加密业务数据
 *
 * @param session [IN] 会话
 * @param data [IN] 业务数据
 * @param dataLen [IN] 数据长度
 * @param encData [OUT] 加密数据
 * @param encDataLen [OUT] 加密数据长度
 * @return 0成功，非0失败
 * @attention 调用方需要释放encData
 */
int32_t SpekeEncryptData(SpekeSession *session, const uint8_t *data, uint32_t dataLen,
    uint8_t **encData, uint32_t *encDataLen);

#ifdef __cplusplus
}
#endif

#endif /* SECURITY_SPEKE_H */
