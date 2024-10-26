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
#include "service_manager.h"
#include "service_proxy.h"
#include "utils_list.h"
#include "utils_mutex_global.h"
#include "securec.h"
#include "iotc_errcode.h"
#include "iotc_mem.h"
#include "iotc_log.h"
#include "utils_assert.h"
#include "utils_common.h"
#include "utils_bit_map.h"
#include "utils_mutex_ex.h"
#include "security_random.h"

typedef enum {
    SERVICE_ID = 0,
    INSTANCE_ID,
} ServiceIdType;
typedef enum {
    SERVICE_BIT_MAP_RUNNING = 0,
} ServiceBitMap;

typedef struct {
    ListEntry node;
    int32_t serviceId;
    ServiceMessageHandler subHandler;
} MessageSubNode;

typedef struct {
    ListEntry node;
    int32_t messageId;
    uint32_t subNum;
    ListEntry subList;
} MessageNode;

typedef struct {
    ListEntry node;
    int32_t serviceId;
    int32_t instanceId;
    uint32_t bitMap;
    const char *name;
    const ServiceHandler *handler;
    const void *apiHandler;
    int32_t *depends;
    uint32_t depNum;
    uint32_t msgNum;
    ListEntry msgList;
} ServiceNode;

typedef struct {
    int32_t instanceIdBase;
    UtilsExMutex *mutex;
    ListEntry serviceList;
    ServiceAsyncExecutor executor;
} ServiceManagerContext;

typedef struct {
    ServiceRequestInfo reqInfo;
    ServiceMessageFreeHandler freeHandler;
    ServiceResponseHandler respHandler;
} ServiceAsyncMsgInfo;

static ServiceManagerContext *GetSvcMngrCtx(void)
{
    static ServiceManagerContext ctx;
    return &ctx;
}

#define LOCK_CTX_RETURN(ctx) \
    do { \
        if (!UtilsExMutexLock((ctx)->mutex)) { \
            IOTC_LOGW("lock error"); \
            return IOTC_ERR_TIMEOUT; \
        } \
    } while (0)

#define LOCK_CTX_V_RETURN(ctx) \
    do { \
        if (!UtilsExMutexLock((ctx)->mutex)) { \
            IOTC_LOGW("lock error"); \
            return; \
        } \
    } while (0)

#define UNLOCK_CTX(ctx) UtilsExMutexUnlock((ctx)->mutex)

#define SERVICE_NODE_LOGW_CODE(tag, code, node) IOTC_LOGW("%s %d, svc:%s/%d/%d", \
    tag, (code), NON_NULL_STR((node)->name), (node)->serviceId, (node)->instanceId)

#define SERVICE_NODE_LOGW(tag, node) IOTC_LOGW("%s, svc:%s/%d/%d", \
    tag, NON_NULL_STR((node)->name), (node)->serviceId, (node)->instanceId)

#define SERVICE_NODE_LOGI(tag, node) IOTC_LOGI("%s, svc:%s/%d/%d", \
    tag, NON_NULL_STR((node)->name), (node)->serviceId, (node)->instanceId)

static MessageNode *GetMessageNode(ServiceNode *svcNode, int32_t mid)
{
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &svcNode->msgList) {
        MessageNode *node = CONTAINER_OF(item, MessageNode, node);
        if (node->messageId != mid) {
            continue;
        }
        return node;
    }
    return  NULL;
}

static ServiceNode *GetServiceNode(ServiceManagerContext *ctx, int32_t id, ServiceIdType type)
{
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &ctx->serviceList) {
        ServiceNode *node = CONTAINER_OF(item, ServiceNode, node);
        if (type == SERVICE_ID && id != node->serviceId) {
            continue;
        } else if (type == INSTANCE_ID && id != node->instanceId) {
            continue;
        }

        return node;
    }
    return  NULL;
}

