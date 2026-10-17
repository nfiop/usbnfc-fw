
/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Liav A
 */

#include "platform/platform.h"

#include <stdlib.h>
#include <string.h>

extern int app_main(void);

enum platform_mode determine_platform_mode(void)
{
    // TODO: Implement this.
    return PLATFORM_MODE_CONFIGURATION;
}

void initialize_usb_stack(void)
{
    // TODO: Implement this.
}

void initialize_aux_uart_channel(void)
{
    // TODO: Implement this.
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

int main(void)
{
    app_main();
}
