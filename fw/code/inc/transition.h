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
 * @file transition.h
 * @brief Responsible for T0 / T-1 / T0 mode transition.
 */

#ifndef EAGLESTREAM_INC_TRANSITION_H_
#define EAGLESTREAM_INC_TRANSITION_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "tmin1_routines.h"
#include "hierarchical_pfr.h"

/**
 * @brief Prepare the platform to enter T-1 mode.
 * 
 * Steps:
 * 1. Drive the SLP_SUS_N pin high prior to setting RSMRST# and DSWPWROK.
 * 2. Assert resets on BMC and PCH through SRST# and RSMRST# pin. 
 * 3. Take over the SPI flash devices controls through the SPI control block.
 * 4. Drive DSWPWROK to 0.
 * 
 * If hierarchical PFR is supported, Nios firmware also notifies other CPLDs that
 * it is entering T-1 mode.
 */
static void perform_entry_to_tmin1()
{
#ifdef USE_SYSTEM_MOCK
    SYSTEM_MOCK::get()->incr_t_minus_1_counter();
#endif

    // log that we are entering T-1 mode
    log_platform_state(PLATFORM_STATE_ENTER_TMIN1);

    // Notify other CPLDs if HPFR is supported
    perform_hpfr_entry_to_tmin1();

    // PFR should drive the new PFR_SLP_SUS_N pin high, during a T0 to T-1 transition.
    // It should drive that high before setting RSMRST# and DSWPWROK low.
    set_bit(U_GPO_1_ADDR, GPO_1_FM_PFR_SLP_SUS_N);

    // Assert reset on BMC and PCH
    clear_bit(U_GPO_1_ADDR, GPO_1_RST_SRST_BMC_PLD_R_N);
    clear_bit(U_GPO_1_ADDR, GPO_1_RST_RSMRST_PLD_R_N);

    // Take over controls on the SPI flash devices
    takeover_spi_ctrls();

    // Set Deep SX. This will drive 0 on PWRGD_DSW_PWROK_R
    set_bit(U_GPO_1_ADDR, GPO_1_PWRGD_DSW_PWROK_R);
}

/**
 * @brief Perform the platform to enter T0 mode. 
 *
 * The main tasks here are:
 *   - release controls of SPI flash devices
 *   - release BMC and PCH from resets
 *   - launch watchdog timers if the system is provisioned.
 * 
 * If a SPI flash device is found to have invalid images for active, recovery and staging regions, 
 * then Nios firmware will keep that associated component in reset. 
 * 
 * If only BMC SPI flash has valid image(s), then boot BMC in isolation with watchdog timer turned off. 
 * BMC's behavior is unknown when PCH is in reset. 
 * 
 * When both SPI flash device have invalid images, Nios firmware enters a lockdown mode in T-1. 
 * 
 * If hierarchical PFR is supported, Nios firmware also notifies other CPLDs that
 * it is entering T0 mode.
 */
static void perform_entry_to_t0()
{
    // Notify other CPLDs if HPFR is supported
    perform_hpfr_entry_to_t0();

    // If both BMC and PCH flashes failed authentication for all regions, stay in T-1 mode.
    if (check_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_ALL_REGIONS_FAILED_AUTH_MASK) &&
            check_spi_flash_state(SPI_FLASH_PCH, SPI_FLASH_STATE_ALL_REGIONS_FAILED_AUTH_MASK))
    {
        log_platform_state(PLATFORM_STATE_AUTHENTICATION_FAILED_LOCKDOWN);
        never_exit_loop();
    }

    // Boot BMC and PCH
    tmin1_boot_bmc_and_pch();

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)
    if (check_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_READY_FOR_ATTESTATION_MASK))
    {
        // Start attestation timer
        start_timer(WDT_CPLD_ATTEST_HOST_ADDR, 0);
    }
#endif

    log_platform_state(PLATFORM_STATE_ENTER_T0);
}

/**
 * @brief Transition the platform to T-1 mode, perform T-1 operations 
 * if the PFR system is provisioned, and then transition the platform back to T0 mode.
 */
