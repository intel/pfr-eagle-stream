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
 * @file key_cancellation.h
 * @brief Responsible for key cancellation flow.
 */

#ifndef EAGLESTREAM_INC_KEY_CANCELLATION_H_
#define EAGLESTREAM_INC_KEY_CANCELLATION_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "keychain_utils.h"
#include "pfr_pointers.h"
#include "ufm_utils.h"
#include "utils.h"

/**
 * @brief Given a protected content type and Key ID, cancel that ID in the corresponding
 * key cancellation policy in UFM. 
 * 
 * Nios first finds the correspoinding bit in the appropriate key cancellation policy 
 * in UFM. The key id is cancelled by setting that bit to 0. The first bit corresponds with
 * key ID 0. 
 * For example, if the PC type is CPLD and key ID is 20, Nios finds the key cancellation
 * policy for CPLD and then set the 21st bit to 0. 
 * 
 * Prior to calling this function, Nios should have authenticated the corresponding key 
 * cancellation certificate. It's assumed that the input key ID is a valid ID (i.e. 
 * within 0 - 127).
 * 
 * @param pc_type protected content type
 * @param key_id CSK key ID
 */
static void cancel_key(alt_u32 pc_type, alt_u32 key_id)
{
    // Key ID must be within 0-127 (checked in authentication)
    PFR_ASSERT(key_id <= KCH_MAX_KEY_ID)

    // Get the pointer to key cancellation policy for the given payload type.
    alt_u32* key_cancel_ptr = get_ufm_key_cancel_policy_ptr(pc_type);

    // Move pointer to the word that this key ID is mapped to. (e.g. key ID 5 is in the first word)
    key_cancel_ptr += key_id / 32;
    // Count the bit from the left. (e.g. 1111_1011_1111_... means key ID 5 is cancelled)
    alt_u32 bit_offset = 31 - (key_id % 32);

    // Cancel the key ID
    *key_cancel_ptr &= ~(0b1 << bit_offset);
}

/**
 * @brief Return 1 if @p key_id is unused before cancellation.
 *
 * @param pc_type protected content type
 * @param key_id The ID of this key
 * @return 1 if CSK key is un-used; 0, otherwise.
 */
static alt_u32 is_csk_key_unused(alt_u32 pc_type, alt_u32 key_id)
{
    SPI_FLASH_TYPE_ENUM spi_flash = SPI_FLASH_BMC;

    // Check PC type
    if (pc_type == KCH_PC_PFR_PCH_PFM)
    {
        spi_flash = SPI_FLASH_PCH;
    }
    else if (pc_type == KCH_PC_PFR_BMC_PFM)
    {
        spi_flash = SPI_FLASH_BMC;
    }
    else
    {
        return 1;
    }
    switch_spi_flash(spi_flash);

    alt_u32 active_pfm_addr = get_active_region_offset(spi_flash);
    alt_u32 recovery_pfm_addr = get_recovery_region_offset(spi_flash);

    KCH_SIGNATURE* signature = (KCH_SIGNATURE*) get_spi_flash_ptr_with_offset(active_pfm_addr);
    KCH_BLOCK1* b1 = (KCH_BLOCK1*) &signature->b1;
    KCH_BLOCK1_CSK_ENTRY* csk_entry = &b1->csk_entry;

    // Matching active key ID, hence this csk key is being used and should not be cancelled
    if (csk_entry->key_id == key_id)
    {
        return 0;
    }

    signature = (KCH_SIGNATURE*) get_spi_flash_ptr_with_offset(recovery_pfm_addr + SIGNATURE_SIZE);
    b1 = (KCH_BLOCK1*) &signature->b1;
    csk_entry = &b1->csk_entry;

    // Matching recovery key ID, hence this csk key is being used and should not be cancelled
    if (csk_entry->key_id == key_id)
    {
        return 0;
    }
    return 1;
}

/**
 * @brief Return 1 if @p key_id is a valid ID and the given @p key_id is cancelled in the 
 * key cancellation policy for @p pc_type.
 *
 * @param pc_type protected content type
 * @param key_id The ID of this key
 * @return 1 if signing with this CSK key is valid; 0, otherwise.
 */
static alt_u32 is_csk_key_valid(alt_u32 pc_type, alt_u32 key_id)
{
    if (key_id > KCH_MAX_KEY_ID)
    {
        // This is an invalid key
        return 0;
    }

    // Get the pointer to key cancellation policy for the given payload type.
    alt_u32* key_cancel_ptr = get_ufm_key_cancel_policy_ptr(pc_type);

    // Move pointer to the word that this key ID is mapped to. (e.g. key ID 5 is in the first word)
    key_cancel_ptr += key_id / 32;
    // Count the bit from the left. (e.g. 1111_1011_1111_... means key ID 5 is cancelled)
    alt_u32 bit_offset = 31 - (key_id % 32);

    // Return 0 if this key is cancelled
    return *key_cancel_ptr & (0b1 << bit_offset);
}

#endif /* EAGLESTREAM_INC_KEY_CANCELLATION_H_ */
