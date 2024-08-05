# Copyright (c) 2024-2024 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#     http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

add_subdirectory(adapter)
add_subdirectory(core)
add_subdirectory(sdk)

set(IOTC_SRC_LIST ${IOTC_SRC_LIST} PARENT_SCOPE)
set(IOTC_INC_LIST
    ${IOTC_INC_LIST}
    ${CMAKE_CURRENT_SOURCE_DIR}/interfaces/kits/common
    ${CMAKE_CURRENT_SOURCE_DIR}/interfaces/kits/oh_connect
)

set(IOTC_INC_LIST ${IOTC_INC_LIST} PARENT_SCOPE)

set(IOTC_TEST_INC_LIST ${IOTC_TEST_INC_LIST} PARENT_SCOPE)

# 输出静态库
set(IOT_CONNECT_LIB_NAME "iotc")

add_library(${IOTC_TARGET} STATIC ${IOTC_SRC_LIST})
target_include_directories(${IOTC_TARGET} PRIVATE ${IOTC_INC_LIST})
target_compile_options(${IOTC_TARGET} PRIVATE ${CC_ALL_OPTIONS})
set_target_properties(${IOTC_TARGET} PROPERTIES OUTPUT_NAME ${IOT_CONNECT_LIB_NAME})