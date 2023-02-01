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
 * @file hierarchical_pfr.h
 * @brief functions that are supposed to be called when Hierarchical PFR is being used
 * the signal "LEGACY" Should be set to 1 for the master only so it is used to identify if we are a master or slave.
 */

#ifndef EAGLESTREAM_INC_HIERARCHICAL_PFR_H_
#define EAGLESTREAM_INC_HIERARCHICAL_PFR_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "gen_gpi_signals.h"
#include "gen_gpo_controls.h"
#include "utils.h"
#include "platform_log.h"

/**
 * @brief Perform hierarchical portion of routine for entry to T-1 if HPFR feature is in use.
 * 
 * If this CPLD is a slave CPLD, Nios would proceed to T-1 mode. 
 * If this CPLD is a master CPLD, Nios would not proceed to T-1 mode until all 
 * slave CPLDs have transitioned to T-1 mode. 
 * 
 * This function should be called when Nios transitions to T-1 mode.
 */
static void perform_hpfr_entry_to_tmin1()
{
#ifdef PLATFORM_MULTI_NODES_ENABLED
    // Proceed to perform HPFR routine if HPFR feature is in use.
    if (check_bit(U_GPI_1_ADDR, GPI_1_HPFR_ACTIVE))
    {
        // Notify the other CPLDs that this CPLD is entering T-1.
        clear_bit(U_GPO_1_ADDR, GPO_1_HPFR_OUT);
        // The SGPIO interface that HPFR_IN and OUT use are very slow,
        // so all cplds should wait here to let the signals propagate in all nodes
        sleep_20ms(50);
    }
#endif
}

/**
 * @brief Perform hierarchical portion of routine for entry to T0 if HPFR feature is in use.
 *
 * If this CPLD is a slave CPLD, Nios would notify the master CPLD that it's ready to transition
 * to T0 mode. When the master CPLD signals, Nios can then proceed to T0 mode.
 * If this CPLD is a master CPLD, Nios would wait for the signal that all slave CPLDs are
 * ready for transition to T0 mode. When that happens, Nios signals to let the slave CPLDs to
 * proceed and then transitions to T0 mode.
 *
 * This function should be called when Nios transitions to T0 mode.
 */
static void perform_hpfr_entry_to_t0 ()
{
#ifdef PLATFORM_MULTI_NODES_ENABLED
    // Proceed to perform HPFR routine if HPFR feature is in use.
    if (check_bit(U_GPI_1_ADDR, GPI_1_HPFR_ACTIVE))
    {
        alt_u32 is_legacy_node = check_bit(U_GPI_1_ADDR, GPI_1_LEGACY);

        // Slave routine
        if (!is_legacy_node)
        {
            // Notify the master CPLD that this slave node is waiting to get into T0 mode.
            set_bit(U_GPO_1_ADDR, GPO_1_HPFR_OUT);
        }

        // Wait for all nodes to get ready to go into T0 mode.
        // Master CPLD will get pass this once all slave CPLDs have sent the signal for entering T0 mode.
        while (!check_bit(U_GPI_1_ADDR, GPI_1_HPFR_IN))
        {
            reset_hw_watchdog();
        }

        // Master routine
        if (is_legacy_node)
        {
            // Allow the slave CPLDs to go into T0 mode.
            set_bit(U_GPO_1_ADDR, GPO_1_HPFR_OUT);
        }

        // The SGPIO interface that HPFR_IN and OUT use are very slow,
        // so all cplds should wait here to let the signals propagate in all nodes
        sleep_20ms(50);
    }
#endif
}

#endif /* EAGLESTREAM_INC_HIERARCHICAL_PFR_H_ */
