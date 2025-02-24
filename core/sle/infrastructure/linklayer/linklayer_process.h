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
#ifndef SLE_LINKLAYER_PROCESS_H
#define SLE_LINKLAYER_PROCESS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PKG_HEAD_LEN    7

enum {
    PKG_HEAD_TYPE_IDX = 0,
    PKG_HEAD_TOKEN_IDX,
    PKG_HEAD_PKGNUM_IDX,
    PKG_HEAD_INDEX_IDX,
    PKG_HEAD_RESERVED_IDX,
    PKG_HEAD_ENCRYPT_TYPE_IDX,
    PKG_HEAD_RET_IDX,
};

int32_t LinkLayerReportEncryptCmdData(const uint8_t *buff, uint32_t len);

int32_t LinkLayerReportCmdData(const uint8_t *buff, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* SLE_LINKLAYER_PROCESS_H */