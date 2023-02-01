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
 * @file tmin1_update.h
 * @brief In T-1, perform update action as requested in the BMC/PCH update intent.
 */

#ifndef EAGLESTREAM_INC_TMIN1_UPDATE_H_
#define EAGLESTREAM_INC_TMIN1_UPDATE_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "capsule_validation.h"
#include "mailbox_utils.h"
#include "spi_flash_state.h"

/**
 * @brief Read the BMC update intent and perform firmware or CPLD update accordingly.
 * 
 * Nios firmware clears the update intent register to 0, after reading from it.
 * It is expected that the update intent value has been validated in the T0 (in mb_update_intent_handler()).
 * If the update intent is invalid, Nios firmware wouldn't transition to T-1 mode to perform the update.  
 * 
 * The update intent is a bit mask. Hence, multiple updates can be requested together.
 * In the current release, only BMC flash has reserved area for multiple capsule for BMC/PCH/CPLD updates.
 * This function can process all these requests in the same T-1 cycle.
 *
 * This function looks for a match with any firmware update encoding first. BMC firmware update is processed first.
 * Before getting to this function, Nios should have processed any OOB PCH firmware update request.
 *
 * After processing the firmware updates, this function proceeds to check CPLD update. If there's a CPLD update
 * request, but a PCH/BMC recovery firmware update is in progress, then defer this CPLD update. The CPLD update intent
 * will be written back to BMC update intent register. This is done so that CPLD won't lose track of the
 * recovery firmware update. Update intents and internal global variables will be reset after switching to a 
 * different CPLD image for performing CPLD update. If there's no recovery firmware update in progress, perform the
 * CPLD update.
 * 
 * @see mb_update_intent_handler
 * @see prep_for_oob_pch_fw_update
 */
static void act_on_bmc_update_intent()
{
    switch_spi_flash(SPI_FLASH_BMC);
    // Get pointer to the update capsule
    // In PCH flash, the PCH fw update capsule and CPLD update capsule are located at the same place
    // In BMC flash, there's designated locations for BMC fw update capsule, PCH fw update capsule and CPLD update capsule.
    alt_u32 bmc_signed_capsule_addr = get_staging_region_offset(SPI_FLASH_BMC);

    // Read the update intent and then clear it
    alt_u32 update_intent = read_from_mailbox(MB_BMC_UPDATE_INTENT_PART1);
    write_to_mailbox(MB_BMC_UPDATE_INTENT_PART1, 0);

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)
    // Read the update intent part 2 and then clear it
    alt_u32 update_intent_2 = read_from_mailbox(MB_BMC_UPDATE_INTENT_PART2);
    // Careful not to clear seamless update intent
    clear_mailbox_register_bit(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_BIT_POS);
    clear_mailbox_register_bit(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_RECOVERY_BIT_POS);
#endif

    // Perform update according to the update intent
    // Nios supports multiple updates from BMC at once. BMC/PCH/CPLD images
    // have dedicated locations on the BMC flash.
    if (update_intent & MB_UPDATE_INTENT_BMC_FW_UPDATE_MASK)
    {
        // This is a BMC firmware update, which can only be triggered from BMC (i.e. BMC update intent register + BMC flash)
        if (check_capsule_before_update(
                bmc_signed_capsule_addr,
                update_intent & MB_UPDATE_INTENT_BMC_FW_UPDATE_MASK,
                0))
        {
            perform_active_firmware_update(SPI_FLASH_BMC, update_intent);
        }
    }
#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)
    // Perform AFM update if there is any, only update AFM when there is no normal update
    else if ((update_intent_2 & MB_UPDATE_INTENT_AFM_FW_UPDATE_MASK) && !(update_intent & MB_UPDATE_INTENT_CPLD_MASK))
    {
        // This is a BMC AFM firmware update, which can only be triggered from BMC (i.e. BMC update intent 2 register + BMC flash)
        if (check_capsule_before_update(
                bmc_signed_capsule_addr,
                0,
                update_intent_2 & MB_UPDATE_INTENT_AFM_FW_UPDATE_MASK))
        {
            perform_active_afm_update(update_intent_2);
        }
    }
