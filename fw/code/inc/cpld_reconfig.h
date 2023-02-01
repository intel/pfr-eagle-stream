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
 * @file cpld_reconfig.h
 * @brief Functions that perform CPLD reconfiguration.
 */

#ifndef EAGLESTREAM_INC_CPLD_RECONFIG_H_
#define EAGLESTREAM_INC_CPLD_RECONFIG_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "gen_gpi_signals.h"
#include "gen_gpo_controls.h"
#include "timer_utils.h"
#include "utils.h"

/**
 * @brief Preparation before reconfiguring CPLD.
 */
static void prep_for_reconfig()
{
    // Clear Deep SX. This will drive Z onto PWRGD_DSW_PWROK_R
    clear_bit(U_GPO_1_ADDR, GPO_1_PWRGD_DSW_PWROK_R);

    // Add some delays between DSW_PWROK and PFR_SLP_SUS
    // The max time for this transition is 100ms. Set a delay of 120ms to be safe.
    // If this delay is not added or is too small, platform would enter
    //   a power loop (constantly shutting down and then power up).

    // IMPORTANT NOTE: In certain platform, PFR_SLP_SUS Minimum Assertion Width might be different.
    // BIOS can set PFR_SLP_SUS Minimum Assertion Width from 100ms to 4s.
    // In this reference, it is needed to set some delay between
    // DSW_PWROK and PFR_SLP_SUS to approx 100ms seconds.
    sleep_20ms(6);

}

/**
 * @brief Perform switching of CPLD image to use either CFM0 or CFM1
 *
 * @param cfm_one_or_zero an enum value indicating which CFM to switch to
 */
static void perform_cfm_switch(CPLD_CFM_TYPE_ENUM cfm_one_or_zero)
{
    // Set ConfigSelect register to either CFM1 or CFM0, bit[1] represent CFM selection, bit[0] commit to register
    IOWR(U_DUAL_CONFIG_BASE, 1, (cfm_one_or_zero << 1) | 1);

    // Check if written new ConfigSelect value being consumed
    poll_on_dual_config_ip_busy_bit();

    // We need to deassert deep sleep before starting reconfiguration
    // We will lose power when all pins to go high-Z otherwise
    prep_for_reconfig();

    // Trigger reconfiguration
    IOWR(U_DUAL_CONFIG_BASE, 0, 0x1);

#ifdef USE_SYSTEM_MOCK
    SYSTEM_MOCK::get()->reset_after_cpld_reconfiguration();

    // User can enable this block to exit the program after a CFM switch
    // On platform, Nios program would have exited here.
    if (SYSTEM_MOCK::get()->should_exec_code_block(
            SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_AFTER_CFM_SWITCH))
    {
        PFR_ASSERT(0)
    }
#endif
}

#endif /* EAGLESTREAM_INC_CPLD_RECONFIG_H_ */
