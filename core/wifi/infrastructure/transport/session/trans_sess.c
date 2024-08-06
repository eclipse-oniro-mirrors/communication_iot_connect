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
#include "trans_sess.h"
#include "utils_list.h"
#include "iotc_log.h"
#include "utils_common.h"
#include "securec.h"
#include "utils_assert.h"
#include "iotc_errcode.h"

typedef struct {
    ListEntry node;
    SessMsgProcess handler;
    const char *comment;
    void *corData;
} MsgCor;

struct TransSess {
    const char *name;
    TransLink *link;
    uint32_t msgSize;
    uint8_t *msg;
    /* 收发责任链 */
    ListEntry recvCor;
    ListEntry sendCor;
    void *userData;
};

typedef enum {
    COR_TYPE_RECV_TAIL = 0,
    COR_TYPE_RECV_HEAD,
    COR_TYPE_SEND_TAIL,
    COR_TYPE_SEND_HEAD,
} CorType;

MsgCor *MsgCorNew(SessMsgProcess handler, const char *comment, void *corData)
{
    if (handler == NULL) {
        return NULL;
    }
    MsgCor *cor = AdapterMalloc(sizeof(MsgCor));
    if (cor == NULL) {
        IOTC_LOGW("malloc error");
        return NULL;
    }
    (void)memset_s(cor, sizeof(MsgCor), 0, sizeof(MsgCor));
    LIST_INIT(&cor->node);
    cor->handler = handler;
    cor->comment = comment;
    cor->corData = corData;
    return cor;
}

static int32_t SessionOnLinkRecvData(TransLink *link, UtilsBuffer *data, void *userData, const SocketAddr *addr)
{
    if (link == NULL || data == NULL || userData == NULL) {
        return IOTC_ERR_PARAM_INVALID;
    }
    TransSess *sess = (TransSess *)userData;
    return TransSessMsgRecv(sess, data, addr);
}

TransSess *TransSessNew(TransLink *link, uint32_t msgSize, const char *name, void *userData)
{
    if (link == NULL || msgSize == 0 || msgSize > TRANS_SESS_MSG_MAX_SIZE) {
        return NULL;
    }
    TransSess *sess = (TransSess *)AdapterMalloc(sizeof(TransSess));
    if (sess == NULL) {
        IOTC_LOGW("malloc error");
        return NULL;
    }
    (void)memset_s(sess, sizeof(TransSess), 0, sizeof(TransSess));
    LIST_INIT(&sess->recvCor);
    LIST_INIT(&sess->sendCor);
    sess->userData = userData;
    sess->msgSize = msgSize;
    sess->link = link;
    sess->name = name;
    sess->msg = (uint8_t *)AdapterCalloc(msgSize, sizeof(uint8_t));
    if (sess->msg == NULL) {
        TransSessFree(sess);
        return NULL;
    }

    int32_t ret = TransLinkRegDataCallback(link, SessionOnLinkRecvData, sess);
    if (ret != IOTC_OK) {
        IOTC_LOGW("reg data callback error");
        TransSessFree(sess);
        return NULL;
    }

    return sess;
}

void TransSessFree(TransSess *sess)
{
    if (sess == NULL) {
        return;
    }
    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &sess->recvCor) {
        MsgCor *node = CONTAINER_OF(item, MsgCor, node);
        LIST_REMOVE(item);
        AdapterFree(node);
    }
    LIST_FOR_EACH_ITEM_SAFE(item, next, &sess->sendCor) {
        MsgCor *node = CONTAINER_OF(item, MsgCor, node);
        LIST_REMOVE(item);
        AdapterFree(node);
    }
    TransLinkUnregDataCallback(sess->link);
    UTILS_FREE_2_NULL(sess->msg);
    AdapterFree(sess);
}

static void AddCorListHandler(TransSess *sess, SessMsgProcess handler, const char *comment, void *corData, CorType type)
{
    MsgCor *cor = MsgCorNew(handler, comment, corData);
    if (cor == NULL) {
        return;
    }
    switch (type) {
        case COR_TYPE_RECV_TAIL:
            LIST_INSERT_BEFORE(&cor->node, &sess->recvCor);
            return;
        case COR_TYPE_RECV_HEAD:
            LIST_INSERT(&cor->node, &sess->recvCor);
            return;
        case COR_TYPE_SEND_TAIL:
            LIST_INSERT_BEFORE(&cor->node, &sess->sendCor);
            return;
        case COR_TYPE_SEND_HEAD:
            LIST_INSERT(&cor->node, &sess->sendCor);
            return;
        default:
            return;
    };
}

void TransSessAddTailRecvHandler(TransSess *sess, SessMsgProcess next, const char *comment, void *corData)
{
    if (sess == NULL || next == NULL) {
        return;
    }
    AddCorListHandler(sess, next, comment, corData, COR_TYPE_RECV_TAIL);
}

void TransSessAddTailSendHandler(TransSess *sess, SessMsgProcess next, const char *comment, void *corData)
{
    if (sess == NULL || next == NULL) {
        return;
    }
    AddCorListHandler(sess, next, comment, corData, COR_TYPE_SEND_TAIL);
}

