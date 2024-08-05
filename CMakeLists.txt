cmake_minimum_required(VERSION 3.14.1)
project(IotConnect)

# 设置公共编译选项
set(COMMON_DEFINE "")

# 设置二级路径名
set(ADAPTER_DIR "${PROJECT_SOURCE_DIR}/adapter")
set(CORE_DIR "${PROJECT_SOURCE_DIR}/core")
set(INTERFACES_DIR "${PROJECT_SOURCE_DIR}/interfaces")
set(SDK_DIR "${PROJECT_SOURCE_DIR}/sdk")
set(MBEDTLS_PATH "${PROJECT_SOURCE_DIR}/test/open_source/mbedtls")
set(SECUREC_PATH "${PROJECT_SOURCE_DIR}/test/open_source/securec")
set(CJSON_PATH "${PROJECT_SOURCE_DIR}/test/open_source/cJSON/")
set(OPENHARMOEY_PATH "${PROJECT_SOURCE_DIR}/test/open_source/OpenHarmony")
set(UTILS_PATH "${CORE_DIR}/infrastructure/utils")
set(OPEN_SOURCE_PATH "${PROJECT_SOURCE_DIR}/test/open_source")
set(DEMO_DIR "${PROJECT_SOURCE_DIR}/demo")

# mbedtls
include_directories("${MBEDTLS_PATH}/include")
aux_source_directory("${MBEDTLS_PATH}/library"  IOT_CONNECT_SOURCES)

# securec
include_directories("${SECUREC_PATH}/include")
aux_source_directory("${SECUREC_PATH}/src"  IOT_CONNECT_SOURCES)

# cjson
include_directories("${CJSON_PATH}")
list(APPEND IOT_CONNECT_SOURCES ${CJSON_PATH}/cJSON.c)

# bluetooth
include_directories("${OPEN_SOURCE_PATH}/communication_bluetooth/interfaces/inner_api/include/c_header")

# adapter
include_directories("${ADAPTER_DIR}/include")
aux_source_directory("${ADAPTER_DIR}/mem/stdlib"  IOT_CONNECT_SOURCES)
aux_source_directory("${ADAPTER_DIR}/os/posix"  IOT_CONNECT_SOURCES)
aux_source_directory("${ADAPTER_DIR}/log/printf"  IOT_CONNECT_SOURCES)
aux_source_directory("${ADAPTER_DIR}/log"  IOT_CONNECT_SOURCES)
aux_source_directory("${ADAPTER_DIR}/json/cjson" IOT_CONNECT_SOURCES)
aux_source_directory("${ADAPTER_DIR}/tls/mbedtls" IOT_CONNECT_SOURCES)
aux_source_directory("${ADAPTER_DIR}/crypto/mbedtls" IOT_CONNECT_SOURCES)
aux_source_directory("${ADAPTER_DIR}/trng/weak"  IOT_CONNECT_SOURCES)
include_directories("${OPENHARMOEY_PATH}/wifi_lite/interfaces/wifiservice")
aux_source_directory("${ADAPTER_DIR}/socket"  IOT_CONNECT_SOURCES)
aux_source_directory("${ADAPTER_DIR}/wifi/oh_standard"  IOT_CONNECT_SOURCES)
include_directories("${OPENHARMOEY_PATH}/include")
aux_source_directory("${ADAPTER_DIR}/kv/posix"  IOT_CONNECT_SOURCES)
aux_source_directory("${ADAPTER_DIR}/ble/ohos_lite"  IOT_CONNECT_SOURCES)

# device
include_directories("${CORE_DIR}/device/define")

include_directories("${CORE_DIR}/device/init/include")
aux_source_directory("${CORE_DIR}/device/init"  IOT_CONNECT_SOURCES)
include_directories("${CORE_DIR}/device/control/include")
aux_source_directory("${CORE_DIR}/device/control"  IOT_CONNECT_SOURCES)
include_directories("${CORE_DIR}/device/model/include")
aux_source_directory("${CORE_DIR}/device/model"  IOT_CONNECT_SOURCES)

aux_source_directory("${CORE_DIR}/device/product"  IOT_CONNECT_SOURCES)
include_directories("${CORE_DIR}/device/product/include")
aux_source_directory("${CORE_DIR}/device/report"  IOT_CONNECT_SOURCES)
include_directories("${CORE_DIR}/device/report/include")
include_directories("${CORE_DIR}/device/config/include")
aux_source_directory("${CORE_DIR}/device/config"  IOT_CONNECT_SOURCES)
include_directories("${CORE_DIR}/device/service/include")
aux_source_directory("${CORE_DIR}/device/service"  IOT_CONNECT_SOURCES)


# kitapi
include_directories("${INTERFACES_DIR}/kits/common")
include_directories("${INTERFACES_DIR}/kits/oh_connect")

# core
add_subdirectory(core)

# oh_connect
aux_source_directory("${SDK_DIR}/oh_connect/biz"  IOT_CONNECT_SOURCES)
include_directories("${SDK_DIR}/oh_connect/biz/include")
aux_source_directory("${SDK_DIR}/oh_connect/api"  IOT_CONNECT_SOURCES)
include_directories("${SDK_DIR}/oh_connect/api/include")
aux_source_directory("${SDK_DIR}/oh_connect/config"  IOT_CONNECT_SOURCES)
include_directories("${SDK_DIR}/oh_connect/config/include")
aux_source_directory("${SDK_DIR}/oh_connect/main"  IOT_CONNECT_SOURCES)
include_directories("${SDK_DIR}/oh_connect/main/include")
include_directories("${SDK_DIR}/oh_connect/define")

# demo
aux_source_directory("${DEMO_DIR}/oh_connect" IOT_CONNECT_SOURCES)

# 输出静态库
set(TARGET IotConnect)
set(IOT_CONNECT_LIB_NAME "iotconnect")
add_library(${TARGET} STATIC ${IOT_CONNECT_SOURCES} ${IOTC_SRC_LIST})
target_include_directories(${TARGET} PRIVATE ${IOTC_INC_LIST})
target_compile_options(${TARGET} PRIVATE ${CC_ALL_OPTIONS} ${COMMON_DEFINE})
set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME ${IOT_CONNECT_LIB_NAME})

if (UT_TEST STREQUAL "1")
    add_subdirectory(test)
endif()

message("${IOT_CONNECT_LIB_NAME} cmake run success!!!")
