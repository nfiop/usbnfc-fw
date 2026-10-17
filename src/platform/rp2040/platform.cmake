cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(usbnfc_fw
    ${CMAKE_CURRENT_LIST_DIR}/platform.c
)

target_link_libraries(usbnfc_fw
    pico_stdlib 
)

pico_enable_stdio_usb(usbnfc_fw 1)
pico_enable_stdio_uart(usbnfc_fw 0)

pico_add_extra_outputs(usbnfc_fw)
