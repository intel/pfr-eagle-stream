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
 * @file flash_validation.h
 * @brief Perform validations on active/recovery/staging regions of the SPI flash memory in T-1 mode.
 */

#ifndef EAGLESTREAM_INC_FLASH_VALIDATION_H
#define EAGLESTREAM_INC_FLASH_VALIDATION_H

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "capsule_validation.h"
#include "firmware_update.h"
#include "firmware_recovery.h"
#include "keychain.h"
#include "keychain_utils.h"
#include "mailbox_utils.h"
#include "pfm_validation.h"
#include "pfr_pointers.h"
#include "spi_flash_state.h"
#include "ufm_utils.h"
#include "ufm.h"


/**
 * @brief Steps to run before doing flash authentication.
 *
 * Nios logs the platform state to indicate start of flash authentication.
 * Nios clears any previous authentication failure log in Major/Minor Error Mailbox registers.
 * Nios also clears any previous authentication results in spi_flash_state.
 *
 * @see authenticate_and_recover_spi_flash
 * @param spi_flash_type indicate BMC or PCH SPI flash device
 */
static void prep_for_flash_authentication(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    // Log platform state
    if (spi_flash_type == SPI_FLASH_BMC)
    {
        log_platform_state(PLATFORM_STATE_BMC_FLASH_AUTHENTICATION);

        // Clear previous major/minor error if there's any
        if (read_from_mailbox(MB_MAJOR_ERROR_CODE) == MAJOR_ERROR_BMC_AUTH_FAILED)
        {
            log_errors(0, 0);
        }
    }
    else // spi_flash_type == SPI_FLASH_PCH
    {
        log_platform_state(PLATFORM_STATE_PCH_FLASH_AUTHENTICATION);

        // Clear previous major/minor error if there's any
        if (read_from_mailbox(MB_MAJOR_ERROR_CODE) == MAJOR_ERROR_PCH_AUTH_FAILED)
        {
            log_errors(0, 0);
        }
    }

    // Clear previous SPI state on authentication result
    clear_spi_flash_state(spi_flash_type, SPI_FLASH_STATE_CLEAR_AUTH_RESULT);

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)
    // Clear previous AFM state on authentication result
    clear_spi_flash_state(spi_flash_type, SPI_FLASH_STATE_CLEAR_AFM_AUTH_RESULT);
#endif

}

/**
 * @brief Log any authentication failure in the mailbox major and minor error registers.
 *
 * @param spi_flash_type indicate BMC or PCH SPI flash device
 * @param is_active_invalid 1 if signed active PFM failed authentication
 * @param is_recovery_invalid 1 if signed recovery capsule failed authentication
 */
static void log_auth_results(SPI_FLASH_TYPE_ENUM spi_flash_type, alt_u32 is_active_invalid, alt_u32 is_recovery_invalid)
{
    if (is_active_invalid)
    {
        log_auth_failure(spi_flash_type, MINOR_ERROR_AUTH_ACTIVE);

        if (is_recovery_invalid)
        {
            log_auth_failure(spi_flash_type, MINOR_ERROR_AUTH_ACTIVE_AND_RECOVERY);
        }
    }
    else if (is_recovery_invalid)
    {
        log_auth_failure(spi_flash_type, MINOR_ERROR_AUTH_RECOVERY);
    }
}

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)

/**
 * @brief Log any authentication failure in the mailbox major and minor error registers.
 *
 * @param spi_flash_type indicate BMC or PCH SPI flash device
 * @param is_active_afm_invalid 1 if signed active AFM failed authentication
 * @param is_recovery_afm_invalid 1 if signed recovery AFM capsule failed authentication
 */