#endif

    // Since CPLD update causes reconfiguration, CPLD update needs to be checked last.
    if (update_intent & MB_UPDATE_INTENT_CPLD_MASK)
    {
        // This is an out-of-band CPLD update
        if (check_spi_flash_state(SPI_FLASH_PCH, SPI_FLASH_STATE_HAS_PENDING_RECOVERY_FW_UPDATE_MASK)
                || check_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_HAS_PENDING_RECOVERY_FW_UPDATE_MASK)
                || (read_from_mailbox(MB_PCH_UPDATE_INTENT_PART1) & MB_UPDATE_INTENT_PCH_FW_UPDATE_MASK))
        {
            // If there's any firmware update pending, wait till that recovery update is completed.
            // Write the update intent back to the mailbox register
            write_to_mailbox(MB_BMC_UPDATE_INTENT_PART1, update_intent & MB_UPDATE_INTENT_CPLD_MASK);
        }
        else
        {
            // In BMC flash, there's a designated offset for CPLD update capsule.
            // Check the update capsule for CPLD update
            alt_u32 cpld_signed_capsule_addr = bmc_signed_capsule_addr + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET;

            switch_spi_flash(SPI_FLASH_BMC);
            if (check_capsule_before_update(
                    cpld_signed_capsule_addr,
                    update_intent & MB_UPDATE_INTENT_CPLD_MASK,
                    0))
            {
                // The CPLD capsule passed authentication, so proceed with this CPLD update.
                // This function call will end with a CPLD reconfiguration, so the Nios process will terminate here.
                trigger_cpld_update(update_intent & MB_UPDATE_INTENT_CPLD_RECOVERY_MASK);
            }
        }
    }
}

/**
 * @brief Read the PCH update intent and perform PCH firmware update.
 *
 * Nios firmware clears the update intent register to 0, after reading from it.
 * It is expected that the update intent value has been validated in the T0 (in mb_update_intent_handler()).
 * If the update intent is invalid, Nios firmware wouldn't transition to T-1 mode to perform the update.
 *
 * Only PCH firmware update is supported from PCH. CPLD update and BMC firmware update are not valid requests.
 *
 * @see mb_update_intent_handler
 */
static void act_on_pch_update_intent()
{
    // Read the update intent and then clear it
    alt_u32 update_intent = read_from_mailbox(MB_PCH_UPDATE_INTENT_PART1);
    write_to_mailbox(MB_PCH_UPDATE_INTENT_PART1, 0);

    if (update_intent & MB_UPDATE_INTENT_PCH_FW_UPDATE_MASK)
    {
        switch_spi_flash(SPI_FLASH_PCH);

        if (check_capsule_before_update(
                get_staging_region_offset(SPI_FLASH_PCH),
                update_intent & MB_UPDATE_INTENT_PCH_FW_UPDATE_MASK,
                0))
        {
            perform_active_firmware_update(SPI_FLASH_PCH, update_intent);
        }
    }
}

/**
 * @brief Trigger a recovery firmware update if the global variable spi_flash_state indicates that there's a
 * pending recovery update for the @p spi_flash_type flash.
 *
 * Prior to performing the update, Nios authenticates the staging capsule and makes sure it's identical with
 * active firmware. Once that's passing, Nios overwrites the recovery capsule with staging capsule.
 *
 * @param spi_flash_type indicate BMC or PCH SPI flash device
 *
 * @see check_capsule_before_fw_recovery_update
 * @see perform_firmware_recovery_update
 */
