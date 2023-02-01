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
 * @file firmware_update.h
 * @brief Responsible for firmware updates, as specified in the BMC/PCH update intent, in T-1 mode.
 */

#ifndef EAGLESTREAM_INC_FIRMWARE_UPDATE_H
#define EAGLESTREAM_INC_FIRMWARE_UPDATE_H

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "capsule_validation.h"
#include "decompression.h"
#include "gen_gpo_controls.h"
#include "key_cancellation.h"
#include "keychain_utils.h"
#include "platform_log.h"
#include "spi_ctrl_utils.h"
#include "spi_flash_state.h"
#include "utils.h"
#include "watchdog_timers.h"


/**
 * @brief Perform firmware update for the @p spi_flash_type flash.
 * If @p is_recovery_update is set to 1, do active update first and then set a flag
 * that will later trigger the recovery firmware update flow in T0 mode.
 *
 * Nios firmware needs to verify that the firmware from the update capsule passes authentication
 * and can boot up correctly, before promoting that firmware to the recovery region. That's why
 * active firmware update is required before performing recovery firmware update.
 *
 * Active firmware update is done by decompressing the static SPI regions from the update capsule. If
 * @p is_recovery_update is set to 1, then both static and dynamic SPI regions are being decompressed
 * from the update capsule. Active firmware PFM will also be overwritten by the PFM in the update capsule.
 *
 * If @p is_recovery_update is set to 1, Nios firmware will apply write protection on the
 * staging region. Hence, the staging update capsule won't be changed from active firmware update to
 * recovery firmware update.
 *
 * @param spi_flash_type indicate BMC or PCH SPI flash device
 * @param update_intent the value of the update intent register
 *
 * @see decompress_capsule
 * @see post_update_routine
 * @see perform_firmware_recovery_update
 */
static void perform_active_firmware_update(SPI_FLASH_TYPE_ENUM spi_flash_type, alt_u32 update_intent)
{
    log_firmware_update(spi_flash_type);

    // Make sure we are updating the right flash device
    switch_spi_flash(spi_flash_type);

    // Check if this is a recovery update
    alt_u32 is_recovery_update = update_intent & MB_UPDATE_INTENT_BMC_RECOVERY_MASK;
    if (spi_flash_type == SPI_FLASH_PCH)
    {
        is_recovery_update = update_intent & MB_UPDATE_INTENT_PCH_RECOVERY_MASK;
    }

    // Get pointers to staging capsule
    alt_u32* signed_staging_capsule = get_spi_staging_region_ptr(spi_flash_type);

    // Only overwrite static regions in active update
    DECOMPRESSION_TYPE_MASK_ENUM decomp_event = DECOMPRESSION_STATIC_REGIONS_MASK;

    if (is_recovery_update)
    {
        // The actual update to the recovery region will be executed, after timed boot of the
        // new active firmware is completed. This flag will remind Nios of the update to recovery region.
        set_spi_flash_state(spi_flash_type, SPI_FLASH_STATE_HAS_PENDING_RECOVERY_FW_UPDATE_MASK);

        // Overwrite both static and dynamic regions in recovery update
        decomp_event = DECOMPRESSION_STATIC_AND_DYNAMIC_REGIONS_MASK;
    }
    else if (update_intent & MB_UPDATE_INTENT_UPDATE_DYNAMIC_MASK)
    {
        // Overwrite both static and dynamic regions in recovery update
        decomp_event = DECOMPRESSION_STATIC_AND_DYNAMIC_REGIONS_MASK;
    }

    // Perform decompression
    decompress_capsule(signed_staging_capsule, spi_flash_type, decomp_event);

    // Clear failed update attempts and major/minor error from previous update if any
    clear_update_failure();

    // Re-enable watchdog timers in case they were disabled after 3 WDT timeouts.
    if (spi_flash_type == SPI_FLASH_PCH)
    {
        wdt_enable_status |= WDT_ENABLE_PCH_TIMERS_MASK;
    }
    else // spi_flash_type == SPI_FLASH_BMC
    {
        wdt_enable_status |= WDT_ENABLE_BMC_TIMER_MASK;
    }

}

/**
 * @brief Perform recovery firmware update for @p spi_flash_type flash.
 *
 * At this point, it is assumed that the staging capsule is authentic and matches the active
 * firmware. Nios simply copies the staging capsule and overwrite the recovery capsule. After
 * that, Nios update the SVN policy with the new SVN
 *
 * @param spi_flash_type indicate BMC or PCH SPI flash device
 *
 * @see perform_active_firmware_update
 * @see post_update_routine
 * @see process_pending_recovery_update
 */
