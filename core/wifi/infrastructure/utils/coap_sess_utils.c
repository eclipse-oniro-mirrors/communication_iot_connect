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
#include <string.h>
#include "coap_sess_utils.h"
#include "coap_codec_utils.h"
#include "utils_assert.h"
#include "iotc_errcode.h"

/* uriWhiteList以NULL作为结束符 */
bool CoapUriWhiteListMatch(const CoapPacket *packet, const char *uriWhiteList[])
{
    CHECK_RETURN_LOGW(packet != NULL && uriWhiteList != NULL && *uriWhiteList != NULL,
        false, "param invalid");
    if (COAP_CODE_CLASS(packet->header.code) != COAP_CODE_CLASS_REQ) {
        return false;
    }

    char uriBuf[COAP_URI_MAX_LEN + 1] = {0};
    int32_t ret = CoapUtilsGetUri(packet, uriBuf, COAP_URI_MAX_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get uri error %d", ret);
        return false;
    }

    for (const char **whiteUri = uriWhiteList; *whiteUri != NULL; ++whiteUri) {
        if (strcmp(*whiteUri, uriBuf) != 0) {
            continue;
        }
        return true;
    }
    return false;
}