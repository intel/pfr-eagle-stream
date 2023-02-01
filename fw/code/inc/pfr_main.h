// (C) 2019 Intel Corporation. All rights reserved.
// Your use of Intel Corporation's design tools, logic functions and other
// software and tools, and its AMPP partner logic functions, and any output
// files from any of the foregoing (including device programming or simulation
// files), and any associated documentation or information are expressly subject
// to the terms and conditions of the Intel Program License Subscription
// Agreement, Intel FPGA IP License Agreement, or other applicable
// license agreement, including, without limitation, that your use is for the
// sole purpose of programming logic devices manufactured by Intel and sold by
// Intel or its authorized distributors.  Please refer to the applicable
// agreement for further details.

/**
 * @file pfr_main.h
 * @brief Mainline function
 */

// Includes

// Always include pfr_sys first
#include "pfr_sys.h"

#include "initialization.h"
#include "t0_routines.h"


/**
 * @brief Mainline of PFR system. Called from main()
 *
 * Before running the firmware flow, Nios firmware should wait for the Common Core signals.
 * When the Common Core attempts to release BMC and PCH from resets, Nios firmware intercepts those 
 * signals and starts running the firmware flows.
 *
 * First, Nios firmware initializes the PFR system. For example, it writes the CPLD RoT Static Identifier 
 * to the mailbox register. After that, Nios firmware runs the T-1 operations, including PIT checks and 
 * authentication of SPI flashes. If the PFR system is unprovisioned, this step will be skipped. 
 * 
 * Upon completing the T0 operations, Nios firmware transitions the platform to T0 mode and runs the never
 * exit T0 loop. In the T0 loop, Nios firmware monitors for UFM provisioning commands. If the PFR system is 
 * provisioned, Nios firmware also monitors the boot progress, update intent registers, etc. 
 * 
 * @return None
 */
static PFR_ALT_INLINE void PFR_ALT_ALWAYS_INLINE pfr_main()
{
    // Wait for ready from common core
    log_platform_state(PLATFORM_STATE_CPLD_NIOS_WAITING_TO_START);
    while (!check_ready_for_nios_start())
    {
        reset_hw_watchdog();
    }

    // Nios starts now
    log_platform_state(PLATFORM_STATE_CPLD_NIOS_STARTED);

    // Initialize the system
    initialize_system();

    // Go into T-1 mode and do T-1 operations (e.g. authentication). Then transition back to T0.
    perform_platform_reset();

    // Perform T0 operations. This will never exit.
    perform_t0_operations();
}