static alt_u32 perform_firmware_recovery_update(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    log_firmware_update(spi_flash_type);

    switch_spi_flash(spi_flash_type);

    // Copy staging capsule to overwrite recovery capsule
    alt_u32 staging_capsule_addr = get_staging_region_offset(spi_flash_type);
    alt_u32* staging_capsule = get_spi_flash_ptr_with_offset(staging_capsule_addr);
    if (!memcpy_signed_payload(get_recovery_region_offset(spi_flash_type), staging_capsule))
    {
        return 0;
    }

    // Update the SVN policy now that recovery update has completed
    PFM* capsule_pfm = get_capsule_pfm(staging_capsule);
    alt_u32* svn_policy = UFM_SVN_POLICY_PCH;
    if (spi_flash_type == SPI_FLASH_BMC)
    {
        svn_policy = UFM_SVN_POLICY_BMC;
    }
    write_ufm_svn(svn_policy, capsule_pfm->svn);

#ifdef PLATFORM_SEAMLESS_FEATURES_ENABLED

    alt_u32 pfm_body_size = capsule_pfm->length - PFM_HEADER_SIZE;
    alt_u32* pfm_body = capsule_pfm->pfm_body;

    // Iterate through the PFM body to find FVM address definitions
    while (pfm_body_size)
    {
        alt_u32 incr_size = 0;

        // Process data according to the definition type
        alt_u8 def_type = *((alt_u8*) pfm_body);
        if (def_type == SMBUS_RULE_DEF_TYPE)
        {
            // Skip SMBus Rule Definition
            incr_size = sizeof(PFM_SMBUS_RULE_DEF);
        }
        else if (def_type == SPI_REGION_DEF_TYPE)
        {
            // Skip SPI Region Definition
            PFM_SPI_REGION_DEF* region_def = (PFM_SPI_REGION_DEF*) pfm_body;
            incr_size = get_size_of_spi_region_def(region_def);
        }
        else if (def_type == FVM_ADDR_DEF_TYPE)
        {
            // Found an FVM address definition. Assume Nios is reading PFM from PCH flash.
            PFM_FVM_ADDR_DEF* fvm_addr_def = (PFM_FVM_ADDR_DEF*) pfm_body;
            alt_u32 fvm_addr_in_capsule = get_addr_of_signed_fvm_in_capsule(fvm_addr_def->fvm_addr, staging_capsule_addr);
            FVM* fvm = (FVM*) get_spi_flash_ptr_with_offset(fvm_addr_in_capsule + SIGNATURE_SIZE);

            // Update the SVN policy for this firmware volume type
            alt_u32* svn_policy = UFM_SVN_POLICY_FVM(fvm->fv_type);
            write_ufm_svn(svn_policy, fvm->svn);

            incr_size = sizeof(PFM_FVM_ADDR_DEF);
        }
        else if (def_type == PFM_CAPABILITY_DEF_TYPE)
        {
            // CPLD doesn't use this structure; skip it.
            incr_size = sizeof(PFM_CAPABILITY_DEF);
        }
        else
        {
            // Break when there is no more region/rule definition in PFM body
            break;
        }

        // Move the PFM body pointer to the next definition
        pfm_body = incr_alt_u32_ptr(pfm_body, incr_size);
        pfm_body_size -= incr_size;
    }

#endif /* PLATFORM_SEAMLESS_FEATURES_ENABLED */
    return 1;
}

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)

/**
 * @brief Perform recovery afm update for @p spi_flash_type flash.
 *
 * At this point, it is assumed that the staging capsule is authentic and matches the active
 * afm. Nios simply copies the staging capsule and overwrite the recovery capsule. After
 * that, Nios update the SVN policy with the new SVN
 *
 * @param spi_flash_type indicate BMC or PCH SPI flash device
 *
 * @return 1 if successful; 0 otherwise
 */

static alt_u32 perform_afm_recovery_update(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    log_firmware_update(spi_flash_type);

    switch_spi_flash(spi_flash_type);

    // Copy staging capsule to overwrite recovery capsule
    alt_u32 staging_capsule_addr = get_staging_region_offset(spi_flash_type);
    alt_u32 recovery_addr = get_staging_region_offset(spi_flash_type) + BMC_STAGING_REGION_AFM_RECOVERY_OFFSET;
    alt_u32* staging_capsule = get_spi_flash_ptr_with_offset(staging_capsule_addr);

    if (!memcpy_signed_payload(recovery_addr, staging_capsule))
    {
        return 0;
    }

    // Update to the SVN policy now that recovery update has completed will be done separately
    // For the AFM(s), call write_afm_svn_to_ufm() after as this is the most optimised way
    return 1;
}