static void log_afm_auth_results(SPI_FLASH_TYPE_ENUM spi_flash_type, alt_u32 is_active_afm_invalid, alt_u32 is_recovery_afm_invalid)
{
    if (is_active_afm_invalid)
    {
        log_auth_failure(spi_flash_type, MINOR_ERROR_AFM_AUTH_ACTIVE);

        if (is_recovery_afm_invalid)
        {
            log_auth_failure(spi_flash_type, MINOR_ERROR_AFM_AUTH_ACTIVE_AND_RECOVERY);
        }
    }
    else if (is_recovery_afm_invalid)
    {
        log_auth_failure(spi_flash_type, MINOR_ERROR_AFM_AUTH_RECOVERY);
    }
}
#endif
/**
 * @brief Authenticate the active/recovery/staging region for @p spi_flash_type flash and 
 * perform recovery where appropriate. 
 * 
 * Nios authenticates the signed active PFM and signed recovery capsule. If any failure is seen, 
 * Nios proceeds to perform recovery according to the Recovery Matrix in the High-level Architecture Spec. 
 * 
 * Active | Recovery | Staging | Action
 * 0      | 0        | 0       | unrecoverable; do not allow the corresponding device to boot
 * 0      | 0        | 1       | Copy Staging capsule over to Recovery region; then perform an active recovery
 * 0      | 1        | 0       | Perform an active recovery
 * 0      | 1        | 1       | Perform an active recovery
 * 1      | 0        | 0       | Allow the device to boot but restrict active update; only allow recovery udpate
 * 1      | 0        | 1       | Copy the Staging capsule over to Recovery region
 * 1      | 1        | 0       | No action needed
 * 1      | 1        | 1       | No action needed
 *
 * Recovery of active image:
 * Nios performs a static recovery. Static recovery only recover the static regions (i.e. regions that allow read
 * but not write) of the active firmware. This recovery can be triggered by either authentication failure or forced
 * recovery.
 *
 * Recovery of recovery image:
 * If the staging image is valid, Nios copies it over to the recovery area. It's assumed that this scenario may occur
 * only after a power failure.
 *
 * If the active firmware is authentic, Nios applies the SPI and SMBus filtering rules. Only SPI filters are enabled
 * here. SMBus filters are enabled only when BMC has completed boot. 
 * 
 * Nios writes the PFMs' information to mailbox after all the above tasks are done. 
 *
 * @param spi_flash_type indicate BMC or PCH SPI flash device
 */
