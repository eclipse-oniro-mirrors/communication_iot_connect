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
#ifndef NETCFG_SOFTAP_METHOD_H
#define NETCFG_SOFTAP_METHOD_H

#include "coap_endpoint_server.h"
#include "trans_sess.h"

#ifdef __cplusplus
extern "C" {
#endif

void SoftapCoapSpekeReqHandler(CoapEndpoint *endpoint, const CoapPacket *req, const SocketAddr *addr, void *userData);

void SoftapCoapSetupReqHandler(CoapEndpoint *endpoint, const CoapPacket *req, const SocketAddr *addr, void *userData);

void SoftapCoapE2eCtrlHandler(CoapEndpoint *endpoint, const CoapPacket *req, const SocketAddr *addr, void *userData);

SessCode SoftapCoapMsgRecvPreProcess(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info);

SessCode SoftapCoapMsgRecvBase64DecodeProcess(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info);

SessCode SoftapCoapMsgRecvDecryptProcess(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info);

SessCode SoftapCoapMsgSendEncryptProcess(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info);

SessCode SoftapCoapMsgSendBase64EncodeProcess(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info);

SessCode SoftapCoapMsgSendFinalProcess(SessMsg *msg, UtilsBuffer *buf, SessAddtlInfo *info);

#ifdef __cplusplus
}
#endif

#endif /* NETCFG_SOFTAP_METHOD_H */