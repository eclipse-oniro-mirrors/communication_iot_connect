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
#include "svc_softap_sess.h"
#include "iotc_log.h"
#include "utils_assert.h"
#include "comm_def.h"
#include "trans_buffer_inner.h"
#include "coap_endpoint.h"
#include "coap_codec_udp.h"
#include "coap_endpoint_server.h"
#include "coap_endpoint_retrans.h"
#include "coap_endpoint_event_source.h"
#include "utils_common.h"
#include "wifi_sched_fd_watch.h"
#include "securec.h"
#include "adapter_network.h"
#include "svc_softap_coap.h"
#include "svc_softap_retrans.h"
#include "sched_event_loop.h"
#include "sched_timer.h"
#include "iotc_conf.h"
#include "security_random.h"
#include "adapter_wifi.h"
#include "service_proxy.h"
#include "iotc_svc.h"
#include "iotc_svc_dev.h"
#include "iotc_errcode.h"
#include "product_adapter.h"

/* 开始speke协商后需要在30s内协商完成 */
#define PEER_WAIT_SPEKE_NEGO_TIMEOUT UTILS_SEC_TO_MS(30)
/* 每5秒检查一次sta列表，检查当前对端是否已断开softap */
#define SOFTAP_STA_CHECK_TIMER_PERIOD UTILS_SEC_TO_MS(5)

static const char *SOFTAP_NAME = "softap";

static SoftapPeerSess *FindSoftapPeerSession(SoftapSess *sess, const SocketAddr *addr)
{
    SoftapPeerSess *empty = NULL;
    for (uint32_t i = 0; i < IOTC_CONF_SOFTAP_MAX_PEER_SESS_NUM; ++i) {
        if (addr->addr == sess->peerSess[i].addrInfo.addr) {
            return &sess->peerSess[i];
        }
        if (!UTILS_IS_BIT_SET(sess->peerSess[i].bitMap, SOFTAP_PEER_SESS_BIT_MAP_LINK_ACTIVE)) {
            empty = &sess->peerSess[i];
        }
    }
    return empty;
}

static int32_t SoftapGetPinCode(SpekeSession *sess, void *user, uint8_t *pinCode, uint32_t *len)
{
    NOT_USED(user);
    CHECK_RETURN(len != NULL && *len >= IOTC_PINCODE_LEN, IOTC_ERR_PARAM_INVALID);

    *len = IOTC_PINCODE_LEN;
    int32_t ret = ProductProfGetPincode(pinCode, *len);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get pin err %d", ret);
        return ret;
    }
    return IOTC_OK;
}

static void DestroyPeerSession(SoftapPeerSess *peerSess)
{
    if (UTILS_IS_BIT_SET(peerSess->bitMap, SOFTAP_PEER_SESS_BIT_MAP_LINK_ACTIVE)) {
        (void)AdapterSoftapDisassociateSta(peerSess->mac, sizeof(peerSess->mac));
    }
    if (peerSess->timer >= 0) {
        SchedTimerRemove(peerSess->timer);
    }
    if (peerSess->speke != NULL) {
        SpekeFreeSession(peerSess->speke);
    }
    (void)memset_s(peerSess, sizeof(SoftapPeerSess), 0, sizeof(SoftapPeerSess));
    peerSess->timer = EVENT_SOURCE_INVALID_TIMER_FD;
}

static void PeerSessTimeoutTimerCallback(int32_t id, void *userData)
{
    CHECK_V_RETURN_LOGW(userData != NULL, "param invalid");
    /* userdata pointer only use to match peer */
    SoftapPeerSess *peerSess = (SoftapPeerSess *)userData;
    SoftapServiceContext *ctx = GetSoftapServiceContext();
    if (ctx == NULL) {
        SchedTimerRemove(id);
        return;
    }

    for (uint32_t i = 0; i < IOTC_CONF_SOFTAP_MAX_PEER_SESS_NUM; ++i) {
        if (&ctx->sess.peerSess[i] != peerSess) {
            continue;
        }
        DestroyPeerSession(&ctx->sess.peerSess[i]);
    }
    return;
}

