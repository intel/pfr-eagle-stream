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
 * @file t0_update.h
 * @brief Monitor the BMC and PCH update intent registers and trigger panic event appropriately.
 */

#ifndef EAGLESTREAM_INC_T0_UPDATE_H_
#define EAGLESTREAM_INC_T0_UPDATE_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "firmware_update.h"
#include "transition.h"
#include "mailbox_utils.h"
#include "platform_log.h"
#include "spi_flash_state.h"

#ifdef PLATFORM_SEAMLESS_FEATURES_ENABLED
#include "seamless_update.h"
#endif

/**
 * @brief Monitor PCH update intent register (part 1) and transition to T-1 mode to perform the update if necessary.
 *
 * This function reads the PCH update intent from the mailbox first.
 *
 * If the update intent is empty, exit.
 * If the update intent indicates a deferred update, exit.
 * If the update intent is not a valid request (e.g. BMC update), exit.
 * Nios also allows qualifier bit, such as UPDATE_DYNAMIC, to be written first.
 *
 * For the following scenarios, log some error, clear the update intent register and exit.
 * - It's a firmware update and maximum failed update attempts have been reached for that flash device.
 * - It's a CPLD update and maximum failed CPLD update attempts have been reached.
 * - It's a active firmware update and the PCH recovery image is corrupted
 *
 * Once all these checks have passed, Nios firmware proceed to trigger the update.
 * If it's a PCH active firmware update, Nios performs a PCH-only reset. Otherwise, Nios brings down the whole platform.
 * The actual update is done as part of the T-1 operations.
 *
 * @see act_on_update_intent
 * @see perform_tmin1_operations_for_pch
 */
static void mb_update_intent_handler_for_pch()
{
    // Read the update intent
    alt_u32 update_intent = read_from_mailbox(MB_PCH_UPDATE_INTENT_PART1);

    // Check if there's any update intent.
    // If the update is not a deferred update, proceed to validate the update intent value.
    if (update_intent &&
            ((update_intent & MB_UPDATE_INTENT_UPDATE_AT_RESET_MASK) != MB_UPDATE_INTENT_UPDATE_AT_RESET_MASK))
    {
        /*
         * Validate the update intent
         */
        if (update_intent == MB_UPDATE_INTENT_UPDATE_DYNAMIC_MASK)
        {
            // Nios allows the qualifier bit is written (i.e. UPDATE_DYNAMIC bit in this case) to be written first.
            // Do nothing and wait until the main bits to be written.
            return;
        }

        alt_u32 minor_err = 0;
        if ((update_intent & MB_UPDATE_INTENT_PCH_FW_UPDATE_MASK) == 0)
        {
            // This value is not valid for PCH update intent register
            minor_err = MINOR_ERROR_INVALID_UPDATE_INTENT;
        }

        if (num_failed_fw_or_cpld_update_attempts >= MAX_FAILED_UPDATE_ATTEMPTS_ALLOWED)
        {
            // If there are too many failed firmware update attempts, reject this update.
            minor_err = MINOR_ERROR_EXCEEDED_MAX_FAILED_ATTEMPTS;
        }

        if (check_spi_flash_state(SPI_FLASH_PCH, SPI_FLASH_STATE_RECOVERY_FAILED_AUTH_MASK))
        {
            // There's no valid recovery image

            // Block Active update
            if (update_intent & MB_UPDATE_INTENT_PCH_ACTIVE_MASK)
            {
                minor_err = MINOR_ERROR_ACTIVE_FW_UPDATE_NOT_ALLOWED;
            }

            // Allow update to recovery image directly
            if (update_intent & MB_UPDATE_INTENT_PCH_RECOVERY_MASK)
            {
                // In T-1 step, Nios would do the followings when this flag is set:
                // 1. Ensure update capsule matches the active image
                // 2. Promote the update capsule to the recovery region
                set_spi_flash_state(SPI_FLASH_PCH, SPI_FLASH_STATE_HAS_PENDING_RECOVERY_FW_UPDATE_MASK);
                write_to_mailbox(MB_PCH_UPDATE_INTENT_PART1, 0);
            }
        }

        if (minor_err)
        {
            // If there's any error, abort
            // No need to increment the failed update attempts counter, since Nios is in T0
            log_update_failure(minor_err, 0);
            write_to_mailbox(MB_PCH_UPDATE_INTENT_PART1, 0);
        }
        else
        {
            /*
             * Proceed to transition to T-1 mode and perform the update
             */
            // Log panic event
            log_panic(LAST_PANIC_PCH_UPDATE_INTENT);

            // Enter T-1 mode to do the update there
            if (update_intent & MB_UPDATE_INTENT_PCH_ACTIVE_MASK)
            {
                // Only bring down PCH if only PCH active firmware is being updated
                perform_pch_only_reset();
            }
            else
            {
                perform_platform_reset();
            }
        }
    }
}

