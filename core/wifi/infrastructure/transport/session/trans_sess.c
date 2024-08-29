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
    void *nodeData;
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

static MsgCor *MsgCorNew(SessMsgProcess handler, const char *comment, void *nodeData)
{
    MsgCor *cor = IotcMalloc(sizeof(MsgCor));
    if (cor == NULL) {
        IOTC_LOGW("malloc error");
        return NULL;
    }
    (void)memset_s(cor, sizeof(MsgCor), 0, sizeof(MsgCor));
    LIST_INIT(&cor->node);
    cor->handler = handler;
    cor->comment = comment;
    cor->nodeData = nodeData;
    return cor;
}

static int32_t SessionOnLinkRecvData(TransLink *link, UtilsBuffer *data, void *userData, const SocketAddr *addr)
{
    CHECK_RETURN_LOGW(link != NULL && data != NULL && userData != NULL,
        IOTC_ERR_PARAM_INVALID, "param invalid");
    TransSess *sess = (TransSess *)userData;
    return TransSessMsgRecv(sess, data, addr);
}

TransSess *TransSessNew(TransLink *link, uint32_t msgSize, const char *name, void *userData)
{
    CHECK_RETURN_LOGW(link != NULL && msgSize != 0 && msgSize <= TRANS_SESS_MSG_MAX_SIZE,
        NULL, "param invalid");

    TransSess *sess = (TransSess *)IotcMalloc(sizeof(TransSess));
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
    sess->msg = (uint8_t *)IotcCalloc(msgSize, sizeof(uint8_t));
    if (sess->msg == NULL) {
        IOTC_LOGW("calloc error %u", msgSize);
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
    CHECK_V_RETURN(sess != NULL);
    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &sess->recvCor) {
        MsgCor *node = CONTAINER_OF(item, MsgCor, node);
        LIST_REMOVE(item);
        IotcFree(node);
    }
    LIST_FOR_EACH_ITEM_SAFE(item, next, &sess->sendCor) {
        MsgCor *node = CONTAINER_OF(item, MsgCor, node);
        LIST_REMOVE(item);
        IotcFree(node);
    }
    TransLinkUnregDataCallback(sess->link);
    UTILS_FREE_2_NULL(sess->msg);
    IotcFree(sess);
}

static void AddCorListHandler(TransSess *sess, SessMsgProcess handler, const char *comment,
    void *nodeData, CorType type)
{
    MsgCor *cor = MsgCorNew(handler, comment, nodeData);
    CHECK_V_RETURN_LOGW(cor != NULL, "cor node new error");

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

void TransSessAddRecvTailHandler(TransSess *sess, SessMsgProcess next, const char *comment, void *nodeData)
{
    CHECK_V_RETURN_LOGW(sess != NULL && next != NULL, "param invalid");
    AddCorListHandler(sess, next, comment, nodeData, COR_TYPE_RECV_TAIL);
}

void TransSessAddSendTailHandler(TransSess *sess, SessMsgProcess next, const char *comment, void *nodeData)
{
    CHECK_V_RETURN_LOGW(sess != NULL && next != NULL, "param invalid");
    AddCorListHandler(sess, next, comment, nodeData, COR_TYPE_SEND_TAIL);
}

void TransSessAddRecvHeadHandler(TransSess *sess, SessMsgProcess before, const char *comment, void *nodeData)
{
    CHECK_V_RETURN_LOGW(sess != NULL && before != NULL, "param invalid");
    AddCorListHandler(sess, before, comment, nodeData, COR_TYPE_RECV_HEAD);
}

void TransSessAddSendHeadHandler(TransSess *sess, SessMsgProcess before, const char *comment, void *nodeData)
{
    CHECK_V_RETURN_LOGW(sess != NULL && before != NULL, "param invalid");
    AddCorListHandler(sess, before, comment, nodeData, COR_TYPE_SEND_HEAD);
}

void TransSessRemoveHandler(TransSess *sess, SessMsgProcess handler)
{
    CHECK_V_RETURN_LOGW(sess != NULL && handler != NULL, "param invalid");

    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &sess->recvCor) {
        MsgCor *node = CONTAINER_OF(item, MsgCor, node);
        if (node->handler != handler) {
            continue;
        }
        LIST_REMOVE(item);
        IotcFree(node);
    }
    LIST_FOR_EACH_ITEM_SAFE(item, next, &sess->sendCor) {
        MsgCor *node = CONTAINER_OF(item, MsgCor, node);
        if (node->handler != handler) {
            continue;
        }
        LIST_REMOVE(item);
        IotcFree(node);
    }
}

static bool TransSessMsgCorNodeProcess(TransSess *sess, SessMsg *msg,
    UtilsBuffer *buf, const SocketAddr *addr, ListEntry *corList)
{
    const char *nodeCmt = NULL;
    ListEntry *item = NULL;
    SessCode code = SESS_CODE_OK;
    SessAddtlInfo info = { addr, sess->userData, NULL };

    LIST_FOR_EACH_ITEM(item, corList) {
        MsgCor *node = CONTAINER_OF(item, MsgCor, node);
        nodeCmt = node->comment;
        if (node->handler == NULL) {
            IOTC_LOGW("invalid handler");
            code = SESS_CODE_ERR;
            break;
        }

        info.nodeData = node->nodeData;
        code = node->handler(msg, buf, &info);
        if (code != SESS_CODE_CONTINUE) {
            break;
        }
        IOTC_LOGD("sess[%s] process msg [%s] ok", NON_NULL_STR(sess->name), NON_NULL_STR(nodeCmt));
    }

    if (code == SESS_CODE_ERR) {
        IOTC_LOGW("sess[%s] process msg [%s] error", NON_NULL_STR(sess->name), NON_NULL_STR(nodeCmt));
        return false;
    }
    return true;
}

int32_t TransSessMsgSend(TransSess *sess, SessMsg *msg, UtilsBuffer *buf, const SocketAddr *addr)
{
    CHECK_RETURN_LOGW(sess != NULL && msg != NULL && buf != NULL && buf->buffer != NULL &&
        buf->len != 0 && buf->size >= buf->len && buf->size != 0,
        IOTC_ERR_PARAM_INVALID, "param invalid");

    if (TransSessMsgCorNodeProcess(sess, msg, buf, addr, &sess->sendCor)) {
        return TransLinkSendData(sess->link, buf->buffer, buf->len, addr);
    } else {
        IOTC_LOGW("sess cor send error");
        return IOTC_CORE_WIFI_TRANS_ERR_SESS_SEND_COR;
    }
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
    if (TransSessMsgCorNodeProcess(sess, sess->msg, buf, addr, &sess->recvCor)) {
        return IOTC_OK;
    } else {
        IOTC_LOGW("sess cor recv error");
        return IOTC_CORE_WIFI_TRANS_ERR_SESS_SEND_COR;
    }
}