static void ClearServiceMsgSub(ServiceNode *svcNode, int32_t serviceId)
{
    ListEntry *msgItem = NULL;
    LIST_FOR_EACH_ITEM(msgItem, &svcNode->msgList) {
        MessageNode *msgNode = CONTAINER_OF(msgItem, MessageNode, node);
        if (msgNode->subNum == 0) {
            continue;
        }
        ListEntry *next = NULL;
        ListEntry *subItem = NULL;
        LIST_FOR_EACH_ITEM_SAFE(subItem, next, &msgNode->subList) {
            MessageSubNode *subNode = CONTAINER_OF(subItem, MessageSubNode, node);
            if (subNode->serviceId != serviceId) {
                continue;
            }
            LIST_REMOVE(subItem);
            IotcFree(subNode);
            msgNode->subNum = msgNode->subNum > 0 ? msgNode->subNum - 1 : 0;
        }
    }
}

static void ClearServiceAllMsgSub(ServiceManagerContext *ctx, int32_t serviceId)
{
    ListEntry *svcItem = NULL;
    LIST_FOR_EACH_ITEM(svcItem, &ctx->serviceList) {
        ServiceNode *svcNode = CONTAINER_OF(svcItem, ServiceNode, node);
        if (svcNode->serviceId == serviceId || svcNode->msgNum == 0) {
            continue;
        }
        ClearServiceMsgSub(svcNode, serviceId);
    }
}

static void OnServiceFinish(int32_t instanceId)
{
    ServiceManagerContext *ctx = GetSvcMngrCtx();
    LOCK_CTX_V_RETURN(ctx);
    ServiceNode *svcNode = GetServiceNode(ctx, instanceId, INSTANCE_ID);
    if (svcNode == NULL) {
        UNLOCK_CTX(ctx);
        IOTC_LOGW("invalid service instance %d", instanceId);
        return;
    }
    UTILS_BIT_RESET(svcNode->bitMap, SERVICE_BIT_MAP_RUNNING);
    ClearServiceAllMsgSub(ctx, svcNode->serviceId);
    SERVICE_NODE_LOGI("service finish", svcNode);
    UNLOCK_CTX(ctx);
}

int32_t ServiceProxyStartService(int32_t serviceId, void *param)
{
    ServiceManagerContext *ctx = GetSvcMngrCtx();
    LOCK_CTX_RETURN(ctx);
    ServiceNode *svcNode = GetServiceNode(ctx, serviceId, SERVICE_ID);
    if (svcNode == NULL) {
        UNLOCK_CTX(ctx);
        IOTC_LOGW("invalid service %d", serviceId);
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_INVALID;
    }
    if (UTILS_IS_BIT_SET(svcNode->bitMap, SERVICE_BIT_MAP_RUNNING)) {
        SERVICE_NODE_LOGI("service already start", svcNode);
        UNLOCK_CTX(ctx);
        return IOTC_OK;
    }

    int32_t instanceId = svcNode->instanceId;
    const char *name = svcNode->name;
    const ServiceHandler *handler = svcNode->handler;
    svcNode = NULL;
    UNLOCK_CTX(ctx);

    if (handler->onStart == NULL) {
        IOTC_LOGW("service no start handler %s/%d/%d", NON_NULL_STR(name), serviceId, instanceId);
        return IOTC_ERR_CALLBACK_NULL;
    }
    int32_t ret = handler->onStart(instanceId, OnServiceFinish, param);
    if (ret != IOTC_OK) {
        IOTC_LOGW("service start error %d/%s/%d/%d", ret, NON_NULL_STR(name), serviceId, instanceId);
    }

    LOCK_CTX_RETURN(ctx);
    svcNode = GetServiceNode(ctx, serviceId, SERVICE_ID);
    if (svcNode == NULL) {
        UNLOCK_CTX(ctx);
        IOTC_LOGW("invalid service %d", serviceId);
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_INVALID;
    }
    if (UTILS_IS_BIT_SET(svcNode->bitMap, SERVICE_BIT_MAP_RUNNING)) {
        SERVICE_NODE_LOGI("service already start", svcNode);
        UNLOCK_CTX(ctx);
        return IOTC_OK;
    }
    if (ret == IOTC_OK) {
        SERVICE_NODE_LOGI("service start success", svcNode);
        UTILS_BIT_SET(svcNode->bitMap, SERVICE_BIT_MAP_RUNNING);
    }
    UNLOCK_CTX(ctx);
    return ret;
}