/**
 * @brief Monitor BMC update intent register (part 1) and transition to T-1 mode to perform the update if necessary.
 *
 * This function reads the BMC update intent from the mailbox first.
 *
 * If the update intent is empty, exit.
 * If the update intent indicates a deferred update, exit.
 * Nios also allows qualifier bit, such as UPDATE_DYNAMIC, to be written first.
 *
 * Note that all the bits in update intent are valid in BMC update intent register, since Nios supports OOB BMC,
 * PCH and CPLD active/recovery update.
 *
 * For the following scenarios, log some error, clear the update intent register and exit.
 * - It's a firmware update and maximum failed update attempts have been reached for that flash device.
 * - It's a CPLD update and maximum failed CPLD update attempts have been reached.
 * - It's a BMC active firmware update and the BMC recovery image is corrupted
 *
 * Once all these checks have passed, Nios firmware proceed to trigger the update.
 * If it's a BMC active firmware update, Nios performs a BMC-only reset. Otherwise, Nios brings down the whole platform.
 * The actual update is done as part of the T-1 operations.
 *
 * @see act_on_update_intent
 * @see perform_tmin1_operations_for_bmc
 */
static void mb_update_intent_handler_for_bmc()
{
    // Read the update intent
    alt_u32 update_intent = read_from_mailbox(MB_BMC_UPDATE_INTENT_PART1);

    // Read update intent part 2 for AFM
    alt_u32 update_intent_2 = read_from_mailbox(MB_BMC_UPDATE_INTENT_PART2) & MB_UPDATE_INTENT_AFM_FW_UPDATE_MASK;

    // If two update intent is received, always prioritize update_intent_part_1
    if (update_intent & MB_UPDATE_INTENT_FW_OR_CPLD_UPDATE_MASK)
    {
        update_intent_2 = 0;
    }

    // Check if there's any update intent.
    // If the update is not a deferred update, proceed to validate the update intent value.
    if ((update_intent || update_intent_2) &&
            ((update_intent & MB_UPDATE_INTENT_UPDATE_AT_RESET_MASK) != MB_UPDATE_INTENT_UPDATE_AT_RESET_MASK))
    {
        /*
         * Validate the update intent
         */
        if ((update_intent == MB_UPDATE_INTENT_UPDATE_DYNAMIC_MASK) && (!update_intent_2))
        {
            // Nios allows the qualifier bit is written (i.e. UPDATE_DYNAMIC bit in this case) to be written first.
            // Do nothing and wait until the main bits to be written.
            return;
        }

        alt_u32 minor_err = 0;
        alt_u32 minor_err_2 = 0;

        if (num_failed_fw_or_cpld_update_attempts >= MAX_FAILED_UPDATE_ATTEMPTS_ALLOWED)
        {
            // If there are too many failed firmware/CPLD update attempts, reject this update.
            minor_err = MINOR_ERROR_EXCEEDED_MAX_FAILED_ATTEMPTS;
        }

        if (check_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_RECOVERY_FAILED_AUTH_MASK))
        {
            // There's no valid recovery image

            // Block Active update
            if (update_intent & MB_UPDATE_INTENT_BMC_ACTIVE_MASK)
            {
                minor_err = MINOR_ERROR_ACTIVE_FW_UPDATE_NOT_ALLOWED;
            }

            // Allow update to recovery image directly
            if (update_intent & MB_UPDATE_INTENT_BMC_RECOVERY_MASK)
            {
                // In T-1 step, Nios would do the followings when this flag is set:
                // 1. Ensure update capsule matches the active image
                // 2. Promote the update capsule to the recovery region
                set_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_HAS_PENDING_RECOVERY_FW_UPDATE_MASK);
                write_to_mailbox(MB_BMC_UPDATE_INTENT_PART1, 0);
            }

            // If BMC recovery image is bad, any update to AFM should be prohibited
            if (update_intent_2)
            {
                minor_err_2 = MINOR_ERROR_AFM_UPDATE_NOT_ALLOWED;
            }
        }

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)

        if (check_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_AFM_RECOVERY_FAILED_AUTH_MASK))
        {
            // Do not allow update for active AFM
            if (update_intent_2 & MB_UPDATE_INTENT_AFM_ACTIVE_MASK)
            {
                minor_err_2 = MINOR_ERROR_AFM_UPDATE_NOT_ALLOWED;
            }

            // Allow update to recovery image directly
            if (update_intent_2 & MB_UPDATE_INTENT_AFM_RECOVERY_MASK)
            {
                if (check_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_READY_FOR_ATTESTATION_MASK))
                {
                    // In T-1 step, Nios would do the followings when this flag is set:
                    // 1. Ensure update capsule matches the active image
                    // 2. Promote the update capsule to the recovery region
                    set_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_HAS_PENDING_RECOVERY_AFM_UPDATE_MASK);
                    // Careful not to clear seamless update intent
                    clear_mailbox_register_bit(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_BIT_POS);
                    clear_mailbox_register_bit(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_RECOVERY_BIT_POS);
                }
                else
                {
                    minor_err_2 = MINOR_ERROR_AFM_UPDATE_NOT_ALLOWED;
                }
            }
        }