/**
 * @brief Perform active afm update for the @p spi_flash_type flash.
 * If @p is_recovery_update is set to 1, do active update first and then set a flag
 * that will later trigger the recovery firmware update flow in T0 mode.
 *
 * Nios firmware needs to verify that the firmware from the update capsule passes authentication
 * and can boot up correctly, before promoting that firmware to the recovery region. That's why
 * active firmware update is required before performing recovery firmware update.
 *
 * Active firmware update is done by simply copying the capsule for AFM. If
 * Active firmware AFM will also be overwritten by the AFM in the update capsule.
 *
 * If @p is_recovery_update is set to 1, Nios firmware will apply write protection on the
 * staging region. Hence, the staging update capsule won't be changed from active firmware update to
 * recovery firmware update.
 *
 * @param update_intent_2 the value of the update intent register
 *
 * @return 1 if success; 0 otherwise
 */
static void perform_active_afm_update(alt_u32 update_intent_2)
{
    log_firmware_update(SPI_FLASH_BMC);

    // Make sure we are updating the right flash device
    switch_spi_flash(SPI_FLASH_BMC);

    // Check if this is a recovery update
    alt_u32 is_recovery_update = update_intent_2 & MB_UPDATE_INTENT_AFM_RECOVERY_MASK;

    if (is_recovery_update)
    {
        // The actual update to the recovery region will be executed, after timed boot of the
        // new active firmware is completed. This flag will remind Nios of the update to recovery region.
        set_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_HAS_PENDING_RECOVERY_AFM_UPDATE_MASK);
    }

    alt_u32 staging_capsule_addr = get_staging_region_offset(SPI_FLASH_BMC);
    AFM* capsule_afm = (AFM*)get_capsule_afm_ptr(SPI_FLASH_BMC);

    // Get the signature of first AFM
    alt_u32* afm_data = get_capsule_first_afm_ptr(staging_capsule_addr);

    alt_u32* afm_header = capsule_afm->afm_addr_def;
    alt_u32 afm_header_size = capsule_afm->length;

    // Dont have to copy afm and afm header over
    while (afm_header_size)
    {
        // Process data according to the definition type
        alt_u8 def_type = *((alt_u8*) afm_header);
        AFM_ADDR_DEF* afm_addr_def = (AFM_ADDR_DEF*) afm_header;
        if (def_type == AFM_SPI_REGION_DEF_TYPE)
        {
            // Get location in the active region
            alt_u32* active_afm_addr_def = get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr);

            // Make sure to erase the AFM region first
            erase_spi_region(afm_addr_def->afm_addr, afm_addr_def->afm_length);

            // Copy the AFM over to active region
            //alt_u32_memcpy_s(active_afm_addr_def, UFM_CPLD_PAGE_SIZE, afm_data, afm_addr_def->afm_length);
            if (!memcpy_s_flash_successful(active_afm_addr_def, UFM_CPLD_PAGE_SIZE, afm_data, afm_addr_def->afm_length))
            {
                return;
            }
        }
        else
        {
            // No more definition, break out from loop
            break;
        }

        // Move the AFM body pointer to the next afm definition
        afm_header = incr_alt_u32_ptr(afm_header, sizeof(AFM_ADDR_DEF));
        afm_data = incr_alt_u32_ptr(afm_data, afm_addr_def->afm_length);
        afm_header_size -= sizeof(AFM_ADDR_DEF);
    }

    // Copy AFM header and signature over
    alt_u32 active_afm_start_addr = get_staging_region_offset(SPI_FLASH_BMC) + BMC_STAGING_REGION_AFM_ACTIVE_OFFSET;
    alt_u32* recovery_afm_sig_addr_ptr = get_spi_flash_ptr_with_offset(staging_capsule_addr + SIGNATURE_SIZE);

    alt_u32 num_of_afm_present = MAX_NO_OF_AFM_SUPPORTED - get_ufm_empty_afm_number();

    if ((capsule_afm->length / sizeof(AFM_ADDR_DEF)) == num_of_afm_present)
    {
        // Erase destination area first
        erase_spi_region(active_afm_start_addr, SIGNATURE_SIZE + ((KCH_BLOCK0*) recovery_afm_sig_addr_ptr)->pc_length);

        // Copy the signature and afm header over
        // Do not need to poll WIP here as no more write after, saving some code space
        alt_u32_memcpy_s(get_spi_active_afm_ptr(SPI_FLASH_BMC), UFM_CPLD_PAGE_SIZE, recovery_afm_sig_addr_ptr, SIGNATURE_SIZE + ((KCH_BLOCK0*) recovery_afm_sig_addr_ptr)->pc_length);
    }
    // Clear failed update attempts and major/minor error from previous update if any
    clear_update_failure();

    // Re-enable watchdog timers in case they were disabled after 3 WDT timeouts.
    wdt_enable_status |= WDT_ENABLE_BMC_TIMER_MASK;
}

#endif


#endif /* EAGLESTREAM_INC_FIRMWARE_UPDATE_H */
