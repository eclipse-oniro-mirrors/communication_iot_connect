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
#include "base64_codec.h"
#include "utils_assert.h"
#include "iotc_base64.h"
#include "iotc_errcode.h"
#include "iotc_mem.h"

uint8_t *GetBase64CodecData(const uint8_t *in, uint32_t inLen, uint32_t *dataLen,
    Base64CodecType type, uint32_t maxSize)
{
    CHECK_RETURN_LOGW(in != NULL && inLen != 0 && dataLen != NULL, NULL, "param invalid");
    int32_t ret;
    /* 获取编解码后的大小 */
    if (type == BASE64_CODEC_TYPE_DECODE) {
        ret = IotcBase64Decode(in, inLen, NULL, dataLen);
    } else {
        ret = IotcBase64Encode(in, inLen, NULL, dataLen);
    }
    if (ret != IOTC_OK || *dataLen == 0 || *dataLen > maxSize) {
        IOTC_LOGW("calc base64 len error, %d/%d/%u/%u", ret, type, *dataLen, maxSize);
        return NULL;
    }

    uint8_t *data = (uint8_t *)IotcCalloc(*dataLen, sizeof(uint8_t));
    CHECK_RETURN_LOGW(data != NULL, NULL, "calloc error %u", *dataLen);

    if (type == BASE64_CODEC_TYPE_DECODE) {
        ret = IotcBase64Decode(in, inLen, data, dataLen);
    } else {
        ret = IotcBase64Encode(in, inLen, data, dataLen);
    }
    if (ret != IOTC_OK) {
        IOTC_LOGW("base64 codec error, ret=%d, type=%d", ret, type);
        IotcFree(data);
        return NULL;
    }
    return data;
}