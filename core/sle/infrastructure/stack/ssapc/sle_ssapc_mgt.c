/*
 * Copyright (c) 2024-2024 Shenzhen Kaihong Device Co., Ltd.
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

#include "sle_ssap_mgt.h"
#include "iotc_errcode.h"
#include "securec.h"
#include "utils_assert.h"
#include "iotc_log.h"
#include "iotc_mem.h"
#include "iotc_sle_client.h"
#define MAX_HEX_DISPLAY_LEN 8
#define BYTES_PER_LINE 3
int32_t SleSendIndicateDataInner(
    const char *svcUuid, const char *charUuid, uint16_t connId, const uint8_t *value, uint32_t valueLen)
{
    CHECK_RETURN_LOGW((svcUuid != NULL) && (charUuid != NULL) && (value != NULL) && (valueLen != 0),
        IOTC_ERR_PARAM_INVALID, "invalid param");
    if ((GetSleSsapMgtApp()->connNum == 0) || (GetSleSsapMgtApp()->peerDevInfo == NULL)) {
        IOTC_LOGE("no connect");
        return IOTC_CORE_SLE_NO_CONNECT;
    }

    char hexStr[64] = {0};
    int len = (valueLen > MAX_HEX_DISPLAY_LEN) ? MAX_HEX_DISPLAY_LEN : valueLen;
    for (int i = 0; i < len; i++) {
        if (snprintf_s(hexStr + i * BYTES_PER_LINE, sizeof(hexStr) - i * BYTES_PER_LINE,
                       sizeof(hexStr) - i * BYTES_PER_LINE - 1, "%02X ", value[i]) < 0) {
            hexStr[0] = '\0';
            IOTC_LOGE("snprintf_s fail");
        }
    }

    IotcAdptSleSendIndicateParam param;
    (void)memset_s(&param, sizeof(param), 0, sizeof(param));

    SlePeerDevInfo *devInfo = GetSleSsapMgtPeerDevInfo(connId);
    if (devInfo == NULL) {
        IOTC_LOGE("no find peer dev info");
        return IOTC_CORE_SLE_INVALID_CONNID;
    }
    if (devInfo->connState != IOTC_SLE_SSAP_CONNECT_STATE_CONNECTED) {
        IOTC_LOGE("IOTC_CORE_SLE_CONNECT_STATE_ERRO,R devInfo->connState%d:", devInfo->connState);
        return IOTC_CORE_SLE_CONNECT_STATE_ERROR;
    }
    param.handle = (devInfo->handler.startHdl);
    param.type = devInfo->type;
    param.value = (uint8_t *)value;
    param.valueLen = valueLen;

    int32_t ret = IotcSleSendSsapsIndicate(devInfo->serverId, connId, &param);
    if (ret != IOTC_OK) {
        IOTC_LOGE("send indicate msg err ret=%d", ret);
    }
    return ret;
}

void SleSsapDisconnectAll(void)
{
    ListEntry *item;
    SleSsapMgtApp *app = GetSleSsapMgtApp();
    if (app == NULL) {
        return;
    }
    if (app->peerDevInfo == NULL) {
        IOTC_LOGE("invalid param");
        return;
    }
    LIST_FOR_EACH_ITEM(item, &(app->peerDevInfo->node))
    {
        SlePeerDevInfo *devInfo = CONTAINER_OF(item, SlePeerDevInfo, node);
        if (devInfo != NULL) {
            int32_t ret = IotcSleDisconnectSsap(app->peerDevInfo->devAddr.addr, IOTC_ADPT_SLE_ADDR_LEN);
            if (ret != IOTC_OK) {
                continue;
            }
        }
        IotcFree(devInfo);
    }
}