static void authenticate_and_recover_spi_flash(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
#ifdef USE_SYSTEM_MOCK
    if (SYSTEM_MOCK::get()->should_exec_code_block(
            SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION))
    {
        return;
    }
#endif

    switch_spi_flash(spi_flash_type);
    prep_for_flash_authentication(spi_flash_type);

    /*
     * Authentication of Active region and Recovery region
     */
    alt_u32 is_active_valid = 0;
    alt_u32 recovery_region_addr = get_recovery_region_offset(spi_flash_type);
    alt_u32 is_recovery_valid = 0;
    alt_u32 auth_retry = 3;

    /*
     * Possible state (1 means valid; 0 means invalid)
     * Active | Recovery | Action
     * 0      | 0        | Retry authentication up to 3 times
     * 0      | 1        | Proceed
     * 1      | 1        | Proceed
     * 1      | 0        | Proceed
     */
    do
    {
        // Takeover SPI control of the targeted SPI Flash
        takeover_spi_ctrl(spi_flash_type);
        // Verify the signature and content of the active section PFM
        is_active_valid = is_active_region_valid(spi_flash_type);
        // Verify the signature of the recovery section capsule
        is_recovery_valid = is_signature_valid(recovery_region_addr);
#ifdef PLATFORM_SEAMLESS_FEATURES_ENABLED
        is_recovery_valid &= is_fw_capsule_fv_type_valid(recovery_region_addr);
#endif
    } while ((!(is_active_valid || is_recovery_valid)) && (auth_retry-- != 0));

    // Check for FORCE_RECOVERY GPI signal
    alt_u32 require_force_recovery = !check_bit(U_GPI_1_ADDR, GPI_1_FM_PFR_FORCE_RECOVERY_N);

    // Log the authentication results
    log_auth_results(spi_flash_type, !is_active_valid, !is_recovery_valid);

    // Takeover SPI control of the targeted SPI Flash
    takeover_spi_ctrl(spi_flash_type);

    /*
     * Perform recovery where appropriate
     * Please refer to Recovery Matrix in the High-level Architecture Spec
     *
     * Possible regions state (1 means valid; 0 means invalid)
     * Active | Recovery | Staging
     * 0      | 0        | 0
     * 0      | 0        | 1
     * 0      | 1        | 0
     * 0      | 1        | 1
     * 1      | 0        | 0
     * 1      | 0        | 1
     * 1      | 1        | 0 (no action required)
     * 1      | 1        | 1 (no action required)
     */
    // Deal with the cases where Recovery image is invalid first
    if (!is_recovery_valid)
    {
        if (check_capsule_before_fw_recovery_update(spi_flash_type, !is_active_valid, 0))
        {
            // Log this recovery action
            log_tmin1_recovery_on_recovery_image(spi_flash_type);

            /* Scenarios
             * Active | Recovery | Staging
             * 0      | 0        | 1
             * 1      | 0        | 1
             *
             * Corrective action required: Copy Staging capsule to overwrite Recovery capsule
             */
            perform_firmware_recovery_update(spi_flash_type);
            is_recovery_valid = is_signature_valid(recovery_region_addr);
        }
        // Unable to recovery recovery image
        if (!is_recovery_valid)
        {
            if (is_active_valid)
            {
                /* Scenarios
                 * Active | Recovery | Staging
                 * 1      | 0        | 0
                 *
                 * Cannot recover the recovery image. Save the flash state.
                 */
                set_spi_flash_state(spi_flash_type, SPI_FLASH_STATE_RECOVERY_FAILED_AUTH_MASK);
            }
            else
            {
                /* Scenarios
                 * Active | Recovery | Staging
                 * 0      | 0        | 0
                 *
                 * Hence, Active/Recovery/Staging images are all bad
                 * Cannot recover the Active and Recovery images. Save the flash state.
                 */
                set_spi_flash_state(spi_flash_type, SPI_FLASH_STATE_ALL_REGIONS_FAILED_AUTH_MASK);

                // Critical platform error
                log_auth_failure(spi_flash_type, MINOR_ERROR_AUTH_ALL_REGIONS);
            }
        }
    }

    // Takeover SPI control of the targeted SPI Flash
    takeover_spi_ctrl(spi_flash_type);
    /* Scenarios
     * Active | Recovery | Staging
     * 0      | 1        | 0
     * 0      | 1        | 1
     *
     * Simply recover active image if needed.
     */
    if (is_recovery_valid)
    {
        alt_u32* recovery_region_ptr = get_spi_flash_ptr_with_offset(recovery_region_addr);
        alt_u32 is_active_recovered = 0;

        if (require_force_recovery)
        {
            // Log event
            log_platform_state(PLATFORM_STATE_TMIN1_FORCED_ACTIVE_FW_RECOVERY);
            log_recovery(LAST_RECOVERY_FORCED_ACTIVE_FW_RECOVERY);

            // Recover the entire active firmware upon forced recovery request.
            decompress_capsule(recovery_region_ptr, spi_flash_type, DECOMPRESSION_STATIC_AND_DYNAMIC_REGIONS_MASK);
            is_active_valid = is_active_region_valid(spi_flash_type);;
        }
        else if (!is_active_valid)
        {
            // Log event
            log_tmin1_recovery_on_active_image(spi_flash_type);

            // Recover the entire active firmware when it failed authentication
            decompress_capsule(recovery_region_ptr, spi_flash_type, DECOMPRESSION_STATIC_AND_DYNAMIC_REGIONS_MASK);
            is_active_recovered = is_active_region_valid(spi_flash_type);
        }

        if (!is_active_recovered && !is_active_valid)
        {
            /* Scenarios
             * Cannot recover the Active. Save the flash state.
             */
            set_spi_flash_state(spi_flash_type, SPI_FLASH_STATE_ALL_REGIONS_FAILED_AUTH_MASK);

            // Critical platform error
            log_auth_failure(spi_flash_type, MINOR_ERROR_AUTH_ALL_REGIONS);
        }
    }

    // Print the active & recovery PFM information to mailbox
    // Do this last in case there was some recovery action.
    mb_write_pfm_info(spi_flash_type);

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)
    // After verifying the active section of PFM and the recovery section, if CPLD is set to do attestation by the user,
    // CPLD will verify the active AFM. Although it is called active, it is stored in the staging region of BMC SPI FLASH
    // Check from ufm status bit before calling the function
    // If BMC flash has bad recovery image or no good images at all, no point authenticating AFM and perform attestation services.
    if (spi_flash_type == SPI_FLASH_BMC &&
        check_ufm_status(UFM_STATUS_ENABLE_CPLD_TO_CHALLENGE) &&
        !check_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_RECOVERY_FAILED_AUTH_MASK) &&
        !check_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_ALL_REGIONS_FAILED_AUTH_MASK))
    {
        // Authenticate active and recovery AFM
        alt_u32 is_active_afm_region_valid = 0;
        alt_u32 is_recovery_afm_region_valid = 0;
        alt_u32 afm_auth_retry = 3;

        /*
         * Possible state (1 means valid; 0 means invalid)
         * Active | Recovery | Action
         * 0      | 0        | Retry authentication up to 3 times
         * 0      | 1        | Proceed
         * 1      | 1        | Proceed
         * 1      | 0        | Proceed
         */
        do
        {
            // Takeover SPI control of the targeted SPI Flash
            takeover_spi_ctrl(spi_flash_type);
            // Verify the signature and content of the active section AFM
            is_active_afm_region_valid = is_staging_active_afm_valid(spi_flash_type);
            // Verify the signature of the recovery section capsule
            is_recovery_afm_region_valid = is_staging_recovery_afm_valid(spi_flash_type);

        } while ((!(is_active_afm_region_valid || is_recovery_afm_region_valid)) && (afm_auth_retry-- != 0));

        // Log results
        log_afm_auth_results(spi_flash_type, !is_active_afm_region_valid, !is_recovery_afm_region_valid);

        // Takeover SPI control of the targeted SPI Flash
        takeover_spi_ctrl(spi_flash_type);

        // If recovery AFM is invalid, checks if AFM is present in staging region
        if (!is_recovery_afm_region_valid)
        {
            // check capsule
            if (check_capsule_before_fw_recovery_update(spi_flash_type, !is_active_afm_region_valid, 1))
            {
                // Log this recovery action, share same log with normal recovery
                log_tmin1_recovery_on_recovery_image(spi_flash_type);

                /* Scenarios for AFM
                 * Active | Recovery | Staging
                 * 0      | 0        | 1
                 * 1      | 0        | 1
                 */
                perform_afm_recovery_update(spi_flash_type);
                // The recovery AFM is recovered with good capsule
                is_recovery_afm_region_valid = is_staging_recovery_afm_valid(spi_flash_type);
            }

            if (!is_recovery_afm_region_valid)
            {
                if (is_active_afm_region_valid)
                {
                    /* Scenarios for AFM
                     * Active | Recovery | Staging
                     * 1      | 0        | 0
                     */
                    // Allow for attestation as active AFM is good
                    set_spi_flash_state(spi_flash_type, SPI_FLASH_STATE_READY_FOR_ATTESTATION_MASK);
                    // No good recovery AFM image
                    set_spi_flash_state(spi_flash_type, SPI_FLASH_STATE_AFM_RECOVERY_FAILED_AUTH_MASK);
                }
                else
                {
                    /* Scenarios for AFM
                     * Active | Recovery | Staging
                     * 0      | 0        | 0
                     */
                    // Critical platform error
                    log_auth_failure(spi_flash_type, MINOR_ERROR_AFM_AUTH_ALL_REGIONS);
                    // No good recovery AFM image
                    set_spi_flash_state(spi_flash_type, SPI_FLASH_STATE_AFM_RECOVERY_FAILED_AUTH_MASK);
                }
            }
        }

        // Takeover SPI control of the targeted SPI Flash
        takeover_spi_ctrl(spi_flash_type);

        // Recovery AFM is good
        if (is_recovery_afm_region_valid)
        {
            alt_u32* afm_recovery_region_ptr = get_spi_recovery_afm_ptr(spi_flash_type);
            alt_u32 is_active_afm_recovered = 0;

            // Recovery active AFM if it is bad
            if (!is_active_afm_region_valid)
            {
                // Log event
                log_tmin1_recovery_on_active_image(spi_flash_type);

                // Recover the entire active firmware when it failed authentication
                perform_afm_active_recovery(afm_recovery_region_ptr);
                // Authenticate the new active image and save the AFM body
                is_active_afm_recovered = is_staging_active_afm_valid(spi_flash_type);
            }

            // After recovery active AFM, Check the recovered active AFM and save new AFM(s) to UFM
            // Only do so if active AFM are recovered in this stage
            if (is_active_afm_region_valid || is_active_afm_recovered)
            {
                // At this stage, the active AFM has either been recovered or both the active and recovery AFM are trusted
                set_spi_flash_state(spi_flash_type, SPI_FLASH_STATE_READY_FOR_ATTESTATION_MASK);

                // Get the SVN of the recovery AFM
                alt_u32 recovery_addr = get_staging_region_offset(spi_flash_type) + BMC_STAGING_REGION_AFM_RECOVERY_OFFSET;
                AFM* recovery_afm = (AFM*) get_spi_flash_ptr_with_offset(recovery_addr + (SIGNATURE_SIZE * 2));

                // Write the SVN last in case there is any recovery action
                write_ufm_svn(UFM_SVN_POLICY_AFM, recovery_afm->svn);
                // Save other AFM(s) recovery SVN into UFM
                write_afm_svn_to_ufm(is_active_afm_recovered | is_active_afm_region_valid);
            }
        }

        // Write AFM info to the mailbox
        mb_write_bmc_afm_info();
    }
#endif
}


#endif /* EAGLESTREAM_INC_FLASH_VALIDATION_H */
