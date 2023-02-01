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
 * @file dual_config_utils.h
 * @brief Utility functions to communicate with the dual config IP.
 */

#ifndef EAGLESTREAM_INC_DUAL_CONFIG_UTILS_H_
#define EAGLESTREAM_INC_DUAL_CONFIG_UTILS_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#define RECONFIG_REASON_POWER_UP_OR_SWITCH_TO_CFM0 0b010

#define RECONFIG_STATE_REG_OFFSET 4
#define RECONFIG_STATE_BIT_OFFSET 13

/**
 * @brief Polling on the busy bit of the Dual Config IP.
 * Exit the function when the busy bit becomes 0.
 */
static void poll_on_dual_config_ip_busy_bit()
{
    while (IORD(U_DUAL_CONFIG_BASE, 3) & 0x1) {}
}

/**
 * @brief Reset the hardware watchdog timer.
 * This must be done every 1 second or the system will reboot into recovery.
 */
static void reset_hw_watchdog()
{
    poll_on_dual_config_ip_busy_bit();
    // Reset the watchdog timer
    // The reset bit is bit[1] of the dual config IP
    IOWR(U_DUAL_CONFIG_BASE, 0, (1 << 1));
}

/**
 * @brief Reads the master state machine value of the dual config IP to determine the
 * reason for the last reconfiguration.
 */
static alt_u32 read_reconfig_reason()
{
    // request all available information on the current and previous configuration state
    IOWR(U_DUAL_CONFIG_BASE, 2, 0xf);

    // Wait until the read data has been fetched
    poll_on_dual_config_ip_busy_bit();

    // The currents master state machine is at bits 16:13
    return (IORD(U_DUAL_CONFIG_BASE, RECONFIG_STATE_REG_OFFSET) & 0x1E000 ) >> RECONFIG_STATE_BIT_OFFSET;
}

#endif /* EAGLESTREAM_INC_DUAL_CONFIG_UTILS_H_ */
