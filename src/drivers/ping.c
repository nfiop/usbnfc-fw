/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Liav A
 */

#include "common/types.h"
#include "drivers/driver.h"

static driver_cmd_rc_t ping_command(
    const uint8_t *arg_buf, uint8_t *ret_buf, size_t *retlen)

{
	memcpy(ret_buf, "pong", sizeof("pong"));
	*retlen = 4;
	return DRIVER_CMD_RC_SUCCESS;
}

static struct driver_cmd ping_driver_cmd = {
    .name = "ping",
    .arg_buf_size = 0,
    .requires_arg_buf = false,
    .requires_ret_buf = true,
    .callback = ping_command,
};

static struct driver_cmd *driver_cmds[] = {
    &ping_driver_cmd,
};

static struct driver ping_driver = {
    .name = "ping",
    .cmd_table =
	{
	    .cmds = driver_cmds,
	    .cmds_count = 1,
	},
};

static void start_ping_driver(void)
{
	register_driver(&ping_driver);
}

REGISTER_DRIVER_INIT(start_ping_driver);
