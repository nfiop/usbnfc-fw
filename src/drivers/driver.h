/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Liav A
 */

#ifndef __DRIVER___H__
#define __DRIVER___H__

#include "common/types.h"

#define RET_BUF_SIZE 64

typedef void (*init_driver_fn_t)(void);

#define REGISTER_DRIVER_INIT(fn) \
    static init_driver_fn_t const __ctor_##fn \
    __attribute__((used, section("driver_init_ctors"))) = fn

typedef enum {
    /* This indicates a ACK response, while any other
     * code should return a NACK with a specific error.
     */
    DRIVER_CMD_RC_SUCCESS = 0,

    /* Common return codes, should be returned in the 
     * common dispatch handler.
     */
    DRIVER_CMD_RC_ARG_BUF_BAD_LENGTH,
    DRIVER_CMD_RC_INVALID_ARG,
    DRIVER_CMD_RC_UNKNOWN_CMD,
} driver_cmd_rc_t;

struct driver_cmd {
    char *name;
    size_t arg_buf_size;
    bool requires_arg_buf;
    bool requires_ret_buf;
    driver_cmd_rc_t (*callback)(const uint8_t *arg_buf, uint8_t *ret_buf, size_t *retlen);
};

struct driver_cmd_table {
    struct driver_cmd **cmds;
    size_t cmds_count;
};

struct driver {
    char* name;

    struct driver_cmd_table cmd_table;
};

void register_driver(struct driver* driver);

void start_all_drivers(void);

driver_cmd_rc_t execute_driver_command(const char *cmd,
                            const uint8_t *arg,
                            size_t arg_len,
                            uint8_t *ret_buf,
                            size_t *ret_buf_len);

#endif