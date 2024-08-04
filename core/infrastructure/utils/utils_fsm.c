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
#include "utils_fsm.h"
#include "securec.h"
#include "iotc_log.h"
#include "comm_def.h"
#include "utils_assert.h"
#include "utils_common.h"
#include "iotc_errcode.h"

int32_t UtilsFsmRunning(UtilsFsm *fsm, void *param)
{
    CHECK_RETURN(fsm != NULL && fsm->tbl != NULL, IOTC_ERR_PARAM_INVALID);
    if (fsm->cur < 0) {
        return fsm->cur;
    }

    int32_t before = fsm->cur;
    bool find = false;
    for (uint32_t i = 0; i < fsm->num; ++i) {
        if (fsm->tbl[i].state == fsm->cur && fsm->tbl[i].handler != NULL) {
            fsm->cur = fsm->tbl[i].handler(param, fsm->cur);
            find = true;
            break;
        }
    }
    if (!find) {
        fsm->cur = before;
    } else if (fsm->cur != before && fsm->onChange != NULL) {
        fsm->onChange(fsm, before, fsm->cur);
    }
    return fsm->cur;
}

void UtilsFsmSwitch(UtilsFsm *fsm, uint32_t nextId)
{
    CHECK_V_RETURN(fsm != NULL && nextId >= 0);
    if (nextId != fsm->cur) {
        if (fsm->onChange != NULL) {
            fsm->onChange(fsm, fsm->cur, nextId);
        }
        fsm->cur = nextId;
    }
}

void UtilsFsmLogdOnStateChange(UtilsFsm *fsm, int32_t before, int32_t next)
{
    CHECK_V_RETURN(fsm != NULL);
    IOTC_LOGD("FSM[%s %d => %d]", NON_NULL_STR(fsm->name), before, next);
}

void UtilsFsmLogiOnStateChange(UtilsFsm *fsm, int32_t before, int32_t next)
{
    CHECK_V_RETURN(fsm != NULL);
    IOTC_LOGI("FSM[%s %d => %d]", NON_NULL_STR(fsm->name), before, next);
}

void UtilsFsmLognOnStateChange(UtilsFsm *fsm, int32_t before, int32_t next)
{
    CHECK_V_RETURN(fsm != NULL);
    IOTC_LOGN("FSM[%s %d => %d]", NON_NULL_STR(fsm->name), before, next);
}

int32_t UtilsFsmStateSelfCirculation(void *param, int32_t cur)
{
    NOT_USED(param);
    return cur;
}