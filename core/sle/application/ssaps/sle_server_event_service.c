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
#include "sle_ssap_service.h"
#include "iotc_errcode.h"
#include "sle_svc_ctx.h"
#include "sle_adv.h"
#include "sle_adv_ctrl.h"
#include "sle_common.h"
#include "utils_common.h"

int32_t SleSsapServiceSvcInit(SleSvcCtx *ctx)
{
    int32_t ret = SleConnectionEventInit();
    if (ret != IOTC_OK) {
        IOTC_LOGE("sle conn init err ret=%d", ret);
        return ret;
    }

    ret = SleAdvInit();
    if (ret != IOTC_OK) {
        IOTC_LOGW("adv init error %d", ret);
        return ret;
    }

    SleSetAdvType(ctx->initParam.advType);

    if (ctx->initParam.onCustomAdv != NULL) {
        ret = RegSleCustomAdvDataCb(ctx->initParam.onCustomAdv);
        if (ret != IOTC_OK) {
            IOTC_LOGW("reg custom adv cb error %d", ret);
            return ret;
        }
    }

    ret = SleAdvCtrlStart(GetSleStartUpAdvTimeout());
    if (ret != IOTC_OK) {
        IOTC_LOGW("start sle adv error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

int32_t SleScanServiceStart(void)
{
    return IOTC_OK;
}

int32_t SleSendCustomSecDataService(const char *devId, uint8_t protType, const uint8_t *data, uint32_t len)
{
    NOT_USED(devId);
    NOT_USED(protType);
    NOT_USED(data);
    NOT_USED(len);
    return IOTC_OK;
}

int32_t SleAdvServiceStart(uint32_t ms)
{
    int32_t ret = SleAdvCtrlStart(ms);
    if (ret != IOTC_OK) {
        IOTC_LOGW("start sle adv error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

int32_t SleAdvServiceStop(void)
{
    int32_t ret = SleAdvCtrlStop();
    if (ret != IOTC_OK) {
        IOTC_LOGW("stop sle adv error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

void SleAdvSetType(SleSvcAdvDataType type)
{
    SleSetAdvType(type);
}

int32_t IotcOhSleFindDeviceInfoService(const char *devId, void **info)
{
    NOT_USED(devId);
    NOT_USED(info);
    return IOTC_OK;
}
