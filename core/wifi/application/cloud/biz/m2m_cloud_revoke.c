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
#include "m2m_cloud_revoke.h"
#include <stdlib.h>
#include "utils_json.h"
#include "utils_assert.h"
#include "config_login_info.h"
#include "securec.h"
#include "comm_def.h"
#include "utils_common.h"
#include "m2m_cloud_token.h"
#include "m2m_cloud_errcode.h"
#include "utils_bit_map.h"
#include "iotc_errcode.h"
#include "iotc_log.h"

IotcJson *M2mCloudBuildRevokeRequest(M2mCloudContext *ctx)
{
    CHECK_RETURN_LOGW(ctx != NULL, NULL, "param invalid");
    IotcJson *rootJson = IotcJsonCreate();
    if (rootJson == NULL) {
        IOTC_LOGW("create json error");
        return NULL;
    }

    return rootJson;
}

const CloudOption *M2mCloudGetRevokeOption(void)
{
    static const char *sysRevoke[] = {STR_URI_PATH_SYS, STR_URI_PATH_REVOKE};
    static const CloudOption REVOKE_OPTION = {
        .uri = SYS_REVOKE,
        .num = ARRAY_SIZE(SYS_REVOKE),
        .opBitMap = UTILS_BIT(CLOUD_OPTION_BIT_SEQ_NUM_ID) | \
                    UTILS_BIT(CLOUD_OPTION_BIT_REQ_ID) | \
                    UTILS_BIT(CLOUD_OPTION_BIT_DEV_ID) | \
                    UTILS_BIT(CLOUD_OPTION_BIT_ACCESS_TOKEN_ID),
    };
    return &REVOKE_OPTION;
}