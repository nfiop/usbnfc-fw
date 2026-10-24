/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Liav A
 */

#ifndef __PLATFORM___H__
#define __PLATFORM___H__

#include "common/types.h"
#include "drivers/driver.h"

enum platform_mode {
    PLATFORM_MODE_MSC,
    PLATFORM_MODE_DEBUG_AND_TEST,
    PLATFORM_MODE_NAND_CONTROLLER,
};

typedef enum {
    USB_MODE_MSC,
    USB_MODE_CDC,
    USB_MODE_VENDOR_SPECIFIC_RAW_NFC,
    USB_MODE_VENDOR_SPECIFIC_SPI_NFC,
} usb_mode_t;

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

inline void assert_not_reached_with_msg(const char *msg)
{
    send_msg_on_debug_uart("Assertion not reached!\n");
    send_msg_on_debug_uart(msg);
    halt_all_cpus();
}

#define ASSERT(__x) assert_with_msg(__x, TO_STRING(__x))
#define ASSERT_NOT_REACHED(__msg) assert_not_reached_with_msg(__msg)

void *mem_alloc(size_t);
void *mem_realloc(void *, size_t);
void *mem_alloc_counted(size_t, size_t);
void *mem_zalloc(size_t);
void mem_free(void *);

int config_uart_send_bytes(const uint8_t *data, size_t len);
int config_uart_read_non_blocking(uint8_t *data, size_t max_len);
void start_test_uart_mode(void (*rx_poll_timeout)(void));

/* In contrast to other modes, this mode is strictly 
 * implemented by the platform and there's little intervention
 * by "generic" code.
 * The platform should provide a USB mass-storage mode to allow
 * a user to submit their configuration file. 
 */
void start_msc_config_mode(void);


/* This macro is inspired from the Linux kernel - it should allocate
 * a memory area and zero it afterwards.
 */
#define zalloc_obj(__x) mem_zalloc(sizeof(*__x))

#endif
