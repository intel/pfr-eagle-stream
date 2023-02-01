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
 * @file seamless_update.h
 * @brief Support Seamless update feature
 */

#ifndef EAGLESTREAM_INC_SEAMLESS_UPDATE_H_
#define EAGLESTREAM_INC_SEAMLESS_UPDATE_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "decompression.h"
#include "pfr_pointers.h"
#include "spi_ctrl_utils.h"
#include "spi_flash_state.h"

/**
 * @brief Check if the fvn address matches between capsule and active region
 *
 * @return 1 if good, 0 otherwise
 */
static alt_u32 is_seamless_capsule_spi_addr_good()
{
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* signed_capsule = get_spi_staging_region_ptr(SPI_FLASH_PCH);
    alt_u32* signed_capsule_fvm = incr_alt_u32_ptr(signed_capsule, SIGNATURE_SIZE);

    FVM* capsule_fvm = (FVM*) incr_alt_u32_ptr(signed_capsule_fvm, SIGNATURE_SIZE);
    alt_u32 fvm_body_size = capsule_fvm->length - FVM_HEADER_SIZE;
    alt_u32* fvm_body = capsule_fvm->fvm_body;

    while (fvm_body_size)
    {
        alt_u32 incr_size = 0;

        // Process data according to the definition type
        alt_u8 def_type = *((alt_u8*) fvm_body);
        if (def_type == SPI_REGION_DEF_TYPE)
        {
            // This is a SPI Region Definition
            PFM_SPI_REGION_DEF* region_def = (PFM_SPI_REGION_DEF*) fvm_body;

            if (!get_capsule_fvm_spi_region(region_def->start_addr,
                 region_def->end_addr, capsule_fvm->fv_type))
            {
                log_update_failure(MINOR_ERROR_AUTHENTICATION_FAILED, 1);
                return 0;
            }
            incr_size = get_size_of_spi_region_def(region_def);
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
        fvm_body = incr_alt_u32_ptr(fvm_body, incr_size);
        fvm_body_size -= incr_size;
    }
    return 1;
}

/**
 * @brief Perform the Seamless Firmware Volume update with Seamless capsule in the PCH staging area.
 *
 * @param dest_signed_fvm_addr : destination location for fvm update
 * @param update_dynamic_regions : to update dynamic or not
 *
 * @return none
 */
static void seamless_fv_update(alt_u32 dest_signed_fvm_addr, alt_u32 update_dynamic_regions)
{
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* signed_capsule = get_spi_staging_region_ptr(SPI_FLASH_PCH);
    alt_u32* signed_capsule_fvm = incr_alt_u32_ptr(signed_capsule, SIGNATURE_SIZE);

    // Copy the signed FVM from capsule to Active PFR region
    alt_u32 nbytes = get_signed_payload_size(signed_capsule_fvm);
    erase_spi_region(dest_signed_fvm_addr, nbytes + (4096 - nbytes % 4096));
    alt_u32_memcpy_s(get_spi_flash_ptr_with_offset(dest_signed_fvm_addr), nbytes, signed_capsule_fvm, nbytes);

    // Decompress the FV from Seamless capsule
    FVM* capsule_fvm = (FVM*) incr_alt_u32_ptr(signed_capsule_fvm, SIGNATURE_SIZE);
    alt_u32 fvm_body_size = capsule_fvm->length - FVM_HEADER_SIZE;
    alt_u32* fvm_body = capsule_fvm->fvm_body;

    // Go through the FVM Body to validate all SPI Region Definitions
    while (fvm_body_size)
    {
        alt_u32 incr_size = 0;

        // Process data according to the definition type
        alt_u8 def_type = *((alt_u8*) fvm_body);
        if (def_type == SPI_REGION_DEF_TYPE)
        {
            // This is a SPI Region Definition
            PFM_SPI_REGION_DEF* region_def = (PFM_SPI_REGION_DEF*) fvm_body;

            // Always update the static SPI regions
            // Only update the dynamic SPI regions when the UPDATE_DYNAMIC bit is set in update intent
            if (is_spi_region_static(region_def) | update_dynamic_regions)
            {
                decompress_spi_region_from_capsule(region_def->start_addr, region_def->end_addr, signed_capsule, 0);
            }

            incr_size = get_size_of_spi_region_def(region_def);
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
        fvm_body = incr_alt_u32_ptr(fvm_body, incr_size);
        fvm_body_size -= incr_size;
    }
}

/**
 * @brief After the a valid seamless update request comes in, Nios does this routine to validate
 * capsule and perform the Seamless update.
 *
 * @param update_src_spi_flash : source spi flash
 *
 * @return none
 */
static void t0_seamless_update_routine(SPI_FLASH_TYPE_ENUM update_src_spi_flash)
{
    // Log the start of Seamless update
    log_platform_state(PLATFORM_STATE_PCH_FV_SEAMLESS_UPDATE);

    /*
     * Prep
     *
     * Take over access/control on SPI flash device(s)
     * Get address of the seamless capsule
     */
    // Always take over PCH SPI flash, since that's where the Seamless update would occur
    set_bit(U_GPO_1_ADDR, GPO_1_FM_SPI_PFR_PCH_MASTER_SEL);
    alt_u32 src_signed_capsule_addr = get_staging_region_offset(SPI_FLASH_PCH);
    MB_REGFILE_OFFSET_ENUM update_intent_reg_part1 = MB_PCH_UPDATE_INTENT_PART1;

    if (update_src_spi_flash == SPI_FLASH_BMC)
    {
        // If the update capsule/request comes from BMC, take over its SPI flash
        set_bit(U_GPO_1_ADDR, GPO_1_FM_SPI_PFR_BMC_BT_MASTER_SEL);

        // Update request from BMC
        src_signed_capsule_addr = get_staging_region_offset(SPI_FLASH_BMC) + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET;
        update_intent_reg_part1 = MB_BMC_UPDATE_INTENT_PART1;
    }

    // Read part1 of the update intent and see if UPDATE_DYNAMIC bit is set
    alt_u32 update_dynamic_regions = 0;
    if (read_from_mailbox(update_intent_reg_part1) == MB_UPDATE_INTENT_UPDATE_DYNAMIC_MASK)
    {
        update_dynamic_regions = 1;
        write_to_mailbox(update_intent_reg_part1, 0);
    }

    /*
     * Validate the seamless capsule
     */
    switch_spi_flash(update_src_spi_flash);
    alt_u32* signed_capsule = get_spi_flash_ptr_with_offset(src_signed_capsule_addr);

    // Do a quick authentication before the possible copy operation
    if (is_capsule_sig_valid(src_signed_capsule_addr, 0, MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK))
    {
        alt_u32 signed_capsule_addr_in_pch_flash = get_staging_region_offset(SPI_FLASH_PCH);

        // Move the capsule to PCH flash if necessary
        if (update_src_spi_flash == SPI_FLASH_BMC)
        {
            // Copy the update capsule from BMC flash to PCH flash
            copy_between_flashes(
                    signed_capsule_addr_in_pch_flash,
                    src_signed_capsule_addr,
                    SPI_FLASH_PCH,
                    SPI_FLASH_BMC,
                    get_signed_payload_size(signed_capsule)
            );
        }

        // Pre-process the capsule before doing the Seamless Update
        switch_spi_flash(SPI_FLASH_PCH);
        if(check_capsule_before_update(signed_capsule_addr_in_pch_flash, 0, MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK) &&
           is_seamless_capsule_spi_addr_good())
        {
            // Perform the seamless update in PCH flash
            switch_spi_flash(SPI_FLASH_PCH);
            alt_u32* signed_fvm = get_spi_flash_ptr_with_offset(signed_capsule_addr_in_pch_flash + SIGNATURE_SIZE);

            FVM* capsule_fvm = (FVM*) incr_alt_u32_ptr(signed_fvm, SIGNATURE_SIZE);
            alt_u32 dest_signed_fvm_addr = get_active_fvm_addr(capsule_fvm->fv_type);

            /*
             * Perform the Seamless FV update
             */
            seamless_fv_update(dest_signed_fvm_addr, update_dynamic_regions);

            /*
             * Verify the update
             * Authenticate the new FVM and validate the hashes of its SPI regions
             */
            if (!is_signed_fvm_valid(dest_signed_fvm_addr, capsule_fvm->fv_type))
            {
                log_update_failure(MINOR_ERROR_AUTH_FAILED_AFTER_SEAMLESS_UPDATE, 0);
            }
            else
            {
                // Clear failed update attempts and major/minor error from previous update if any
                clear_update_failure();
            }

            /*
             * Invalidate the update capsule by writing 0 to the first word (magic number of block 0) of its signature
             */
            switch_spi_flash(update_src_spi_flash);
            KCH_SIGNATURE* signed_capsule = (KCH_SIGNATURE*) get_spi_flash_ptr_with_offset(src_signed_capsule_addr);
            KCH_BLOCK0* signed_capsule_b0 = &signed_capsule->b0;
            signed_capsule_b0->magic = 0;
        }
    }

    /*
     * Wrap up the routine
     */
    // Release SPI controls
    release_spi_ctrl(update_src_spi_flash);
    release_spi_ctrl(SPI_FLASH_PCH);

    // Log completion of the Seamless Update
    log_platform_state(PLATFORM_STATE_PCH_FV_SEAMLESS_UPDATE_DONE);

    // Start BMC WDT timer for 30 seconds (30s decided based on discussion with BMC team)
    // Important: The assumption is that BMC has boot up (provisioned) before seamless update is done.
    set_spi_flash_state(update_src_spi_flash, SPI_FLASH_STATE_POST_SEAMLESS_UPDATE_MASK);
    start_timer(WDT_BMC_TIMER_ADDR, 0x5DC);
#ifdef USE_SYSTEM_MOCK
    start_timer(WDT_BMC_TIMER_ADDR, 0);
#endif
}


/**
 * @brief Perform an un-provisioned seamless update after receiving a valid update.
 *
 * This update routine perform less checks than a provisioned seamless update.
 * In this routine, capsule signature is not verified.
 *
 * Attributes verified here are:
 * -    FV Type
 * -    FV Length
 * -    FVM SVN
 * -    SPI start and end address
 *
 * @param update_src_spi_flash: SPI Flash type
 *
 * @return none
 */
static void t0_unprovisioned_seamless_update(SPI_FLASH_TYPE_ENUM update_src_spi_flash)
{
    // Log the start of Seamless update
    log_platform_state(PLATFORM_STATE_PCH_FV_SEAMLESS_UPDATE);

    /*
     * Take over access/control on SPI flash device(s)
     * Get address of the seamless capsule
     */
    // Always take over PCH SPI flash, since that's where the Seamless update would occur
    set_bit(U_GPO_1_ADDR, GPO_1_FM_SPI_PFR_PCH_MASTER_SEL);
    alt_u32 src_signed_capsule_addr = get_staging_region_offset(SPI_FLASH_PCH);
    MB_REGFILE_OFFSET_ENUM update_intent_reg_part1 = MB_PCH_UPDATE_INTENT_PART1;

    if (update_src_spi_flash == SPI_FLASH_BMC)
    {
        // If the update capsule/request comes from BMC, take over its SPI flash
        set_bit(U_GPO_1_ADDR, GPO_1_FM_SPI_PFR_BMC_BT_MASTER_SEL);
        // Update request from BMC
        src_signed_capsule_addr = get_staging_region_offset(SPI_FLASH_BMC) + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET;
        update_intent_reg_part1 = MB_BMC_UPDATE_INTENT_PART1;
    }

    // Read part1 of the update intent and see if UPDATE_DYNAMIC bit is set
    alt_u32 update_dynamic_regions = 0;
    if (read_from_mailbox(update_intent_reg_part1) == MB_UPDATE_INTENT_UPDATE_DYNAMIC_MASK)
    {
        update_dynamic_regions = 1;
        write_to_mailbox(update_intent_reg_part1, 0);
    }

    /*
     * Validate the seamless capsule.
     * Skip capsule signature verification.
     * Check critical attributes.
     */
    switch_spi_flash(update_src_spi_flash);
    alt_u32* signed_capsule = get_spi_flash_ptr_with_offset(src_signed_capsule_addr);

    KCH_SIGNATURE* signed_capsule_sig = (KCH_SIGNATURE*) get_spi_flash_ptr_with_offset(src_signed_capsule_addr);

    // Check if update intent is matching
    if (does_pc_type_match_update_intent(&signed_capsule_sig->b0, 0, MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK))
    {
        alt_u32 signed_capsule_addr_in_pch_flash = get_staging_region_offset(SPI_FLASH_PCH);

        // Move the capsule to PCH flash if coming from OOB update
        if (update_src_spi_flash == SPI_FLASH_BMC)
        {
            // Copy the update capsule from BMC flash to PCH flash
            copy_between_flashes(
                    signed_capsule_addr_in_pch_flash,
                    src_signed_capsule_addr,
                    SPI_FLASH_PCH,
                    SPI_FLASH_BMC,
                    get_signed_payload_size(signed_capsule)
            );
        }

        // Pre-process the capsule before doing the Seamless Update
        switch_spi_flash(SPI_FLASH_PCH);

        // Perform the seamless update in PCH flash
        // Always switch before update to be safe
        switch_spi_flash(SPI_FLASH_PCH);
        alt_u32* signed_fvm = get_spi_flash_ptr_with_offset(signed_capsule_addr_in_pch_flash + SIGNATURE_SIZE);

        FVM* capsule_fvm = (FVM*) incr_alt_u32_ptr(signed_fvm, SIGNATURE_SIZE);

        // Capsule FVM has been checked, thus safe to get FVM address from active region in SPI
        alt_u32 dest_signed_fvm_addr = get_active_fvm_addr(capsule_fvm->fv_type);

        /*
         * Perform the Seamless FV update
         */
        seamless_fv_update(dest_signed_fvm_addr, update_dynamic_regions);

        // Clear failed update attempts and major/minor error from previous update if any
        clear_update_failure();
    }
    else
    {
        log_update_failure(MINOR_ERROR_AUTHENTICATION_FAILED, 1);
    }

    // Release SPI controls
    release_spi_ctrl(update_src_spi_flash);
    release_spi_ctrl(SPI_FLASH_PCH);

    // Log completion of the Seamless Update
    log_platform_state(PLATFORM_STATE_PCH_FV_SEAMLESS_UPDATE_DONE);

    // Start BMC WDT timer for 30 seconds (30s decided based on discussion with BMC team)
    set_spi_flash_state(update_src_spi_flash, SPI_FLASH_STATE_POST_SEAMLESS_UPDATE_MASK);
    start_timer(WDT_BMC_TIMER_ADDR, 0x5DC);
#ifdef USE_SYSTEM_MOCK
    start_timer(WDT_BMC_TIMER_ADDR, 0);
#endif
}

#endif /* EAGLESTREAM_INC_SEAMLESS_UPDATE_H_ */
