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
 * @file firmware_recovery.h
 * @brief Responsible for firmware recovery in T-1 mode.
 */

#ifndef EAGLESTREAM_INC_FIRMWARE_RECOVERY_H
#define EAGLESTREAM_INC_FIRMWARE_RECOVERY_H

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "authentication.h"
#include "decompression.h"
#include "global_state.h"
#include "mailbox_utils.h"
#include "spi_ctrl_utils.h"
#include "spi_flash_state.h"
#include "ufm_utils.h"
#include "watchdog_timers.h"

// Keep track of the next recovery level
static alt_u8 current_recovery_level_mask_for_pch = SPI_REGION_PROTECT_MASK_RECOVER_ON_FIRST_RECOVERY;
static alt_u8 current_recovery_level_mask_for_bmc = SPI_REGION_PROTECT_MASK_RECOVER_ON_FIRST_RECOVERY;

/**
 * @brief Reset the current firmware recovery level for @p spi_flash_type SPI flash.
 *
 * @param spi_flash_type indicates BMC or PCH flash
 */
static void reset_fw_recovery_level(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    if (spi_flash_type == SPI_FLASH_PCH)
    {
        current_recovery_level_mask_for_pch = SPI_REGION_PROTECT_MASK_RECOVER_ON_FIRST_RECOVERY;
    }
    else // spi_flash_type == SPI_FLASH_BMC
    {
        current_recovery_level_mask_for_bmc = SPI_REGION_PROTECT_MASK_RECOVER_ON_FIRST_RECOVERY;
    }
}

/**
 * @brief Increment the current firmware recovery level for @p spi_flash_type SPI flash.
 *
 * @param spi_flash_type indicates BMC or PCH flash
 */
static void incr_fw_recovery_level(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    // Since current recovery level variables are bit mask,
    // increment the level by shifting 1 bit to the left.
    if (spi_flash_type == SPI_FLASH_PCH)
    {
        // Shift the recovery level bit mask by 1 bit
        current_recovery_level_mask_for_pch <<= 1;
    }
    else // spi_flash_type == SPI_FLASH_BMC
    {
        current_recovery_level_mask_for_bmc <<= 1;
    }
}

/**
 * @brief Return the current firmware recovery level for @p spi_flash_type SPI flash.
 *
 * @param spi_flash_type indicates BMC or PCH flash
 */
static alt_u8 get_fw_recovery_level(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    if (spi_flash_type == SPI_FLASH_PCH)
    {
        return current_recovery_level_mask_for_pch;
    }
    // spi_flash_type == SPI_FLASH_BMC
    return current_recovery_level_mask_for_bmc;
}

/**
 * @brief Clear any Top Swap configuration when performing the last level of WDT recovery for PCH image.
 * Top Swap reset is realized by driving RTCRST and SRTCRST pins low for 1s. The GPO control bit will drive
 * both of those pins low, if it's set.
 *
 * @param spi_flash_type indicates BMC or PCH flash
 * @param recovery_level the current recovery level
 */
static void perform_top_swap_for_pch_flash(SPI_FLASH_TYPE_ENUM spi_flash_type, alt_u32 recovery_level)
{
    if ((spi_flash_type == SPI_FLASH_PCH)
            && (recovery_level == SPI_REGION_PROTECT_MASK_RECOVER_ON_THIRD_RECOVERY))
    {
        set_bit(U_GPO_1_ADDR, GPO_1_TRIGGER_TOP_SWAP_RESET);

        // Wait 1 second
        sleep_20ms(50);

        clear_bit(U_GPO_1_ADDR, GPO_1_TRIGGER_TOP_SWAP_RESET);
    }
}

/**
 * @brief A recovery procedure performed after a watchdog timer timeouts.
 * In watchdog timeout recovery, a dynamic region is recovered, when RPLM bits 2-4 indicate that a
 * recovery is required for the current level of recovery. If any of the static regions has any of
 * RPLM bits 2-4 set, all static regions are recovered in this WDT recovery.
 *
 * A dynamic region is a SPI region that is read allowed and write allowed in PFM.
 * A static region is a SPI region that is read allowed but not write allowed in PFM.
 *
 * The current level of recovery is tracked by a global variable, which will be reset after a power
 * cycle or a successful timed boot.
 *
 * In T0 mode, Nios simply sets a bit in the global variable bmc_flash_state or pch_flash_state that
 * indicates a WDT recovery is needed. Then Nios performs a platform reset for PCH timer timeouts or
 * a BMC-only reset for BMC timer timeouts. In the subsequent T-1 cycle, this function is always run.
 *
 * It's assumed that Nios firmware is currently in T-1 mode and has control over the @p spi_flash_type
 * flash device.
 *
 * @param spi_flash_type indicates BMC or PCH flash
 *
 * @see watchdog_timeout_handler
 * @see perform_top_swap_for_pch_flash
 */
