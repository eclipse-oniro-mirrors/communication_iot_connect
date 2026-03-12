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
#include <stddef.h>
#include "sle_svc.h"
#include "iotc_svc.h"
#include "iotc_svc_sle.h"
#include "service_proxy.h"
#include "iotc_errcode.h"
#include "sle_svc_ctx.h"
#include "sle_adv.h"
#include "securec.h"
#include "sle_ssap_data_svc.h"
#include "sle_profile.h"
#include "sle_session_mngr.h"
#include "utils_common.h"
#include "sle_svc_report.h"
#include "utils_assert.h"
#include "sle_ssap_mgt.h"
#include "iotc_sle.h"
#include "sle_adv_ctrl.h"
#include "ble_linklayer.h"
#include "sle_common.h"
#include "sle_svc_netcfg_status.h"

static const char *SLE_SERVICE_NAME = "SLE";

static void SleStackDeinit(void)
{
    SleSsapDisconnectAll();
    int32_t ret = IotcSleDeInitStack();
    if (ret != IOTC_OK) {
        IOTC_LOGW("stack deinit error %d", ret);
    }
    SleSsapMgtDestroy();
}

static int32_t SleStackInit(void)
{
    int32_t ret = SetSleConnectParam();
    if (ret != IOTC_OK) {
        IOTC_LOGE("set connect param error %d", ret);
        return ret;
    }
    ret = IotcSleInitStack();
    if (ret != IOTC_OK) {
        IOTC_LOGE("init stack error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static void SleSsapDeinit(void)
{
    SleSsapMgtDestroy();
}

static int32_t SleSsapInit(void)
{
    int32_t ret = SleSsapDataSvcInit();
    if (ret != IOTC_OK) {
        IOTC_LOGE("sle ssap svc init err ret=%d", ret);
        return ret;
    }
    ret = SleSsapMgtInit();
    if (ret != IOTC_OK) {
        IOTC_LOGE("start ssap service err ret=%d", ret);
        return ret;
    }
    return IOTC_OK;
}

static void SleServiceStopInner(void)
{
    SleSsapDeinit();
    SleSvcNetCfgDeinit();
    SleSvcReportDeinit();
    SleSessReset();
    SleProfileDeinit();
    SleStackDeinit();
    LinkLayerClearAllCachePkg();
    SleSvcCtx *ctx = GetSleSvcCtx();
    if (ctx->onFinish != NULL) {
        ctx->onFinish(ctx->instanceId);
    }
}

static int32_t SleAdvSvcInit(SleSvcCtx *ctx)
{
    int32_t ret = SleAdvInit();
    if (ret != IOTC_OK) {
        IOTC_LOGW("adv init error %d", ret);
        return ret;
    }

    SleSetAdvType(ctx->initParam.advType);

    if (ctx->initParam.onCustomAdv != NULL) {
        ret = RegCustomAdvDataCb(ctx->initParam.onCustomAdv);
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

static int32_t SleServiceInit(SleSvcCtx *ctx)
{
    int32_t ret = SleStackInit();
    if (ret != IOTC_OK) {
        IOTC_LOGW("sle stack init error %d", ret);
        return ret;
    }

    ret = SleScheduleEventInit();
    if (ret != IOTC_OK) {
        IOTC_LOGW("sle event init error %d", ret);
        return ret;
    }

    ret = SleSsapInit();
    if (ret != IOTC_OK) {
        IOTC_LOGW("ssap init error %d", ret);
        return ret;
    }

    ret = SleAdvSvcInit(ctx);
    if (ret != IOTC_OK) {
        IOTC_LOGW("adv init error %d", ret);
        return ret;
    }

    ret = SleProfileInit();
    if (ret != IOTC_OK) {
        IOTC_LOGW("profile init error %d", ret);
        return ret;
    }

    ret = SleSessInit();
    if (ret != IOTC_OK) {
        IOTC_LOGW("profile init error %d", ret);
        return ret;
    }

    ret = SleSvcReportInit();
    if (ret != IOTC_OK) {
        IOTC_LOGW("report init error %d", ret);
        return ret;
    }

    ret = SleSvcNetCfgInit();
    if (ret != IOTC_OK) {
        IOTC_LOGW("report status init error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static int32_t SleServiceStart(int32_t instanceId, ServiceFinishCallback onFinish, void *param)
{
    CHECK_RETURN_LOGW(onFinish != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    int32_t ret = SleSvcCtxInit();
    if (ret != IOTC_OK) {
        IOTC_LOGW("ctx init error %d", ret);
        return ret;
    }

    SleSvcCtx *ctx = GetSleSvcCtx();
    ctx->instanceId = instanceId;
    ctx->onFinish = onFinish;

    const SleSvcInitParam *initParam = param;
    ret = memcpy_s(&ctx->initParam, sizeof(SleSvcInitParam), initParam, sizeof(SleSvcInitParam));
    if (ret != EOK) {
        return IOTC_ERR_SECUREC_MEMCPY;
    }

    ret = SleServiceInit(ctx);
    if (ret != IOTC_OK) {
        SleServiceStopInner();
    }

    return ret;
}

static int32_t SleServiceStop(int32_t instanceId, void *param)
{
    NOT_USED(param);
    SleSvcCtx *ctx = GetSleSvcCtx();
    if (ctx->instanceId != instanceId) {
        IOTC_LOGW("invalid instance id %d/%d", instanceId, ctx->instanceId);
        return IOTC_ERR_PARAM_INVALID;
    }
    SleServiceStopInner();
    (void)SleSvcCtxInit();
    return IOTC_OK;
}

static int32_t SendCustomSecData(const uint8_t *data, uint32_t len)
{
    CHECK_RETURN_LOGW(data != NULL || len != 0, IOTC_ERR_PARAM_INVALID, "param invalid");
    return LinkLayerReportSvcDataEnc(SLE_SVC_CUSTOM_SEC_DATA, data, len);
}

int32_t SleConnectServiceInit(void)
{
    static const ServiceHandler SLE_SERVICE_HANDLER = {
        .onStart = SleServiceStart,
        .onStop = SleServiceStop,
        .onReset = NULL,
    };

    int32_t msgs[SLE_SERVICE_MSG_ID_MAX] = {0};
    for (uint32_t i = 0 ; i < SLE_SERVICE_MSG_ID_MAX; ++i) {
        msgs[i] = i;
    }

    static const SleSvcApi SLE_SERVICE_API = {
        .onStartAdv = SleAdvCtrlStart,
        .onStopAdv = SleAdvCtrlStop,
        .onSetAdvType = SleSetAdvType,
        .onSendCustomSecData = SendCustomSecData,
        .onSendIndicateData = IotcSleSendIndicateData,
    };

    ServiceInstance instance = {
        .serviceId = IOTC_SERVICE_ID_SLE,
        .name = SLE_SERVICE_NAME,
        .handler = &SLE_SERVICE_HANDLER,
        .msgNum = SLE_SERVICE_MSG_ID_MAX,
        .msgIds = msgs,
        .apiHandler = &SLE_SERVICE_API,
    };

    int32_t ret = ServiceManagerRegisterInstance(&instance);
    if (ret != IOTC_OK) {
        IOTC_LOGE("reg device service instance error %d", ret);
        return ret;
    }

    return IOTC_OK;
}

void SleConnectServiceDeinit(void)
{
    SleServiceStopInner();
    ServiceManagerUnregisterInstance(IOTC_SERVICE_ID_SLE);
}