void TransSessAddHeadRecvHandler(TransSess *sess, SessMsgProcess before, const char *comment, void *corData)
{
    if (sess == NULL || before == NULL) {
        return;
    }
    AddCorListHandler(sess, before, comment, corData, COR_TYPE_RECV_HEAD);
}

void TransSessAddHeadSendHandler(TransSess *sess, SessMsgProcess before, const char *comment, void *corData)
{
    if (sess == NULL || before == NULL) {
        return;
    }
    AddCorListHandler(sess, before, comment, corData, COR_TYPE_SEND_HEAD);
}

void TransSessRemoveHandler(TransSess *sess, SessMsgProcess handler)
{
    if (sess == NULL || handler == NULL) {
        return;
    }
    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &sess->recvCor) {
        MsgCor *node = CONTAINER_OF(item, MsgCor, node);
        if (node->handler != handler) {
            continue;
        }
        LIST_REMOVE(item);
        AdapterFree(node);
    }
    LIST_FOR_EACH_ITEM_SAFE(item, next, &sess->sendCor) {
        MsgCor *node = CONTAINER_OF(item, MsgCor, node);
        if (node->handler != handler) {
            continue;
        }
        LIST_REMOVE(item);
        AdapterFree(node);
    }
}

int32_t TransSessMsgSend(TransSess *sess, SessMsg *msg, UtilsBuffer *buf, const SocketAddr *addr)
{
    if (sess == NULL || msg == NULL || buf == NULL || buf->buffer == NULL ||
        buf->len == 0 || buf->size < buf->len || buf->size == 0) {
        IOTC_LOGW("param invalid");
        return IOTC_ERR_PARAM_INVALID;
    }
    const char *nodeCmt = NULL;
    ListEntry *item = NULL;
    SessCode code = SESS_CODE_OK;
    LIST_FOR_EACH_ITEM(item, &sess->sendCor) {
        MsgCor *node = CONTAINER_OF(item, MsgCor, node);
        nodeCmt = node->comment;
        if (node->handler == NULL) {
            code = SESS_CODE_ERR;
            break;
        }
        SessAddtlInfo info = { addr, sess->userData, node->corData };
        code = node->handler(msg, buf, &info);
        if (code != SESS_CODE_CONTINUE) {
            break;
        }
        IOTC_LOGD("sess [%s] send msg [%s] ok", NON_NULL_STR(sess->name), NON_NULL_STR(nodeCmt));
    }

    if (code == SESS_CODE_ERR) {
        IOTC_LOGW("sess [%s] send msg [%s] error", NON_NULL_STR(sess->name), NON_NULL_STR(nodeCmt));
        return IOTC_CORE_WIFI_TRANS_ERR_SESS_RECV_COR;
    }
    
    return TransLinkSendData(sess->link, buf->buffer, buf->len, addr);
}

int32_t TransSessSendRaw(TransSess *sess, const CommData *data, const SocketAddr *addr)
{
    CHECK_RETURN_LOGW(sess != NULL && data != NULL && data->data != NULL && data->len != 0,
        IOTC_ERR_PARAM_INVALID, "param invalid");
    
    return TransLinkSendData(sess->link, data->data, data->len, addr);
}

int32_t TransSessMsgRecv(TransSess *sess, UtilsBuffer *buf, const SocketAddr *addr)
{
    CHECK_RETURN_LOGW(sess != NULL && buf != NULL && buf->buffer != NULL && buf->size != 0 &&
        buf->len < buf->size, IOTC_ERR_PARAM_INVALID, "param invalid");

    if (sess->msg == NULL || sess->msgSize == 0) {
        IOTC_LOGW("invalid sess msg %u", sess->msgSize);
        return IOTC_ERR_PARAM_INVALID;
    }

    (void)memset_s(sess->msg, sess->msgSize, 0, sess->msgSize);
    const char *nodeCmt = NULL;
    ListEntry *item = NULL;
    SessCode code = SESS_CODE_OK;
    LIST_FOR_EACH_ITEM(item, &sess->recvCor) {
        MsgCor *node = CONTAINER_OF(item, MsgCor, node);
        nodeCmt = node->comment;
        if (node->handler == NULL) {
            code = SESS_CODE_ERR;
            break;
        }
        SessAddtlInfo info = { addr, sess->userData, node->corData };
        code = node->handler(sess->msg, buf, &info);
        if (code != SESS_CODE_CONTINUE) {
            break;
        }
        IOTC_LOGD("sess [%s] recv msg [%s] ok", NON_NULL_STR(sess->name), NON_NULL_STR(nodeCmt));
    }

    if (code == SESS_CODE_ERR) {
        IOTC_LOGW("sess [%s] recv msg [%s] error", NON_NULL_STR(sess->name), NON_NULL_STR(nodeCmt));
        return IOTC_CORE_WIFI_TRANS_ERR_SESS_RECV_COR;
    }
    return IOTC_OK;
}