#endif

        if (check_spi_flash_state(SPI_FLASH_PCH, SPI_FLASH_STATE_RECOVERY_FAILED_AUTH_MASK) ||
                check_spi_flash_state(SPI_FLASH_PCH, SPI_FLASH_STATE_ALL_REGIONS_FAILED_AUTH_MASK))
        {
            // Block PCH active update when there's no valid recovery image in PCH flash
            if (update_intent & MB_UPDATE_INTENT_PCH_ACTIVE_MASK)
            {
                minor_err = MINOR_ERROR_ACTIVE_FW_UPDATE_NOT_ALLOWED;
            }
        }

        if (minor_err)
        {
            // If there's any error, abort
            // No need to increment the failed update attempts counter, since Nios is in T0
            log_update_failure(minor_err, 0);
            write_to_mailbox(MB_BMC_UPDATE_INTENT_PART1, 0);
        }
        else if (minor_err_2)
        {
            // If there's any error, abort
            // No need to increment the failed update attempts counter, since Nios is in T0
            log_update_failure(minor_err_2, 0);
            // Careful not to clear seamless update intent
            clear_mailbox_register_bit(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_BIT_POS);
            clear_mailbox_register_bit(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_RECOVERY_BIT_POS);
        }
        else
        {
            /*
             * Proceed to transition to T-1 mode and perform the update
             * If there is pending FW recovery update and cpld update in process, skip entering T-1 here.
             * Wait for updated active devices to be booted up first
             */
            if ((check_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_HAS_PENDING_RECOVERY_FW_UPDATE_MASK) ||
                check_spi_flash_state(SPI_FLASH_PCH, SPI_FLASH_STATE_HAS_PENDING_RECOVERY_FW_UPDATE_MASK)) &&
                (update_intent & MB_UPDATE_INTENT_CPLD_MASK))
            {
                return;
            }

            // Log panic event
            log_panic(LAST_PANIC_BMC_UPDATE_INTENT);

            // Enter T-1 mode to do the update there
            if ((update_intent & MB_UPDATE_INTENT_BMC_REQUIRE_PLATFORM_RESET_MASK) || (update_intent_2 & MB_UPDATE_INTENT_AFM_FW_UPDATE_MASK))
            {
                perform_platform_reset();
            }
            else
            {
                // Only bring down BMC if only BMC active firmware is being updated
                perform_bmc_only_reset();
            }
        }
    }
}

#ifdef PLATFORM_SEAMLESS_FEATURES_ENABLED

/**
 * @brief Monitor PCH update intent register (part 2) and perform Seamless update if requested.
 *
 * @see act_on_update_intent
 */
