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
#include "sle_ssap_data_svc.h"
#include "sle_ssap_mgt.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "sle_linklayer.h"
#include "utils_assert.h"
#include "utils_common.h"
#include "sched_executor.h"

#define DATA_SVC_UUID "15F1E600A27743FCA484DD39EF8A9100"
#define DATA_SVC_READ_UUID "15F1E601A27743FCA484DD39EF8A9100"
#define DATA_SVC_WRITE_UUID "15F1E602A27743FCA484DD39EF8A9100"

static int32_t SleDataCharWrite(uint32_t connId, uint8_t *buff, uint32_t len);
static int32_t SleDataCharRead(uint32_t connId, uint8_t *buff, uint32_t *len);

static IotcSleSsapProfileChar g_dataChar[] = {
    {
        .uuid = DATA_SVC_WRITE_UUID,
        .permission = IOTC_SLE_SSAP_PERMISSION_READ_ENCRYPTED | IOTC_SLE_SSAP_PERMISSION_WRITE_ENCRYPTED,
        .property = IOTC_SLE_SSAP_CHARACTER_PROPERTY_BIT_WRITE,
        .readFunc = NULL,
        .writeFunc = SleDataCharWrite,
        .indicateFunc = NULL,
        .desc = NULL,
        .descNum = 0,
        .serviceId = 0,
    },
    {
        .uuid = DATA_SVC_READ_UUID,
        .permission = IOTC_SLE_SSAP_PERMISSION_READ_ENCRYPTED | IOTC_SLE_SSAP_PERMISSION_WRITE_ENCRYPTED,
        .property = IOTC_SLE_SSAP_CHARACTER_PROPERTY_BIT_READ | IOTC_SLE_SSAP_CHARACTER_PROPERTY_BIT_INDICATE,
        .readFunc = SleDataCharRead,
        .writeFunc = NULL,
        .indicateFunc = NULL,
        .desc = NULL,
        .descNum = 0,
        .serviceId = 0,
    }
};

static const IotcSleSsapProfileSvc g_sleDataSvc = {
    .uuid = DATA_SVC_UUID,
    .character = g_dataChar,
    .charNum = (sizeof(g_dataChar) / sizeof(IotcSleSsapProfileChar)),
};

typedef struct {
    uint32_t connId;
    uint8_t *value;
    uint32_t valueLen;
} LinkLayerExecutorParam;

static int32_t SleSendIndicate(uint32_t connId, const uint8_t *buff, uint32_t len)
{
    return SleSendIndicateDataInner(DATA_SVC_UUID, DATA_SVC_READ_UUID, connId, buff, len);
}

static int32_t LinkLayerExecutorWaitCallback(void *inData, void **outData)
{
    NOT_USED(outData);
    CHECK_RETURN_LOGE(inData != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    const LinkLayerExecutorParam *param = (const LinkLayerExecutorParam *)inData;

    int32_t ret = SleLinkLayerProcessData(param->connId,param->value, param->valueLen);
    IOTC_LOGI("process sle data, len=%u,ret=%d", param->valueLen, ret);
    return ret;
}

static int32_t SleDataCharWrite(uint32_t connId, uint8_t *buff, uint32_t len)
{
    CHECK_RETURN_LOGE((buff != NULL) && (len > 0), IOTC_ERR_PARAM_INVALID, "invalid param");
    int32_t errcode;
    LinkLayerExecutorParam param = {.connId = connId, .value = buff, .valueLen = len};
    int32_t ret = SchedAsyncExecutorWait(LinkLayerExecutorWaitCallback, &param, NULL, &errcode, UTILS_SEC_TO_MS(1));
    if (ret != IOTC_OK) {
        IOTC_LOGE("async executor error %d", ret);
        return ret;
    }
    IOTC_LOGI("sched async finish %d", errcode);
    return errcode;
}

static int32_t SleDataCharRead(uint32_t connId, uint8_t *buff, uint32_t *len)
{
    CHECK_RETURN_LOGE((buff != NULL) && (len != NULL), IOTC_ERR_PARAM_INVALID, "invalid param");

    *len = 0;
    IOTC_LOGI("start read bt data");
    return IOTC_OK;
}

int32_t SleSsapDataSvcInit(void)
{
    int32_t ret = SleAddSsapSvc(&g_sleDataSvc);
    if (ret != IOTC_OK) {
        IOTC_LOGE("add sle ssap svc err");
        return ret;
    }
    ret = SleLinkLayerRegisterDataSendCb(SleSendIndicate);
    if (ret != IOTC_OK) {
        IOTC_LOGE("reg bt data send cb err");
        return ret;
    }
    return ret;
}