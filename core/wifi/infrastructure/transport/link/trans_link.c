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
#include "trans_link.h"
#include "iotc_log.h"
#include "iotc_mem.h"
#include "securec.h"
#include "utils_common.h"
#include "utils_bit_map.h"
#include "iotc_os.h"
#include "utils_assert.h"
#include "iotc_errcode.h"

struct TransLink {
    int32_t fd;
    uint32_t recvTmo;
    uint32_t sendTmo;
    OnLinkError errFunc;
    void *userErrData;
    OnLinkRecvData dataFunc;
    void *userData;
    TransSocket *socket;
    UtilsBufferCtx *buffer;
    const char *name;
};

#define TRANS_LINK_DEFAULT_SEND_TIMEOUT UTILS_SEC_TO_MS(10)
#define TRANS_LINK_DEFAULT_RECV_TIMEOUT UTILS_SEC_TO_MS(10)

TransLink *TransLinkNew(TransSocket *socket, UtilsBufferCtx *buffer, const char *name)
{
    CHECK_RETURN_LOGW(socket != NULL && buffer != NULL, NULL, "param invalid");

    TransLink *link = (TransLink *)IotcMalloc(sizeof(TransLink));
    if (link == NULL) {
        IOTC_LOGW("malloc error");
        return NULL;
    }
    (void)memset_s(link, sizeof(TransLink), 0, sizeof(TransLink));

    link->socket = socket;
    link->buffer = buffer;
    link->name = name;
    link->fd = TRANS_SOCKET_INVALID_FD;
    link->recvTmo = TRANS_LINK_DEFAULT_RECV_TIMEOUT;
    link->sendTmo = TRANS_LINK_DEFAULT_SEND_TIMEOUT;
    IOTC_LOGD("link[%s] create", NON_NULL_STR(link->name));
    return link;
}

void TransLinkFree(TransLink *link)
{
    CHECK_V_RETURN_LOGW(link != NULL, "param invalid");
    TransLinkClose(link);
    TransSocketFree(link->socket);
    link->socket = NULL;
    IOTC_LOGD("link[%s] destroy", NON_NULL_STR(link->name));
    IotcFree(link);
}

