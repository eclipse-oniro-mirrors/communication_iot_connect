/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** speke会话句柄 */
typedef struct TagSpekeSession SpekeSession;

/* SPEKE 帧几何：版本字节 + 12B IV + 尾部 16B TAG（自 session.c 上移共享） */
#define GCM_VER_LEN 1
#define GCM_IV_LEN 12
#define GCM_TAG_LEN 16
#define SPEKE_ENC_DATA_MIN_LEN (GCM_VER_LEN + GCM_IV_LEN + GCM_TAG_LEN)

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
 * @param payloadLen [IN] 消息字节长度（不依赖 NUL 结尾）
 * @param msg [OUT] 回复对端的数据
 * @param len [OUT] 数据长度
 * @param payloadWritable [IN] requestPayload 宿主缓冲是否可写（true 时 epk 视图就地
 *                            unhexify，省贯穿 exp_mod 的中转堆分配）
 * @return 0成功，非0失败
 * @attention 调用方需要释放msg, 若无需要回复的数据, 则 *msg 为 NULL
 */
/* 收包视图：报文数据 + 宿主缓冲可写性（可写时 epk 允许就地 unhexify） */
typedef struct {
    const char *data;   /* 报文数据（flat 视图宿主） */
    uint32_t len;       /* 报文长度 */
    bool writable;      /* 宿主缓冲可写（BLE mergeBuff=true；SLE/CoAP 待核实=false） */
} SpekePktView;

/* 新入口（BLE）：PktView 携带宿主可写性，epk 可就地 unhexify */
int32_t SpekeProcessPacketView(SpekeSession *session, const SpekePktView *pkt,
    uint8_t **msg, uint32_t *len);

/* 加解密输出缓冲聚合（加密直写/解密直写共用，避免 6 参超限） */
typedef struct {
    uint8_t *buff;      /* 输出缓冲（调用方分配并管理） */
    uint32_t buffCap;   /* 缓冲容量 */
    uint32_t *buffLen;  /* 实际长度出参 */
} SpekeDataBuf;

/* 旧入口（WiFi/SLE 兼容）：基线签名（长度取 NUL 结尾，与基线一致），宿主按只读处理 */
int32_t SpekeProcessPacket(SpekeSession *session, const char *data,
    uint8_t **msg, uint32_t *len);

/**
 * @brief SPEKE协商过程协议报文消息处理接口（返回缓冲前置预留段，）
 *
 * 与 SpekeProcessPacket 行为一致，区别：返回缓冲前部预留 prefixLen 字节（置零）供
 * 调用方写帧头，消息体位于 [prefixLen, prefixLen+消息长度)，结尾 NUL，
 * *len = prefixLen + 消息长度。SERVER_RSP 大报文直接在偏移处序列化（零中转拷贝），
 * 其余小报文经一次拷贝前缀化。
 *
 * @param session [IN] 会话
 * @param requestPayload [IN] 对端发送消息字符串
 * @param payloadLen [IN] 消息字节长度（不依赖 NUL 结尾）
 * @param payloadWritable [IN] requestPayload 宿主缓冲是否可写
 * @param prefixLen [IN] 返回缓冲的前置预留字节数
 * @param msg [OUT] 回复对端的数据（含前缀预留段）
 * @param len [OUT] prefixLen + 消息长度
 * @return 0成功，非0失败
 * @attention 调用方需要释放msg, 若无需要回复的数据, 则 *msg 为 NULL
 */
int32_t SpekeProcessPacketPrefix(SpekeSession *session, const SpekePktView *pkt,
    uint32_t prefixLen, uint8_t **msg, uint32_t *len);

/**
 * @brief 释放SPEKE协商会话
 *
 * @param session [IN] 会话
 */
void SpekeFreeSession(SpekeSession *session);

/**
 * @brief 释放SPEKE的NegoContext
 *
 * @param session [IN] 会话
 */
void SpekeFreeNegoContext(SpekeSession *session);

/**
 * @brief 解密收到的业务数据（就地解密，明文写回 data；BLE 使用，宿主缓冲需可写）
 *
 * @param session [IN] 会话
 * @param data [IN/OUT] 业务数据（解密就地写回）
 * @param dataLen [IN] 数据长度
 * @param decDataLen [OUT] 解密后数据长度
 * @return 0成功，非0失败
 */
int32_t SpekeDecryptDataInPlace(SpekeSession *session, uint8_t *data, uint32_t dataLen, uint32_t *decDataLen);

/**
 * @brief 解密收到的业务数据（WiFi/SLE 兼容：基线签名，库外 malloc 输出缓冲、
 *        所有权移交调用方；行为与基线一致）
 *
 * @param session [IN] 会话
 * @param data [IN] 密文（只读）
 * @param dataLen [IN] 数据长度
 * @param decData [OUT] 解密输出缓冲（调用方释放）
 * @param decDataLen [OUT] 解密后数据长度
 * @return 0成功，非0失败
 */
int32_t SpekeDecryptData(SpekeSession *session, const uint8_t *data, uint32_t dataLen,
    uint8_t **decData, uint32_t *decDataLen);

/**
 * @brief 解密收到的业务数据（const 输入，输出写入独立缓冲；供收包缓冲不可写的通道使用）
 *
 * @param session [IN] 会话句柄
 * @param data [IN] 密文数据
 * @param dataLen [IN] 密文长度
 * @param outBuf [OUT] 明文输出缓冲，可写长度至少为 dataLen - SPEKE_ENC_DATA_MIN_LEN
 * @param outBufCap [IN] 输出缓冲容量
 * @param decDataLen [OUT] 解密后数据长度
 * @return 0成功，非0失败
 */
int32_t SpekeDecryptDataTo(SpekeSession *session, const uint8_t *data, uint32_t dataLen,
    const SpekeDataBuf *out);

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

/**
 * @brief 加密业务数据（直写调用方提供的缓冲，无中转分配）
 *
 * @param session [IN] 会话
 * @param data [IN] 业务数据
 * @param dataLen [IN] 数据长度
 * @param outBuf [OUT] 预分配输出缓冲
 * @param outBufCap [IN] 输出缓冲容量，需不小于 dataLen + SPEKE_ENC_DATA_MIN_LEN
 * @param outDataLen [OUT] 实际写入长度
 * @return 0成功，非0失败
 */
int32_t SpekeEncryptDataInto(SpekeSession *session, const uint8_t *data, uint32_t dataLen,
    const SpekeDataBuf *out);

#ifdef __cplusplus
}
#endif

#endif /* SECURITY_SPEKE_H */
