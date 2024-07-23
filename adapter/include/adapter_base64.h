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
#ifndef ADAPTER_BASE64_H
#define ADAPTER_BASE64_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief base64编码
 *
 * @param inData [IN] 待编码的数据，有效可读长度至少为inLen
 * @param inLen [IN] 待编码的数据长度
 * @param outData [OUT] 编码后输出数据缓冲区，有效可写长度至少为*outLen
 * @param outLen [IN,OUT] 输入为缓冲区长度，输出为编码后的数据长度
 * @return 0成功，非0失败
 * @attention outData为NULL或者outLen为0时，不会实际输出编码数据，只输出编码数据后的长度
 */
int32_t AdapterBase64Encode(const uint8_t *inData, uint32_t inLen, uint8_t *outData, uint32_t *outLen);

/**
 * @brief base64解码
 *
 * @param inData [IN] 待解码的数据，有效可读长度至少为inLen
 * @param inLen [IN] 待解码的数据长度
 * @param outData [OUT] 解码后输出数据缓冲区，有效可写长度至少为*outLen
 * @param outLen [IN,OUT] 输入为缓冲区长度，输出为解码后的数据长度
 * @return 0成功，非0失败
 * @attention outData为NULL或者outLen为0时，不会实际输出解码数据，只输出解码数据后的长度
 */
int AdapterBase64Decode(const uint8_t *inData, uint32_t inLen, uint8_t *outData, uint32_t *outLen);

#ifdef __cplusplus
}
#endif

#endif /* ADAPTER_BASE64_H */