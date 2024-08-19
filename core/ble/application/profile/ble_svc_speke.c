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
#include "ble_svc_speke.h"
#include "ble_speke_session.h"
#include "utils_common.h"
#include "utils_assert.h"
#include "security_speke.h"
#include "utils_json.h"
#include "iotc_errcode.h"

int32_t PutBleSvcSpeke(const BtCmdParam *param, uint8_t **out, uint32_t *outLen)
{
    CHECK_RETURN_LOGW((param != NULL) && (param->request != NULL) && (param->requestLen != 0) &&
        (out != NULL) && (outLen != NULL), IOTC_ERR_PARAM_INVALID, "invalid param");

    *out = NULL;
    *outLen = 0;
    int32_t ret = CreateBleSpekeSess();
    if (ret != IOTC_OK) {
        IOTC_LOGE("create ble speke session err ret=%d", ret);
        return ret;
    }

    ret = SpekeProcessPacket(GetBleSpekeSess(), (const char *)param->request, out, outLen);
    if (ret != IOTC_OK) {
        IOTC_LOGW("process speke packet %d", ret);
        if (*out != NULL) {
            IotcFree(*out);
            *out = NULL;
            *outLen = 0;
        }
        int32_t spekeErr = GetBleSpekeErrCode();
        return UtilsGenErrcodeJsonStr(spekeErr != IOTC_OK ? spekeErr : ret, (char **)out, outLen);
    }

    return IOTC_OK;
}