static int32_t SoftapNotifySpekeFinished(SpekeSession *sess, void *user, int32_t errorCode)
{
    NOT_USED(user);
    SoftapServiceContext *ctx = GetSoftapServiceContext();
    if (ctx == NULL) {
        IOTC_LOGW("softap ctx is null");
        return IOTC_CORE_WIFI_NETCFG_ERR_SOFTAP_INVALID_CTX;
    }

    SoftapPeerSess *peerSess = NULL;
    for (uint32_t i = 0; i < IOTC_CONF_SOFTAP_MAX_PEER_SESS_NUM; ++i) {
        if (sess == ctx->sess.peerSess[i].speke) {
            peerSess = &ctx->sess.peerSess[i];
        }
    }
    if (peerSess == NULL) {
        IOTC_LOGW("invalid speke sess");
        return IOTC_CORE_WIFI_NETCFG_ERR_SOFTAP_INVALID_CTX;
    }

    if (errorCode == IOTC_OK) {
        IOTC_LOGN("speke finish ok");
        UTILS_BIT_SET(peerSess->bitMap, SOFTAP_PEER_SESS_BIT_MAP_SPEKE_SESS_CREATED);
        UTILS_BIT_RESET(peerSess->bitMap, SOFTAP_PEER_SESS_BIT_MAP_E2E_SEQ_SET);
        if (peerSess->timer >= 0) {
            SchedTimerRemove(peerSess->timer);
            peerSess->timer = EVENT_SOURCE_INVALID_TIMER_FD;
        }
        return IOTC_OK;
    }

    IOTC_LOGW("speke finish err %d", errorCode);
    DestroyPeerSession(peerSess);
    return IOTC_OK;
}

static int32_t CreatePeerSession(SoftapPeerSess *peerSess, const SocketAddr *addrInfo)
{
    DestroyPeerSession(peerSess);
    AdapterStationList *staList = NULL;
    int32_t ret = AdapterGetSoftapStationInfo(&staList);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get station list error %d", ret);
        return ret;
    }
    ret = IOTC_CORE_WIFI_NETCFG_ERR_SOFTAP_INVALID_STATION;
    for (uint32_t i = 0; i < staList->num; ++i) {
        AdapterStationInfo *staInfo = &staList->stationList[i];
        if (staInfo->ip != addrInfo->addr) {
            continue;
        }
        ret = memcpy_s(peerSess->mac, sizeof(peerSess->mac), staInfo->mac, sizeof(staInfo->mac));
        if (ret != EOK) {
            ret = IOTC_ERR_SECUREC_MEMCPY;
        }
        break;
    }
    AdapterFreeSoftapStationInfo(staList);
    staList = NULL;
    if (ret != IOTC_OK) {
        IOTC_LOGW("get sta mac error %d", ret);
        return ret;
    }

    /* start timer to clean sess if timeout */
    peerSess->timer = SchedTimerAdd(EVENT_SOURCE_TIMER_TYPE_ONCE, PeerSessTimeoutTimerCallback,
            PEER_WAIT_SPEKE_NEGO_TIMEOUT, peerSess);
    if (peerSess->timer < 0) {
        IOTC_LOGW("start speke timer error %d", peerSess->timer);
        return peerSess->timer;
    }

    UTILS_BIT_SET(peerSess->bitMap, SOFTAP_PEER_SESS_BIT_MAP_LINK_ACTIVE);
    peerSess->addrInfo = *addrInfo;
    peerSess->sendSeq = SecurityRandomUint32() % UINT16_MAX;

    /* create speke sess when recv speke packet */
    return IOTC_OK;
}

