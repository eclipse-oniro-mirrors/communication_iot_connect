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
#include "ble_svc_clear_dev_reg_info.h"
#include "utils_assert.h"
#include "utils_json.h"
#include "event_bus_pub.h"
#include "iotc_svc_dev.h"
#include "securec.h"
#include "iotc_errcode.h"
#include "iotc_event.h"

static int32_t ClearRegInfo(char *in)
{
    IotcJson *req = IotcJsonParse((const char *)in);
    CHECK_RETURN_LOGE(req != NULL, IOTC_ADAPTER_JSON_ERR_PARSE, "parse json err");

    char reqDevId[DEVICE_ID_MAX_STR_LEN + 1];
    int32_t ret = UtilsJsonGetString(req, STR_JSON_DEVID, reqDevId, sizeof(reqDevId));
    CHECK_RETURN_LOGE(ret == IOTC_OK, ret, "get req devId err %d", ret);

    DevAuthInfo authInfo = {0};
    bool isAuthInfoExist = false;
    ret = DevSvcProxyGetAuthInfo(&isAuthInfoExist, &authInfo);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get auth info error %d", ret);
        return ret;
    }

    if (!isAuthInfoExist || strcmp(reqDevId, authInfo.devId) != 0) {
        IOTC_LOGW("devid not match");
        (void)memset_s(&authInfo, sizeof(DevAuthInfo), 0, sizeof(DevAuthInfo));
        return IOTC_CORE_BLE_DEVID_NOT_MATCH;
    }

    (void)memset_s(&authInfo, sizeof(DevAuthInfo), 0, sizeof(DevAuthInfo));
    return EventBusPublishAsync(IOTC_CORE_COMM_EVENT_MAIN_RESTORE, NULL, 0, NULL);
}

int32_t PutBleSvcClearDevRegInfo(const BtCmdParam *param, uint8_t **out, uint32_t *outLen)
{
    CHECK_RETURN_LOGW((param != NULL) && (param->request != NULL) && (param->requestLen != 0) &&
        (out != NULL) && (outLen != NULL), IOTC_ERR_PARAM_INVALID, "invalid param");

    *out = NULL;
    *outLen = 0;
    int32_t ret = ClearRegInfo((char*)param->request);
    return UtilsGenErrcodeJsonStr(ret, (char **)out, outLen);
}