static void perform_wdt_recovery(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    if (check_spi_flash_state(spi_flash_type, SPI_FLASH_STATE_REQUIRE_WDT_RECOVERY_MASK) ||
        check_spi_flash_state(spi_flash_type, SPI_FLASH_STATE_ATTESTATION_REQUIRE_RECOVERY_MASK))
    {
        // Log the platform state
    	if (check_spi_flash_state(spi_flash_type, SPI_FLASH_STATE_REQUIRE_WDT_RECOVERY_MASK))
    	{
            log_platform_state(PLATFORM_STATE_WDT_TIMEOUT_RECOVERY);
    	}
    	else
    	{
            log_platform_state(PLATFORM_STATE_TMIN1_FW_RECOVERY);
    	}

        // Recover the targeted flash
        switch_spi_flash(spi_flash_type);

        alt_u32 recovery_capsule_addr = get_recovery_region_offset(spi_flash_type);
        alt_u32* signed_recovery_capsule = get_spi_flash_ptr_with_offset(recovery_capsule_addr);

        // Only perform recovery when the Recovery capsule is authentic
        if (is_signature_valid(recovery_capsule_addr))
        {
            alt_u32 recovery_level = get_fw_recovery_level(spi_flash_type);
            perform_top_swap_for_pch_flash(spi_flash_type, recovery_level);

            PFM* capsule_pfm = get_capsule_pfm(signed_recovery_capsule);
            alt_u32 pfm_body_size = capsule_pfm->length - PFM_HEADER_SIZE;
            alt_u32* pfm_body = capsule_pfm->pfm_body;
            alt_u32 should_recover_static_region = 0;

            // Go through the PFM body
            // Recover the dynamic region, if RPLM indicates a recovery is needed for this recovery level.
            // Set the flag should_recover_static_region, if a static region definition, that has any RPLM bit 2-4 set, is found.
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
                    // This is a SPI Region Definition
                    PFM_SPI_REGION_DEF* region_def = (PFM_SPI_REGION_DEF*) pfm_body;

                    if (is_spi_region_static(region_def))
                    {
                        if (region_def->protection_mask & SPI_REGION_PROTECT_MASK_RECOVER_BITS)
                        {
                            should_recover_static_region = 1;
                        }
                    }
                    else if (is_spi_region_dynamic(region_def))
                    {
                        if (region_def->protection_mask & recovery_level)
                        {
                            decompress_spi_region_from_capsule(region_def->start_addr, region_def->end_addr, signed_recovery_capsule, 0);
                        }
                    }

                    incr_size = get_size_of_spi_region_def(region_def);
                }
#ifdef PLATFORM_SEAMLESS_FEATURES_ENABLED
                else if (def_type == FVM_ADDR_DEF_TYPE)
                {
                    // This is an FVM address definition
                    PFM_FVM_ADDR_DEF* fvm_addr_def = (PFM_FVM_ADDR_DEF*) pfm_body;

                    FVM* fvm = (FVM*) get_spi_flash_ptr_with_offset(fvm_addr_def->fvm_addr + SIGNATURE_SIZE);
                    alt_u32 fvm_body_size = fvm->length - FVM_HEADER_SIZE;
                    alt_u32* fvm_body = fvm->fvm_body;

                    // Go through the FVM Body to validate all SPI Region Definitions
                    while (fvm_body_size)
                    {
                        alt_u32 fvm_incr_size = 0;

                        // Process data according to the definition type
                        alt_u8 def_type = *((alt_u8*) fvm_body);
                        if (def_type == SPI_REGION_DEF_TYPE)
                        {
                            // This is a SPI Region Definition
                            PFM_SPI_REGION_DEF* region_def = (PFM_SPI_REGION_DEF*) fvm_body;

                            if (is_spi_region_static(region_def))
                            {
                                if (region_def->protection_mask & SPI_REGION_PROTECT_MASK_RECOVER_BITS)
                                {
                                    should_recover_static_region = 1;
                                }
                            }
                            else if (is_spi_region_dynamic(region_def))
                            {
                                if (region_def->protection_mask & recovery_level)
                                {
                                    decompress_spi_region_from_capsule(region_def->start_addr, region_def->end_addr, signed_recovery_capsule, 0);
                                }
                            }

                            fvm_incr_size = get_size_of_spi_region_def(region_def);
                        }
                        else if (def_type == PFM_CAPABILITY_DEF_TYPE)
                        {
                            // CPLD doesn't use this structure; skip it.
                            fvm_incr_size = sizeof(PFM_CAPABILITY_DEF);
                        }
                        else
                        {
                            // Break when there is no more region/rule definition in PFM body
                            break;
                        }

                        // Move the PFM body pointer to the next definition
                        fvm_body = incr_alt_u32_ptr(fvm_body, fvm_incr_size);
                        fvm_body_size -= fvm_incr_size;
                    }

                    incr_size = sizeof(PFM_FVM_ADDR_DEF);
                }
                else if (def_type == PFM_CAPABILITY_DEF_TYPE)
                {
                    // Skip capability definition
                    incr_size = sizeof(PFM_CAPABILITY_DEF);
                }
#endif /* PLATFORM_SEAMLESS_FEATURES_ENABLED */
                else
                {
                    // Break when there is no more region/rule definition in PFM body
                    break;
                }

                // Move the PFM body pointer to the next definition
                pfm_body = incr_alt_u32_ptr(pfm_body, incr_size);
                pfm_body_size -= incr_size;
            }

            if (should_recover_static_region)
            {
                // If a static PFM entry has any of RPLM.BIT[4:2] set,
                // WDT time out triggers T-1 static recovery (entire static portion of flashes).
                decompress_capsule(signed_recovery_capsule, spi_flash_type, DECOMPRESSION_STATIC_REGIONS_MASK);
            }

            // Increment recovery level after performing a firmware recovery
            incr_fw_recovery_level(spi_flash_type);

            // Clear the state
            if (check_spi_flash_state(spi_flash_type, SPI_FLASH_STATE_REQUIRE_WDT_RECOVERY_MASK))
            {
                clear_spi_flash_state(spi_flash_type, SPI_FLASH_STATE_REQUIRE_WDT_RECOVERY_MASK);
            }
            else
            {
                clear_spi_flash_state(spi_flash_type, SPI_FLASH_STATE_ATTESTATION_REQUIRE_RECOVERY_MASK);
            }
        }
    }
}

#endif /* EAGLESTREAM_INC_FIRMWARE_RECOVERY_H */
