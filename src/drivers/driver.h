/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Liav A
 */

#ifndef __DRIVER___H__
#define __DRIVER___H__

#include "common/types.h"

#define RET_BUF_SIZE 64

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
    char *cmd_name;
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

#endif