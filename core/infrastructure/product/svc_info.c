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
#include <stdbool.h>
#include "svc_info.h"
#include "securec.h"
#include "utils_assert.h"
#include "iotc_errcode.h"
#include "adapter_mem.h"
#include "utils_common.h"

static uint32_t g_svcNum = 0;
static IotcServiceInfo *g_svcInfo = NULL;

static int32_t SvcCopy(const IotcServiceInfo *src, IotcServiceInfo *dst, uint32_t num)
{
    /* free mem outside if failed */
    for (uint32_t i = 0; i < num; ++i) {
        dst[i].svcId = UtilsStrDup(src[i].svcId);
        if (dst[i].svcId == NULL) {
            IOTC_LOGW("copy svc id error %u", i);
            return IOTC_CORE_COMM_UTILS_ERR_STR_DUP;
        }
        dst[i].svcType = UtilsStrDup(src[i].svcType);
        if (dst[i].svcType == NULL) {
            IOTC_LOGW("copy svc type error %u", i);
            return IOTC_CORE_COMM_UTILS_ERR_STR_DUP;
        }
    }
    return IOTC_OK;
}

int32_t ModelSvcInfoInit(const IotcServiceInfo svc[], uint32_t num)
{
    CHECK_RETURN_LOGW(svc != NULL && num != 0 && num <= IOTC_CONF_PROF_MAX_SVC_NUM,
        IOTC_ERR_PARAM_INVALID, "param invalid");

    ModelSvcInfoDeinit();
    g_svcInfo = (IotcServiceInfo *)AdapterCalloc(num, sizeof(IotcServiceInfo));
    if (g_svcInfo == NULL) {
        IOTC_LOGW("calloc error %u", num);
        return IOTC_ADAPTER_MEM_ERR_CALLOC;
    }
    g_svcNum = num;
    int32_t ret = SvcCopy(svc, g_svcInfo, num);
    if (ret != IOTC_OK) {
        ModelSvcInfoDeinit();
        return ret;
    }

    return IOTC_OK;
}

void ModelSvcInfoDeinit(void)
{
    for (uint32_t i = 0; i < g_svcNum && g_svcInfo != NULL; ++i) {
        UTILS_FREE_2_NULL(g_svcInfo[i].svcId);
        UTILS_FREE_2_NULL(g_svcInfo[i].svcType);
    }
    UTILS_FREE_2_NULL(g_svcInfo);
    g_svcNum = 0;
}


const IotcServiceInfo *ModelGetSvcInfo(uint32_t *num)
{
    CHECK_RETURN_LOGW(num != NULL, NULL, "invalid param");

    *num = g_svcNum;
    return g_svcInfo;
}

bool ModelSvcIsValidSid(const char *sid)
{
    CHECK_RETURN_LOGW(!UtilsIsEmptyStr(sid), false, "invalid param");

    for (uint32_t i = 0; i < g_svcNum; i++) {
        if (strcmp(g_svcInfo[i].svcId, sid) == 0) {
            return true;
        }
    }
    return false;
}

bool ModelSvcIsValidSvc(const char *sid, const char *st)
{
    CHECK_RETURN_LOGW(!UtilsIsEmptyStr(sid) && !UtilsIsEmptyStr(st), false, "invalid param");

    for (uint32_t i = 0; i < g_svcNum; i++) {
        if ((strcmp(g_svcInfo[i].svcId, sid) == 0) &&
            (strcmp(g_svcInfo[i].svcType, st) == 0)) {
            return true;
        }
    }
    return false;
}