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
 * @file pit_utils.h
 * @brief Responsible to support Protect-in-Transit feature.
 */

#ifndef EAGLESTREAM_INC_PIT_UTILS_H_
#define EAGLESTREAM_INC_PIT_UTILS_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "crypto.h"
#include "pfr_pointers.h"
#include "rfnvram_utils.h"
#include "spi_rw_utils.h"
#include "timer_utils.h"
#include "ufm_utils.h"


/**
 * @brief Enforce the PIT L1 protection, if it's enabled. 
 * 
 * If PIT L1 is enabled, Nios fetches the PIT ID from RFNVRAM and compares
 * it against the stored PIT ID in UFM. If they do not match, Nios enters
 * a lockdown mode and platform stays in T-1. If the IDs match, exit
 * the function. 
 */
static void perform_pit_l1_check()
{
    if (check_ufm_status(UFM_STATUS_PIT_L1_ENABLE_BIT_MASK))
    {
        // PIT L1 protection is enabled
        // Check the PIT ID in UFM against the ID in RFNVRAM
        alt_u32 rfnvram_pit_id[RFNVRAM_PIT_ID_LENGTH / 4] = {};
        read_from_rfnvram((alt_u8*) rfnvram_pit_id, RFNVRAM_PIT_ID_OFFSET, RFNVRAM_PIT_ID_LENGTH);

        alt_u32* ufm_pit_id = get_ufm_pfr_data()->pit_id;
        for (alt_u32 word_i = 0; word_i < (RFNVRAM_PIT_ID_LENGTH / 4); word_i++)
        {
            if (ufm_pit_id[word_i] != rfnvram_pit_id[word_i])
            {
                // Stay in T-1 mode if there's a ID mismatch
                log_platform_state(PLATFORM_STATE_PIT_L1_LOCKDOWN);
                never_exit_loop();
            }
        }
    }
}

/**
 * @brief If the PIT L2 is enabled and firmware hashes are stored, enforce the Firmware Sealing
 * protection. If the PIT L2 is enabled but firmware hashes have not been stored, Nios calculates
 * firmware hashes for both PCH and BMC flashes. 
 * 
 * When Firmware Sealing (PIT L2) is enforced, if any of the calculated firmware hash does not match 
 * the expected firmware hash in UFM, Nios enters a lockdown mode and platform stays in T-1 mode. 
 */
static void perform_pit_l2_check()
{
    if (!check_ufm_status(UFM_STATUS_PIT_L2_PASSED_BIT_MASK))
    {
        if (check_ufm_status(UFM_STATUS_PIT_L2_ENABLE_BIT_MASK))
        {
            switch_spi_flash(SPI_FLASH_BMC);
            // Preparing for crypto configuration
            CRYPTO_MODE crypto_mode = CRYPTO_256_MODE;

            // Get the supported type of crypto by reading from spi flash
            KCH_SIGNATURE* signature = (KCH_SIGNATURE*) get_spi_flash_ptr_with_offset(get_active_region_offset(SPI_FLASH_BMC));
            KCH_BLOCK1* b1 = (KCH_BLOCK1*) &signature->b1;

            // Get information from root entry placeholder in block 1
            KCH_BLOCK1_ROOT_ENTRY* root_entry = &b1->root_entry;

            if (root_entry->curve_magic == KCH_EXPECTED_CURVE_MAGIC_FOR_384)
            {
                crypto_mode = CRYPTO_384_MODE;
            }
            // L2 protection is enabled
            UFM_PFR_DATA* ufm_data = get_ufm_pfr_data();

            if (check_ufm_status(UFM_STATUS_PIT_HASH_STORED_BIT_MASK))
            {
                // Firmware hash has been stored.

                // Compute PCH firmware hash
                // If the hash doesn't match the stored hash, hang in T-1
                switch_spi_flash(SPI_FLASH_PCH);
                if (!verify_sha(ufm_data->pit_pch_fw_hash, 0, get_spi_flash_ptr(), PCH_SPI_FLASH_SIZE, crypto_mode, ENGAGE_DMA_SPI))
                {
                    // Remain in T-1 mode if there's a hash mismatch
                    log_platform_state(PLATFORM_STATE_PIT_L2_PCH_HASH_MISMATCH_LOCKDOWN);
                    never_exit_loop();
                }

                // Compute BMC firmware hash
                // If the hash doesn't match the stored hash, hang in T-1
                switch_spi_flash(SPI_FLASH_BMC);
                if (!verify_sha(ufm_data->pit_bmc_fw_hash, 0, get_spi_flash_ptr(), BMC_SPI_FLASH_SIZE, crypto_mode, ENGAGE_DMA_SPI))
                {
                    // Remain in T-1 mode if there's a hash mismatch
                    log_platform_state(PLATFORM_STATE_PIT_L2_BMC_HASH_MISMATCH_LOCKDOWN);
                    never_exit_loop();
                }

                // PIT L2 passed.
                // Save this result in UFM and share with other components through UFM status mailbox register.
                // In the subsequent power up, L2 checks will not be triggered
                set_ufm_status(UFM_STATUS_PIT_L2_PASSED_BIT_MASK);
                mb_set_ufm_provision_status(MB_UFM_PROV_UFM_PIT_L2_PASSED_MASK);
            }
            else
            {
                // Pending PIT hash to be stored
                // Compute and store PCH flash hash
                switch_spi_flash(SPI_FLASH_PCH);
                calculate_and_save_sha(ufm_data->pit_pch_fw_hash, 0, get_spi_flash_ptr(), PCH_SPI_FLASH_SIZE, crypto_mode, ENGAGE_DMA_SPI);

                // Compute and store BMC flash hash
                switch_spi_flash(SPI_FLASH_BMC);
                calculate_and_save_sha(ufm_data->pit_bmc_fw_hash, 0, get_spi_flash_ptr(), BMC_SPI_FLASH_SIZE, crypto_mode, ENGAGE_DMA_SPI);

                // Indicate that the firmware hashes have been stored
                set_ufm_status(UFM_STATUS_PIT_HASH_STORED_BIT_MASK);
                log_platform_state(PLATFORM_STATE_PIT_L2_FW_SEALED);

                // Remain in T-1 mode
                never_exit_loop();
            }
        }
    }
}

/**
 * @brief Apply the Protect-in-Transit Level 1 & 2 protections. 
 */ 
static void perform_pit_protection()
{
    perform_pit_l1_check();
    perform_pit_l2_check();
}

#endif /* EAGLESTREAM_INC_PIT_UTILS_H_ */
