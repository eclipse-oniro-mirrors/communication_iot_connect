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
#include "m2m_cloud_fsm.h"
#include "utils_assert.h"
#include "iotc_errcode.h"
#include "sched_timer.h"
#include "iotc_svc_dev.h"
#include "utils_bit_map.h"
#include "iotc_svc_conn.h"
#include "iotc_event.h"
#include "m2m_cloud_backoff.h"
#include "m2m_cloud_reg.h"
#include "m2m_cloud_errcode.h"
#include "m2m_cloud_login.h"
#include "m2m_cloud_sync.h"
#include "m2m_cloud_ctl.h"
#include "m2m_cloud_dev_del.h"
#include "m2m_cloud_revoke.h"
#include "m2m_cloud_heartbeat.h"
#include "m2m_cloud_psk.h"
#include "m2m_cloud_authcode.h"
#include "m2m_cloud_dev_mgr.h"
#include "event_bus.h"
#include "m2m_cloud_link.h"
#include "securec.h"
#include "service_proxy.h"
#include "iotc_svc.h"
#include "iotc_svc_ble.h"

#define M2M_CLOUD_REGISTER_TIMEOUT_MS UTILS_MIN_TO_MS(1)

#define CHANGE_FSM_TO(ctx, state) UtilsFsmSwitch((ctx)->stateManager.fsmCtx, (state))

static void M2mCloudFsmOnStateChange(UtilsFsm *fsm, int32_t before, int32_t next)
{
    CHECK_V_RETURN(fsm != NULL);
    IOTC_LOGN("CLOUD FSM [%d => %d]", before, next);
}

static void RegisterTimeoutTimerHandler(int32_t id, void *userData)
{
    CHECK_V_RETURN_LOGW(userData != NULL, "param invalid");
    M2mCloudContext *ctx = (M2mCloudContext *)userData;
    IOTC_LOGW("register timeout");
    EventBusPublishSync(IOTC_SDK_AILIFE_EVENT_WIFI_UPLINK_REGISTER_FAILED, NULL, 0);
    CHANGE_FSM_TO(ctx, M2M_CLOUD_FSM_STATE_EXIT);
    if (ctx->stateManager.regTimer >= 0) {
        SchedTimerRemove(ctx->stateManager.regTimer);
        ctx->stateManager.regTimer = EVENT_SOURCE_INVALID_TIMER_FD;
    }
}

typedef struct {
    int32_t errcode;
    char devId[64];  // 或者 char *devId; 取决于你是否动态分配
} StatusSyncResult;

static void RemoveRegisterTimeoutTimer(uint32_t event, void *param, uint32_t len)
{
    NOT_USED(event);
    NOT_USED(param);
    NOT_USED(len);

    M2mCloudContext *ctx = GetM2mCloudCtx();
    if (ctx == NULL) {
        return;
    }
    if (ctx->stateManager.regTimer >= 0) {
        SchedTimerRemove(ctx->stateManager.regTimer);
        ctx->stateManager.regTimer = EVENT_SOURCE_INVALID_TIMER_FD;
    }
    IOTC_LOGI("remove register timeout timer");
}