static void mb_update_intent_part2_handler_for_pch()
{
    // Read and clear the update intent register
    alt_u32 update_intent = read_from_mailbox(MB_PCH_UPDATE_INTENT_PART2);

    // Only act when there's a Seamless Update request
    if (update_intent & MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK)
    {
        //write_to_mailbox(MB_PCH_UPDATE_INTENT_PART2, update_intent & MB_UPDATE_INTENT_BMC_SEAMLESS_RESET_MASK);
        clear_mailbox_register_bit(MB_PCH_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_PCH_FV_SEAMLESS_BIT_POS);

        if (num_failed_fw_or_cpld_update_attempts >= MAX_FAILED_UPDATE_ATTEMPTS_ALLOWED)
        {
            // If there are too many failed firmware update attempts, reject this update.
            log_update_failure(MINOR_ERROR_EXCEEDED_MAX_FAILED_ATTEMPTS, 0);
            return;
        }

        /*
         * Proceed with this Seamless update
         */
        t0_seamless_update_routine(SPI_FLASH_PCH);
    }

    if (check_spi_flash_state(SPI_FLASH_PCH, SPI_FLASH_STATE_POST_SEAMLESS_UPDATE_MASK) &&
        ((read_from_mailbox(MB_PCH_UPDATE_INTENT_PART2) & MB_UPDATE_INTENT_BMC_SEAMLESS_RESET_MASK) == 0))
    {
        bmc_seamless_complete_actions(SPI_FLASH_PCH);
    }
}

/**
 * @brief Monitor BMC update intent register (part 2) and perform Seamless update if requested.
 *
 * @see act_on_update_intent
 */
static void mb_update_intent_part2_handler_for_bmc()
{
    // Read and clear the update intent register
    alt_u32 update_intent = read_from_mailbox(MB_BMC_UPDATE_INTENT_PART2);

    // Only act when there's a Seamless Update request
    if (update_intent & MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK)
    {
        //write_to_mailbox(MB_BMC_UPDATE_INTENT_PART2, update_intent & MB_UPDATE_INTENT_BMC_SEAMLESS_RESET_MASK);
        clear_mailbox_register_bit(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_PCH_FV_SEAMLESS_BIT_POS);

        if (num_failed_fw_or_cpld_update_attempts >= MAX_FAILED_UPDATE_ATTEMPTS_ALLOWED)
        {
            // If there are too many failed firmware update attempts, reject this update.
            log_update_failure(MINOR_ERROR_EXCEEDED_MAX_FAILED_ATTEMPTS, 0);
            return;
        }

        /*
         * Proceed with this Seamless update
         */
        t0_seamless_update_routine(SPI_FLASH_BMC);
    }

    if (check_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_POST_SEAMLESS_UPDATE_MASK) &&
        ((read_from_mailbox(MB_BMC_UPDATE_INTENT_PART2) & MB_UPDATE_INTENT_BMC_SEAMLESS_RESET_MASK) == 0))
    {
        bmc_seamless_complete_actions(SPI_FLASH_BMC);
    }
}

/**
 * @brief Monitor PCH update intent register (part 2) and perform unprovisioned
 * Seamless update if requested.
 *
 * @see act_on_update_intent
 */
static void mb_unprovisioned_seamless_handler_for_pch()
{
    // Read and clear the update intent register
    alt_u32 update_intent = read_from_mailbox(MB_PCH_UPDATE_INTENT_PART2);

    // Only act when there's a Seamless Update request
    if (update_intent & MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK)
    {
        //write_to_mailbox(MB_PCH_UPDATE_INTENT_PART2, update_intent & MB_UPDATE_INTENT_BMC_SEAMLESS_RESET_MASK);
        clear_mailbox_register_bit(MB_PCH_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_PCH_FV_SEAMLESS_BIT_POS);
        /*
         * Proceed with this Seamless update
         */
        t0_unprovisioned_seamless_update(SPI_FLASH_PCH);
    }

    if (check_spi_flash_state(SPI_FLASH_PCH, SPI_FLASH_STATE_POST_SEAMLESS_UPDATE_MASK) &&
        ((read_from_mailbox(MB_PCH_UPDATE_INTENT_PART2) & MB_UPDATE_INTENT_BMC_SEAMLESS_RESET_MASK) == 0))
    {
        bmc_seamless_complete_actions(SPI_FLASH_PCH);
    }
}

/**
 * @brief Monitor BMC update intent register (part 2) and perform unprovisioned
 * Seamless update if requested.
 *
 * @see act_on_update_intent
 */
