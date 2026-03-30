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
#include "char_state_mdl.h"
#include <string.h>
#include "utils_assert.h"
#include "iotc_errcode.h"
#include "iotc_mem.h"
#include "utils_common.h"
#include "utils_json.h"
#include "comm_def.h"

typedef enum {
    CHAR_STATE_NEED_DATA = 0,
    CHAR_STATE_NOT_NEED_DATA,
} CharStateDataType;

void MdlCharStatesFree(IotcCharState **states, uint32_t size)
{
    CHECK_V_RETURN_LOGE(states != NULL && *states != NULL, "param invalid");
    for (uint32_t i = 0; i < size; ++i) {
        UTILS_FREE_2_NULL((*states)[i].data);
        UTILS_FREE_2_NULL((*states)[i].svcId);
    }
    UTILS_FREE_2_NULL(*states);
}

static int32_t JsonArrayToCharStates(
    const IotcJson *json, IotcCharState **states, uint32_t size, CharStateDataType type)
{
    IotcCharState *statesTmp = (IotcCharState *)IotcCalloc(size, sizeof(IotcCharState));
    if (statesTmp == NULL) {
        IOTC_LOGW("calloc error %u", size);
        return IOTC_ADAPTER_MEM_ERR_CALLOC;
    }

    int32_t ret = IOTC_OK;
    IotcJson *cur = NULL;
    for (uint32_t i = 0; i < size; ++i) {
        cur = IotcJsonGetArrayItem(json, i);
        if (cur == NULL) {
            ret = IOTC_ADAPTER_JSON_ERR_GET_ARRAY_ITEM;
            break;
        }
        statesTmp[i].svcId = (char *)UtilsStrDup(IotcJsonGetStr(IotcJsonGetObj(cur, STR_JSON_SID)));
        if (statesTmp[i].svcId == NULL) {
            ret = IOTC_CORE_PROF_MDL_ERR_SVC_NO_SID;
            break;
        }
#if IOTC_CONF_DEV_TYPE == 2
        statesTmp[i].devId = (char *)UtilsStrDup(IotcJsonGetStr(IotcJsonGetObj(cur, STR_JSON_DEV_ID)));
        statesTmp[i].msgId = (char *)UtilsStrDup(IotcJsonGetStr(IotcJsonGetObj(cur, STR_JSON_MSG_ID)));
#endif
        statesTmp[i].data = (char *)UtilsJsonPrintByMalloc(IotcJsonGetObj(cur, STR_JSON_DATA));
        if (statesTmp[i].data != NULL) {
            statesTmp[i].len = strlen(statesTmp[i].data);
        } else if (type == CHAR_STATE_NEED_DATA) {
            IOTC_LOGW("json no data %s", statesTmp[i].svcId);
            ret = IOTC_CORE_PROF_MDL_ERR_SVC_NO_DATA;
            break;
        }
    }
    if (ret == IOTC_OK) {
        *states = statesTmp;
        return IOTC_OK;
    }

    MdlCharStatesFree(&statesTmp, size);
    return ret;
}

static int32_t MdlJsonArrayToCharStates(
    const IotcJson *json, IotcCharState **states, uint32_t *size, CharStateDataType type)
{
    int32_t ret = IotcJsonGetArraySize(json, size);
    if (ret != IOTC_OK) {
        IOTC_LOGW("get array size error %d", ret);
        return ret;
    }
    if (*size == 0 || *size > IOTC_CONF_PROF_MAX_SVC_NUM) {
        IOTC_LOGW("svc size error %u", *size);
        return IOTC_CORE_PROF_MDL_ERR_SVC_NUM_INVALID;
    }

    ret = JsonArrayToCharStates(json, states, *size, type);
    if (ret == IOTC_OK) {
        return IOTC_OK;
    }
    *size = 0;
    return ret;
}

