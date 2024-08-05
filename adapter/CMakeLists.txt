# 输出静态库
set(ADAPTER_TARGET IOTC_ADAPTER)
add_library(${ADAPTER_TARGET} STATIC ${IOT_CONNECT_SOURCES})
target_compile_options(${ADAPTER_TARGET} PRIVATE ${CC_ALL_OPTIONS} ${COMMON_DEFINE})
set_target_properties(${ADAPTER_TARGET} PROPERTIES OUTPUT_NAME "iotcadapter")

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
include_directories("${PROJECT_SOURCE_DIR}/open_source/communication_bluetooth/interfaces/inner_api/include/c_header")

