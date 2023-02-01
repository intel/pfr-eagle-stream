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
 * @file global_state.h
 * @brief Global state machine and state for the PFR Nios System.
 */

#ifndef EAGLESTREAM_INC_PFR_GLOBAL_STATE_H
#define EAGLESTREAM_INC_PFR_GLOBAL_STATE_H

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "status_enums.h"
#include "utils.h"

#define U_GLOBAL_STATE_REG_ADDR __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_GLOBAL_STATE_REG_BASE, 0)

/**
 * @brief Set the global state to an enum defined state.
 * This state is shown on the 7-seg display on the platform, if there's one. 
 * The same state encoding is used for the platform state register in the mailbox. 
 *
 * @param state : platform state
 */
static void set_global_state(const STATUS_PLATFORM_STATE_ENUM state)
{
    // Write the bottom 16 bits of the global state register using the state
    alt_u32 val = IORD_32DIRECT(U_GLOBAL_STATE_REG_ADDR, 0);
    val &= (0xFFFF0000);
    val |= (0x0000FFFF & (alt_u32) state);
    IOWR_32DIRECT(U_GLOBAL_STATE_REG_ADDR, 0, val);
}

#endif /* EAGLESTREAM_INC_PFR_GLOBAL_STATE_H */
