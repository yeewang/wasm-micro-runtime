
cmake_minimum_required (VERSION 2.8)

project (iot-uniapp)

################  wamr runtime settings  ################
message(STATUS "WAMR_BUILD_SDK_PROFILE=${WAMR_BUILD_SDK_PROFILE}")

# Reset default linker flags
set (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

set (WAMR_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)

## use library and headers in the SDK
link_directories(${WAMR_ROOT_DIR}/wamr-sdk/out/${WAMR_BUILD_SDK_PROFILE}/runtime-sdk/lib)
include_directories(
    ${WAMR_ROOT_DIR}/wamr-sdk/out/${WAMR_BUILD_SDK_PROFILE}/runtime-sdk/include
    ${WAMR_ROOT_DIR}/core/shared/utils
    ${WAMR_ROOT_DIR}/core/shared/platform/linux
)

################  application related  ################

include_directories(${CMAKE_CURRENT_LIST_DIR}/src)

#Note: uncomment below line to use UART mode
#add_definitions (-DCONNECTION_UART)

add_executable (iot-uniapp src/main.c src/native.c)
target_link_libraries (iot-uniapp vmlib -lm -ldl -lpthread -lrt)
