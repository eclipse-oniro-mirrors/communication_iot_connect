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
#ifndef UTILS_GSM_H
#define UTILS_GSM_H

#include <stdint.h>
#include "utils_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UtilsFsm UtilsFsm;

typedef int32_t (*UtilsFsmStateHandler)(void *param, int32_t cur);

typedef void (*UtilsFsmOnStateChange)(UtilsFsm *gsm, int32_t before, int32_t next);

typedef struct {
    int32_t state;
    UtilsFsmStateHandler handler;
} UtilsFsmStateNode;

struct UtilsFsm {
    const char *name;
    int32_t cur;
    UtilsFsmOnStateChange onChange;
    const UtilsFsmStateNode *tbl;
    uint32_t num;
};

#define UTILS_FSM_DECLARE_INIT(_name, _init, _onChange, _tbl) \
    { \
        .name = (_name), \
        .cur = (_init), \
        .onChange = (_onChange), \
        .tbl = (_tbl), \
        .num = ARRAY_SIZE(_tbl) \
    }

#define UTILS_FSM_INIT(_fsm, _name, _init, _onChange, _tbl) \
    do { \
        (_fsm)->name = (_name), \
        (_fsm)->cur = (_init), \
        (_fsm)->onChange = (_onChange), \
        (_fsm)->tbl = (_tbl), \
        (_fsm)->num = ARRAY_SIZE(_tbl) \
    } while (0)

/**
 * @brief 状态机运行
 *
 * @param fsm [IN] 状态机数据
 * @param param [IN] 给状态状态机执行函数的数据
 * @return 0 成功， 非0 失败
 */
int32_t UtilsFsmRunning(UtilsFsm *fsm, void *param);

/**
 * @brief 状态机状态主动切换
 *
 * @param fsm [IN] 状态机数据
 * @param nextId [IN] 状态机下一节点Id
 * @warning 请勿在onChange回调中执行，避免递归
 */
void UtilsFsmSwitch(UtilsFsm *fsm, uint32_t nextId);

/**
 * @brief DEBUG级别的状态切换打印函数
 */
void UtilsFsmLogdOnStateChange(UtilsFsm *fsm, int32_t before, int32_t next);

/**
 * @brief INFO级别的状态切换打印函数
 */
void UtilsFsmLogiOnStateChange(UtilsFsm *fsm, int32_t before, int32_t next);

/**
 * @brief NOTICE级别的状态切换打印函数
 */
void UtilsFsmLognOnStateChange(UtilsFsm *fsm, int32_t before, int32_t next);

int32_t UtilsFsmStateSelfCirculation(void *param, int32_t cur);

#ifdef __cplusplus
}
#endif

#endif /* UTILS_GSM_H */