int32_t SoftapPeerSessInitSpeke(SoftapPeerSess *peerSess)
{
    CHECK_RETURN_LOGW(peerSess != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    if (peerSess->speke != NULL) {
        return IOTC_OK;
    }

    SpekeCallback cb = {SoftapGetPinCode, SoftapNotifySpekeFinished};
    peerSess->speke = SpekeInitSession(SPEKE_TYPE_SERVER, &cb, NULL);
    if (peerSess->speke == NULL) {
        IOTC_LOGW("speke init error");
        return IOTC_CORE_COMM_SEC_ERR_SPEKE_CREATE;
    }
    return IOTC_OK;
}

SoftapPeerSess *SoftapGetPeerSess(const SocketAddr *addrInfo, SoftapSess *sess)
{
    CHECK_RETURN_LOGE(addrInfo != NULL && sess != NULL, NULL, "param invalid");
    SoftapPeerSess *peerSess = FindSoftapPeerSession(sess, addrInfo);
    if (peerSess == NULL) {
        IOTC_LOGW("no invalid sess");
        return NULL;
    }
    if (!UTILS_IS_BIT_SET(peerSess->bitMap, SOFTAP_PEER_SESS_BIT_MAP_LINK_ACTIVE)) {
        return NULL;
    }
    return peerSess;
}

SoftapPeerSess *SoftapGetPeerSessCreateIfNotExist(const SocketAddr *addrInfo, SoftapSess *sess)
{
    CHECK_RETURN_LOGE(addrInfo != NULL && sess != NULL, NULL, "param invalid");
    SoftapPeerSess *peerSess = FindSoftapPeerSession(sess, addrInfo);
    if (peerSess == NULL) {
        IOTC_LOGW("no invalid sess");
        return NULL;
    }
    int32_t ret;
    if (!UTILS_IS_BIT_SET(peerSess->bitMap, SOFTAP_PEER_SESS_BIT_MAP_LINK_ACTIVE)) {
        ret = CreatePeerSession(peerSess, addrInfo);
        if (ret != IOTC_OK) {
            IOTC_LOGW("create peer sess error %d", ret);
            return NULL;
        }
    }
    return peerSess;
}

static int32_t CreateSoftapLink(SoftapSess *sess)
{
    char local[ADAPTER_IP_STR_MAX_LEN + 1] = {0};
    int32_t ret = AdapterGetLocalIp(local, ADAPTER_IP_STR_MAX_LEN);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get local ip error %d", ret);
        return ret;
    }

    SocketUdpInitParam udp = {
        .port = WIFI_SOFTAP_UDP_PORT,
        .localAddr = local,
        .multiAddr = NULL,
        .broadAddr = NULL,
    };

    TransSocket *socket = TransSocketUdpNew(&udp);
    if (socket == NULL) {
        IOTC_LOGW("create socket error");
        return IOTC_CORE_WIFI_TRANS_ERR_SOCKET_UDP_CREATE;
    }

    sess->link = TransLinkNew(socket, sess->recvBuf, SOFTAP_NAME);
    if (sess->link == NULL) {
        TransSocketFree(socket);
        IOTC_LOGW("create link error");
        return IOTC_CORE_WIFI_TRANS_ERR_LINK_CREATE;
    }

    return IOTC_OK;
}

static int32_t CreateSoftapSession(SoftapSess *sess)
{
    sess->sess = TransSessNew(sess->link, sizeof(CoapPacket), SOFTAP_NAME, sess);
    if (sess->sess == NULL) {
        IOTC_LOGW("create session error");
        return IOTC_CORE_WIFI_TRANS_ERR_SESS_CREATE;
    }

    TransSessAddTailRecvHandler(sess->sess, SoftapCoapMsgRecvPreProcess, "pre", NULL);
    TransSessAddTailRecvHandler(sess->sess, SoftapCoapMsgRecvBase64DecodeProcess, "base64_decode", NULL);
    TransSessAddTailRecvHandler(sess->sess, SoftapCoapMsgRecvDecryptProcess, "decrypt", NULL);
    
    TransSessAddTailSendHandler(sess->sess, SoftapCoapMsgSendEncryptProcess, "encrypt", NULL);
    TransSessAddTailSendHandler(sess->sess, SoftapCoapMsgSendBase64EncodeProcess, "base64_encode", NULL);
    TransSessAddTailRecvHandler(sess->sess, SoftapCoapMsgSendFinalProcess, "final", NULL);

    return IOTC_OK;
}