static void perform_platform_reset()
{
    // Prepare for transition to T-1 mode
    perform_entry_to_tmin1();

    // In unprovisioned state, skip T-1 operations, filter enabling and boot monitoring.
    if (is_ufm_provisioned())
    {
        // Perform T-1 operations
        perform_tmin1_operations();
    }

#ifdef USE_QUAD_IO
    // Unset the QE bit
    if (read_spi_chip_id() == SPI_FLASH_MACRONIX)
    {
        configure_qe_bit(RESTORE_QE_BIT);
    }
#endif

    // Perform the entry to T0
    perform_entry_to_t0();
}

/********************************************
 *
 * BMC only reset
 *
 ********************************************/

/**
 * @brief This function transitions BMC to a reset state through EXTRST#. 
 *
 * When in T0 mode, Nios firmware may detect some panic events such as BMC 
 * watchdog timer timeout and BMC firmware update. In order to keep the host
 * up while recovering or updating BMC, Nios firmware performs a BMC-only reset. 
 */
static void perform_entry_to_tmin1_bmc_only()
{
#ifdef USE_SYSTEM_MOCK
    SYSTEM_MOCK::get()->incr_t_minus_1_bmc_only_counter();
#endif
    // Assert EXTRST# to reset only the BMC
    clear_bit(U_GPO_1_ADDR, GPO_1_RST_PFR_EXTRST_N);

    // Take over control on the BMC flash device
    takeover_spi_ctrl(SPI_FLASH_BMC);
}

/**
 * @brief This function transitions BMC back to T0 state through EXTRST#. 
 * 
 * After releasing SPI control of BMC flash back to BMC, Nios firmware releases
 * BMC from reset by setting EXTRST#. 
 * 
 * Since BMC-only reset only happens in a provisioned system, BMC watchdog timer 
 * is always ran after releasing the reset.
 */
static void perform_entry_to_t0_bmc_only()
{
    // Boot BMC only if it has valid image in its SPI flash
    if (!(check_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_ALL_REGIONS_FAILED_AUTH_MASK) ||
          check_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_ATTESTATION_LOCKDOWN_RESET_MASK)))
    {
        // Release SPI flash control to BMC
        release_spi_ctrl(SPI_FLASH_BMC);

        // Release reset on BMC
        set_bit(U_GPO_1_ADDR, GPO_1_RST_PFR_EXTRST_N);

        // Don't turn on BMC WDT when PCH is in reset, because BMC may have undefined behavior.
        if (!check_spi_flash_state(SPI_FLASH_PCH, SPI_FLASH_STATE_ALL_REGIONS_FAILED_AUTH_MASK))
        {
            // Track the BMC boot progress
            // Clear any previous BMC boot done status
            wdt_boot_status &= ~WDT_BMC_BOOT_DONE_MASK;

            // Start the BMC watchdog timer
            start_timer(WDT_BMC_TIMER_ADDR, WD_BMC_TIMEOUT_VAL);
        }
    }
}

/**
 * @brief This function transitions BMC back to T0 state through EXTRST#.
 *
 * After releasing SPI control of BMC flash back to BMC, Nios firmware releases
 * BMC from reset by setting EXTRST#.
 *
 */
static void perform_unprovisioned_bmc_only_reset()
{
#ifdef USE_SYSTEM_MOCK
    SYSTEM_MOCK::get()->incr_t_minus_1_bmc_only_counter();
#endif
    log_platform_state(PLATFORM_STATE_ENTER_TMIN1);

    // Assert EXTRST# to reset only the BMC
    //clear_bit(U_GPO_1_ADDR, GPO_1_RST_PFR_EXTRST_N);

    // Enter T-1 for BMC
    perform_entry_to_tmin1_bmc_only();

    // Sleep for a short while
    sleep_20ms(1);

    // Release SPI flash control to BMC, just release to guarantee although we did not take over
    release_spi_ctrl(SPI_FLASH_BMC);

    // Release reset on BMC
    set_bit(U_GPO_1_ADDR, GPO_1_RST_PFR_EXTRST_N);

    log_platform_state(PLATFORM_STATE_ENTER_T0);
}

/**
 * @brief Transition the BMC to T-1 mode, perform T-1 operations, 
 * and then transition the BMC back to T0 mode.
 * 
 * Since BMC-only reset only happens in a provisioned system, Nios firmware always
 * perform the BMC specific T-1 operations.
 */
