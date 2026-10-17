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

void halt(void);

void *mem_alloc(size_t);
void *mem_alloc_counted(size_t, size_t);
void *mem_zalloc(size_t);

/* This macro is inspired from the Linux kernel - it should allocate
 * a memory area and zero it afterwards.
 */
#define zalloc_obj(__x) mem_zalloc(sizeof(*__x))

#endif
