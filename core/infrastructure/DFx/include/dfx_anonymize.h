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
#ifndef DFX_ANONYMIZE_H
#define DFX_ANONYMIZE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    /*
     * ID类字符串匿名化原则
     * len(ID)∈[1] 匿名[1]
     * len(ID)∈[2,3] 匿名[2,end]
     * len(ID)∈[4,5] 匿名[3,end]
     * len(ID)∈[6,10] 匿名[5,end]
     * len(ID)∈[11] 匿名[4,7]
     * len(ID)∈[12,18] 匿名[7,end]
     * len(ID)∈[19, ∞) 匿名[10,end]
     */
    ANONYMIZE_ID = 0,
    /*
     * MAC字符串匿名化原则
     * 匿名化格式xx:xx:xx:xx:xx:xx:xx -> xx:xx:xx:xx:**:**
     */
    ANONYMIZE_MAC,
    /*
     * SN字符串匿名化原则
     * len(SN)∈[1,5] 匿名[1,end]
     * len(ID)∈[6,12] 匿名[3,end-2]
     * len(ID)∈[13,∞) 匿名[4,end-3]
     */
    ANONYMIZE_SN,
    /*
     * IP字符串匿名化原则
     * 匿名化格式xxx.xxx.xxx.xxx -> xxx.*.*.xxx
     */
    ANONYMIZE_IP,
    /*
     * NAME类字符串匿名化原则
     * len(NAME)∈[1] 匿名[1]
     * len(NAME)∈[2,3] 匿名[2]
     * len(NAME)∈[4,5] 匿名[2,3]
     * len(NAME)∈[6, ∞) 匿名[3,3+len(NAME)/3]
     */
    ANONYMIZE_NAME,
} AnonymizeType;

/* 需修改为具体的值 */
#define ANONYMIZE_ID_MIN_BUF_LEN 40
#define ANONYMIZE_MAC_MIN_BUF_LEN 18
#define ANONYMIZE_SN_MIN_BUF_LEN 40
#define ANONYMIZE_IP_MIN_BUF_LEN 16
#define ANONYMIZE_NAME_MIN_BUF_LEN 40

/* src和dstBuf可以指向同样的内存地址 */
int32_t DfxAnonymizeStrWithBuffer(const char *src, AnonymizeType type, char *dstBuf, uint32_t bufLen);

int32_t DfxAnonymizeStr(const char *src, AnonymizeType type, char **dst);

int32_t DfxAnonymizeIpAddrWithBuffer(uint32_t ip, char *dstBuf, uint32_t bufLen);

int32_t DfxAnonymizeIpAddr(uint32_t ip, char **dst);

int32_t DfxAnonymizeMacAddrWithBuffer(const uint8_t *mac, uint32_t macLen, char *dstBuf, uint32_t bufLen);

int32_t DfxAnonymizeMacAddr(const uint8_t *mac, uint32_t macLen, char **dst);

#define DFX_ANONYMIZE_MAC_STR(_anonyMac, _macStr) \
    char (_anonyMac)[ANONYMIZE_MAC_MIN_BUF_LEN]; \
    do { \
        (void)DfxAnonymizeStrWithBuffer(_macStr, ANONYMIZE_MAC, (_anonyMac), sizeof(_anonyMac)); \
    } while (0)

#define DFX_ANONYMIZE_MAC_ADDR(_anonyMac, _macAddr) \
    char (_anonyMac)[ANONYMIZE_MAC_MIN_BUF_LEN]; \
    do { \
        (void)DfxAnonymizeMacAddrWithBuffer(_macAddr, sizeof(_macAddr), (_anonyMac), sizeof(_anonyMac)); \
    } while (0)

#define DFX_ANONYMIZE_IP_STR(_anonyIp, _ipStr) \
    char (_anonyIp)[ANONYMIZE_IP_MIN_BUF_LEN]; \
    do { \
        (void)DfxAnonymizeStrWithBuffer(_ipStr, ANONYMIZE_IP, (_anonyIp), sizeof(_anonyIp)); \
    } while (0)

#define DFX_ANONYMIZE_IP_ADDR(_anonyIp, _ipAddr) \
    char (_anonyIp)[ANONYMIZE_IP_MIN_BUF_LEN]; \
    do { \
        (void)DfxAnonymizeIpAddrWithBuffer(_ipAddr, (_anonyIp), sizeof(_anonyIp)); \
    } while (0)

#define DFX_ANONYMIZE_SN_STR(_anonySn, _snStr) \
    char (_anonySn)[ANONYMIZE_SN_MIN_BUF_LEN]; \
    do { \
        (void)DfxAnonymizeStrWithBuffer(_snStr, ANONYMIZE_SN, (_anonySn), sizeof(_anonySn)); \
    } while (0)

#define DFX_ANONYMIZE_ID_STR(_anonyId, _idStr) \
    char (_anonyId)[ANONYMIZE_ID_MIN_BUF_LEN]; \
    do { \
        (void)DfxAnonymizeStrWithBuffer(_idStr, ANONYMIZE_ID, (_anonyId), sizeof(_anonyId)); \
    } while (0)

#define DFX_ANONYMIZE_NAME_STR(_anonyName, _nameStr) \
    char (_anonyName)[ANONYMIZE_NAME_MIN_BUF_LEN]; \
    do { \
        (void)DfxAnonymizeStrWithBuffer(_nameStr, ANONYMIZE_NAME, (_anonyName), sizeof(_anonyName)); \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* DFX_ANONYMIZE_H */
