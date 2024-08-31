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
#ifndef BASE64_CODEC_H
#define BASE64_CODEC_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BASE64_CODEC_TYPE_DECODE = 0,
    BASE64_CODEC_TYPE_ENCODE,
} Base64CodecType;

uint8_t *GetBase64CodecData(const uint8_t *in, uint32_t inLen, uint32_t *dataLen,
    Base64CodecType type, uint32_t maxSize);

#ifdef __cplusplus
}
#endif

#endif /* BASE64_CODEC_H */