/*
 * Copyright (c) 2024-2024 ShenZhen Kaihong Device Co., Ltd.
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
 
#include "sle_server_event_service.h"
#include "sle_ssap_service.h"
#include "iotc_errcode.h"
#include "sle_svc_ctx.h"
#include "sle_adv.h"
#include "sle_adv_ctrl.h"
#include "sle_common.h"
#include "utils_common.h"
#include "iotc_log.h"
#include "event_bus.h"
#include "iotc_event.h"
#include "sle_profile.h"
#include "sle_linklayer.h"

static uint16_t g_connectId = 0;
static void SleConnectStateCallback(uint32_t event, void *param, uint32_t len)
{
    if (param == NULL) {
        IOTC_LOGE("SleConnectStateCallback  invalid param");
        return;
    }
    IotcAdptSleConnectionEventParam *eventParam =  (IotcAdptSleConnectionEventParam *)param;
    g_connectId = eventParam->sleConnectStateChanged.conn_id;
    IOTC_LOGE("SleConnectStateCallback con_id:%d", g_connectId);
}

int32_t SleSsapServiceSvcInit(SleSvcCtx *ctx)
{
    int32_t ret = SleAdvInit();
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

    ret = EventBusSubscribe(SleConnectStateCallback, IOTC_CORE_SLE_EVENT_CONNECT_STATE_CHANGED);
    IOTC_LOGI("svc subscribe sle connect state change result:%d", ret);
    return IOTC_OK;
}
int32_t SleScanServiceStart(void)
{
    return IOTC_OK;
}
int32_t SleSendCustomSecDataService(const char *devId, uint8_t protType, const uint8_t *data, uint32_t len)
{
    if (data == NULL) {
        IOTC_LOGE("SleSendCustomSecDataService Invalid input parameters (data=%p)", devId, data);
        return IOTC_ERR_PARAM_INVALID;
    }

    //HI3863_SDK_CONFIG_PATH
    return SleLinkLayerReportSvcDataEnc(g_connectId, SLE_SVC_CUSTOM_SEC_DATA, data, len, SLE_OPTYPE_PUT);
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

int32_t SleScanServiceStop(void)
{
    // 停止扫描
    return IOTC_OK;
}