int32_t MdlPutJsonArrayToCharStates(const IotcJson *json, IotcCharState **states, uint32_t *size)
{
    CHECK_RETURN_LOGE(
        json != NULL && states != NULL && *states == NULL && size != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    return MdlJsonArrayToCharStates(json, states, size, CHAR_STATE_NEED_DATA);
}

int32_t MdlGetJsonArrayToCharStates(const IotcJson *json, IotcCharState **states, uint32_t *size)
{
    CHECK_RETURN_LOGE(
        json != NULL && states != NULL && *states == NULL && size != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");

    return MdlJsonArrayToCharStates(json, states, size, CHAR_STATE_NOT_NEED_DATA);
}

int32_t MdlBuildGetCharStatesData(uint32_t size, char ***data, uint32_t **len)
{
    CHECK_RETURN_LOGE(size != 0 && data != NULL && len != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    *data = (char **)IotcCalloc(size, sizeof(char *));
    *len = (uint32_t *)IotcCalloc(size, sizeof(uint32_t));
    if (*data == NULL || *len == NULL) {
        IOTC_LOGW("calloc error %u", size);
        UTILS_FREE_2_NULL(*data);
        UTILS_FREE_2_NULL(*len);
        return IOTC_ADAPTER_MEM_ERR_CALLOC;
    }
    return IOTC_OK;
}

int32_t MdlInitGetCharStatesData(uint32_t size, GetCharStatesData *charData)
{
    CHECK_RETURN_LOGW(size != 0 && charData != NULL, IOTC_ERR_PARAM_INVALID, "param invalid");
    charData->data = (char **)IotcCalloc(size, sizeof(char *));
    charData->len = (uint32_t *)IotcCalloc(size, sizeof(uint32_t));
    if (charData->data == NULL || charData->len == NULL) {
        IOTC_LOGW("calloc error %u", size);
        UTILS_FREE_2_NULL(charData->data);
        UTILS_FREE_2_NULL(charData->len);
        return IOTC_ADAPTER_MEM_ERR_CALLOC;
    }
    return IOTC_OK;
}

void MdlFreeGetCharStatesData(GetCharStatesData *charData)
{
    CHECK_V_RETURN_LOGW(charData != NULL, "param invalid");
    UTILS_FREE_2_NULL(charData->data);
    UTILS_FREE_2_NULL(charData->len);
}

static int32_t BuildCharStateJson(const IotcCharState state[], uint32_t num, IotcJson *data)
{
    IotcJson *array = IotcJsonCreateArray();
    if (IotcJsonAddItem2Obj(data, STR_JSON_VENDOR, array) != IOTC_OK) {
        IOTC_LOGE("add services error");
        IotcJsonDelete(array);
        return IOTC_ADAPTER_JSON_ERR_ADD;
    }
#if IOTC_CONF_DEV_TYPE == 2
    if (num > 0) {
        IotcJsonAddStr2Obj(data, STR_JSON_DEV_ID, state[0].devId);
        IotcJsonAddStr2Obj(data, STR_JSON_MSG_ID, state[0].msgId);
    }
#endif
    for (uint32_t i = 0; i < num; ++i) {
        IotcJson *curJsonObj = IotcJsonCreate();
        CHECK_RETURN(curJsonObj != NULL, IOTC_ADAPTER_JSON_ERR_CREATE);

        int32_t ret = IotcJsonAddItem2Array(array, curJsonObj);
        if (ret != IOTC_OK) {
            IOTC_LOGW("add to array error %d", ret);
            IotcJsonDelete(curJsonObj);
            return IOTC_ADAPTER_JSON_ERR_ADD;
        }
#if IOTC_CONF_DEV_TYPE == 2
        if (state[i].devId != NULL) {
            IotcJsonAddStr2Obj(curJsonObj, STR_JSON_DEV_ID, state[i].devId);
        } else {
            IotcJsonAddStr2Obj(curJsonObj, STR_JSON_DEV_ID, "0");
        }

        if (state[i].msgId != NULL) {
            IotcJsonAddStr2Obj(curJsonObj, STR_JSON_MSG_ID, state[i].msgId);
        } else {
            IotcJsonAddStr2Obj(curJsonObj, STR_JSON_MSG_ID, "0");
        }
#endif
        ret = IotcJsonAddStr2Obj(curJsonObj, STR_JSON_SID, state[i].svcId);
        CHECK_RETURN_LOGE(ret == IOTC_OK, IOTC_ADAPTER_JSON_ERR_ADD, "add sid error %d", ret);

        IotcJson *dataObj = IotcJsonParseWithLen(state[i].data, state[i].len);
        CHECK_RETURN_LOGE(dataObj != NULL, IOTC_ADAPTER_JSON_ERR_PARSE, "parse data error %d", ret);

        ret = IotcJsonAddItem2Obj(curJsonObj, STR_JSON_DATA, dataObj);
        if (ret != IOTC_OK) {
            IotcJsonDelete(dataObj);
            IOTC_LOGW("add data error %d", ret);
            return IOTC_ADAPTER_JSON_ERR_ADD;
        }
    }
    return IOTC_OK;
}

int32_t MdlCharStatesToJson(const IotcCharState state[], uint32_t num, IotcJson **array)
{
    CHECK_RETURN_LOGE(state != NULL && num != 0 && num <= IOTC_CONF_PROF_MAX_SVC_NUM && array != NULL,
        IOTC_ERR_PARAM_INVALID, "param invalid");

    IotcJson *dataJson = IotcJsonCreate();
    if (dataJson == NULL) {
        IOTC_LOGW("create array error");
        return IOTC_ADAPTER_JSON_ERR_CREATE;
    }

    int32_t ret = BuildCharStateJson(state, num, dataJson);
    if (ret != IOTC_OK) {
        IOTC_LOGW("build char states array error %d", ret);
        IotcJsonDelete(dataJson);
        return ret;
    }

    *array = dataJson;
    return IOTC_OK;
}

int32_t MdlUpdateCharStates(IotcCharState states[], const GetCharStatesData *charData, uint32_t num)
{
    CHECK_RETURN_LOGE(
        states != NULL && charData != NULL && charData->data != NULL && charData->len != NULL && num != 0,
        IOTC_ERR_PARAM_INVALID, "param invalid");

    for (uint32_t i = 0; i < num; ++i) {
        if (states[i].data != NULL) {
            IotcFree((char *)states[i].data);
        }
        states[i].data = UtilsStrDupWithLen(charData->data[i], charData->len[i]);
        if (states[i].data == NULL) {
            IOTC_LOGW("update data error %s", NON_NULL_STR(states[i].svcId));
            return IOTC_CORE_COMM_UTILS_ERR_STR_DUP;
        }
        states[i].len = charData->len[i];
    }

    return IOTC_OK;
}