int32_t ServiceProxyStopService(int32_t serviceId, void *param)
{
    ServiceManagerContext *ctx = GetSvcMngrCtx();
    LOCK_CTX_RETURN(ctx);
    ServiceNode *svcNode = GetServiceNode(ctx, serviceId, SERVICE_ID);
    if (svcNode == NULL) {
        UNLOCK_CTX(ctx);
        IOTC_LOGW("invalid service %d", serviceId);
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_INVALID;
    }

    if (!UTILS_IS_BIT_SET(svcNode->bitMap, SERVICE_BIT_MAP_RUNNING)) {
        SERVICE_NODE_LOGI("service already stop", svcNode);
        UNLOCK_CTX(ctx);
        return IOTC_OK;
    }

    int32_t instanceId = svcNode->instanceId;
    const char *name = svcNode->name;
    const ServiceHandler *handler = svcNode->handler;
    svcNode = NULL;
    UNLOCK_CTX(ctx);

    if (handler->onStop == NULL) {
        IOTC_LOGW("service no stop handler %s/%d/%d", NON_NULL_STR(name), serviceId, instanceId);
        return IOTC_ERR_CALLBACK_NULL;
    }
    int32_t ret = handler->onStop(instanceId, param);
    if (ret != IOTC_OK) {
        IOTC_LOGW("service stop error %d/%s/%d/%d", ret, NON_NULL_STR(name), serviceId, instanceId);
    }
    return ret;
}

int32_t ServiceProxyResetService(int32_t serviceId, void *param)
{
    ServiceManagerContext *ctx = GetSvcMngrCtx();
    LOCK_CTX_RETURN(ctx);
    ServiceNode *svcNode = GetServiceNode(ctx, serviceId, SERVICE_ID);
    if (svcNode == NULL) {
        UNLOCK_CTX(ctx);
        IOTC_LOGW("invalid service %d", serviceId);
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_INVALID;
    }

    if (!UTILS_IS_BIT_SET(svcNode->bitMap, SERVICE_BIT_MAP_RUNNING)) {
        SERVICE_NODE_LOGI("service not running", svcNode);
        UNLOCK_CTX(ctx);
        return IOTC_OK;
    }

    int32_t instanceId = svcNode->instanceId;
    const char *name = svcNode->name;
    const ServiceHandler *handler = svcNode->handler;
    svcNode = NULL;
    UNLOCK_CTX(ctx);

    if (handler->onReset == NULL) {
        IOTC_LOGW("service no reset handler %s/%d/%d", NON_NULL_STR(name), serviceId, instanceId);
        return IOTC_ERR_CALLBACK_NULL;
    }
    int32_t ret = handler->onReset(instanceId, param);
    if (ret != IOTC_OK) {
        IOTC_LOGW("service reset error %d/%s/%d/%d", ret, NON_NULL_STR(name), serviceId, instanceId);
    }
    return ret;
}

static MessageSubNode *MessageSubNodeNew(int32_t serviceId, ServiceMessageHandler subHandler)
{
    MessageSubNode *newNode = (MessageSubNode *)IotcMalloc(sizeof(MessageSubNode));
    if (newNode == NULL) {
        IOTC_LOGW("malloc error");
        return NULL;
    }
    (void)memset_s(newNode, sizeof(MessageSubNode), 0, sizeof(MessageSubNode));
    newNode->serviceId = serviceId;
    newNode->subHandler = subHandler;
    return newNode;
}

