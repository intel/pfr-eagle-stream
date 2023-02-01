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
 * @file cpld_recovery.h
 * @brief Responsible for updating CPLD bitstream in the case of a forced recovery.
 */

#ifndef EAGLESTREAM_INC_CPLD_RECOVERY_H_
#define EAGLESTREAM_INC_CPLD_RECOVERY_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "global_state.h"
#include "transition.h"
#include "spi_ctrl_utils.h"
#include "ufm_utils.h"
#include "authentication.h"
#include "tmin1_routines.h"

/**
 * @Boot the BMC and wait for a recovery update.
 *
 * This is used in the case that the active image is corrupted and there are no valid images
 * in either the staging or recovery regions of the bmc spi flash
 * This function will boot the BMC in a secure manner, apply all SPI/SMBus write protections 
 * and then wait until we recieve a CPLD recovery update
 */
static void boot_bmc_and_wait_for_recovery_update()
{
    switch_spi_flash(SPI_FLASH_BMC);

    alt_u32 cpld_staged_capsule_addr = get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET;

    // All the steps needed to boot the BMC
    authenticate_and_recover_spi_flash(SPI_FLASH_BMC);

    // Manually apply the spi write protection rules for the CPLD recovery region because we cannot trust the PFM for this
    write_protect_cpld_recovery_region();
    wdt_enable_status &= ~WDT_ENABLE_BMC_TIMER_MASK;

    // Boot the BMC
    tmin1_boot_bmc();
    log_platform_state(PLATFORM_STATE_CPLD_RECOVERY_IN_RECOVERY_MODE);

    while (1)
    {
        // Poll until we receive a BMC recovery update, all other updates are ignored
        alt_u32 update_intent = 0;
        while(!(update_intent & MB_UPDATE_INTENT_CPLD_RECOVERY_MASK))
        {
            reset_hw_watchdog();
            update_intent = read_from_mailbox(MB_BMC_UPDATE_INTENT_PART1);
            write_to_mailbox(MB_BMC_UPDATE_INTENT_PART1, 0);

#ifdef USE_SYSTEM_MOCK
            if ((update_intent == 0) && SYSTEM_MOCK::get()->should_exec_code_block(
                    SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_FROM_ROM_IMAGE_TERMINAL_STATE))
            {
                PFR_ASSERT(0)
            }
#endif
        }

        perform_entry_to_tmin1();
        switch_spi_flash(SPI_FLASH_BMC);

        // Validate the incoming CPLD capsule
        if (check_capsule_before_update(cpld_staged_capsule_addr, MB_UPDATE_INTENT_CPLD_RECOVERY_MASK, 0))
        {
            // The CPLD capsule passed authentication, so proceed with this CPLD recovery update.
            // This function call will end with a CPLD reconfiguration, so the Nios process will terminate here.
            trigger_cpld_update(1);
        }
        else
        {
            // Boot BMC again and return to polling loop
            authenticate_and_recover_spi_flash(SPI_FLASH_BMC);
            write_protect_cpld_recovery_region();

            tmin1_boot_bmc();
        }
    }
}

/**
 * @brief performs the CPLD recovery flow
 *
 * Normally the CPLD will authenticate its recovery capsule and apply it. 
 * If that is not possible we will copy the staging region into the recovery and use that.
 * in the case that that is not possible either we will boot the BMC and allow the BMC to apply a cpld recovery update
 */
static void perform_cpld_recovery()
{
    switch_spi_flash(SPI_FLASH_BMC);
    reset_hw_watchdog();

    // Clear status word
    //ufm_erase_sector(UFM_CPLD_UPDATE_STATUS_SECTOR_ID);
    ufm_erase_page(UFM_CPLD_UPDATE_STATUS_OFFSET);
    if(!is_signature_valid(BMC_CPLD_RECOVERY_IMAGE_OFFSET))
    {
        alt_u32 cpld_staged_capsule_addr = get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET;
        if(is_signature_valid(cpld_staged_capsule_addr))
        {
            // Copy staging region into recovery region
            erase_spi_region(BMC_CPLD_RECOVERY_IMAGE_OFFSET, MAX_CPLD_UPDATE_CAPSULE_SIZE);
            alt_u32* cpld_staged_capsule = get_spi_flash_ptr_with_offset(cpld_staged_capsule_addr);
            if (!memcpy_signed_payload(BMC_CPLD_RECOVERY_IMAGE_OFFSET, cpld_staged_capsule))
            {
                // All images broken wait for BMC update
                boot_bmc_and_wait_for_recovery_update();
            }
        }
        else
        {
            // All images broken wait for BMC update
            boot_bmc_and_wait_for_recovery_update();
        }
    }

    alt_u32* cpld_recovery_capsule_ptr = get_spi_flash_ptr_with_offset(BMC_CPLD_RECOVERY_IMAGE_OFFSET);
    CPLD_UPDATE_PC* cpld_update_protected_content = (CPLD_UPDATE_PC*) incr_alt_u32_ptr(cpld_recovery_capsule_ptr, SIGNATURE_SIZE);
    alt_u32* spi_cpld_bitstream_ptr = cpld_update_protected_content->cpld_bitstream;

    // Copy the new image into cfm backwards so that we don't have a partially functional image in the case of a power outage
    ufm_erase_sector(CMF1_TOP_HALF_SECTOR_ID);
    ufm_erase_sector(CMF1_BOTTOM_HALF_SECTOR_ID);
    reset_hw_watchdog();

    // Pointer to the top of CFM1
    alt_u32* cfm1_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ACTIVE_IMAGE_OFFSET);
    for (alt_u32 i = (UFM_CPLD_ACTIVE_IMAGE_LENGTH / 4 - 1); i > 0; i--)
    {
        cfm1_ptr[i] = spi_cpld_bitstream_ptr[i];
    }
    cfm1_ptr[0] = spi_cpld_bitstream_ptr[0];
    reset_hw_watchdog();

    // The CPLD recovery is done. Run the recovered image.
    perform_cfm_switch(CPLD_CFM1);
}


#endif /* EAGLESTREAM_INC_CPLD_RECOVERY_H_ */
