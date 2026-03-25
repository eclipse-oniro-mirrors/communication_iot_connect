/*
 * Copyright (c) 2024-2024 Shenzhe Kaihong Device Co., Ltd.
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

#include "m2m_cloud_dev_mgr.h"
#include "utils_assert.h"
#include "iotc_event.h"
#include "iotc_errcode.h"
#include "iotc_log.h"
#include "utils_list.h"
#include "securec.h"
#include "iotc_mem.h"
#include "m2m_cloud_ctx.h"
#include "m2m_cloud_send.h"
#include "m2m_cloud_errcode.h"
#include "event_bus_sub.h"
#include "event_bus_pub.h"
#include "utils_mutex_global.h"
#include "utils_fsm.h"
#include "utils_bit_map.h"

#define MGR_CHANGE_FSM_TO(ctx, state) UtilsFsmSwitch((ctx)->stateManager.fsmCtx, (state));

typedef enum{
    M2M_CLOUD_DEV_STATE_OFFLINE = 0,
    M2M_CLOUD_DEV_STATE_ONLINE = 1,
    M2M_CLOUD_DEV_STATE_PENDING = 2,
} M2MCloudDevState;

typedef struct {
    M2mEndDeviceInfo info;
    M2MCloudDevState state;
    ListEntry list_node;       // 单链表节点（共用）
} CloudDevInfo;

typedef struct {
    ListEntry pending_list;    // 待上报链表头
    ListEntry online_list;     // 已上线链表头
    uint32_t pending_count;
    uint32_t online_count;
} CloudDeviceListManager;        // 统一管理器

static CloudDeviceListManager g_manager = {
    .pending_list = LIST_DECLARE_INIT(&g_manager.pending_list),
    .online_list = LIST_DECLARE_INIT(&g_manager.online_list),
    .pending_count = 0,
    .online_count = 0
};

typedef struct
{
    char devId[DEVICE_ID_MAX_STR_LEN + 1];
    char authCodeId[BLE_AUTHCODE_ID_LEN + 1];
    char authCode[BLE_AUTHCODE_LEN];
    int32_t timeout;
}CloudIssueAuthCode;

static CloudDevInfo *GetCloudDeviceInfo(const char *devId, ListEntry *list)
{
    if (devId == NULL || devId[0] == '\0' || list == NULL) {
        return false;
    }

    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, list) {
        CloudDevInfo *devInfo = CONTAINER_OF(item, CloudDevInfo, list_node);
        if (strcmp(devId, devInfo->info.deviceId) == 0) {
            return devInfo;
        }
    }
    return NULL;
}

static CloudDevInfo *GetCloudDeviceInfoOnOne(void)
{
    if(LIST_EMPTY(&g_manager.pending_list))
    {
        IOTC_LOGW("pending list is empty");
        return NULL;
    }

    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &g_manager.pending_list) {
        CloudDevInfo *devInfo = CONTAINER_OF(item, CloudDevInfo, list_node);
        if(devInfo->state == M2M_CLOUD_DEV_STATE_PENDING)
        {
            return devInfo;
        }
    }
    return NULL;

}


static bool IsCloudDeviceInfoExist(const char *devId, ListEntry *list)
{
    if (devId == NULL || devId[0] == '\0' || list == NULL) {
        return false;
    }

    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, list) {
        CloudDevInfo *devInfo = CONTAINER_OF(item, CloudDevInfo, list_node);
        if (strcmp(devId, devInfo->info.deviceId) == 0) {
            return true;
        }
    }
    return false;
}

static int32_t AddCloudDeviceInfo(M2mEndDeviceInfo *devInfo, ListEntry *list)
{
    CHECK_RETURN_LOGW(devInfo != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");
    CHECK_RETURN_LOGW(list != NULL, IOTC_ERR_PARAM_INVALID, "invalid param");

    if (IsCloudDeviceInfoExist((const char*)devInfo->deviceId, list)) {
        IOTC_LOGE("device already exist");
        return IOTC_ERROR;
    }

    CloudDevInfo *newNode = (CloudDevInfo *)IotcMalloc(sizeof(CloudDevInfo));
    if (newNode == NULL) {
        IOTC_LOGW("malloc error");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }

    (void)memset_s(newNode, sizeof(CloudDevInfo), 0, sizeof(CloudDevInfo));

    newNode->info = *devInfo;

    LIST_INIT(&newNode->list_node);
    (void)UtilsGlobalMutexLock();
    LIST_INSERT_BEFORE(&newNode->list_node, list);
    (void)UtilsGlobalMutexUnlock();
    return IOTC_OK;
}

static int32_t MoveCloudDeviceToOnline(const char *devId)
{
    if(devId == NULL || devId[0] == '\0')
    {
        IOTC_LOGE("devId is null");
        return IOTC_ERR_PARAM_INVALID;
    }

    CloudDevInfo * cdInfo = GetCloudDeviceInfo(devId, &g_manager.pending_list);
    if(cdInfo == NULL)
    {
        IOTC_LOGW("Device not found in pending list");
        return IOTC_ERROR;
    }

    (void)UtilsGlobalMutexLock();
    LIST_REMOVE(&cdInfo->list_node);
    (void)UtilsGlobalMutexUnlock();

    if (IsCloudDeviceInfoExist(devId, &g_manager.online_list)) {
        IOTC_LOGE("Device already exists in online list");
        return IOTC_ERROR;
    }

    (void)UtilsGlobalMutexLock();
    LIST_INSERT_BEFORE(&cdInfo->list_node, &g_manager.online_list);
    (void)UtilsGlobalMutexUnlock();
    return IOTC_OK;

}


IotcJson *M2mCloudBuildDevIdRequest(M2mCloudContext *ctx){

    CHECK_RETURN_LOGW(ctx != NULL, NULL, "param invalid");
    IotcJson *devInfoArr = IotcJsonCreateArray();
    CHECK_RETURN_LOGW(devInfoArr != NULL, NULL, "create json error");

    IotcJson *devinfoObj = NULL;
    int32_t ret;
    do {
        devinfoObj = IotcJsonCreate();
        if (devinfoObj == NULL) {
            IOTC_LOGW("create json error");
            ret = IOTC_ADAPTER_JSON_ERR_CREATE;
            break;
        }

        CloudDevInfo *oneDevId = GetCloudDeviceInfoOnOne();
        if(oneDevId == NULL)
        {
            IOTC_LOGW("no device info found");
            ret = IOTC_ERROR;
            break;
        }

        ret = IotcJsonAddStr2Obj(devinfoObj, STR_JSON_DEVID, oneDevId->info.deviceId);
        if(ret != IOTC_OK)
        {
            IOTC_LOGW("add devId error %d", ret);
            break;
        }

        ret = IotcJsonAddItem2Array(devInfoArr, devinfoObj);
        if (ret != IOTC_OK) {
            IotcJsonDelete(devinfoObj);
            IOTC_LOGW("add svc info error %d", ret);
            break;
        }
        return devInfoArr;
    } while (0);

    IotcJsonDelete(devInfoArr);
    return NULL;

}

int32_t M2mCloudParseAuthCodeByResponse(M2mCloudContext *ctx, const CoapPacket *resp, int32_t *errcode)
{
    CHECK_RETURN_LOGW(ctx != NULL && resp != NULL && errcode != NULL && resp->payload.data != NULL &&
        resp->payload.len != 0, IOTC_ERR_PARAM_INVALID, "invalid param");

    IotcJson *respJson = IotcJsonParseWithLen((const char *)resp->payload.data, resp->payload.len);
    if (respJson == NULL) {
        IOTC_LOGW("create json error");
        return IOTC_ADAPTER_JSON_ERR_PARSE;
    }

    int32_t ret = UtilsJsonGetNum(respJson, STR_ERRCODE, errcode);
    if (ret != IOTC_OK) {
        IOTC_LOGE("json get errcode error %d", ret);
        IotcJsonDelete(respJson);
        return ret;
    }

    if (*errcode != CLOUD_ERRCODE_OK) {
        IOTC_LOGE("cloud authCode error %d", *errcode);
        IotcJsonDelete(respJson);
        return IOTC_OK;
    }

    CloudIssueAuthCode respAuthcode = {0};
    ret = UtilsJsonGetString(respJson, STR_JSON_DEVID, respAuthcode.devId, sizeof(respAuthcode.devId));
    if(ret != IOTC_OK)
    {
        IOTC_LOGE("cloud devId error %d", ret);
        IotcJsonDelete(respJson);
        return IOTC_OK;
    }

    ret = UtilsJsonGetString(respJson, STR_JSON_AUTHCODE, respAuthcode.authCode, sizeof(respAuthcode.authCode));
    if (ret != IOTC_OK) {
        IOTC_LOGE("json get authcode error %d", ret);
        IotcJsonDelete(respJson);
        return ret;
    }
    IOTC_LOGI("%s json get STR_JSON_AUTHCODE  %s", __func__, respAuthcode.authCode);

    ret = UtilsJsonGetString(respJson, STR_JSON_AUTHCODE_ID, respAuthcode.authCodeId, sizeof(respAuthcode.authCodeId));
    if (ret != IOTC_OK) {
        IOTC_LOGE("json get authcodeId error %d", ret);
        IotcJsonDelete(respJson);
        return ret;
    }
    IOTC_LOGI("%s json get STR_JSON_AUTHCODE_ID  %s", __func__, respAuthcode.authCodeId);
    ret = UtilsJsonGetNum(respJson, STR_JSON_TIMEOUT, &respAuthcode.timeout);

    //通知sle模块
    EventBusPublishSync(IOTC_CORE_SLE_EVENT_AUTH_CODE_SETUP, (void *)&respAuthcode,  sizeof(respAuthcode));
    IotcJsonDelete(respJson);

    //切换云测存储的状态
    if(MoveCloudDeviceToOnline(respAuthcode.devId)!= IOTC_OK)
    {
        IOTC_LOGE("move cloud device to online error");
        return IOTC_ERROR;
    }
    return IOTC_OK;
}

void M2mCloudDevIdRespHandler(const CoapPacket *resp,
    const SocketAddr *addr, void *userData, bool timeout)
{
    NOT_USED(addr);
    CHECK_V_RETURN_LOGW(userData != NULL, "param invalid");
    M2mCloudContext *ctx = (M2mCloudContext *)userData;

    if (ctx->stateManager.fsmCtx == NULL ||
        ctx->stateManager.fsmCtx->cur != M2M_CLOUD_FSM_STATE_GET_AUTHCODE_WAIT_RESP) {
        IOTC_LOGW("recv psk resp invalid state %d", ctx->stateManager.fsmCtx->cur);
        return;
    }

    if (timeout) {
        MGR_CHANGE_FSM_TO(ctx, M2M_CLOUD_FSM_STATE_CONNECT);
        IOTC_LOGW("authcode wait resp timeout");
        return;
    }
    CHECK_V_RETURN_LOGW(resp != NULL, "param invalid");

    int32_t errcode;

    int32_t ret = M2mCloudParseAuthCodeByResponse(ctx, resp, &errcode);
    if (ret != IOTC_OK) {
        MGR_CHANGE_FSM_TO(ctx, M2M_CLOUD_FSM_STATE_CONNECT);
        IOTC_LOGW("authcode resp parse error %d", ret);
        return;
    }
}

const CloudOption *M2mCloudGetAuthCodeByDevIdSyncOption(void)
{
    static const char *SYS_SYNC[] = {STR_URI_PATH_SYS, STR_URI_PATH_SYNC};
    static const CloudOption SYNC_OPTION = {
        .uri = SYS_SYNC,
        .num = ARRAY_SIZE(SYS_SYNC),
        .opBitMap = UTILS_BIT(CLOUD_OPTION_BIT_SEQ_NUM_ID) | \
                    UTILS_BIT(CLOUD_OPTION_BIT_REQ_ID) | \
                    UTILS_BIT(CLOUD_OPTION_BIT_DEV_ID) | \
                    UTILS_BIT(CLOUD_OPTION_BIT_ACCESS_TOKEN_ID),
    };
    return &SYNC_OPTION;
}

static void CloudProcessSubData(uint32_t event, void *param, uint32_t len)
{
    M2mEndDeviceInfo * devInfo = (M2mEndDeviceInfo *)param;
    if (devInfo == NULL) {
        IOTC_LOGE("devInfo is NULL");
        return;
    }

    //已经在待同步云测的设备列表中，无需再刷新
    if(IsCloudDeviceInfoExist(devInfo->deviceId, &g_manager.pending_list))
    {
        IOTC_LOGE("deviceId %s is already in pending list", devInfo->deviceId);
        return;
    }

    if(IsCloudDeviceInfoExist(devInfo->deviceId, &g_manager.online_list))
    {
        IOTC_LOGE("deviceId %s is already in  online list", devInfo->deviceId);
        return;
    }

    M2mEndDeviceInfo *deviceInfo = (M2mEndDeviceInfo *)IotcMalloc(sizeof(M2mEndDeviceInfo));
    if (deviceInfo == NULL) {
        IOTC_LOGW("malloc error");
        return ;
    }

    (void)memset_s(deviceInfo, sizeof(M2mEndDeviceInfo), 0, sizeof(M2mEndDeviceInfo));
    strcpy_s(deviceInfo->deviceId, DEVICE_ID_MAX_STR_LEN + 1, devInfo->deviceId);
    deviceInfo->online = devInfo->online;

    if(AddCloudDeviceInfo(deviceInfo, &g_manager.pending_list) != IOTC_OK)
    {
        IotcFree(deviceInfo);
        return ;
    }

    //把状态切换到获取认证码状态
    M2mCloudContext *ctx = GetM2mCloudCtx();
    MGR_CHANGE_FSM_TO(ctx, M2M_CLOUD_FSM_STATE_GET_AUTHCODE);
    return ;

}

int32_t SleDeviceCloudFsmInit()
{

    static bool isInitialized = false;

    if (isInitialized) {
        IOTC_LOGW("SleEcoDeviceCloudManagerInit already initialized");
        return IOTC_OK;
    }

    int32_t ret = EventBusSubscribe(CloudProcessSubData, IOTC_EVENT_INNER_CLOUD_SLE_SETUP);
    if (ret != IOTC_OK) {
        IOTC_LOGE("EventBusSubscribe failed with error code: %d", ret);
        return ret;
    }

    EventBusPublishSync(IOTC_CORE_SLE_EVENT_START_SEEK_SETUP, NULL,  0);

    isInitialized = true;
    return ret;
}