int32_t ServiceProxyGetApiHandler(int32_t serviceId, const void **apiHandler)
{
    CHECK_RETURN_LOGW(apiHandler != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    ServiceManagerContext *ctx = GetSvcMngrCtx();
    LOCK_CTX_RETURN(ctx);

    ServiceNode *svc = GetServiceNode(ctx, serviceId, SERVICE_ID);
    if (svc == NULL || !UTILS_IS_BIT_SET(svc->bitMap, SERVICE_BIT_MAP_RUNNING)) {
        UNLOCK_CTX(ctx);
        IOTC_LOGW("invalid service %d", serviceId);
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_INSTANCE_INVALID;
    }

    *apiHandler = svc->apiHandler;
    UNLOCK_CTX(ctx);
    if (*apiHandler == NULL) {
        IOTC_LOGW("svc %d no api", serviceId);
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_NO_API;
    }
    return IOTC_OK;
}

int32_t ServiceProxySubscribeMessage(int32_t instanceId, int32_t serviceId,
    int32_t msgId, ServiceMessageHandler handler)
{
    CHECK_RETURN_LOGW(handler != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    ServiceManagerContext *ctx = GetSvcMngrCtx();
    LOCK_CTX_RETURN(ctx);

    ServiceNode *srcSvc = GetServiceNode(ctx, instanceId, INSTANCE_ID);
    if (srcSvc == NULL) {
        UNLOCK_CTX(ctx);
        IOTC_LOGW("invalid instance %d", instanceId);
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_INSTANCE_INVALID;
    }
    if (srcSvc->serviceId == serviceId) {
        UNLOCK_CTX(ctx);
        IOTC_LOGW("sub self %d", instanceId);
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_SUB_MSG_SELF;
    }
    ServiceNode *dstSvc = GetServiceNode(ctx, serviceId, SERVICE_ID);
    if (dstSvc == NULL) {
        UNLOCK_CTX(ctx);
        IOTC_LOGW("invalid service to sub %d", serviceId);
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_INVALID;
    }
    MessageNode *msg = GetMessageNode(dstSvc, msgId);
    if (msg == NULL) {
        UNLOCK_CTX(ctx);
        IOTC_LOGW("invalid msg to sub %d/%d", serviceId, msgId);
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_MESSAGE_INVALID;
    }
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &msg->subList) {
        MessageSubNode *node = CONTAINER_OF(item, MessageSubNode, node);
        if (node->serviceId != srcSvc->serviceId) {
            continue;
        }
        IOTC_LOGI("already sub %d/%d/%d", srcSvc->serviceId, serviceId, msgId);
        UNLOCK_CTX(ctx);
        return IOTC_OK;
    }
    MessageSubNode *newSubNode = MessageSubNodeNew(srcSvc->serviceId, handler);
    if (newSubNode == NULL) {
        UNLOCK_CTX(ctx);
        return IOTC_ADAPTER_MEM_ERR_CALLOC;
    }
    LIST_INSERT_BEFORE(&newSubNode->node, &msg->subList);
    msg->subNum++;
    IOTC_LOGI("service [%s/%d] sub [%s/%d] msg %d", NON_NULL_STR(srcSvc->name), srcSvc->serviceId,
        NON_NULL_STR(dstSvc->name), dstSvc->serviceId, msgId);
    UNLOCK_CTX(ctx);
    return IOTC_OK;
}

int32_t ServiceProxyRemoveSubscribe(int32_t instanceId, int32_t serviceId, int32_t msgId)
{
    ServiceManagerContext *ctx = GetSvcMngrCtx();
    LOCK_CTX_RETURN(ctx);

    ServiceNode *srcSvc = GetServiceNode(ctx, instanceId, INSTANCE_ID);
    if (srcSvc == NULL) {
        UNLOCK_CTX(ctx);
        IOTC_LOGW("invalid instance %d", instanceId);
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_INSTANCE_INVALID;
    }
    ServiceNode *dstSvc = GetServiceNode(ctx, serviceId, SERVICE_ID);
    if (dstSvc == NULL) {
        UNLOCK_CTX(ctx);
        IOTC_LOGW("invalid service to remove sub %d", serviceId);
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_INVALID;
    }
    MessageNode *msg = GetMessageNode(dstSvc, msgId);
    if (msg == NULL) {
        UNLOCK_CTX(ctx);
        IOTC_LOGW("invalid msg to remove sub %d/%d", serviceId, msgId);
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_MESSAGE_INVALID;
    }
    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &msg->subList) {
        MessageSubNode *node = CONTAINER_OF(item, MessageSubNode, node);
        if (node->serviceId != srcSvc->serviceId) {
            continue;
        }
        LIST_REMOVE(item);
        IotcFree(node);
        msg->subNum = msg->subNum > 0 ? msg->subNum - 1 : 0;
        IOTC_LOGI("remove sub %d/%d/%d", srcSvc->serviceId, serviceId, msgId);
        break;
    }
    UNLOCK_CTX(ctx);
    return IOTC_OK;
}

