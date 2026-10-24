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
extern usb_mode_t g_usb_mode;

enum platform_mode determine_platform_mode(void)
{
    // TODO: Implement this.
    g_usb_mode = USB_MODE_MSC;
    return PLATFORM_MODE_MSC;
}

void *mem_realloc(void *ptr, size_t size)
{
    return realloc(ptr, size);
}

void *mem_alloc(size_t mem_size)
{
    return malloc(mem_size);
}

void *mem_alloc_counted(size_t count, size_t obj_size)
{
    return mem_alloc(count * obj_size);
}

void mem_free(void *ptr)
{
    free(ptr);
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
    uart_puts(uart0, msg);
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

static void enable_uart0(void)
{
    uart_init(uart0, 115200);

    gpio_set_function(0, GPIO_FUNC_UART);  // GP0 = TX
    gpio_set_function(1, GPIO_FUNC_UART);  // GP1 = RX

    uart_puts(uart0, "UART enabled!\r\n");
}

int main(void)
{
    setup_halt_interrupt();
    stdio_init_all();

    enable_uart0();

    app_main();
}
