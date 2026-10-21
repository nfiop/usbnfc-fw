/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Liav A
 */

#include "common/hashmap.h"
#include "drivers/driver.h"
#include "platform/platform.h"

extern char __driver_init_ctors_start[];
extern char __driver_init_ctors_end[];

static struct hashmap *drivers;
static struct hashmap *cmds;

static void driver_destroy(void *item)
{
	/* The hashmap has pointers to the driver objects -
	 * it doesn't actually _own_ any of the objects.
	 */
	UNUSED(item);

    ASSERT_NOT_REACHED("driver_destroy invoked!");
}

static int driver_compare(const void *a, const void *b, void *udata)
{
	UNUSED(udata);
	const struct driver **ua = (const struct driver **)a;
	const struct driver *driver1 = *ua;
	const struct driver **ub = (const struct driver **)b;
	const struct driver *driver2 = *ub;
	return strcmp(driver1->name, driver2->name);
}

static uint64_t driver_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
	const struct driver **driverp = (const struct driver **)item;
	const struct driver *driver = *driverp;
	return hashmap_sip(
	    driver->name, strlen(driver->name), seed0, seed1);
}

static void driver_cmd_destroy(void *item)
{
	/* The hashmap has pointers to the driver_cmd objects -
	 * it doesn't actually _own_ any of the objects.
	 */
	UNUSED(item);

    ASSERT_NOT_REACHED("driver_cmd_destroy invoked!");
}

static int driver_cmd_compare(const void *a, const void *b, void *udata)
{
	UNUSED(udata);
	const struct driver_cmd **ua = (const struct driver_cmd **)a;
	const struct driver_cmd *driver_cmd1 = *ua;
	const struct driver_cmd **ub = (const struct driver_cmd **)b;
	const struct driver_cmd *driver_cmd2 = *ub;
	return strcmp(driver_cmd1->name, driver_cmd2->name);
}

static uint64_t driver_cmd_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
	const struct driver_cmd **driver_cmdp = (const struct driver_cmd **)item;
	const struct driver_cmd *driver_cmd = *driver_cmdp;
	return hashmap_sip(
	    driver_cmd->name, strlen(driver_cmd->name), seed0, seed1);
}

void register_driver(struct driver* driver)
{
    size_t idx;
    void *hashmap_ret;
    struct driver *tmp = driver;
    struct driver_cmd *cur_cmd;

    hashmap_ret = (void *)hashmap_set(drivers, &tmp);
    if (!hashmap_ret && hashmap_oom(drivers)) {
        ASSERT_NOT_REACHED("Driver hashmap OOM");
    } else if (hashmap_ret) {
        ASSERT_NOT_REACHED("Driver hashmap entry already set");
    }

    for (idx = 0; idx < driver->cmd_table.cmds_count; idx++) {
        cur_cmd = driver->cmd_table.cmds[idx];
        hashmap_ret = (void *)hashmap_set(cmds, &cur_cmd);
        if (!hashmap_ret && hashmap_oom(cmds)) {
            ASSERT_NOT_REACHED("Driver commands hashmap OOM");
        } else if (hashmap_ret) {
            ASSERT_NOT_REACHED("Driver commands hashmap entry already set");
        }
    }
}

driver_cmd_rc_t execute_driver_command(const char *cmd,
                            const uint8_t *arg,
                            size_t arg_len,
                            uint8_t *ret_buf,
                            size_t *ret_buf_len)
{
    struct driver_cmd compare_cmd;
    struct driver_cmd *compare_cmdp = &compare_cmd;
    
    struct driver_cmd **cmdp;

    compare_cmd.name = (char*)cmd;

    cmdp = (struct driver_cmd **)hashmap_get(cmds, &compare_cmdp);
	if (!cmdp) {
        *ret_buf_len = 0;
		return DRIVER_CMD_RC_UNKNOWN_CMD;
	}

    compare_cmdp = *cmdp;
    if (arg_len != compare_cmdp->arg_buf_size) {
        *ret_buf_len = 0;
        return DRIVER_CMD_RC_INVALID_ARG;
    }

    if (compare_cmdp->arg_buf_size > 0) {
        ASSERT(compare_cmdp->requires_arg_buf);
    } else {
        arg = NULL;
    }

    if (!compare_cmdp->requires_ret_buf) {
        ret_buf = NULL;
        ret_buf_len = 0;
    }

    return compare_cmdp->callback(arg, ret_buf, ret_buf_len);
}

void start_all_drivers(void)
{
    init_driver_fn_t *funcp;
    init_driver_fn_t fn;
    drivers = hashmap_new(sizeof(struct driver*), 0, 0, 0, 
        driver_hash, driver_compare, driver_destroy, NULL);
    ASSERT(drivers != NULL);

    cmds = hashmap_new(sizeof(struct driver_cmd*), 0, 0, 0, 
        driver_cmd_hash, driver_cmd_compare, driver_cmd_destroy, NULL);
    ASSERT(cmds != NULL);

    
    for (funcp = (init_driver_fn_t*)__driver_init_ctors_start; 
        funcp < (init_driver_fn_t*)__driver_init_ctors_end;
        funcp++) 
    {
        ASSERT(*funcp != NULL);
        fn = *funcp;
        fn();
    }
}