static int32_t CreateSoftapCoapEndpoint(SoftapSess *sess, const SoftapSvcInitParam *initParam)
{
    sess->endpoint = CoapEndpointNew(sess->sendBuf, sess->sess,
        CoapUdpEncode, CoapUdpDecode, sess);
    if (sess->endpoint == NULL) {
        IOTC_LOGW("create coap endpoint error");
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_ENDPOINT_CREATE;
    }

    /* uri white list for recv not decrypt or not base64 decode */
    static const char *PLAIN_URI[] = {STR_URI_SPEKE};
    static const char *NO_BASE64_URI[] = {STR_URI_SPEKE};

    sess->plainUri = PLAIN_URI;
    sess->plainUriNum = ARRAY_SIZE(PLAIN_URI);
    sess->noBase64Uri = NO_BASE64_URI;
    sess->noBase64Num = ARRAY_SIZE(NO_BASE64_URI);

    static const CoapResource SPEKE_RES[] = {
        {UTILS_BIT(COAP_METHOD_TYPE_POST), STR_URI_SPEKE, NULL, SoftapCoapSpekeReqHandler},
    };
    static const CoapResource CLOUD_SETUP_V2_RES[] = {
        {UTILS_BIT(COAP_METHOD_TYPE_POST), STR_URI_CLOUD_SETUP_V2, NULL, SoftapCoapSetupReqHandler},
    };
    static const CoapResource E2E_CTL_RES[] = {
        {UTILS_BIT(COAP_METHOD_TYPE_POST), STR_E2E_CONTROL, NULL, SoftapCoapE2eCtrlHandler},
    };

    int32_t ret = CoapServerAddResource(sess->endpoint, SPEKE_RES, ARRAY_SIZE(SPEKE_RES));
    if (ret != IOTC_OK) {
        IOTC_LOGW("add speke coap res error %d", ret);
        return ret;
    }

    if (UTILS_IS_BIT_SET(initParam->bitMap, IOTC_WIFI_SERVICE_SOFTAP_NETCFG)) {
        ret = CoapServerAddResource(sess->endpoint, CLOUD_SETUP_V2_RES, ARRAY_SIZE(CLOUD_SETUP_V2_RES));
        if (ret != IOTC_OK) {
            IOTC_LOGW("add cloud setup coap res error %d", ret);
            return ret;
        }
    }

    if (UTILS_IS_BIT_SET(initParam->bitMap, IOTC_WIFI_SERVICE_SOFTAP_E2E_CTRL)) {
        ret = CoapServerAddResource(sess->endpoint, E2E_CTL_RES, ARRAY_SIZE(E2E_CTL_RES));
        if (ret != IOTC_OK) {
            IOTC_LOGW("add e2e ctrl coap res error %d", ret);
            return ret;
        }
    }

    /* use max send buffer for retrans, ensure single message can be retransmitted */
    ret = CoapEndpointRetransEnable(sess->endpoint, SoftapCoapRetransCheckFunc, TransGetSendBufferResSize());
    if (ret != IOTC_OK) {
        IOTC_LOGW("enable coap retrans error %d", ret);
        return ret;
    }

    return IOTC_OK;
}

static int32_t SoftapServerStart(SoftapSess *sess)
{
    int32_t ret = TransLinkConnect(sess->link);
    if (ret != IOTC_OK) {
        IOTC_LOGW("link connect error %d", ret);
        return ret;
    }

    ret = WifiSchedLinkRecvWatch(sess->link);
    if (ret != IOTC_OK) {
        IOTC_LOGW("link watch error %d", ret);
        return ret;
    }

    sess->coapSource = CoapEndpointEventSourceNew(sess->endpoint);
    if (sess->coapSource == NULL) {
        IOTC_LOGW("create coap source error");
        return IOTC_CORE_WIFI_TRANS_ERR_COAP_ENDPOINT_SOURCE_NEW;
    }

    ret = EventLoopAddSource(GetSchedEventLoop(), sess->coapSource);
    if (ret != IOTC_OK) {
        IOTC_LOGW("add coap source error %d", ret);
        return ret;
    }

    return IOTC_OK;
}

static void StationCheckTimerCallback(int32_t id, void *userData)
{
    NOT_USED(userData);
    SoftapServiceContext *ctx = GetSoftapServiceContext();
    if (ctx == NULL) {
        SchedTimerRemove(id);
        return;
    }

    bool isStaExist = false;
    for (uint32_t i = 0; i < IOTC_CONF_SOFTAP_MAX_PEER_SESS_NUM; ++i) {
        if (!UTILS_IS_BIT_SET(ctx->sess.peerSess[i].bitMap, SOFTAP_PEER_SESS_BIT_MAP_LINK_ACTIVE)) {
            continue;
        }
        isStaExist = true;
        break;
    }

    if (!isStaExist) {
        return;
    }

    AdapterStationList *staList = NULL;
    int32_t ret = AdapterGetSoftapStationInfo(&staList);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get station list error %d", ret);
        return;
    }

    for (uint32_t i = 0; i < IOTC_CONF_SOFTAP_MAX_PEER_SESS_NUM; ++i) {
        SoftapPeerSess *peerSess = &ctx->sess.peerSess[i];
        if (!UTILS_IS_BIT_SET(ctx->sess.peerSess[i].bitMap, SOFTAP_PEER_SESS_BIT_MAP_LINK_ACTIVE)) {
            continue;
        }

        bool isPeerExits = false;
        for (uint32_t j = 0; j < staList->num; ++j) {
            AdapterStationInfo *staInfo = &staList->stationList[j];
            if (peerSess->addrInfo.addr != staInfo->ip ||
                memcmp(peerSess->mac, staInfo->mac, sizeof(peerSess->mac)) != 0) {
                continue;
            }
            isPeerExits = true;
            break;
        }
        if (!isPeerExits) {
            DestroyPeerSession(peerSess);
        }
    }
    AdapterFreeSoftapStationInfo(staList);
    return;
}

