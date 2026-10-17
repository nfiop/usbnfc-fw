/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Liav A
 */

#include "platform/platform.h"

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
    while (1) {
    }
}
