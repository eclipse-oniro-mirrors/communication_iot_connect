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
#include <string.h>
#include "m2m_cloud_send.h"
#include "comm_def.h"
#include "utils_common.h"
#include "utils_assert.h"
#include "coap_codec_utils.h"
#include "utils_bit_map.h"
#include "m2m_cloud_token.h"
#include "iotc_errcode.h"

static CoapOption *BuildCloudOption(M2mCloudContext *ctx, const CloudOption *option, uint32_t *opsNum)
{
    uint32_t opNum = option->num;
    if (UTILS_IS_BIT_SET(option->opBitMap, CLOUD_OPTION_BIT_ACCESS_TOKEN_ID)) {
        ++opNum;
    }
    if (UTILS_IS_BIT_SET(option->opBitMap, CLOUD_OPTION_BIT_SEQ_NUM_ID)) {
        ++opNum;
    }

    CoapOption *ops = AdapterCalloc(opNum, sizeof(CoapOption));
    if (ops == NULL) {
        IOTC_LOGW("calloc error %u", opNum);
        return NULL;
    }

    uint32_t index = 0;
    for (uint32_t i = 0; i < option->num; ++i) {
        ops[index].option = COAP_OPTION_TYPE_URI_PATH;
        ops[index].value.data = (const uint8_t *)option->uri[i];
        ops[index++].value.len = option->uri[i] == NULL ? 0 : strlen(option->uri[i]);
    }
    if (UTILS_IS_BIT_SET(option->opBitMap, CLOUD_OPTION_BIT_ACCESS_TOKEN_ID)) {
        ops[index].option = COAP_OPTION_TYPE_ACCESS_TOKEN_ID;
        ops[index].value.data = (const uint8_t *)ctx->tokenInfo.access;
        ops[index++].value.len = strlen(ctx->tokenInfo.access);
    }
    if (UTILS_IS_BIT_SET(option->opBitMap, CLOUD_OPTION_BIT_SEQ_NUM_ID)) {
        ops[index].option = COAP_OPTION_TYPE_SEQ_NUM_ID;
        ops[index++].value.len = sizeof(uint32_t);
    }

    *opsNum = index;
    return ops;
}

int32_t M2mCloudSendRequest(M2mCloudContext *ctx, CoapClientRespHandler resp,
    M2mBuildRequest build, const CloudOption *option)
{
    CHECK_RETURN_LOGE(ctx != NULL && resp != NULL && build != NULL && option != NULL && option->uri != NULL &&
        option->num != 0 && ctx->linkInfo.endpoint != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    uint32_t opNum;
    CoapOption *options = BuildCloudOption(ctx, option, &opNum);
    if (options == NULL) {
        IOTC_LOGE("build option error");
        return IOTC_SDK_AILIFE_WIFI_ERR_CLOUD_BUILD_OPTION;
    }

    AdapterJson *reqJson = build(ctx);
    if (reqJson == NULL) {
        AdapterFree(options);
        IOTC_LOGE("build req error");
        return IOTC_SDK_AILIFE_WIFI_ERR_CLOUD_BUILD_REQ_JSON;
    }

    CoapClientReqParam param = {
        .type = COAP_MSG_TYPE_NCON,
        .code = COAP_METHOD_TYPE_POST,
        .opNum = opNum,
        .options = options,
        .payload = NULL,
        .payloadBuilder = CoapUtilsBuildJsonPayloadFunc,
        .respHandler = resp,
        .payloadUserData = reqJson,
        .preSize = 0,
    };

    CoapPacket packet;
    int32_t ret = CoapClientSendReq(ctx->linkInfo.endpoint, &param, NULL, &packet);
    AdapterFree(options);
    AdapterJsonDelete(reqJson);
    if (ret != IOTC_OK) {
        IOTC_LOGW("send req error %d", ret);
        return ret;
    }
    return IOTC_OK;
}