static int32_t GetMsgSubList(ServiceNode *svcNode, int32_t msgId,
    ServiceMessageHandler **subHandler, ServiceMessage **respMsg, uint32_t *num)
{
    MessageNode *msgNode = GetMessageNode(svcNode, msgId);
    if (msgNode == NULL)  {
        IOTC_LOGW("invalid msgid %d/%d/%d", svcNode->instanceId, svcNode->serviceId, msgId);
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_MESSAGE_INVALID;
    }

    *num = msgNode->subNum;
    if (*num == 0) {
        IOTC_LOGI("msg no sub %d", msgNode->messageId);
        return IOTC_OK;
    }
    *subHandler = (ServiceMessageHandler *)IotcCalloc(*num, sizeof(ServiceMessageHandler));
    if (*subHandler == NULL) {
        IOTC_LOGW("calloc error %u", *num);
        return IOTC_ADAPTER_MEM_ERR_CALLOC;
    }
    *respMsg = (ServiceMessage *)IotcCalloc(*num, sizeof(ServiceMessage));
    if (*respMsg == NULL) {
        IotcFree(*subHandler);
        *subHandler = NULL;
        IOTC_LOGW("calloc error %u", *num);
        return IOTC_ADAPTER_MEM_ERR_CALLOC;
    }

    uint32_t i = 0;
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &msgNode->subList) {
        if (i >= *num) {
            break;
        }
        MessageSubNode *node = CONTAINER_OF(item, MessageSubNode, node);
        (*subHandler)[i] = node->subHandler;
        (*respMsg)[i].msgId = msgId;
        (*respMsg)[i].serviceId = node->serviceId;
        (*respMsg)[i++].type = SERVICE_MESSAGE_TYPE_RESPONSE;
    }

    return IOTC_OK;
}

static int32_t ResponseMessageSort(ServiceMessage *resp, uint32_t num, uint32_t *respNum)
{
    uint32_t index = num;
    uint32_t finalNum = 0;
    int32_t ret;

    for (uint32_t i = 0; i < num; ++i) {
        if (resp[i].msg == NULL) {
            if (index == num) {
                index = i;
            }
            continue;
        }

        ++finalNum;
        if (index >= i) {
            continue;
        }

        ret = memcpy_s(resp + index, sizeof(ServiceMessage), resp + i, sizeof(ServiceMessage));
        if (ret != EOK) {
            return IOTC_ERR_SECUREC_MEMCPY;
        }
        (void)memset_s(resp + i, sizeof(ServiceMessage), 0, sizeof(ServiceMessage));
    }
    *respNum = finalNum;
    return IOTC_OK;
}

