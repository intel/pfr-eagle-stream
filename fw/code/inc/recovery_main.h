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
 * @file recovery_main.h
 * @brief Mainline function
 */

// Includes

// Always include pfr_sys first
#include "pfr_sys.h"

#include "initialization.h"
#include "transition.h"
#include "cpld_update.h"
#include "cpld_recovery.h"
#include "attestation.h"

/**
 * @brief Mainline of Recovery system. Called from main()
 *
 * [detailed description]
 *
 * @return None
 */
static PFR_ALT_INLINE void PFR_ALT_ALWAYS_INLINE recovery_main()
{
    // Assert sleep suspend right away
    // We do this to ensure we maintain power because we don't know what state the machine was in before reconfig
    set_bit(U_GPO_1_ADDR, GPO_1_FM_PFR_SLP_SUS_N);

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

#ifdef ENABLE_ATTESTATION_WITH_ECDSA_384
    // Prepare ingredients for key and certificate generation
    prep_boot_flow_pre_requisite_384();
#endif

#ifdef ENABLE_ATTESTATION_WITH_ECDSA_256
    // Prepare ingredients for key and certificate generation
    prep_boot_flow_pre_requisite_256();
#endif

    // Prepare for transition to T-1 mode
    perform_entry_to_tmin1();

    prep_for_reconfig();

    // Check to see if we are in recovery because of a powercycle or a cpld update request
    // Note that the bread crumb may not be cleaned up if CPLD update failed (e.g. authentication failure)
    // If it was because of authentication failure, recovery image will just always perform authentication
    // on the CPLD update capsule and then switch to CFM1 once that fails. Authentication on CPLD update capsule
    // should be pretty quick.

    // read_reconfig_reason() == RECONFIG_REASON_POWER_UP_OR_SWITCH_TO_CFM0 means that we got to this state intentionally
    // and not by a watchdog tripping or a failure to boot
    // The CPLD cannot tell the difference between entering this state due to a AC power cycle 
    // or from using the dual config IP so we need the breadcrumb
    alt_u32* cfm1_breadcrumb = get_ufm_ptr_with_offset(CFM1_BREAD_CRUMB);
    if (read_reconfig_reason() == RECONFIG_REASON_POWER_UP_OR_SWITCH_TO_CFM0 && *cfm1_breadcrumb != 0)
    {
        // Switch to the active image if Nios gets here from a powercycle.
        perform_cfm_switch(CPLD_CFM1);
    }

    if (read_reconfig_reason() == RECONFIG_REASON_POWER_UP_OR_SWITCH_TO_CFM0)
    {
        log_platform_state(PLATFORM_STATE_CPLD_UPDATE_IN_RECOVERY_MODE);
        perform_update_post_reconfig();
    }
    else
    {
        log_platform_state(PLATFORM_STATE_CPLD_RECOVERY_IN_RECOVERY_MODE);
        perform_cpld_recovery();
    }
}

