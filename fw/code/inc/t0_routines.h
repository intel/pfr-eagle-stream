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
 * @file t0_routines.h
 * @brief functions that are executed during normal mode (T0) of operation.
 */

#ifndef EAGLESTREAM_INC_T0_ROUTINES_H_
#define EAGLESTREAM_INC_T0_ROUTINES_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "gen_gpo_controls.h"
#include "gen_gpi_signals.h"
#include "platform_log.h"
#include "t0_watchdog_handler.h"
#include "t0_provisioning.h"
#include "t0_update.h"
#include "transition.h"
#include "mctp_flow.h"

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)

/**
 * @brief Start attestation event after platform devices have boot up.
 *
 * For now, CPLD will only attest through PCIE path as BMC and PCH is not supporting SPDM for EGS
 *
 * @return none
 */
static void cpld_attest_post_boot_complete(MCTP_CONTEXT* mctp_ctx_ptr)
{
#ifdef USE_SYSTEM_MOCK
    if (SYSTEM_MOCK::get()->should_exec_code_block(
            SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS))
    {
        return;
    }
#endif
    alt_u32 boot_status = WDT_ALL_BOOT_DONE_MASK;
#ifdef PLATFORM_MULTI_NODES_ENABLED
    if (!(!check_bit(U_GPI_1_ADDR, GPI_1_HPFR_ACTIVE) || check_bit(U_GPI_1_ADDR, GPI_1_LEGACY)))
    {
        // In 2s system, ACM/BIOS ckpt is tracked
        // In 4s/8s system, only track on legacy board
        boot_status = WDT_BMC_BOOT_DONE_MASK | WDT_ME_BOOT_DONE_MASK;
    }
#endif
    // If BMC, BIOS and ME are still booting, exit
    if (wdt_boot_status == boot_status)
    {
        // By now, devices has completed boot, CPLD begins to challenge them
        // Only challenge if CPLD is required to do that during provisioning command
        if (check_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_READY_FOR_ATTESTATION_MASK) &&
            is_wd_timer_expired(WDT_CPLD_ATTEST_HOST_ADDR))
        {
            // Starts to challenge upon boot up and reset timer once attestation service finished
#ifdef SPDM_DEBUG

            // Set challenger type
            mctp_ctx_ptr->challenger_type = PCIE;

            cpld_challenge_all_devices(mctp_ctx_ptr, SMBUS);
            // Challenge PCIE and starts timer for exactly 2 minutes for debug purposes
            // Customize here for iteration timing
            start_timer(WDT_CPLD_ATTEST_HOST_ADDR, 0x1770);
#else
            // Set challenger type
            mctp_ctx_ptr->challenger_type = BMC;

            // If the duration set for CPLD to re-attest host has passed, reset the timer and re-attest.
            cpld_challenge_all_devices(mctp_ctx_ptr, SMBUS);
            start_timer(WDT_CPLD_ATTEST_HOST_ADDR, CPLD_REATTEST_HOST_TIME);
#endif
        }
        // Wait for attestation request from other devices
        check_incoming_message_i2c(mctp_ctx_ptr);
    }
}

#endif

/**
 * @brief After a seamless update, check if BMC react to the post seamless update bit
 *
 * @return none
 */
static alt_u32 post_seamless_update_routine()
{
    return is_wd_timer_expired(WDT_BMC_TIMER_ADDR) &&
           (check_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_POST_SEAMLESS_UPDATE_MASK)
            || check_spi_flash_state(SPI_FLASH_PCH, SPI_FLASH_STATE_POST_SEAMLESS_UPDATE_MASK));
}

/**
 * @brief Perform a BMC-only reset to perform authentication on BMC firmware, when
 * a BMC reset has been detected by the SPI control IP block.
 *
 * A BMC reading from its IBB is interpreted by the CPLD RoT as BMC reset
 * and triggers an re-authentication procedure by the CPLD RoT.
 *
 * This monitoring routine is started after BMC has completed boot.
 */
