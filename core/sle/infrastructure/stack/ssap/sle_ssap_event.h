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
#ifndef SLE_SSAP_EVENT_H
#define SLE_SSAP_EVENT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化Ssap事件
 *
 * @return 0 成功，非0 失败
 */
int32_t SleSsapEventInit(void);
int32_t SleSsapServiceEventInit(void);

#ifdef __cplusplus
}
#endif

#endif /* SLE_SSAP_EVENT_H */