static void mb_unprovisioned_seamless_handler_for_bmc()
{
    // Read and clear the update intent register
    alt_u32 update_intent = read_from_mailbox(MB_BMC_UPDATE_INTENT_PART2);

    // Only act when there's a Seamless Update request
    if (update_intent & MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK)
    {
        //write_to_mailbox(MB_BMC_UPDATE_INTENT_PART2, update_intent & MB_UPDATE_INTENT_BMC_SEAMLESS_RESET_MASK);
        clear_mailbox_register_bit(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_PCH_FV_SEAMLESS_BIT_POS);
        /*
         * Proceed with this Seamless update
         */
        t0_unprovisioned_seamless_update(SPI_FLASH_BMC);
    }

    if (check_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_POST_SEAMLESS_UPDATE_MASK) &&
        ((read_from_mailbox(MB_BMC_UPDATE_INTENT_PART2) & MB_UPDATE_INTENT_BMC_SEAMLESS_RESET_MASK) == 0))
    {
        bmc_seamless_complete_actions(SPI_FLASH_BMC);
    }
}

/**
 * @brief Monitor the PCH and BMC update intent fields in the mailbox register file.
 * This routine handles un-provisioned seamless update.
 *
 * @see mb_unprovisioned_seamless_handler_for(SPI_FLASH_TYPE_ENUM)
 */
static void mb_unprovisioned_seamless_handler()
{
    mb_unprovisioned_seamless_handler_for_bmc();
    mb_unprovisioned_seamless_handler_for_pch();
}

#endif

/**
 * @brief Monitor the PCH and BMC update intent fields in the mailbox register file.
 *
 * @see mb_update_intent_handler_for(SPI_FLASH_TYPE_ENUM)
 */
static void mb_update_intent_handler()
{
    mb_update_intent_handler_for_bmc();
    mb_update_intent_handler_for_pch();

#ifdef PLATFORM_SEAMLESS_FEATURES_ENABLED

    mb_update_intent_part2_handler_for_bmc();
    mb_update_intent_part2_handler_for_pch();

#endif

}

/**
 * @brief Trigger platform reset to perform (FW/CPLD) recovery update, after the respective active update
 * has completed and platform has booted.
 *
 * Firmware recovery update involves two parts:
 *   1. Update active firmware first and make sure the new firmware can boot
 *   2. Update the recovery firmware
 *
 * This function triggers a platform reset after #1 is done. In T-1, Nios performs #2 when the
 * FW_RECOVERY_UPDATE_IN_PROGRESS flag is set.
 * This function should only be called when timed boot has completed.
 *
 * For CPLD recovery update, there's a CPLD update status word in UFM, which records the original
 * CPLD recovery update requests before the reconfigurations and CPLD active image update.
 *
 * @see process_updates_in_tmin1
 */
static void post_update_routine()
{
    if (is_timed_boot_done())
    {
        STATUS_LAST_PANIC_ENUM panic_reason = LAST_PANIC_DEFAULT;

        // Check if Nios is in the middle of Firmware Recovery update.
        if (check_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_HAS_PENDING_RECOVERY_FW_UPDATE_MASK))
        {
            panic_reason = LAST_PANIC_BMC_UPDATE_INTENT;
        }
        else if (check_spi_flash_state(SPI_FLASH_PCH, SPI_FLASH_STATE_HAS_PENDING_RECOVERY_FW_UPDATE_MASK))
        {
            panic_reason = LAST_PANIC_PCH_UPDATE_INTENT;
        }
#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)
        else if (check_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_HAS_PENDING_RECOVERY_AFM_UPDATE_MASK))
        {
            panic_reason = LAST_PANIC_BMC_UPDATE_INTENT;
        }
#endif

        // Check if Nios is in the middle of CPLD Recovery update.
        if (is_cpld_rc_update_in_progress())
        {
            // CFM1 has successfully booted. Nios can proceed to promote staged CPLD update capsule to CPLD recovery region
            set_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_READY_FOR_CPLD_RECOVERY_UPDATE_MASK);
            // Leave panic_reason as BMC update intent because currently only the BMC can trigger a CPLD update
            panic_reason = LAST_PANIC_BMC_UPDATE_INTENT;
        }

        // Nios has set the proper SPI state for both BMC and PCH at this point.
        // If there are two firmware recovery updates, they will be performed in the same T-1 cycle.
        if (panic_reason)
        {
            // Firmware/CPLD Recovery update will be performed in T-1
            log_panic(panic_reason);
            perform_platform_reset();
        }
    }
}

#endif /* EAGLESTREAM_INC_T0_UPDATE_H_ */