static void bmc_reset_handler()
{
    // If BMC is still booting, exit
    // If a BMC reset has not been detected, exit
    if (((wdt_boot_status & WDT_BMC_BOOT_DONE_MASK)
          && check_bit(U_GPI_1_ADDR, GPI_1_BMC_SPI_IBB_ACCESS_DETECTED))
          || post_seamless_update_routine())
    {
        // Clear IBB detection since we are now in the process of reacting to the reset.
        set_bit(U_GPO_1_ADDR, GPO_1_BMC_SPI_CLEAR_IBB_DETECTED);

        // Perform BMC only reset to re-authenticate its flash
        log_panic(LAST_PANIC_BMC_RESET_DETECTED);
        perform_bmc_only_reset();

        // Always clear post seamless state after BMC only reset,
        // This state is ONLY set immediately after seamless update
        clear_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_POST_SEAMLESS_UPDATE_MASK);
        clear_spi_flash_state(SPI_FLASH_PCH, SPI_FLASH_STATE_POST_SEAMLESS_UPDATE_MASK);
    }
}

/**
 * @brief Perform a BMC-only reset if BMC did not react to post update seamless bit
 *
 * In unprovisioned mode, just trigger a simple reset to the BMC withotu authentication
 *
 * @return none
 */
static void unprovisioned_bmc_reset_handler()
{
    if (post_seamless_update_routine())
    {
        // Clear IBB detection since we are now in the process of reacting to the reset.
        set_bit(U_GPO_1_ADDR, GPO_1_BMC_SPI_CLEAR_IBB_DETECTED);

        // Perform BMC only reset to re-authenticate its flash
        log_panic(LAST_PANIC_BMC_RESET_DETECTED);

        // Perform brief reset for BMC
        perform_unprovisioned_bmc_only_reset();

        // Clear the BMC timer
        IOWR(WDT_BMC_TIMER_ADDR, 0, 0);

        // Always clear post seamless state after BMC only reset,
        // This state is ONLY set immediately after seamless update
        clear_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_POST_SEAMLESS_UPDATE_MASK);
        clear_spi_flash_state(SPI_FLASH_PCH, SPI_FLASH_STATE_POST_SEAMLESS_UPDATE_MASK);
    }
}

/**
 * @brief Arm the ACM/BIOS watchdog timer when Nios firmware detects a platform reset
 * through PLTRST# GPI signal.
 */
static void platform_reset_handler()
{
    // When there's a platform reset, re-arm the ACM/BIOS watchdog timer
    if (check_bit(U_GPI_1_ADDR, GPI_1_PLTRST_DETECTED_REARM_ACM_TIMER))
    {
        // Clear the PLTRST detection flag
        set_bit(U_GPO_1_ADDR, GPO_1_CLEAR_PLTRST_DETECT_FLAG);
        clear_bit(U_GPO_1_ADDR, GPO_1_CLEAR_PLTRST_DETECT_FLAG);

        // Clear ACM/BIOS checkpoints
        write_to_mailbox(MB_ACM_CHECKPOINT, 0);
        write_to_mailbox(MB_BIOS_CHECKPOINT, 0);

        // Clear previous boot done status
        wdt_boot_status &= ~WDT_ACM_BIOS_BOOT_DONE_MASK;

        /*
         * Conditions to arm PCH ACM/BIOS watchdog:
         * 1. The ACM/BIOS watchdog is currently inactive
         * 2. PCH has valid recovery image. Otherwise, when there's a timeout, there's no image to recover to.
         */
        if(!check_bit(WDT_ACM_BIOS_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT)
                && !check_spi_flash_state(SPI_FLASH_PCH, SPI_FLASH_STATE_RECOVERY_FAILED_AUTH_MASK))
        {
#ifdef PLATFORM_MULTI_NODES_ENABLED
            if (!check_bit(U_GPI_1_ADDR, GPI_1_HPFR_ACTIVE)
                    || check_bit(U_GPI_1_ADDR, GPI_1_LEGACY))
            {
                // In 2s system, always arm the ACM/BIOS watchdog timer
                // In 4s/8s system, only arm the ACM/BIOS watchdog timer on the legacy board

                // Start the ACM/BIOS watchdog timer
                start_timer(WDT_ACM_BIOS_TIMER_ADDR, WD_ACM_TIMEOUT_VAL);
            }
            else
            {
                // Disable ACM/BIOS watchdog timer
                wdt_enable_status &= ~WDT_ENABLE_ACM_BIOS_TIMER_MASK;
            }
#else
            // Start the ACM/BIOS watchdog timer
            start_timer(WDT_ACM_BIOS_TIMER_ADDR, WD_ACM_TIMEOUT_VAL);
#endif
        }

        // When detected a platform reset, process the deferred updates
        // Simply clear the update at reset bit. The mb_update_intent_handler function will pick up the update.
        clear_mailbox_register_bit(MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_UPDATE_AT_RESET_BIT_POS);
        clear_mailbox_register_bit(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_UPDATE_AT_RESET_BIT_POS);
    }
}