static void SendMsgToSubHandler(const ServiceMessage *reqMsg, ServiceMessageHandler *subHandler,
    ServiceMessage *respMsg, uint32_t num)
{
    int32_t ret;
    ServiceResponseInfo respInfo;
    for (uint32_t i = 0 ; i < num; ++i) {
        (void)memset_s(&respInfo, sizeof(ServiceResponseInfo), 0, sizeof(ServiceResponseInfo));
        IOTC_LOGD("svc %d send msg %d send to %d", reqMsg->serviceId, reqMsg->msgId, respMsg[i].serviceId);
        if (subHandler[i] == NULL) {
            IOTC_LOGW("subHandler[%u] is null!!", i);
            continue;
        }
        ret = subHandler[i](reqMsg, &respInfo);
        if (ret != IOTC_OK) {
            IOTC_LOGW("svc %d send msg %d to %d ret %d", reqMsg->serviceId, reqMsg->msgId,
                respMsg[i].serviceId, ret);
            continue;
        }
        if (respInfo.resp == NULL) {
            continue;
        }
        respMsg[i].msg = respInfo.resp;
        respMsg[i].ctx = respInfo.freeHandler;
    }
}

/* resp和respNum可以为空 */
int32_t ServiceProxySendMessage(const ServiceRequestInfo *req, ServiceMessage **resp, uint32_t *respNum)
{
    CHECK_RETURN_LOGW(req != NULL && req->req != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    ServiceManagerContext *ctx = GetSvcMngrCtx();
    LOCK_CTX_RETURN(ctx);

    ServiceNode *srcSvc = GetServiceNode(ctx, req->instanceId, INSTANCE_ID);
    if (srcSvc == NULL) {
        UNLOCK_CTX(ctx);
        IOTC_LOGW("invalid instance %d", req->instanceId);
        return IOTC_CORE_COMM_FWK_ERR_SERVICE_INSTANCE_INVALID;
    }

    ServiceMessageHandler *subHandler = NULL;
    ServiceMessage *respMsg = NULL;
    uint32_t num = 0;
    int32_t ret = GetMsgSubList(srcSvc, req->msgId, &subHandler, &respMsg, &num);
    if (ret != IOTC_OK || num == 0) {
        UNLOCK_CTX(ctx);
        return ret;
    }

    ServiceMessage reqMsg = { srcSvc->serviceId, req->msgId, SERVICE_MESSAGE_TYPE_REQUEST, req->req, NULL };
    UNLOCK_CTX(ctx);
    SendMsgToSubHandler(&reqMsg, subHandler, respMsg, num);
    UTILS_FREE_2_NULL(subHandler);
    if (resp == NULL || respNum == NULL) {
        ServiceProxyFreeResponseMessage(respMsg, num);
        return IOTC_OK;
    }

    ret = ResponseMessageSort(respMsg, num, respNum);
    if (ret != IOTC_OK || *respNum == 0) {
        IOTC_LOGD("msg resp sort %d/%u", ret, *respNum);
        ServiceProxyFreeResponseMessage(respMsg, num);
        *respNum = 0;
        return ret;
    }
    *resp = respMsg;
    IOTC_LOGD("service %d send msg %d recv %u resp", reqMsg.serviceId, reqMsg.msgId, *respNum);

    return IOTC_OK;
}

static void ServiceAsyncHandler(void *userData)
{
    CHECK_V_RETURN_LOGW(userData != NULL, "param invalid");

    ServiceAsyncMsgInfo *asyncParam = (ServiceAsyncMsgInfo *)userData;

    int32_t ret;
    ServiceMessage *resp = NULL;
    uint32_t respNum = 0;

    if (asyncParam->respHandler != NULL) {
        ret = ServiceProxySendMessage(&asyncParam->reqInfo, &resp, &respNum);
    } else {
        ret = ServiceProxySendMessage(&asyncParam->reqInfo, NULL, NULL);
    }
    if (ret == IOTC_OK) {
        if (asyncParam->respHandler != NULL) {
            asyncParam->respHandler(asyncParam->reqInfo.msgId, resp, respNum);
        }
        ServiceProxyFreeResponseMessage(resp, respNum);
    } else {
        IOTC_LOGW("send message error %d", ret);
    }

    if (asyncParam->freeHandler != NULL) {
        asyncParam->freeHandler(asyncParam->reqInfo.req);
    }
    IotcFree(asyncParam);
}

int32_t ServiceProxySendAsyncMessage(const ServiceRequestInfo *req, ServiceMessageFreeHandler freeHandler,
    ServiceResponseHandler respHandler)
{
    CHECK_RETURN_LOGW(req != NULL && req->req != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    ServiceManagerContext *ctx = GetSvcMngrCtx();
    LOCK_CTX_RETURN(ctx);
    ServiceAsyncExecutor executor = ctx->executor;
    UNLOCK_CTX(ctx);

    if (executor == NULL) {
        IOTC_LOGW("no executor");
        return IOTC_ERR_CALLBACK_NULL;
    }

    ServiceAsyncMsgInfo *asyncParam = (ServiceAsyncMsgInfo *)IotcMalloc(sizeof(ServiceAsyncMsgInfo));
    if (asyncParam == NULL) {
        IOTC_LOGW("malloc error");
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }

    asyncParam->freeHandler = freeHandler;
    asyncParam->respHandler = respHandler;
    asyncParam->reqInfo = *req;

    int32_t ret = executor(ServiceAsyncHandler, asyncParam);
    if (ret != IOTC_OK) {
        IOTC_LOGW("async executor error %d", ret);
        IotcFree(asyncParam);
        return ret;
    }
    return IOTC_OK;
}

void ServiceProxyFreeResponseMessage(ServiceMessage *resp, uint32_t respNum)
{
    CHECK_V_RETURN(resp != NULL && respNum != 0);
    for (uint32_t i = 0; i < respNum; ++i) {
        ServiceMessage *cur = resp + i;
        if (cur->msg == NULL || cur->ctx == NULL) {
            continue;
        }
        ServiceMessageFreeHandler freeHandler = (ServiceMessageFreeHandler)cur->ctx;
        freeHandler((void *)cur->msg);
        cur->msg = NULL;
    }
    IotcFree(resp);
}

static void MessageNodeFree(MessageNode *node)
{
    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &node->subList) {
        MessageSubNode *subNode = CONTAINER_OF(item, MessageSubNode, node);
        LIST_REMOVE(item);
        IotcFree(subNode);
    }
    IotcFree(node);
}

static void ServiceNodeFree(ServiceNode *node)
{
    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &node->msgList) {
        MessageNode *msgNode = CONTAINER_OF(item, MessageNode, node);
        LIST_REMOVE(item);
        MessageNodeFree(msgNode);
    }
    UTILS_FREE_2_NULL(node->depends);
    IotcFree(node);
}

