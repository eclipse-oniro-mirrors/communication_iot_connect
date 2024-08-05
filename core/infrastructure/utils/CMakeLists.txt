#
# Copyright (c) 2024 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set(IOTC_SRC_LIST
    ${IOTC_SRC_LIST}
    ${CMAKE_CURRENT_SOURCE_DIR}/utils_bit_map.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utils_buffer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utils_combo.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utils_common.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utils_fsm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utils_json.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utils_msg_queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utils_mutex_cas.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utils_mutex_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utils_mutex_global.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utils_queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utils_time.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utils_hash_map.c
    ${CMAKE_CURRENT_SOURCE_DIR}/local_mutex/mini/utils_mutex_local.c
    PARENT_SCOPE
)

set(IOTC_INC_LIST
    ${IOTC_INC_LIST}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    PARENT_SCOPE
)