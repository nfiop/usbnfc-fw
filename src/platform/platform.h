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


void send_msg_on_debug_uart(const char *msg);

void halt(void);
void halt_all_cpus(void);

inline void assert_with_msg(bool condition_has_met, const char *msg)
{
    if (!condition_has_met) {
        send_msg_on_debug_uart("Assertion failed!\n");
        send_msg_on_debug_uart(msg);
        halt_all_cpus();
    }
}
#define ASSERT(__x) assert_with_msg(__x, TO_STRING(x))

void *mem_alloc(size_t);
void *mem_alloc_counted(size_t, size_t);
void *mem_zalloc(size_t);

int config_uart_send_bytes(const uint8_t *data, size_t len);
int config_uart_read_non_blocking(uint8_t *data, size_t max_len);
void start_config_mode(void);

/* This macro is inspired from the Linux kernel - it should allocate
 * a memory area and zero it afterwards.
 */
#define zalloc_obj(__x) mem_zalloc(sizeof(*__x))

#endif
