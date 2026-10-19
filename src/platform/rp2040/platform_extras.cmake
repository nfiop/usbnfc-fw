cmake_minimum_required(VERSION 3.20)

target_link_libraries(usbnfc_fw
    pico_stdlib
    pico_multicore
    pico_sync
    tinyusb_device
)

pico_enable_stdio_usb(usbnfc_fw 0)
pico_enable_stdio_uart(usbnfc_fw 0)

pico_add_extra_outputs(usbnfc_fw)
