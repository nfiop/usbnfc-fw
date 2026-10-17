/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Liav A
 */

#ifndef __PLATFORM___H__
#define __PLATFORM___H__

#include "common/types.h"

enum platform_mode {
    PLATFORM_MODE_CONFIGURATION,
    PLATFORM_MODE_NAND_CONTROLLER,
};

enum platform_mode determine_platform_mode(void);

/* Basic initialization of the USB stack, which will assist registering 
 * USB functions later on.
 */
void initialize_usb_stack(void);

/* Functions to initialize UART channels over USB */
void initialize_aux_uart_channel(void);
void initialize_cdc_uart_channel(void);

void send_msg_on_uart_aux_channel(const char *msg);

void halt(void);

inline void assert_with_msg(bool condition_has_met, const char *msg)
{
    if (!condition_has_met) {
        send_msg_on_uart_aux_channel("Assertion failed!\n");
        send_msg_on_uart_aux_channel(msg);
        halt();
    }
}
#define ASSERT(__x) assert_with_msg(__x, TO_STRING(x))

void *mem_alloc(size_t);
void *mem_alloc_counted(size_t, size_t);
void *mem_zalloc(size_t);

/* This macro is inspired from the Linux kernel - it should allocate
 * a memory area and zero it afterwards.
 */
#define zalloc_obj(__x) mem_zalloc(sizeof(*__x))

#endif