static int32_t StartStationCheckTimer(SoftapSess *sess)
{
    sess->staTimer = SchedTimerAdd(EVENT_SOURCE_TIMER_TYPE_REPEAT, StationCheckTimerCallback,
        SOFTAP_STA_CHECK_TIMER_PERIOD, NULL);
    if (sess->staTimer < 0) {
        IOTC_LOGW("start sta check timer error %d", sess->staTimer);
        return sess->staTimer;
    }
    return IOTC_OK;
}

int32_t CreateSoftapSess(SoftapSess *sess, const SoftapSvcInitParam *initParam)
{
    CHECK_RETURN(sess != NULL && initParam != NULL, IOTC_ERR_PARAM_INVALID);

    int32_t ret;
    do {
        /* 1. init recv and send buffer */
        sess->sendBuf = TransCreateSendBuffer();
        sess->recvBuf = TransCreateRecvBuffer();
        if (sess->sendBuf == NULL || sess->recvBuf == NULL) {
            IOTC_LOGW("create buffer error");
            ret = IOTC_CORE_WIFI_NETCFG_ERR_SOFTAP_CREATE_BUFFER;
            break;
        }

        /* 2. create udp socket */
        ret = CreateSoftapLink(sess);
        if (ret != IOTC_OK) {
            break;
        }

        /* 3. create sess for codec/encrypt/decrypt */
        ret = CreateSoftapSession(sess);
        if (ret != IOTC_OK) {
            break;
        }

        /* 4. create coap endpoint for coap msg process */
        ret = CreateSoftapCoapEndpoint(sess, initParam);
        if (ret != IOTC_OK) {
            break;
        }

        /* 5. start server for recv data */
        ret = SoftapServerStart(sess);
        if (ret != IOTC_OK) {
            break;
        }

        /* 6. start timer to check station disconnect */
        ret = StartStationCheckTimer(sess);
        if (ret != IOTC_OK) {
            break;
        }
        return IOTC_OK;
    } while (0);
    /* 异常处理 */
    DestroySoftapSess(sess);
    return ret;
}

void DestroySoftapSess(SoftapSess *sess)
{
    CHECK_V_RETURN(sess != NULL);
    if (sess->staTimer >= 0) {
        SchedTimerRemove(sess->staTimer);
    }
    if (sess->coapSource != NULL) {
        EventLoopDelSource(GetSchedEventLoop(), sess->coapSource);
    }
    if (sess->link != NULL && TransLinkGetFd(sess->link) >= 0) {
        WifiSchedFdRemove(TransLinkGetFd(sess->link));
        TransLinkClose(sess->link);
    }
    if (sess->endpoint != NULL) {
        CoapEndpointFree(sess->endpoint);
    }
    for (uint32_t i = 0; i < IOTC_CONF_SOFTAP_MAX_PEER_SESS_NUM; ++i) {
        DestroyPeerSession(&sess->peerSess[i]);
    }
    if (sess->sess != NULL) {
        TransSessFree(sess->sess);
    }
    if (sess->link != NULL) {
        TransLinkFree(sess->link);
    }
    if (sess->recvBuf != NULL) {
        TransReleaseBuffer(sess->recvBuf);
    }
    if (sess->sendBuf != NULL) {
        TransReleaseBuffer(sess->sendBuf);
    }
    (void)memset_s(sess, sizeof(SoftapSess), 0, sizeof(SoftapSess));
}