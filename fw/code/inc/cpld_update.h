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
 * @file cpld_update.h
 * @brief Responsible for updating CPLD bitstream and triggering reconfiguration.
 */

#ifndef EAGLESTREAM_INC_CPLD_UPDATE_H_
#define EAGLESTREAM_INC_CPLD_UPDATE_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "authentication.h"
#include "cpld_reconfig.h"
#include "global_state.h"
#include "key_cancellation.h"
#include "pfr_pointers.h"
#include "platform_log.h"
#include "spi_ctrl_utils.h"
#include "ufm_rw_utils.h"
#include "ufm_utils.h"

typedef enum {
    UPDATE_SOURCE_BMC = 0,
    UPDATE_SOURCE_PCH = 1,
} CPLD_UPDATE_SOURCE_ENUM;

/*!
 * Describe the format of the protected content in the CPLD update capsule
 */
typedef struct
{
    alt_u32 svn;
    alt_u32 cpld_bitstream[UFM_CPLD_ACTIVE_IMAGE_LENGTH / 4];
    alt_u32 padding[31];
} CPLD_UPDATE_PC;


/**
 * @brief Trigger the CPLD update by switching to the CPLD ROM image.
 *
 * If this update is CPLD recovery update, Nios writes 0 to the CPLD update status word in UFM.
 * This status differentiates a CPLD active update from a CPLD recovery update. If the CPLD powers up
 * with 0 in CPLD update status, Nios will know that a CPLD active update has been done and on
 * successful boot to T0, Nios needs to promote staged image to the CPLD recovery image.
 *
 * A bread crumb is left in the last word of CFM1, to indicate that a
 * CPLD update is in progress. It's assumed that this last word is not
 * being used. This bread crumb will be cleaned up upon a successful
 * CPLD update.
 *
 * @param is_cpld_recovery_update 1 if this is CPLD recovery update; 0, otherwise.
 */
static void trigger_cpld_update(alt_u32 is_cpld_recovery_update)
{
    log_platform_state(PLATFORM_STATE_CPLD_UPDATE);

    if (is_cpld_recovery_update)
    {
        // Set the CPLD update status word to remember the CPLD recovery update
        set_cpld_rc_update_in_progress_ufm_flag();
    }

    // Leave bread crumb to indicate that the CFM switch was caused by CPLD update
    alt_u32* cfm1_breadcrumb = get_ufm_ptr_with_offset(CFM1_BREAD_CRUMB);
    *cfm1_breadcrumb = 0;

    // Switch to the CPLD ROM image to perform the update
    perform_cfm_switch(CPLD_CFM0);
}

/**
 * @brief Perform the second half of the CPLD update flow from within the CPLD ROM image.
 * This will write into the configuration bitstream into CFM0
 * This should not be called within the main PFR code.
 */
static void perform_update_post_reconfig()
{
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 cpld_update_capsule_addr = get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET;
    CPLD_UPDATE_PC* cpld_capsule_pc = (CPLD_UPDATE_PC*) get_spi_flash_ptr_with_offset(cpld_update_capsule_addr + SIGNATURE_SIZE);

    // Pointers and values in the BMC SPI flash cpld update staging area
    alt_u32 svn = cpld_capsule_pc->svn;
    alt_u32* spi_cpld_bitstream_ptr = cpld_capsule_pc->cpld_bitstream;

    // Return to active image if authentication fails
    if (svn < get_ufm_svn(UFM_SVN_POLICY_CPLD) ||
            !is_signature_valid(cpld_update_capsule_addr))
    {
        perform_cfm_switch(CPLD_CFM1);
        return;
    }

    // CFM1 is stored within 2 sectors
    ufm_erase_sector(CMF1_TOP_HALF_SECTOR_ID);
    ufm_erase_sector(CMF1_BOTTOM_HALF_SECTOR_ID);

    reset_hw_watchdog();

    // Pointer to the top of CFM1
    alt_u32* cfm1_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ACTIVE_IMAGE_OFFSET);
    // Copy the new image into cfm backwards so that we don't have a partially functional image in the case of a power outage
    for (alt_u32 i = (UFM_CPLD_ACTIVE_IMAGE_LENGTH / 4 - 1); i > 0; i--)
    {
        cfm1_ptr[i] = spi_cpld_bitstream_ptr[i];
    }
    reset_hw_watchdog();

    // Copying the first word to CFM0 should be the last thing we do
    // This prevents the situation where a powercycle mid update
    // causes us to have a partially working CPLD image
    cfm1_ptr[0] = spi_cpld_bitstream_ptr[0];

    // The CPLD update is done. Run the new image.
    perform_cfm_switch(CPLD_CFM1);
}

/**
 * @brief Perform the update of the CPLD recovery region
 *
 * This is executed within the active image, after the CPLD has completed a CPLD active update and booted platform successfully.
 * This function updates the CPLD SVN policy and CPLD Recovery capsule inside BMC SPI flash
 */
static void perform_cpld_recovery_update()
{
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* cpld_update_capsule_ptr = get_spi_flash_ptr_with_offset(
            get_ufm_pfr_data()->bmc_staging_region +
            BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET
    );

    // Promote staged CPLD capsule to CPLD recovery region in BMC flash
    // No need to re-authenticate, since we've done that before update to CPLD active in ROM code.
    // Between now and then, the CPLD staging region is write protected.
    erase_spi_region(BMC_CPLD_RECOVERY_IMAGE_OFFSET, MAX_CPLD_UPDATE_CAPSULE_SIZE);
    //alt_u32_memcpy(get_spi_flash_ptr_with_offset(BMC_CPLD_RECOVERY_IMAGE_OFFSET), cpld_update_capsule_ptr, MAX_CPLD_UPDATE_CAPSULE_SIZE);
    alt_u32_memcpy_s(get_spi_flash_ptr_with_offset(BMC_CPLD_RECOVERY_IMAGE_OFFSET), MAX_CPLD_UPDATE_CAPSULE_SIZE, cpld_update_capsule_ptr, MAX_CPLD_UPDATE_CAPSULE_SIZE);
    // Update the CPLD SVN policy in NVRAM, if the input SVN in larger than what is in UFM
    CPLD_UPDATE_PC* cpld_update_protected_content = (CPLD_UPDATE_PC*) incr_alt_u32_ptr(cpld_update_capsule_ptr, SIGNATURE_SIZE);
    write_ufm_svn(UFM_SVN_POLICY_CPLD, cpld_update_protected_content->svn);
    // Update the CPLD SVN in the mailbox
    write_to_mailbox(MB_CPLD_SVN, get_ufm_svn(UFM_SVN_POLICY_CPLD));

    // Erase CPLD update status word and spi flash state
    //ufm_erase_sector(UFM_CPLD_UPDATE_STATUS_SECTOR_ID);
    ufm_erase_page(UFM_CPLD_UPDATE_STATUS_OFFSET);
    clear_spi_flash_state(SPI_FLASH_BMC, SPI_FLASH_STATE_READY_FOR_CPLD_RECOVERY_UPDATE_MASK);
}

#endif /* EAGLESTREAM_INC_CPLD_UPDATE_H_ */
