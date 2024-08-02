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
#ifndef SECURITY_SPEKE_DEFS_H
#define SECURITY_SPEKE_DEFS_H

#ifdef __cplusplus
extern "C" {
#endif

#define SPEKE_SESSION_ID_JSON "sessionId"
#define SPEKE_SEC_DATA_JSON "securityData"
#define SPEKE_SEC_DATA_MESSAGE_JSON "message"
#define SPEKE_SEC_DATA_PAYLOAD_JSON "payload"

#define SPEKE_SEC_DATA_VER_JSON "version"
#define SPEKE_SEC_DATA_CUR_VER_JSON "currentVersion"
#define SPEKE_SEC_DATA_MIN_VER_JSON "minVersion"
#define SPEKE_SEC_DATA_OPCODE_JSON "operationCode"
#define SPEKE_SEC_DATA_256MODE_JSON "support256mod"
#define SPEKE_SEC_DATA_CHALLENGE_JSON "challenge"
#define SPEKE_SEC_DATA_SALT_JSON "salt"
#define SPEKE_SEC_DATA_EPK_JSON "epk"
#define SPEKE_SEC_DATA_KCF_JSON "kcfData"
#define SPEKE_SEC_DATA_ERR_JSON "errorCode"

#define SPEKE_SEC_DATA_MSG_TYPE_CLIENT_REQ 0x0001
#define SPEKE_SEC_DATA_MSG_TYPE_SERVER_RSP 0x8001
#define SPEKE_SEC_DATA_MSG_TYPE_CLIENT_CFM 0x0002
#define SPEKE_SEC_DATA_MSG_TYPE_SERVER_CFM 0x8002
#define SPEKE_SEC_DATA_MSG_TYPE_INFORM_MSG 0x8080

#define SPEKE_VERSION "1.0.0"
#define SPEKE_OPCODE 6

#ifdef __cplusplus
}
#endif

#endif /* SECURITY_SPEKE_DEFS_H */