cmake_minimum_required(VERSION 3.20)

# Initialize platform SDK and any dependecies before declaring the
# project.
include(${CMAKE_SOURCE_DIR}/platform/${CMAKE_PLATFORM}/sdk.cmake)

project(usbnfc_fw C CXX ASM)

# Define ${PLATFORM_SOURCES} and ${PLATFORM_INCLUDES} variables,
# possibly C and C++ standards as well
include(${CMAKE_SOURCE_DIR}/platform/${CMAKE_PLATFORM}/platform.cmake)

add_executable(usbnfc_fw
    ${PLATFORM_SOURCES}
    main.c
    config.c
    nfc.c

    # Common code
    common/hashmap.c

    # QCBOR
    lib/qcbor/src/UsefulBuf.c
    lib/qcbor/src/ieee754.c
    lib/qcbor/src/qcbor_encode.c
    lib/qcbor/src/qcbor_decode.c

    # COBS
    lib/cobs-c/cobs.c

    # Common drivers
    drivers/core.c
    drivers/ping.c
)

target_include_directories(usbnfc_fw PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${PLATFORM_INCLUDES}

    ${CMAKE_CURRENT_LIST_DIR}/lib/qcbor/inc
    ${CMAKE_CURRENT_LIST_DIR}/lib/cobs-c
)

# Define libraries, dependecies, flags for the platform, after the project is
# declared...
include(${CMAKE_SOURCE_DIR}/platform/${CMAKE_PLATFORM}/platform_extras.cmake)
