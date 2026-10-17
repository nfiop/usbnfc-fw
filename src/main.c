/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Liav A
 */

#include "platform/platform.h"

static void initialize_configuration_mode(void)
{
    initialize_cdc_uart_channel();
}

static void initialize_nand_mode(void)
{
}

/* First code to execute which is platform-agnostic. The platform-specific
 * init code should execute this after everything is stable and running -
 * PLLs are running, basic system peripherals are enabled and task spawning
 * is possible.
 * It will select either configuration or NAND controller mode, and that 
 * selection depends on the target platform, but it is expected that either 
 * a push button or a toggle switch will be used for this.
 */
int app_main(void)
{
    enum platform_mode mode;

    initialize_usb_stack();

    /* The first USB endpoint is always the AUX channel, as it is commonly 
     * shared between all modes.
     */
    initialize_aux_uart_channel();

    mode = determine_platform_mode();

    ASSERT(mode == PLATFORM_MODE_CONFIGURATION || 
        mode == PLATFORM_MODE_NAND_CONTROLLER);

    if (mode == PLATFORM_MODE_CONFIGURATION)
        initialize_configuration_mode();
    else
        initialize_nand_mode();

    /* Go to idle, let the spawned tasks to handle async I/O from now on */
    halt();
}
