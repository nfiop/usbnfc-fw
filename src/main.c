/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Liav A
 */

#include "platform/platform.h"

extern int config_main(void);
extern int nand_nfc_main(void);

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

    mode = determine_platform_mode();

    ASSERT(mode == PLATFORM_MODE_CONFIGURATION || 
        mode == PLATFORM_MODE_NAND_CONTROLLER);

    if (mode == PLATFORM_MODE_CONFIGURATION) {
        config_main();
    } else {
    }

    halt();
}
