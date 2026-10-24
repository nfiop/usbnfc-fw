/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Liav A
 */

#include "platform/platform.h"
#include "drivers/driver.h"

extern int test_and_debug_main(void);
extern int nfc_main(void);

/* First code to execute which is platform-agnostic. The platform-specific
 * init code should execute this after everything is stable and running -
 * PLLs are running, basic system peripherals are enabled and task spawning
 * is possible.
 * It will select either configuration (mass-storage), NAND controller mode or
 * test-and-debug mode and thatselection depends on the target platform, but it
 * is expected that either a push button or a toggle switch will be used for this.
 */
int app_main(void)
{
    enum platform_mode mode;

    mode = determine_platform_mode();

    ASSERT(mode == PLATFORM_MODE_MSC || 
        mode == PLATFORM_MODE_NAND_CONTROLLER || 
        mode == PLATFORM_MODE_DEBUG_AND_TEST);

    if (mode == PLATFORM_MODE_MSC) {
        start_msc_config_mode();
        ASSERT_NOT_REACHED("Returned from MSC config main");
    }
    
    start_all_drivers();
    if (mode == PLATFORM_MODE_NAND_CONTROLLER) {
        nfc_main();
    } else {
        test_and_debug_main();
    }

    ASSERT_NOT_REACHED("Invalid boot mode");
}