static void perform_bmc_only_reset()
{
    log_platform_state(PLATFORM_STATE_ENTER_TMIN1);

    // Enter T-1 for BMC
    perform_entry_to_tmin1_bmc_only();

    // Perform T-1 operations on BMC (assuming that Nios is provisioned at this point)
    perform_tmin1_operations_for_bmc();

    // Enter T0 for BMC
    perform_entry_to_t0_bmc_only();

    log_platform_state(PLATFORM_STATE_ENTER_T0);
}

/********************************************
 *
 * PCH only reset
 *
 ********************************************/

/**
 * @brief Transition the PCH to T-1 mode, perform T-1 operations,
 * and then transition the PCH back to T0 mode.
 *
 * Since PCH-only reset only happens in a provisioned system, Nios firmware always
 * perform the PCH specific T-1 operations.
 */
static void perform_entry_to_tmin1_pch_only()
{
#ifdef USE_SYSTEM_MOCK
    SYSTEM_MOCK::get()->incr_t_minus_1_pch_only_counter();
#endif
    // PFR should drive the new PFR_SLP_SUS_N pin high, during a T0 to T-1 transition.
    // It should drive that high before setting RSMRST# and DSWPWROK low.
    set_bit(U_GPO_1_ADDR, GPO_1_FM_PFR_SLP_SUS_N);

    // Assert reset on PCH
    clear_bit(U_GPO_1_ADDR, GPO_1_RST_RSMRST_PLD_R_N);

    // Take over PCH SPI flash control
    takeover_spi_ctrl(SPI_FLASH_PCH);

    // Set Deep SX. This will drive 0 on PWRGD_DSW_PWROK_R
    set_bit(U_GPO_1_ADDR, GPO_1_PWRGD_DSW_PWROK_R);
}

/**
 * @brief Transition the PCH to T-1 mode, perform T-1 operations,
 * and then transition the PCH back to T0 mode.
 *
 * Since PCH-only reset only happens in a provisioned system, Nios firmware always
 * perform the PCH specific T-1 operations.
 */
static void perform_pch_only_reset()
{
    log_platform_state(PLATFORM_STATE_ENTER_TMIN1);

    // Enter T-1 for PCH
    perform_entry_to_tmin1_pch_only();

    // Perform T-1 operations on PCH (assuming that Nios is provisioned at this point)
    perform_tmin1_operations_for_pch();

    deep_sleep_routine();

    // Boot up PCH
    tmin1_boot_pch();

    log_platform_state(PLATFORM_STATE_ENTER_T0);
}


#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)

/**
 * @brief Transition the system to T-1 mode post attestation failure. If reset is to be done,
 * PFR will toggle EXTRST or pltrst accoddingly and even perform recovery if policies are applied.
 *
 * @param spi_flash_type: BMC or PCH
 * @param policy : AFM policy
 *
 */
static void perform_attestation_policy_reset(SPI_FLASH_TYPE_ENUM spi_flash_type, alt_u8 policy)
{
    if (get_fw_recovery_level(spi_flash_type) & SPI_REGION_PROTECT_MASK_RECOVER_BITS)
    {
        // Log panic event
        log_panic(LAST_PANIC_ATTESTATION_FAIURE);

        if (spi_flash_type == SPI_FLASH_BMC)
        {
            if (policy & AFM_POLICY_BMC_ONLY_RESET_BIT_MASK)
            {
                set_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_ATTESTATION_LOCKDOWN_RESET_MASK);
            }
            else
            {
                // Requires recovery action as defined in policy, log event
                log_recovery(LAST_RECOVERY_BMC_ATTESTATION_FAILED);
                set_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_ATTESTATION_REQUIRE_RECOVERY_MASK);
            }
            perform_bmc_only_reset();
        }
        else
        {
            if (policy & AFM_POLICY_CPU_ONLY_RESET_BIT_MASK)
            {
                set_spi_flash_state(SPI_FLASH_PCH, SPI_FLASH_STATE_ATTESTATION_LOCKDOWN_RESET_MASK);
            }
            else
            {
                // Requires recovery action as defined in policy, log event
                log_recovery(LAST_RECOVERY_CPU_ATTESTATION_FAILED);
                set_spi_flash_state(SPI_FLASH_PCH, SPI_FLASH_STATE_ATTESTATION_REQUIRE_RECOVERY_MASK);
            }
            perform_pch_only_reset();
        }
    }
}

#endif

#endif /* EAGLESTREAM_INC_TRANSITION_H_ */