static MessageNode *MessageNodeNew(int32_t messageId)
{
    MessageNode *newNode = (MessageNode *)IotcMalloc(sizeof(MessageNode));
    if (newNode == NULL) {
        IOTC_LOGW("malloc error");
        return NULL;
    }
    (void)memset_s(newNode, sizeof(MessageNode), 0, sizeof(MessageNode));
    LIST_INIT(&newNode->subList);
    newNode->messageId = messageId;
    return newNode;
}

static ServiceNode *ServiceNodeNew(const ServiceInstance *ins, uint32_t instanceId)
{
    ServiceNode *newNode = (ServiceNode *)IotcMalloc(sizeof(ServiceNode));
    if (newNode == NULL) {
        IOTC_LOGW("malloc error");
        return NULL;
    }
    (void)memset_s(newNode, sizeof(ServiceNode), 0, sizeof(ServiceNode));

    LIST_INIT(&newNode->msgList);
    newNode->serviceId = ins->serviceId;
    newNode->instanceId = instanceId;
    newNode->name = ins->name;
    newNode->handler = ins->handler;
    newNode->msgNum = ins->msgNum;
    newNode->apiHandler = ins->apiHandler;

    uint32_t i = 0;
    for (; i < ins->msgNum && ins->msgIds != NULL; ++i) {
        MessageNode *msgNode = MessageNodeNew(ins->msgIds[i]);
        if (msgNode == NULL) {
            break;
        }
        LIST_INSERT_BEFORE(&msgNode->node, &newNode->msgList);
    }
    if (i != ins->msgNum) {
        ServiceNodeFree(newNode);
        return NULL;
    }

    return newNode;
}

