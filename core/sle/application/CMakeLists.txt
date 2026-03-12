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

add_subdirectory(adv)
add_subdirectory(base)
add_subdirectory(ssap)
add_subdirectory(profile)
add_subdirectory(report)

set(IOTC_SRC_LIST
    ${IOTC_SRC_LIST}
    ${CMAKE_CURRENT_SOURCE_DIR}/sle_svc.c
    PARENT_SCOPE
)

set(IOTC_INC_LIST
    ${IOTC_INC_LIST}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    PARENT_SCOPE
)

set(IOTC_TEST_INC_LIST ${IOTC_TEST_INC_LIST} PARENT_SCOPE)