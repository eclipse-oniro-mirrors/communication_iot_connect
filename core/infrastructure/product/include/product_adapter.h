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
#ifndef PRODUCT_ADAPTER_H
#define PRODUCT_ADAPTER_H

#include <stdint.h>
#include <stddef.h>
#include "iotc_def.h"
#include "iotc_prof_def.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PROD_HOOK_REG_POLICY_COVER_ALL = 0,
    PROD_HOOK_REG_POLICY_COVER_NON_NULL,
} ProdHookRegPolicy;

typedef struct {
    int32_t (*onGetRootCaCert)(const char **ca[], uint32_t *num);
    int32_t (*onRecvNetCfgInfo)(const char *netInfo, uint32_t len);
    int32_t (*onRecvCustomSecData)(const uint8_t *data, uint32_t len);
    int32_t (*onGetSurfacePower)(int8_t *power);
    int32_t (*onProfPutCharState)(const IotcCharState state[], uint32_t num);
    int32_t (*onProfGetCharState)(const IotcCharState state[], char *out[], uint32_t len[], uint32_t num);
    int32_t (*onProfReportAll)(void);
    int32_t (*onProfReportByDevId)(const char *devId);
    int32_t (*onProfGetPincode)(uint8_t *buf, uint32_t bufLen);
    int32_t (*onProfGetAcKey)(uint8_t *buf, uint32_t bufLen);
    void (*onProfFree)(void *ptr);
    int32_t (*onDevReboot)(int32_t res);
    int32_t (*onDevTrng)(uint8_t *buf, uint32_t len);
} ProductHooks;

int32_t ProductRegisterHooks(const ProductHooks *hooks, ProdHookRegPolicy policy);
int32_t ProductGetRootCaCert(const char **ca[], uint32_t *num);
int32_t ProductRecvNetCfgInfo(const char *netInfo, uint32_t len);
int32_t ProductRecvCustomSecData(const uint8_t *data, uint32_t len);
int32_t ProductGetSurfacePower(int8_t *power);
int32_t ProductProfPutCharState(const IotcCharState state[], uint32_t num);
int32_t ProductProfGetCharState(const IotcCharState state[], char *out[], uint32_t len[], uint32_t num);
int32_t ProductProfReportAll(void);
int32_t ProductProfReportByDevId(const char *devId);
int32_t ProductProfGetPincode(uint8_t *buf, uint32_t bufLen);
int32_t ProductProfGetAcKey(uint8_t *buf, uint32_t bufLen);
void ProductProfFree(void *ptr);
int32_t ProductDevReboot(int8_t res);
int32_t ProductDevTrng(uint8_t *buf, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* PRODUCT_ADAPTER_H */