/**
 * @brief Check to see if there was a panic event in the other PFR CPLDs.
 * Should be called in main polling loop and can transition the platform into T-1 mode.
 * This function is only executed when the hierarchical PFR feaure is supported.
 */
static void check_for_hpfr_panic_event()
{
#ifdef PLATFORM_MULTI_NODES_ENABLED
    // Check if we are using hierarchical PFR
    if (check_bit(U_GPI_1_ADDR, GPI_1_HPFR_ACTIVE))
    {
        if (!check_bit(U_GPI_1_ADDR, GPI_1_HPFR_IN))
        {
            perform_platform_reset();
        }
    }
#endif
}

/**
 * @brief Perform all T0 operations.
 *
 * These T0 operations include:
 * - Monitor mailbox UFM provisioning commands.
 * - Monitor boot progress of the critical-to-boot components (e.g. BMC) and
 * ensure a successful boot is achieved within the expected time period.
 * - Monitor mailbox update intent registers.
 * - Detect BMC reset and react to it.
 * - Perform post-update flows (e.g. bumping SVN after a successful active update and
 * trigger recovery update).
 *
 * If hierarchical PFR is supported, Nios firmware also checks whether other CPLDs
 * have any panic event.
 *
 * @see mb_ufm_provisioning_handler()
 * @see check_for_hpfr_panic_event()
 * @see watchdog_routine()
 * @see mb_update_intent_handler()
 * @see bmc_reset_handler()
 * @see post_update_routine()
 */
static void perform_t0_operations()
{
    // Initialize MCTP context
    MCTP_CONTEXT mctp_ctx;
    MCTP_CONTEXT* mctp_ctx_ptr = (MCTP_CONTEXT*) &mctp_ctx;
    init_mctp_context(mctp_ctx_ptr);

    while (1)
    {
        // Pet the HW watchdog
        reset_hw_watchdog();

        // Check UFM provisioning request
        mb_ufm_provisioning_handler();

        // Monitor HPFR event if HPFR is supported
        check_for_hpfr_panic_event();

        // Activities for provisioned system
        if (is_ufm_provisioned())
        {
            // Detect PLTRST
            // TO DO: Removing IFDEF when the platform is ready

            platform_reset_handler();

            // Monitor BMC/ME/ACM/BIOS boot progress
            watchdog_routine();

            // Check updates
            mb_update_intent_handler();

            // Monitor BMC reset
            bmc_reset_handler();

            // Finish up any firmware/CPLD update in progress
            post_update_routine();

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)
            // Start attesting devices
            cpld_attest_post_boot_complete(mctp_ctx_ptr);
#endif
        }
#ifdef PLATFORM_SEAMLESS_FEATURES_ENABLED

        // If Spi Geometry has been provisioned
        // Allow seamless update without signature verification
        if (is_ufm_spi_geo_provisioned())
        {
            mb_unprovisioned_seamless_handler();
            unprovisioned_bmc_reset_handler();
        }
#endif

#ifdef USE_SYSTEM_MOCK
        if (SYSTEM_MOCK::get()->should_exec_code_block(
                SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS))
        {
            break;
        }
        if (SYSTEM_MOCK::get()->should_exec_code_block(
                SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_TIMED_BOOT))
        {
            if (wdt_boot_status == WDT_ALL_BOOT_DONE_MASK)
                break;
        }
        if (SYSTEM_MOCK::get()->should_exec_code_block(
                SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS))
        {
            if (SYSTEM_MOCK::get()->get_code_block_counter() > 50)
            {
                break;
            }
            SYSTEM_MOCK::get()->incr_code_block_counter();
        }
        if (SYSTEM_MOCK::get()->should_exec_code_block(
                SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS))
        {
            if (SYSTEM_MOCK::get()->get_code_block_counter() > 50)
            {
                break;
            }
            SYSTEM_MOCK::get()->incr_code_block_counter();
        }
#endif
    }
}

#endif /* EAGLESTREAM_INC_T0_ROUTINES_H_ */
