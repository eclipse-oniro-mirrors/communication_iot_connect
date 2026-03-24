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
#include "sle_print_data.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iotc_log.h"
#include "securec.h"
#include "comm_def.h"
#include "utils_assert.h"
#include "utils_common.h"
#include "iotc_conf.h"
#include "iotc_errcode.h"
#define HEX_BYTE_STR_LEN 3

static void IotcSlePrintData(const uint16_t valueLen, const uint8_t *value)
{
    size_t bufLen = (size_t)valueLen * HEX_BYTE_STR_LEN + 1;
    char *hexStr = malloc(bufLen);
    if (!hexStr) {
        IOTC_LOGI("IotcSlePrintData malloc failed");
        return;
    }
    char *p = hexStr;
    size_t remaining = bufLen;

    for (uint32_t i = 0; i < valueLen; i++) {
        int n = snprintf_s(p, remaining, remaining - 1, "%02X ", value[i]);
        if (n < 0 || (size_t)n >= remaining) {
            break;
        }
        p += n;
        remaining -= n;
    }
    IOTC_LOGI("IotcSlePrintData First %u bytes: %s", valueLen, hexStr);
    free(hexStr);
}

#define SLE_PRINT_CHUNK_SIZE 50

void SlePrintfData(const uint8_t *data, uint16_t totalLen)
{
    const uint16_t chunkSize = SLE_PRINT_CHUNK_SIZE;
    uint16_t processed = 0;

    while (processed < totalLen) {
        uint16_t remaining = totalLen - processed;
        uint16_t chunkLen = (remaining < chunkSize) ? remaining : chunkSize;
        IotcSlePrintData(chunkLen, data + processed);

        processed += chunkLen;
    }
}

int32_t SleJsonGetString(const IotcJson *json, const char *key, const char **outStr)
{
    CHECK_RETURN(json != NULL &&
        key != NULL &&
        outStr != NULL,
        IOTC_ERR_PARAM_INVALID);

    const char *src = IotcJsonGetStr(IotcJsonGetObj(json, key));
    if (src == NULL) {
        IOTC_LOGW("get json str error %s", key);
        return IOTC_ADAPTER_JSON_ERR_GET_STRING;
    }

    if (*outStr != NULL && strcmp(*outStr, src) == 0) {
        IOTC_LOGD("string content is the same, no need to reallocate");
        return IOTC_OK;
    }

    if (*outStr != NULL) {
        IotcFree((char *)*outStr);
        IOTC_LOGW("free old string %s", key);
    }

    char *dup = strdup(src);
    if (dup == NULL) {
        IOTC_LOGE("strdup(%s) failed", key);
        *outStr = NULL;
        return IOTC_ERROR;
    }

    *outStr = dup;
    return IOTC_OK;
}

int32_t SleJsonGetNum(const IotcJson *json, const char *key, int64_t *outNum)
{
    CHECK_RETURN(json != NULL && key != NULL && outNum != NULL, IOTC_ERR_PARAM_INVALID);

    IotcJson *seqObj = IotcJsonGetObj(json, key);
    if (seqObj == NULL) {
        return IOTC_ERROR;
    }

    int32_t ret = IotcJsonGetNum(seqObj, outNum);
    if (ret != IOTC_OK) {
        return ret;
    }

    return IOTC_OK;
}