static int32_t CloudFsmInitHandler(void *param, int32_t cur)
{
    CHECK_RETURN_LOGW(param != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    M2mCloudContext *ctx = (M2mCloudContext *)param;

    UTILS_BIT_CLR(ctx->bitMap);
    int32_t ret;
    DevBindStatus bindStatus = DevSvcProxyGetBindStatus();
    if (bindStatus == DEV_BIND_STATUS_BIND || bindStatus == DEV_BIND_STATUS_REVOKE) {
        bool isLoginExits = false;
        ret = DevSvcProxyGetLoginInfo(&isLoginExits, &ctx->authInfo.loginInfo);
        if (ret != IOTC_OK || !isLoginExits) {
            return cur;
        }
        if (bindStatus == DEV_BIND_STATUS_REVOKE) {
            UTILS_BIT_SET(ctx->bitMap, M2M_CLOUD_CTX_BIT_REVOKE);
        }
        return M2M_CLOUD_FSM_STATE_CREATE_LINK;
    } else {
        bool isRegInfoExist = false;
        ret = DevSvcProxyGetRegisterInfo(&isRegInfoExist, &ctx->authInfo.regInfo);
        if (ret != IOTC_OK || !isRegInfoExist) {
            return cur;
        }
        UTILS_BIT_SET(ctx->bitMap, M2M_CLOUD_CTX_BIT_REGISTER);
        if (ctx->stateManager.regTimer < 0) {
            ret = EventBusSubscribe(RemoveRegisterTimeoutTimer, IOTC_SDK_AILIFE_EVENT_WIFI_UPLINK_ONLINE);
            if (ret != IOTC_OK) {
                IOTC_LOGW("sub event error %d", ret);
                return cur;
            }
            ctx->stateManager.regTimer = SchedTimerAdd(EVENT_SOURCE_TIMER_TYPE_ONCE, RegisterTimeoutTimerHandler,
                M2M_CLOUD_REGISTER_TIMEOUT_MS, ctx);
        }
        return M2M_CLOUD_FSM_STATE_CREATE_LINK;
    }

    return cur;
}

static void CloudSessLinkErrorCallback(M2mCloudContext *ctx)
{
    CHECK_V_RETURN_LOGW(ctx != NULL, "param invalid");
    M2mCloudLinkClose(ctx);
    M2mCloudDisableHeartbeat(ctx);
    CHANGE_FSM_TO(ctx, M2M_CLOUD_FSM_STATE_CONNECT);
}

static int32_t CloudFsmCreateLinkHandler(void *param, int32_t cur)
{
    CHECK_RETURN_LOGW(param != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    M2mCloudContext *ctx = (M2mCloudContext *)param;

    ctx->linkInfo.cloudLinkErrorCallback = CloudSessLinkErrorCallback;
    int32_t ret = M2mCloudLinkCreate(ctx);
    if (ret != IOTC_OK) {
        IOTC_LOGW("create link error %d", ret);
        return cur;
    }
    ret = M2mCloudCtlInit(ctx);
    if (ret != IOTC_OK) {
        IOTC_LOGW("ctl init error %d", ret);
        return cur;
    }
    ret = M2mCloudDeviveDelInit(ctx);
    if (ret != IOTC_OK) {
        IOTC_LOGW("dev del init error %d", ret);
        return cur;
    }
    M2mCloudBackoffInit(ctx);
    return M2M_CLOUD_FSM_STATE_CONNECT;
}

static int32_t CloudFsmConnectHandler(void *param, int32_t cur)
{
    CHECK_RETURN_LOGW(param != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    M2mCloudContext *ctx = (M2mCloudContext *)param;

    if (!ConnSvcProxyIsNetConnected() || IsM2mCloudBackoffTime(ctx)) {
        return cur;
    }

    int32_t ret = M2mCloudLinkConnect(ctx);
    M2mCloudBackoffUpdate(ctx);
    if (ret != IOTC_OK) {
        IOTC_LOGW("connect cloud error %d", ret);
        return cur;
    }

    /* 此处发送CSM */
    M2mCloudSendCSM(ctx);
    return M2M_CLOUD_FSM_STATE_PSK;
}

static void M2mCloudPskRegRespHandler(const CoapPacket *resp,
    const SocketAddr *addr, void *userData, bool timeout)
{
    NOT_USED(addr);
    CHECK_V_RETURN_LOGW(userData != NULL, "param invalid");
    M2mCloudContext *ctx = (M2mCloudContext *)userData;

    if (ctx->stateManager.fsmCtx == NULL ||
        ctx->stateManager.fsmCtx->cur != M2M_CLOUD_FSM_STATE_PSK_WAIT_RESP) {
        IOTC_LOGW("recv psk resp invalid state %d", ctx->stateManager.fsmCtx->cur);
        return;
    }

    if (timeout) {
        CHANGE_FSM_TO(ctx, M2M_CLOUD_FSM_STATE_CONNECT);
        IOTC_LOGW("psk wait resp timeout %d", timeout);
        return;
    }
    CHECK_V_RETURN_LOGW(resp != NULL, "param invalid");

    int32_t errcode;
    int32_t ret = M2mCloudParsePskResponse(ctx, resp, &errcode);
    if (ret != IOTC_OK) {
        CHANGE_FSM_TO(ctx, M2M_CLOUD_FSM_STATE_CONNECT);
        IOTC_LOGW("psk resp parse error %d", ret);
        return;
    }
    /* 注册返回成功以后删除定时器 */
    if (ctx->stateManager.regTimer >= 0) {
        SchedTimerRemove(ctx->stateManager.regTimer);
        ctx->stateManager.regTimer = EVENT_SOURCE_INVALID_TIMER_FD;
    }
    ctx->pskInfo.pskFinish = 1;

    CHANGE_FSM_TO(ctx, UTILS_IS_BIT_SET(ctx->bitMap, M2M_CLOUD_CTX_BIT_REGISTER) ?
        M2M_CLOUD_FSM_STATE_REGISTER : M2M_CLOUD_FSM_STATE_LOGIN);
}

static int32_t CloudFsmPskHandler(void *param, int32_t cur)
{
    CHECK_RETURN_LOGW(param != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    M2mCloudContext *ctx = (M2mCloudContext *)param;

    ctx->pskInfo.pskFinish = 0;
    int32_t ret = M2mCloudSendRequest(ctx, M2mCloudPskRegRespHandler,
        M2mCloudBuildPskRequest, M2mCloudGetPskOption());
    if (ret != IOTC_OK) {
        IOTC_LOGW("%s psk error %d", __func__, ret);
        return M2M_CLOUD_FSM_STATE_CONNECT;
    }

    return M2M_CLOUD_FSM_STATE_PSK_WAIT_RESP;
}

static void M2mCloudRegisterRegRespHandler(const CoapPacket *resp,
    const SocketAddr *addr, void *userData, bool timeout)
{
    NOT_USED(addr);
    CHECK_V_RETURN_LOGW(userData != NULL, "param invalid");
    M2mCloudContext *ctx = (M2mCloudContext *)userData;

    if (ctx->stateManager.fsmCtx == NULL ||
        ctx->stateManager.fsmCtx->cur != M2M_CLOUD_FSM_STATE_REGISTER_WAIT_RESP) {
        IOTC_LOGW("recv reg resp invalid state %d", ctx->stateManager.fsmCtx->cur);
        return;
    }

    if (timeout) {
        CHANGE_FSM_TO(ctx, M2M_CLOUD_FSM_STATE_CONNECT);
        IOTC_LOGW("register wait resp timeout");
        return;
    }
    CHECK_V_RETURN_LOGW(resp != NULL, "param invalid");

    int32_t errcode;
    int32_t ret = M2mCloudParseRegisterResponse(ctx, resp, &errcode);
    if (ret != IOTC_OK) {
        CHANGE_FSM_TO(ctx, M2M_CLOUD_FSM_STATE_CONNECT);
        IOTC_LOGW("register resp parse error %d", ret);
        return;
    }
    UTILS_BIT_RESET(ctx->bitMap, M2M_CLOUD_CTX_BIT_REGISTER);
    (void)memset_s(&ctx->authInfo.regInfo, sizeof(ctx->authInfo.regInfo), 0, sizeof(ctx->authInfo.regInfo));
    if (errcode != CLOUD_ERRCODE_OK) {
        EventBusPublishSync(IOTC_SDK_AILIFE_EVENT_WIFI_UPLINK_REGISTER_FAILED, NULL, 0);
        CHANGE_FSM_TO(ctx, M2M_CLOUD_FSM_STATE_EXIT);
    } else {
        bool isLoginExits = false;
        ret = DevSvcProxyGetLoginInfo(&isLoginExits, &ctx->authInfo.loginInfo);
        if (ret != IOTC_OK || !isLoginExits) {
            IOTC_LOGW("get login info error after reg");
            CHANGE_FSM_TO(ctx, M2M_CLOUD_FSM_STATE_EXIT);
        } else {
            BleSvcProxyStopAdv();
            CHANGE_FSM_TO(ctx, M2M_CLOUD_FSM_STATE_LOGIN);
        }
    }
}

static int32_t CloudFsmRegisterHandler(void *param, int32_t cur)
{
    CHECK_RETURN_LOGW(param != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    M2mCloudContext *ctx = (M2mCloudContext *)param;

    int32_t ret = M2mCloudSendRequest(ctx, M2mCloudRegisterRegRespHandler,
        M2mCloudBuildRegisterRequest, M2mCloudGetRegisterOption());
    if (ret != IOTC_OK) {
        IOTC_LOGW("register error %d", ret);
        return M2M_CLOUD_FSM_STATE_CONNECT;
    }

    return M2M_CLOUD_FSM_STATE_REGISTER_WAIT_RESP;
}

static void CloudLoginRespHandler(const CoapPacket *resp, const SocketAddr *addr, void *userData, bool timeout)
{
    NOT_USED(addr);
    CHECK_V_RETURN_LOGW(userData != NULL, "param invalid");
    M2mCloudContext *ctx = (M2mCloudContext *)userData;

    if (ctx->stateManager.fsmCtx == NULL ||
        ctx->stateManager.fsmCtx->cur != M2M_CLOUD_FSM_STATE_LOGIN_WAIT_RESP) {
        IOTC_LOGW("recv login resp invalid state %d", ctx->stateManager.fsmCtx->cur);
        return;
    }

    if (timeout) {
        CHANGE_FSM_TO(ctx, M2M_CLOUD_FSM_STATE_CONNECT);
        IOTC_LOGW("login wait resp timeout");
        return;
    }
    CHECK_V_RETURN_LOGW(resp != NULL, "param invalid");

    int32_t errcode;
    int32_t ret = M2mCloudLoginResponseParse(ctx, resp, &errcode);
    if (ret != IOTC_OK || errcode != CLOUD_ERRCODE_OK) {
        IOTC_LOGW("login resp parse error %d/%d", ret, errcode);
        CHANGE_FSM_TO(ctx, M2M_CLOUD_FSM_STATE_CONNECT);
        return;
    }
    if (UTILS_IS_BIT_SET(ctx->bitMap, M2M_CLOUD_CTX_BIT_REVOKE)) {
        CHANGE_FSM_TO(ctx, M2M_CLOUD_FSM_STATE_REVOKE);
    } else {
        CHANGE_FSM_TO(ctx, M2M_CLOUD_FSM_STATE_DEV_INFO_SYNC);
    }
}

static void CloudRevokeRespHandler(const CoapPacket *resp, const SocketAddr *addr, void *userData, bool timeout)
{
    NOT_USED(addr);
    NOT_USED(resp);
    CHECK_V_RETURN_LOGW(userData != NULL, "param invalid");
    M2mCloudContext *ctx = (M2mCloudContext *)userData;

    IOTC_LOGN("recv revoke resp, start revoke!!!");
    if (timeout) {
        IOTC_LOGW("revoke wait resp timeout");
    }

    int32_t ret = DevSvcProxyCleanRevokeFlag();
    if (ret != IOTC_OK) {
        IOTC_LOGW("clean revoke flag error %d", ret);
        CHANGE_FSM_TO(ctx, M2M_CLOUD_FSM_STATE_INIT);
        return;
    }
    ret = DevSvcProxyCleanLoginInfo();
    if (ret != IOTC_OK) {
        IOTC_LOGW("clean loginInfo error %d", ret);
        CHANGE_FSM_TO(ctx, M2M_CLOUD_FSM_STATE_INIT);
        return;
    }
    IOTC_LOGN("revoke success!!!");
    CHANGE_FSM_TO(ctx, M2M_CLOUD_FSM_STATE_INIT);
}

static int32_t CloudFsmLoginHandler(void *param, int32_t cur)
{
    NOT_USED(cur);
    M2mCloudContext *ctx = (M2mCloudContext *)param;

    int32_t ret = M2mCloudSendRequest(ctx, CloudLoginRespHandler,
        M2mCloudBuildLoginRequest, M2mCloudGetLoginOption());
    if (ret != IOTC_OK) {
        IOTC_LOGW("login error %d", ret);
        return M2M_CLOUD_FSM_STATE_CONNECT;
    }

    return M2M_CLOUD_FSM_STATE_LOGIN_WAIT_RESP;
}

static int32_t CloudFsmRevokeHandler(void *param, int32_t cur)
{
    NOT_USED(cur);
    M2mCloudContext *ctx = (M2mCloudContext *)param;

    int32_t ret = M2mCloudSendRequest(ctx, CloudRevokeRespHandler,
        M2mCloudBuildRevokeRequest, M2mCloudGetRevokeOption());
    if (ret != IOTC_OK) {
        IOTC_LOGW("revoke error %d", ret);
        return M2M_CLOUD_FSM_STATE_CONNECT;
    }

    return M2M_CLOUD_FSM_STATE_REVOKE_WAIT_RESP;
}

static void M2mCloudAuthCodeRegRespHandler(const CoapPacket *resp,
    const SocketAddr *addr, void *userData, bool timeout)
{
    NOT_USED(addr);
    CHECK_V_RETURN_LOGW(userData != NULL, "param invalid");
    M2mCloudContext *ctx = (M2mCloudContext *)userData;

    if (ctx->stateManager.fsmCtx == NULL ||
        ctx->stateManager.fsmCtx->cur != M2M_CLOUD_FSM_STATE_AUTHCODE_WAIT_RESP) {
        IOTC_LOGW("recv psk resp invalid state %d", ctx->stateManager.fsmCtx->cur);
        return;
    }

    if (timeout) {
        CHANGE_FSM_TO(ctx, M2M_CLOUD_FSM_STATE_CONNECT);
        IOTC_LOGW("authcode wait resp timeout");
        return;
    }
    CHECK_V_RETURN_LOGW(resp != NULL, "param invalid");

    int32_t errcode;

    int32_t ret = M2mCloudParseAuthCodeResponse(ctx, resp, &errcode);
    if (ret != IOTC_OK) {
        CHANGE_FSM_TO(ctx, M2M_CLOUD_FSM_STATE_CONNECT);
        IOTC_LOGW("authcode resp parse error %d", ret);
        return;
    }

    CHANGE_FSM_TO(ctx, M2M_CLOUD_FSM_STATE_ONLINE);
    EventBusPublishSync(IOTC_SDK_AILIFE_EVENT_WIFI_UPLINK_REGISTERED, NULL, 0);

    /* 获取AUTHCODE后开启本地控 */
    ret = ServiceProxyStartService(IOTC_SERVICE_ID_LOCAL_CONTROL, NULL);
    if (ret != IOTC_OK) {
        IOTC_LOGE("start local control service error %d", ret);
    }
    ret = ServiceProxyStartService(IOTC_SERVICE_ID_LAN_SEARCH, NULL);
    if (ret != IOTC_OK) {
        IOTC_LOGE("start lan search error %d", ret);
    }

    //开启Sle扫描以及初始化
    if (SleDeviceCloudFsmInit() != IOTC_OK) {
        IOTC_LOGE("SleDeviceCloudFsmInit error");
    }
}

static int32_t CloudFsmAuthCodeHandler(void *param, int32_t cur)
{
    CHECK_RETURN_LOGW(param != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    M2mCloudContext *ctx = (M2mCloudContext *)param;

    int32_t ret = M2mCloudSendRequest(ctx, M2mCloudAuthCodeRegRespHandler,
        M2mCloudBuildAuthCodeRequest, M2mCloudGetAuthCodeOption());
    if (ret != IOTC_OK) {
        IOTC_LOGW("%s authcode error %d", __func__, ret);
        return M2M_CLOUD_FSM_STATE_CONNECT;
    }

    return M2M_CLOUD_FSM_STATE_AUTHCODE_WAIT_RESP;
}

static void CloudDevInfoSyncRespHandler(const CoapPacket *resp, const SocketAddr *addr, void *userData, bool timeout)
{
    NOT_USED(addr);
    CHECK_V_RETURN_LOGW(userData != NULL, "param invalid");
    M2mCloudContext *ctx = (M2mCloudContext *)userData;

    if (ctx->stateManager.fsmCtx == NULL ||
        ctx->stateManager.fsmCtx->cur != M2M_CLOUD_FSM_STATE_DEV_INFO_SYNC_WAIT_RESP) {
        IOTC_LOGW("recv sync resp invalid state %d", ctx->stateManager.fsmCtx->cur);
        return;
    }

    if (timeout) {
        CHANGE_FSM_TO(ctx, M2M_CLOUD_FSM_STATE_CONNECT);
        IOTC_LOGW("dev sync wait resp timeout");
        return;
    }

    CHECK_V_RETURN_LOGW(resp != NULL, "param invalid");

    int32_t errcode;
    int32_t ret = M2mCloudParseDevInfoSyncResponse(ctx, resp, &errcode);
    if (ret != IOTC_OK || errcode != CLOUD_ERRCODE_OK) {
        IOTC_LOGW("dev info sync resp parse error %d/%d", ret, errcode);
        CHANGE_FSM_TO(ctx, M2M_CLOUD_FSM_STATE_CONNECT);
        return;
    }
    DevSvcProxySetOnlineStatus(true);
    CHANGE_FSM_TO(ctx, M2M_CLOUD_FSM_STATE_AUTHCODE);
    EventBusPublishSync(IOTC_SDK_AILIFE_EVENT_WIFI_UPLINK_ONLINE, NULL, 0);
}

static int32_t CloudFsmDevInfoSyncHandler(void *param, int32_t cur)
{
    NOT_USED(cur);
    M2mCloudContext *ctx = (M2mCloudContext *)param;

    int32_t ret = M2mCloudSendRequest(ctx, CloudDevInfoSyncRespHandler,
        M2mCloudBuildDevInfoSyncRequest, M2mCloudGetDevInfoSyncOption());
    if (ret != IOTC_OK) {
        IOTC_LOGW("dev info sync error %d", ret);
        return M2M_CLOUD_FSM_STATE_CONNECT;
    }

    return M2M_CLOUD_FSM_STATE_DEV_INFO_SYNC_WAIT_RESP;
}

static int32_t CloudFsmDevIdGetAuthCodeHandler(void *param, int32_t cur)
{
    NOT_USED(cur);
    M2mCloudContext *ctx = (M2mCloudContext *)param;

    int32_t ret = M2mCloudSendRequest(ctx, M2mCloudDevIdRespHandler,
        M2mCloudBuildDevIdRequest, M2mCloudGetAuthCodeByDevIdSyncOption());
    if (ret != IOTC_OK) {
        IOTC_LOGW("dev info sync error %d", ret);
        return M2M_CLOUD_FSM_STATE_CONNECT;
    }

    return M2M_CLOUD_FSM_STATE_GET_AUTHCODE_WAIT_RESP;
}
static int32_t CloudFsmEcoDevInfoSyncHandler(M2mCloudContext *param, int32_t cur)
{
    NOT_USED(cur);
    M2mCloudContext *ctx = param;

    int32_t ret = M2mCloudSendRequest(ctx, (CoapClientRespHandler)M2mCloudEcoDevInfoRespHandler,
                                      M2mCloudBuildEcoDevInfoRequest, M2mCloudEcoDevInfoSyncOption());
    if (ret != IOTC_OK) {
        IOTC_LOGW("dev info sync error %d", ret);
        return M2M_CLOUD_FSM_STATE_CONNECT;
    }

    return M2M_CLOUD_FSM_STATE_ECO_DEV_INFO_SYNC_WAIT_RESP;
}

static int32_t CloudFsmEcoDevStateSyncHandler(M2mCloudContext *param, int32_t cur)
{
    NOT_USED(cur);
    M2mCloudContext *ctx = param;
    int32_t ret = M2mCloudSendRequest(ctx, (CoapClientRespHandler)M2mCloudEcoDevStateRespHandler,
                                      M2mCloudBuildEcoDevStateRequest, M2mCloudEcoDevStateSyncOption());
    if (ret != IOTC_OK) {
        IOTC_LOGW("dev info sync error %d", ret);
        return M2M_CLOUD_FSM_STATE_CONNECT;
    }
    return M2M_CLOUD_FSM_STATE_ECO_STATE_SYNC_WAIT_RESP;
}

static UtilsFsm *GetM2mCloudFsm(void)
{
    static const UtilsFsmStateNode CLOUD_FSM[] = {
        { M2M_CLOUD_FSM_STATE_INIT, CloudFsmInitHandler },
        { M2M_CLOUD_FSM_STATE_CREATE_LINK, CloudFsmCreateLinkHandler },
        { M2M_CLOUD_FSM_STATE_CONNECT, CloudFsmConnectHandler },
        { M2M_CLOUD_FSM_STATE_PSK, CloudFsmPskHandler },
        { M2M_CLOUD_FSM_STATE_PSK_WAIT_RESP, UtilsFsmStateSelfCirculation },
        { M2M_CLOUD_FSM_STATE_REGISTER, CloudFsmRegisterHandler },
        { M2M_CLOUD_FSM_STATE_REGISTER_WAIT_RESP, UtilsFsmStateSelfCirculation },
        { M2M_CLOUD_FSM_STATE_LOGIN, CloudFsmLoginHandler },
        { M2M_CLOUD_FSM_STATE_LOGIN_WAIT_RESP, UtilsFsmStateSelfCirculation },
        { M2M_CLOUD_FSM_STATE_AUTHCODE, CloudFsmAuthCodeHandler },
        { M2M_CLOUD_FSM_STATE_AUTHCODE_WAIT_RESP, UtilsFsmStateSelfCirculation },
        { M2M_CLOUD_FSM_STATE_REVOKE, CloudFsmRevokeHandler },
        { M2M_CLOUD_FSM_STATE_REVOKE_WAIT_RESP, UtilsFsmStateSelfCirculation },
        { M2M_CLOUD_FSM_STATE_DEV_INFO_SYNC, CloudFsmDevInfoSyncHandler },
        { M2M_CLOUD_FSM_STATE_DEV_INFO_SYNC_WAIT_RESP, UtilsFsmStateSelfCirculation },
        { M2M_CLOUD_FSM_STATE_ONLINE, UtilsFsmStateSelfCirculation },
        { M2M_CLOUD_FSM_STATE_DELETE, NULL },
        { M2M_CLOUD_FSM_STATE_GET_AUTHCODE, CloudFsmDevIdGetAuthCodeHandler },
        { M2M_CLOUD_FSM_STATE_ECO_DEV_INFO_SYNC, (UtilsFsmStateHandler)CloudFsmEcoDevInfoSyncHandler },
        { M2M_CLOUD_FSM_STATE_ECO_DEV_INFO_SYNC_WAIT_RESP, UtilsFsmStateSelfCirculation },
        { M2M_CLOUD_FSM_STATE_ECO_STATE_SYNC, (UtilsFsmStateHandler)CloudFsmEcoDevStateSyncHandler },
        { M2M_CLOUD_FSM_STATE_ECO_STATE_SYNC_WAIT_RESP, UtilsFsmStateSelfCirculation },
        { M2M_CLOUD_FSM_STATE_GET_AUTHCODE_WAIT_RESP, UtilsFsmStateSelfCirculation },
    };
    static UtilsFsm fsm = UTILS_FSM_DECLARE_INIT("cloud", M2M_CLOUD_FSM_STATE_INIT,
        M2mCloudFsmOnStateChange, CLOUD_FSM);
    return &fsm;
}

static void M2mFsmTimerCallback(int32_t id, void *userData)
{
    NOT_USED(id);
    CHECK_V_RETURN_LOGW(userData != NULL, "param invalid");
    M2mCloudContext *ctx = (M2mCloudContext *)userData;

    (void)UtilsFsmRunning(ctx->stateManager.fsmCtx, ctx);
}

int32_t M2mCloudFsmInit(M2mCloudContext *ctx)
{
    CHECK_RETURN_LOGW(ctx != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    ctx->stateManager.fsmCtx = GetM2mCloudFsm();
    ctx->stateManager.fsmCtx->cur = M2M_CLOUD_FSM_STATE_INIT;
    ctx->stateManager.fsmTimer = SchedTimerAdd(EVENT_SOURCE_TIMER_TYPE_REPEAT, M2mFsmTimerCallback,
        SchedGetDefaultTimerInterval(), ctx);
    if (ctx->stateManager.fsmTimer < 0) {
        IOTC_LOGW("fsm timer start error %d", ctx->stateManager.fsmTimer);
        return ctx->stateManager.fsmTimer;
    }

    return IOTC_OK;
}

void M2mCloudFsmDeinit(M2mCloudContext *ctx)
{
    CHECK_V_RETURN_LOGW(ctx != NULL, "param invalid");
    ctx->stateManager.fsmCtx = NULL;
    if (ctx->stateManager.fsmTimer >= 0) {
        SchedTimerRemove(ctx->stateManager.fsmTimer);
        ctx->stateManager.fsmTimer = EVENT_SOURCE_INVALID_TIMER_FD;
    }
    if (ctx->stateManager.tokenTimer >= 0) {
        SchedTimerRemove(ctx->stateManager.tokenTimer);
        ctx->stateManager.tokenTimer = EVENT_SOURCE_INVALID_TIMER_FD;
    }
    if (ctx->stateManager.regTimer >= 0) {
        SchedTimerRemove(ctx->stateManager.regTimer);
        ctx->stateManager.regTimer = EVENT_SOURCE_INVALID_TIMER_FD;
    }
    if (ctx->stateManager.hbTimer >= 0) {
        SchedTimerRemove(ctx->stateManager.hbTimer);
        ctx->stateManager.hbTimer = EVENT_SOURCE_INVALID_TIMER_FD;
    }
}