int32_t ServiceManagerRegisterInstance(const ServiceInstance *ins)
{
    CHECK_RETURN_LOGW(ins != NULL && ins->handler != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    ServiceManagerContext *ctx = GetSvcMngrCtx();
    LOCK_CTX_RETURN(ctx);
    ListEntry *item = NULL;
    LIST_FOR_EACH_ITEM(item, &ctx->serviceList) {
        ServiceNode *node = CONTAINER_OF(item, ServiceNode, node);
        if (node->serviceId != ins->serviceId) {
            continue;
        }
        UNLOCK_CTX(ctx);
        IOTC_LOGI("service already register %s/%d", NON_NULL_STR(ins->name), ins->serviceId);
        return IOTC_OK;
    }

    ServiceNode *svcNode = ServiceNodeNew(ins, ctx->instanceIdBase++);
    if (svcNode == NULL) {
        UNLOCK_CTX(ctx);
        return IOTC_ADAPTER_MEM_ERR_MALLOC;
    }

    LIST_INSERT_BEFORE(&svcNode->node, &ctx->serviceList);
    IOTC_LOGN("service register %s/%d/%d", NON_NULL_STR(svcNode->name), svcNode->serviceId, svcNode->instanceId);
    UNLOCK_CTX(ctx);
    return IOTC_OK;
}

void ServiceManagerUnregisterInstance(int32_t serviceId)
{
    ServiceManagerContext *ctx = GetSvcMngrCtx();
    LOCK_CTX_V_RETURN(ctx);
    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &ctx->serviceList) {
        ServiceNode *node = CONTAINER_OF(item, ServiceNode, node);
        if (node->serviceId != serviceId) {
            continue;
        }
        IOTC_LOGN("service unregister %s/%d/%d", NON_NULL_STR(node->name), node->serviceId, node->instanceId);
        LIST_REMOVE(item);
        ServiceNodeFree(node);
        UNLOCK_CTX(ctx);
        return;
    }
    UNLOCK_CTX(ctx);
    IOTC_LOGW("invalid service unregister %d", serviceId);
}

void ServiceManagerRegisterExecutor(ServiceAsyncExecutor executor)
{
    CHECK_V_RETURN_LOGW(executor != NULL, "param invalid");
    ServiceManagerContext *ctx = GetSvcMngrCtx();

    LOCK_CTX_V_RETURN(ctx);
    ctx->executor = executor;
    UNLOCK_CTX(ctx);
}

int32_t ServiceManagerInit(void)
{
    ServiceManagerContext *ctx = GetSvcMngrCtx();
    (void)memset_s(ctx, sizeof(ServiceManagerContext), 0, sizeof(ServiceManagerContext));
    LIST_INIT(&ctx->serviceList);
    ctx->mutex = UtilsCreateExMutex();
    if (ctx->mutex == NULL) {
        IOTC_LOGW("create mutex error");
        return IOTC_CORE_COMM_UTILS_ERR_EX_MUTEX_CREATE;
    }
    ctx->instanceIdBase = SecurityRandomUint32() % UINT16_MAX;
    return IOTC_OK;
}

void ServiceManagerDeinit(void)
{
    ServiceManagerContext *ctx = GetSvcMngrCtx();
    ListEntry *item = NULL;
    ListEntry *next = NULL;
    LIST_FOR_EACH_ITEM_SAFE(item, next, &ctx->serviceList) {
        ServiceNode *node = CONTAINER_OF(item, ServiceNode, node);
        LIST_REMOVE(item);
        ServiceNodeFree(node);
    }
    UtilsDestroyExMutex(&ctx->mutex);
}