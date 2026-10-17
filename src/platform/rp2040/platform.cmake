cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PLATFORM_SOURCES ${CMAKE_CURRENT_LIST_DIR}/platform.c)

# ${PICO_SDK_PATH} is declared in sdk.cmake file.
set(PLATFORM_INCLUDES ${CMAKE_CURRENT_LIST_DIR}
    ${PICO_SDK_PATH}/lib/pico-sdk/src/host/pico_multicore/include/
    ${PICO_SDK_PATH}/src/common/pico_base_headers/include
    ${PICO_SDK_PATH}/src/common/pico_stdlib_headers/include
    ${PICO_SDK_PATH}/src/rp2_common/pico_platform_common/include
    ${PICO_SDK_PATH}/src/rp2_common/pico_platform_compiler/include
    ${PICO_SDK_PATH}/src/rp2_common/pico_multicore/include
    ${PICO_SDK_PATH}/src/rp2_common/pico_sync/include
    ${PICO_SDK_PATH}/src/rp2_common/pico_time/include
    ${PICO_SDK_PATH}/src/rp2_common/hardware_sync/include
    ${PICO_SDK_PATH}/src/rp2_common/hardware_base/include
    ${PICO_SDK_PATH}/src/rp2040/hardware_regs/include
    ${PICO_SDK_PATH}/src/rp2040/hardware_structs/include)
