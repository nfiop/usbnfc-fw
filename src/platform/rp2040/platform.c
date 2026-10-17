
/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Liav A
 */

#include "platform/platform.h"

#include "tusb.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/mutex.h"
#include "pico/time.h"
#include "pico/sync.h"

#include <stdlib.h>
#include <string.h>

extern int app_main(void);

enum platform_mode determine_platform_mode(void)
{
    // TODO: Implement this.
    return PLATFORM_MODE_CONFIGURATION;
}

static void usb_worker(void)
{
    while (1) {
        tud_task();  // Process USB events
    }
}

void initialize_usb_stack(void)
{
    tusb_init();
    multicore_launch_core1(usb_worker);
}

void initialize_aux_uart_channel(void)
{
    // TODO: Implement this.
}

static void send_msg_on_uart_aux_channel(const char *msg)
{
    // TODO: Implement this.
    UNUSED(msg);
}

void initialize_cdc_uart_channel(void)
{
    // TODO: Implement this.
}

void *mem_alloc(size_t mem_size)
{
    return malloc(mem_size);
}

void *mem_alloc_counted(size_t count, size_t obj_size)
{
    return mem_alloc(count * obj_size);
}

void *mem_zalloc(size_t mem_size)
{
    void *new_mem = mem_alloc(mem_size);
    if (!new_mem)
        return NULL;

    memset(new_mem, 0, mem_size);
    return new_mem;
}

void assert_with_msg(bool condition_has_met, const char *msg)
{
    if (!condition_has_met) {
        send_msg_on_uart_aux_channel("Assertion failed!\n");
        send_msg_on_uart_aux_channel(msg);
        halt();
    }
}

void halt(void)
{
    while (true) {
        __wfi();  // Wait for interrupt
    }
}


int main(void)
{
    stdio_init_all();
    app_main();
}
