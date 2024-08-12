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
#include "m2m_cloud_svc.h"
#include "iotc_svc.h"
#include "service_manager.h"
#include "service_proxy.h"
#include "iotc_errcode.h"
#include "m2m_cloud_ctx.h"
#include "m2m_cloud_fsm.h"
#include "m2m_cloud_report.h"
#include "utils_assert.h"
#include "m2m_cloud_link.h"

static const char *M2M_CLOUD_SVC_NAME = "CLOUD";

static void M2mCloudServiceStopInner(M2mCloudContext *ctx)
{
    M2mCloudFsmDeinit(ctx);
    M2mCloudLinkDeinit(ctx);
}

static int32_t M2mCloudDeviceReportMessageHandler(const ServiceMessage *req, ServiceResponseInfo *resp)
{
    NOT_USED(resp);
    CHECK_RETURN_LOGW(req != NULL && req->msg != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    int32_t ret = M2mCloudReportMessage(req->msg, GetM2mCloudCtx());
    if (ret != IOTC_OK) {
        IOTC_LOGW("local ctl report error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static int32_t M2mCloudServiceMsgSub(M2mCloudContext *ctx)
{
    int32_t ret = ServiceProxySubscribeMessage(ctx->svcInfo.instanceId, IOTC_SERVICE_ID_DEVICE,
        DEVICE_SERVICE_MSG_ID_REPORT, M2mCloudDeviceReportMessageHandler);
    if (ret != IOTC_OK) {
        IOTC_LOGW("sub device service report error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static int32_t M2mCloudServiceStart(int32_t instanceId, ServiceFinishCallback onFinish, void *param)
{
    NOT_USED(param);
    M2mCloudContext *ctx = GetM2mCloudCtx();

    ctx->svcInfo.instanceId = instanceId;
    ctx->svcInfo.onFinish = onFinish;

    int32_t ret = M2mCloudFsmInit(ctx);
    if (ret != IOTC_OK) {
        IOTC_LOGW("cloud fsm init error %d", ret);
        return ret;
    }

    ret = M2mCloudServiceMsgSub(ctx);
    if (ret != IOTC_OK) {
        IOTC_LOGW("cloud  Service Msg Sub error %d", ret);
    }
    return ret;
}

static int32_t M2mCloudServiceStop(int32_t instanceId, void *param)
{
    NOT_USED(param);
    M2mCloudContext *ctx = GetM2mCloudCtx();
    if (ctx->svcInfo.instanceId != instanceId) {
        IOTC_LOGW("invalid instance id %d/%d", instanceId, ctx->svcInfo.instanceId);
        return IOTC_ERR_PARAM_INVALID;
    }

    if (ctx->svcInfo.onFinish != NULL) {
        ctx->svcInfo.onFinish(instanceId);
    }

    M2mCloudServiceStopInner(ctx);
    return IOTC_OK;
}

int32_t M2mCloudServiceInit(void)
{
    int32_t ret = M2mCloudCtxInit();
    if (ret != IOTC_OK) {
        IOTC_LOGW("m2m cloud ctx init error %d", ret);
        return ret;
    }

    static const ServiceHandler M2M_CLOUD_SERVICE_HANDLER = {
        .onStart = M2mCloudServiceStart,
        .onStop = M2mCloudServiceStop,
        .onReset = NULL,
    };

    ServiceInstance instance = {
        .serviceId = IOTC_SERVICE_ID_M2M_CLOUD,
        .name = M2M_CLOUD_SVC_NAME,
        .handler = &M2M_CLOUD_SERVICE_HANDLER,
        .msgNum = 0,
        .msgIds = NULL,
        .apiHandler = NULL,
    };

    ret = ServiceManagerRegisterInstance(&instance);
    if (ret != IOTC_OK) {
        IOTC_LOGW("reg cloud svc instance error %d", ret);
        return ret;
    }

    return IOTC_OK;
}

void M2mCloudServiceDeinit(void)
{
    M2mCloudServiceStopInner(GetM2mCloudCtx());
    (void)M2mCloudCtxInit();
    ServiceManagerUnregisterInstance(IOTC_SERVICE_ID_M2M_CLOUD);
}