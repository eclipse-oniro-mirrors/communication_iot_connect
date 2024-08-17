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
#include "seq_num_utils.h"
#include "utils_bit_map.h"
#include "utils_common.h"
#include "utils_assert.h"
#include "adapter_socket.h"

static bool SeqNumCheckWithoutOverflow(uint32_t curSeq, uint32_t recvSeq,
    uint32_t window, bool *isSmall, uint32_t *delta)
{
    if (recvSeq > curSeq) {
        /* 接收到的seq较大则在窗口内有效 */
        *delta = recvSeq - curSeq;
        return *delta <= window;
    } else if (curSeq - recvSeq <= window) {
        /* 接收到的seq小且在窗口内 */
        *isSmall = true;
        *delta = curSeq - recvSeq;
        return true;
    } else {
        /* 接收到的seq小且不在窗口内 */
        return false;
    }
}

/* 当前seq较小，本地seq可能已翻转，有可能收到对端未翻转的seq */
static bool SeqNumCheckWithCurSeqSmall(uint32_t curSeq, uint32_t recvSeq,
    uint32_t window, bool *isSmall, uint32_t *delta)
{
    if (recvSeq < curSeq) {
        /* 未翻转，接收到的seq小且在窗口内 */
        *isSmall = true;
        *delta = curSeq - recvSeq;
        return true;
    } else if (recvSeq - curSeq <= window) {
        /* 未翻转，接收到的seq大且在窗口内 */
        *delta = recvSeq - curSeq;
        return true;
    } else if (UINT32_MAX - recvSeq + curSeq + 1 <= window) {
        /* 已翻转，接收到的seq小且在窗口内 */
        *isSmall = true;
        *delta = UINT32_MAX - recvSeq + curSeq + 1;
        return true;
    } else {
        /* 不在窗口期 */
        return false;
    }
}

/* 当前seq较大，有可能收到对端已翻转的seq */
static bool SeqNumCheckWithCurSeqBig(uint32_t curSeq, uint32_t recvSeq,
    uint32_t window, bool *isSmall, uint32_t *delta)
{
    if (recvSeq > curSeq) {
        /* 未翻转，接收到的seq大且在窗口期 */
        *delta = recvSeq - curSeq;
        return true;
    } else if (curSeq - recvSeq <= window) {
        /* 未翻转，接收到的seq小且在窗口期 */
        *isSmall = true;
        *delta = curSeq - recvSeq;
        return true;
    } else if (UINT32_MAX - curSeq + recvSeq + 1 <= window) {
        /* 已翻转，接收到的seq大且在窗口期 */
        *delta = UINT32_MAX - curSeq + recvSeq + 1;
        return true;
    } else {
        /* 不在窗口期 */
        return false;
    }
}

bool SeqNumCheck(uint32_t curSeq, uint32_t recvSeq, uint32_t window, bool *isSmall, uint32_t *delta)
{
    CHECK_RETURN_LOGW(isSmall != NULL && delta != NULL && window <= SEQ_NUM_WINDOW_MAX && window != 0,
        false, "param invalid");
    *isSmall = false;
    *delta = 0;

    if (recvSeq == curSeq) {
        /* 接收到重传报文 */
        return false;
    }

    if (curSeq < window) {
        return SeqNumCheckWithCurSeqSmall(curSeq, recvSeq, window, isSmall, delta);
    } else if (curSeq > UINT32_MAX - window) {
        return SeqNumCheckWithCurSeqBig(curSeq, recvSeq, window, isSmall, delta);
    } else {
        return SeqNumCheckWithoutOverflow(curSeq, recvSeq, window, isSmall, delta);
    }
}

uint32_t CoapSeqToNum(const uint8_t *data, uint32_t len)
{
    CHECK_RETURN_LOGW(data != NULL && len <= sizeof(uint32_t) && len != 0, 0, "param invalid");

    if (len == sizeof(uint8_t)) {
        return data[0];
    } else if (len == sizeof(uint16_t)) {
        return IotcNtohs(*(uint16_t *)data);
    } else if (len == sizeof(uint32_t)) {
        return IotcNtohl(*(uint32_t *)data);
    } else {
        /* CI NOTE: 0字节左移16位得到高位，1字节左移8位得到中间位，2字节为低位 */
        return ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | ((uint32_t)data[2]);
    }
}