static void process_pending_recovery_update(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    // Check for pending firmware recovery update
    if (check_spi_flash_state(spi_flash_type, SPI_FLASH_STATE_HAS_PENDING_RECOVERY_FW_UPDATE_MASK))
    {
        // There's a pending recovery update
        if (check_capsule_before_fw_recovery_update(spi_flash_type, 0, 0))
        {
            perform_firmware_recovery_update(spi_flash_type);
        }
        else
        {
            // Log the error and increment failed update attempts
            log_update_failure(MINOR_ERROR_RECOVERY_FW_UPDATE_AUTH_FAILED, 1);
        }
        // clear the Recovery Update in Progress flag
        clear_spi_flash_state(spi_flash_type, SPI_FLASH_STATE_HAS_PENDING_RECOVERY_FW_UPDATE_MASK);
    }
#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)
    // Check for pending afm recovery update
    if (check_spi_flash_state(spi_flash_type, SPI_FLASH_STATE_HAS_PENDING_RECOVERY_AFM_UPDATE_MASK))
    {
        // There's a pending recovery update for AFM
        if (check_capsule_before_fw_recovery_update(spi_flash_type, 0, 1))
        {
            perform_afm_recovery_update(spi_flash_type);
        }
        else
        {
            // Log the error and increment failed update attempts
            log_update_failure(MINOR_ERROR_RECOVERY_FW_UPDATE_AUTH_FAILED, 1);
        }
        // clear the Recovery Update in Progress flag
        clear_spi_flash_state(spi_flash_type, SPI_FLASH_STATE_HAS_PENDING_RECOVERY_AFM_UPDATE_MASK);
    }
#endif

    // Check for pending CPLD recovery update
    if (check_spi_flash_state(spi_flash_type, SPI_FLASH_STATE_READY_FOR_CPLD_RECOVERY_UPDATE_MASK))
    {
        perform_cpld_recovery_update();
    }
}

/**
 * @brief When there's an out-of-band PCH firmware update request, Nios needs to copy PCH update capsule
 * from BMC SPI flash to PCH SPI flash.
 *
 * In this function, Nios authenticates the PCH update capsule in BMC staging area. If that passes,
 * Nios copies the update capsule to PCH staging area and copies the update intent value to PCH
 * update intent register.
 *
 * @see copy_between_flashes
 * @see process_pending_recovery_update
 * @see act_on_bmc_update_intent
 * @see act_on_pch_update_intent
 */
static void prep_for_oob_pch_fw_update()
{
    // Read the BMC update intent
    // This register is cleared later in act_on_bmc_update_intent. This saves some code space.
    alt_u32 bmc_update_intent = read_from_mailbox(MB_BMC_UPDATE_INTENT_PART1);
    alt_u32 oob_pch_update_intent = bmc_update_intent & (MB_UPDATE_INTENT_PCH_FW_UPDATE_MASK | MB_UPDATE_INTENT_UPDATE_DYNAMIC_MASK);
    if (oob_pch_update_intent & MB_UPDATE_INTENT_PCH_FW_UPDATE_MASK)
    {
        // This is an out-of-band PCH firmware update
        switch_spi_flash(SPI_FLASH_BMC);

        // PCH firmware update capsule has a dedicated location in the BMC staging region
        alt_u32 pch_signed_capsule_addr = get_staging_region_offset(SPI_FLASH_BMC) + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET;
        if (is_capsule_sig_valid(pch_signed_capsule_addr, oob_pch_update_intent, 0))
        {
            // Capsule has passed authentication and sanity checks. Copy it to PCH SPI flash.
            alt_u32* pch_signed_capsule = get_spi_flash_ptr_with_offset(pch_signed_capsule_addr);
            copy_between_flashes(
                    get_staging_region_offset(SPI_FLASH_PCH),
                    pch_signed_capsule_addr,
                    SPI_FLASH_PCH,
                    SPI_FLASH_BMC,
                    get_signed_payload_size(pch_signed_capsule)
            );

            if ((bmc_update_intent & MB_UPDATE_INTENT_PCH_RECOVERY_MASK) &&
                    (check_spi_flash_state(SPI_FLASH_PCH, SPI_FLASH_STATE_RECOVERY_FAILED_AUTH_MASK)))
            {
                // When the recovery image failed authentication, staged capsule can be promoted to recovery region
                //   directly if it matches the active image.
                // Set the intent of promoting staged capsule directly. This will be picked up later.
                set_spi_flash_state(SPI_FLASH_PCH, SPI_FLASH_STATE_HAS_PENDING_RECOVERY_FW_UPDATE_MASK);
            }
            else
            {
                // Pass the update intent to PCH update intent register
                write_to_mailbox(MB_PCH_UPDATE_INTENT_PART1, oob_pch_update_intent | read_from_mailbox(MB_PCH_UPDATE_INTENT_PART1));
            }
        }
    }
}

#endif /* EAGLESTREAM_INC_TMIN1_UPDATE_H_ */
