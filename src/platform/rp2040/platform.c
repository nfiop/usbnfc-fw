/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Liav A
 */

#include "platform/platform.h"

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/mutex.h"
#include "pico/time.h"
#include "pico/sync.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/structs/sio.h"

#include <stdlib.h>
#include <string.h>

#define HALT_CPU_CMD 0xDEADBEEF

extern int app_main(void);

enum platform_mode determine_platform_mode(void)
{
    // TODO: Implement this.
    return PLATFORM_MODE_CONFIGURATION;
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

static void halt_forever(void)
{
    save_and_disable_interrupts();
    while (true) {
        __asm volatile("nop");
    }
}

void send_msg_on_debug_uart(const char *msg)
{
    // TODO: Implement this.
    UNUSED(msg);
}

void halt_all_cpus_from_core0(void)
{
    multicore_launch_core1(halt_forever);
    halt_forever();
}

void halt_all_cpus_from_core1(void)
{
    multicore_fifo_push_blocking(HALT_CPU_CMD);
    halt_forever();
}

void halt_all_cpus(void)
{
    uint32_t core = get_core_num();
    if (core == 0)
        halt_all_cpus_from_core0();
    else if (core == 1)
        halt_all_cpus_from_core1();

    // What should we do here??
    halt_forever();
}

void halt(void)
{
    while (true) {
        __wfi();  // Wait for interrupt
    }
}

static void __isr sio_fifo_irq_handler(void)
{
    uint32_t msg;
    while (sio_hw->fifo_st & SIO_FIFO_ST_VLD_BITS) {
        msg = sio_hw->fifo_rd;
        if (msg == HALT_CPU_CMD) {
            halt_forever();
        }
    }

    sio_hw->fifo_st = SIO_FIFO_ST_ROE_BITS | SIO_FIFO_ST_WOF_BITS;
}

static void setup_halt_interrupt(void)
{
    irq_set_exclusive_handler(SIO_IRQ_PROC0, sio_fifo_irq_handler);

    sio_hw->fifo_st = SIO_FIFO_ST_ROE_BITS | SIO_FIFO_ST_WOF_BITS;
    multicore_fifo_clear_irq();
    sio_hw->fifo_st = SIO_FIFO_ST_VLD_BITS;
}

int main(void)
{
    setup_halt_interrupt();
    stdio_init_all();
    app_main();
}
