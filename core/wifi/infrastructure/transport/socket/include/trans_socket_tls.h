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
#ifndef TRANS_SOCKET_TLS_H
#define TRANS_SOCKET_TLS_H

#include "trans_socket.h"
#include "iotc_tls.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /* name应为常量字符 */
    const char *name;
    int32_t (*onUpdateRemainLen)(const uint8_t *packet, uint32_t curLen, uint32_t *remain);
    IotcTlsHost host;
    IotcTlsCiphersuites suites;
    /* cert.certs 应为常量字符数组 */
    IotcTlsCerts cert;
    IotcTlsPsk psk;
} SocketTlsInitParam;

TransSocket *TransSocketTlsNew(const SocketTlsInitParam *init);

IotcTlsCertVerify TransSocketTlsVerifyCert(TransSocket *socket);

#ifdef __cplusplus
}
#endif

#endif /* TRANS_SOCKET_TLS_H */