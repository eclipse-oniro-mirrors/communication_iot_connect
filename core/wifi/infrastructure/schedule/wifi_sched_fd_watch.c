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
#include "wifi_sched_fd_watch.h"
#include "sched_event_loop.h"
#include "iotc_socket.h"
#include "event_loop.h"
#include "utils_assert.h"
#include "iotc_errcode.h"

static EventSource *g_wifiFdEventSource = NULL;

#define EVENT_SOURCE_FD_SET_TO_ADAPTER(eventSourceFdSet, adapterFdSet) \
    IotcFdSet buffer##adapterFdSet = { 0, NULL }; \
    IotcFdSet *(adapterFdSet) = NULL; \
    if ((eventSourceFdSet) != NULL && (eventSourceFdSet)->num != 0 && \
        (eventSourceFdSet)->num <= EVENT_SOURCE_FD_MAX_WATCH_SIZE) { \
        (buffer##adapterFdSet).fdSet = (eventSourceFdSet)->fd; \
        (buffer##adapterFdSet).num = (eventSourceFdSet)->num; \
        (adapterFdSet) = &(buffer##adapterFdSet); \
    }

#define CLEAR_EVENT_SOURCE_FD_SET(eventSourceFdSet) \
    do { \
        if ((eventSourceFdSet) != NULL) { \
            (eventSourceFdSet)->num = 0; \
        } \
    } while (0)

static int32_t FdSocketPollBySelect(EventSourceFdSet *read, EventSourceFdSet *write,
    EventSourceFdSet *except, uint32_t timeout)
{
    EVENT_SOURCE_FD_SET_TO_ADAPTER(read, pRead);
    EVENT_SOURCE_FD_SET_TO_ADAPTER(write, pWrite);
    EVENT_SOURCE_FD_SET_TO_ADAPTER(except, pExcept);

    int32_t ret = IotcSelect(pRead, pWrite, pExcept, timeout);
    if (ret > 0) {
        return ret;
    }
    CLEAR_EVENT_SOURCE_FD_SET(read);
    CLEAR_EVENT_SOURCE_FD_SET(write);
    CLEAR_EVENT_SOURCE_FD_SET(except);
    return ret;
}

static inline EventSource *GetWifiFdEventSource(void)
{
    return g_wifiFdEventSource;
}

int32_t WifiSchedFdWatchInit(void)
{
    if (GetSchedEventLoop() == NULL) {
        IOTC_LOGW("event loop not init");
        return IOTC_ERR_NOT_INIT;
    }

    if (g_wifiFdEventSource != NULL) {
        return IOTC_OK;
    }

    g_wifiFdEventSource = EventSourceFdNew(FdSocketPollBySelect);
    if (g_wifiFdEventSource == NULL) {
        IOTC_LOGW("event source new error");
        return IOTC_CORE_COMM_FWK_ERR_SOURCE_NEW;
    }
    int32_t ret = EventLoopAddSource(GetSchedEventLoop(), g_wifiFdEventSource);
    if (ret != IOTC_OK) {
        EventSourceFree(g_wifiFdEventSource);
        g_wifiFdEventSource = NULL;
        IOTC_LOGW("add source error %d", ret);
        return ret;
    }
    return IOTC_OK;
}

void WifiSchedFdWatchDeinit(void)
{
    /* 资源由event loop 管理并释放 */
    EventLoopDelSource(GetSchedEventLoop(), GetWifiFdEventSource());
    g_wifiFdEventSource = NULL;
}

int32_t WifiSchedFdWatch(const FdWatchParam *watch)
{
    CHECK_RETURN_LOGW(watch != NULL && watch->fd >= 0, IOTC_ERR_PARAM_INVALID, "param invalid");

    EventSource *fdSrc = GetWifiFdEventSource();
    if (fdSrc == NULL) {
        return IOTC_ERR_NOT_INIT;
    }

    if (!EventSourceFdWatch(fdSrc, watch)) {
        IOTC_LOGW("watch fd error");
        return IOTC_CORE_COMM_FWK_ERR_SOURCE_FD_WATCH;
    }
    return IOTC_OK;
}

int32_t WifiSchedFdSuspend(int32_t fd)
{
    CHECK_RETURN_LOGW(fd >= 0, IOTC_ERR_PARAM_INVALID, "param invalid");

    EventSource *fdSrc = GetWifiFdEventSource();
    if (fdSrc == NULL) {
        return IOTC_ERR_NOT_INIT;
    }
    if (!EventSourceFdSuspend(fdSrc, fd)) {
        IOTC_LOGW("suspend fd error");
        return IOTC_CORE_COMM_FWK_ERR_SOURCE_FD_SUSPEND;
    }
    return IOTC_OK;
}

int32_t WifiSchedFdResume(int32_t fd)
{
    CHECK_RETURN_LOGW(fd >= 0, IOTC_ERR_PARAM_INVALID, "param invalid");

    EventSource *fdSrc = GetWifiFdEventSource();
    if (fdSrc == NULL) {
        return IOTC_ERR_NOT_INIT;
    }

    if (!EventSourceFdResume(fdSrc, fd)) {
        IOTC_LOGW("resume fd error");
        return IOTC_CORE_COMM_FWK_ERR_SOURCE_FD_RESUME;
    }
    return IOTC_OK;
}

void WifiSchedFdRemove(int32_t fd)
{
    CHECK_V_RETURN_LOGW(fd >= 0, "param invalid");

    EventSource *fdSrc = GetWifiFdEventSource();
    if (fdSrc == NULL) {
        return;
    }

    EventSourceFdRemove(fdSrc, fd);
}

static bool LinkFdReadCallback(int32_t fd, void *userData)
{
    CHECK_RETURN_LOGW(fd >= 0 && userData != NULL, false, "param invalid");

    TransLink *link = (TransLink *)userData;
    TransLinkRecvReadEvent(link);
    return true;
}

int32_t WifiSchedLinkRecvWatch(TransLink *link)
{
    CHECK_RETURN_LOGW(link != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    int32_t fd = TransLinkGetFd(link);
    if (fd < 0) {
        IOTC_LOGW("link get fd error %d", fd);
        return fd;
    }

    FdWatchParam fdWatch = {
        .fd = fd,
        .prio = FD_EVENT_PRIORITY_MID,
        .recvEvent = LinkFdReadCallback,
        .sendEvent = NULL,
        .exceEvent = NULL,
        .userData = link,
    };
    int32_t ret = WifiSchedFdWatch(&fdWatch);
    if (ret != IOTC_OK) {
        IOTC_LOGW("fd watch error %d", ret);
        return ret;
    }
    return IOTC_OK;
}