int32_t TransLinkRegErrorCallback(TransLink *link, OnLinkError cb, void *userData)
{
    CHECK_RETURN_LOGW(link != NULL && cb != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    CHECK_RETURN_LOGW(link->errFunc == NULL, IOTC_CORE_WIFI_TRANS_ERR_LINK_HAVE_CALLBACK,
        "err cb exist");

    link->errFunc = cb;
    link->userErrData = userData;
    return IOTC_OK;
}

int32_t TransLinkRegDataCallback(TransLink *link, OnLinkRecvData cb, void *userData)
{
    CHECK_RETURN_LOGW(link != NULL && cb != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    CHECK_RETURN_LOGW(link->dataFunc == NULL, IOTC_CORE_WIFI_TRANS_ERR_LINK_HAVE_CALLBACK,
        "data cb exist");
    link->dataFunc = cb;
    link->userData = userData;
    return IOTC_OK;
}

void TransLinkUnregErrorCallback(TransLink *link)
{
    CHECK_V_RETURN(link != NULL);
    link->errFunc = NULL;
    link->userErrData = NULL;
}

void TransLinkUnregDataCallback(TransLink *link)
{
    CHECK_V_RETURN(link != NULL);
    link->dataFunc = NULL;
    link->userData = NULL;
}

void TransLinkConfTimeout(TransLink *link, uint32_t recvTmo, uint32_t sendTmo)
{
    CHECK_V_RETURN_LOGW(link != NULL, "param invalid");
    link->recvTmo = recvTmo;
    link->sendTmo = sendTmo;
    IOTC_LOGD("link[%s] set tmo %u/%u", NON_NULL_STR(link->name), recvTmo, sendTmo);
}

int32_t TransLinkConnect(TransLink *link)
{
    CHECK_RETURN_LOGW(link != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    int32_t ret = TransSocketConnect(link->socket);
    if (ret == IOTC_OK) {
        IOTC_LOGI("link[%s] connect ok", NON_NULL_STR(link->name));
    } else {
        IOTC_LOGW("link[%s] connect error %d", NON_NULL_STR(link->name), ret);
    }

    return ret;
}

int32_t TransLinkGetFd(TransLink *link)
{
    CHECK_RETURN_LOGW(link != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    return TransSocketGetFd(link->socket);
}

void TransLinkClose(TransLink *link)
{
    CHECK_V_RETURN_LOGW(link != NULL, "param invalid");
    TransSocketClose(link->socket);
    IOTC_LOGD("link[%s] close", NON_NULL_STR(link->name));
}

static int32_t TransLinkReadDataFromSocket(TransLink *link, UtilsBuffer *buf, SocketAddr *addr)
{
    int32_t ret = TransSocketRecv(link->socket, link->recvTmo, buf, addr);
    if (ret == 0) {
        return IOTC_OK;
    } else if (ret < 0) {
        IOTC_LOGW("link[%s] recv error, ret=%d", NON_NULL_STR(link->name), ret);
        return ret;
    }

    /* ret大于0表示当前buffer不足以接收完整数据，其值表示需要扩充的长度 */
    if (buf->len > buf->size || ret > UINT32_MAX - buf->len || ret < buf->size - buf->len) {
        IOTC_LOGW("link[%s] recv invalid, len=%u, size=%u", NON_NULL_STR(link->name), buf->len, buf->size);
        return IOTC_CORE_WIFI_TRANS_ERR_SOCKET_RECV_INVALID;
    }

    uint32_t expectSize = ret + buf->len;
    ret = UtilsExpendBuffer(link->buffer, buf, expectSize);
    if (ret != IOTC_OK) {
        IOTC_LOGW("link[%s] expend buffer error, ret=%d, expect=%u", NON_NULL_STR(link->name),
            ret, expectSize);
        return ret;
    }

    ret = TransSocketRecv(link->socket, link->recvTmo, buf, addr);
    if (ret != 0) {
        IOTC_LOGW("link[%s] recv remain error, ret=%d", NON_NULL_STR(link->name), ret);
        return ret;
    }
    return IOTC_OK;
}

void TransLinkRecvReadEvent(TransLink *link)
{
    CHECK_V_RETURN_LOGW(link != NULL, "param invalid");
    UtilsBuffer *buf = UtilsGetBuffer(link->buffer);
    if (buf == NULL) {
        IOTC_LOGW("link[%s] get buffer error", NON_NULL_STR(link->name));
        return;
    }

    SocketAddr addr = { 0, 0 };
    int32_t ret = TransLinkReadDataFromSocket(link, buf, &addr);
    if (ret != IOTC_OK) {
        IOTC_LOGW("link[%s] read data error, ret=%d", NON_NULL_STR(link->name), ret);
        TransLinkRecvErrorEvent(link);
        return;
    }

    if (link->dataFunc != NULL) {
        ret = link->dataFunc(link, buf, link->userData, &addr);
        if (ret != IOTC_OK) {
            IOTC_LOGW("link[%s] data process error, ret=%d", NON_NULL_STR(link->name), ret);
            TransLinkRecvErrorEvent(link);
        }
    } else {
        /* 无数据处理函数不影响数据接收，以防数据堆积 */
        IOTC_LOGW("link[%s] no data cb", NON_NULL_STR(link->name));
    }

    UtilsReleaseBuffer(link->buffer, &buf);
    return;
}

void TransLinkRecvErrorEvent(TransLink *link)
{
    CHECK_V_RETURN_LOGW(link != NULL, "param invalid");
    if (link->errFunc != NULL) {
        link->errFunc(link, link->userErrData);
    }
}

int32_t TransLinkSendData(TransLink *link, const uint8_t *data, uint32_t len, const SocketAddr *addr)
{
    CHECK_RETURN_LOGW(link != NULL && data != NULL && len != 0, IOTC_ERR_PARAM_INVALID, "param invalid");
    CommData dataObj = { data, len };
    int32_t ret = TransSocketSend(link->socket, link->sendTmo, &dataObj, addr);
    if (ret != IOTC_OK) {
        IOTC_LOGW("link[%s] send error %d/%u", NON_NULL_STR(link->name), ret, len);
        TransLinkRecvErrorEvent(link);
    }
    return ret;
}