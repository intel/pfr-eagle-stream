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
 * @file authentication.h
 * @brief Support authentication of Block 0/Block 1 signature.
 */

#ifndef EAGLESTREAM_INC_AUTHENTICATION_H_
#define EAGLESTREAM_INC_AUTHENTICATION_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "crypto.h"
#include "key_cancellation.h"
#include "keychain.h"
#include "keychain_utils.h"
#include "pfr_pointers.h"
#include "ufm.h"

/**
 * @brief This function validates Block 0.
 * This function validates the Magic number, PC length, PC Type, and
 * most importantly, hash of the protected content.
 *
 * @param b0 pointer to the block 0
 * @param protected_content the start address of protected content
 *
 * @return alt_u32 1 if this Block 0 is valid; 0, otherwise
 */
static alt_u32 is_block0_valid(KCH_BLOCK0* b0, KCH_BLOCK1* b1, alt_u32 pc_addr)
{
    // Verify magic number
    if (b0->magic != BLOCK0_MAGIC)
    {
        return 0;
    }

    // Verify that size of PC must be multiple of 128 bytes
    if ((b0->pc_length < 128) || (b0->pc_length % 128 != 0))
    {
        return 0;
    }
    // Check if PC type is valid
    // kch_pc_type is PC type without the "is key cancellation" or "is decommission capsule" mask
    alt_u32 kch_pc_type = get_kch_pc_type(b0);
    if (kch_pc_type > MAX_KCH_PC_TYPE_VALUE)
    {
        return 0;
    }

    // Verify length of Protected Content (PC) is not larger than allowed
    if (b0->pc_type & (KCH_PC_TYPE_KEY_CAN_CERT_MASK | KCH_PC_TYPE_DECOMM_CAP_MASK))
    {
        // This is a key cancellation certificate or decommission capsule.
        // Both have the same fixed size of 128 bytes.
        if (b0->pc_length != KCH_CAN_CERT_OR_DECOMM_CAP_PC_SIZE)
        {
            return 0;
        }
    }
    else
    {
        // This is not a key cancellation certificate.
        if (kch_pc_type == KCH_PC_PFR_CPLD_UPDATE_CAPSULE)
        {
            // The PC length of a CPLD update capsule must match the expected size.
            // A valid signed CPLD update capsule for a larger device may be sent to
            // a smaller CPLD device for update. That can potentially corrupt the recovery image.
            if (b0->pc_length != EXPECTED_CPLD_UPDATE_CAPSULE_PC_LENGTH)
            {
                return 0;
            }
        }
        else if ((kch_pc_type == KCH_PC_PFR_PCH_PFM) || (kch_pc_type == KCH_PC_PFR_PCH_UPDATE_CAPSULE))
        {
            // For PFM, there's no max size, but it should be smaller than a capsule size for sure.
            if (b0->pc_length > (MAX_PCH_FW_UPDATE_CAPSULE_SIZE - SIGNATURE_SIZE))
            {
                return 0;
            }
        }
        else if ((kch_pc_type == KCH_PC_PFR_BMC_PFM) || (kch_pc_type == KCH_PC_PFR_BMC_UPDATE_CAPSULE))
        {
            // For PFM, there's no max size, but it should be smaller than a capsule size for sure.
            if (b0->pc_length > (MAX_BMC_FW_UPDATE_CAPSULE_SIZE - SIGNATURE_SIZE))
            {
                return 0;
            }
        }
#ifdef PLATFORM_SEAMLESS_FEATURES_ENABLED
        else if (kch_pc_type == KCH_PC_PFR_PCH_SEAMLESS_CAPSULE)
        {
            if (b0->pc_length > (MAX_PCH_FW_UPDATE_CAPSULE_SIZE - SIGNATURE_SIZE))
            {
                return 0;
            }
        }
#endif
    }

    // Check for the 0s in the reserved field
    // This reduces the degree of freedom for attackers
    for (alt_u32 word_i = 0; word_i < BLOCK0_SECOND_RESERVED_SIZE / 4; word_i++)
    {
        if (b0->reserved2[word_i] != 0)
        {
            return 0;
        }
    }

    // Verify hash 256 or hash 384 of PC
    KCH_BLOCK1_B0_ENTRY* b0_entry = &b1->b0_entry;

    if (b0->pc_type & KCH_PC_TYPE_KEY_CAN_CERT_MASK)
    {
        b0_entry = (KCH_BLOCK1_B0_ENTRY*) &b1->csk_entry;
    }

    alt_u32* pc_ptr = get_spi_flash_ptr_with_offset(pc_addr);

    if(b0_entry->sig_magic == KCH_EXPECTED_SIG_MAGIC_FOR_256)
    {
    	return verify_sha((alt_u32*) b0->pc_hash256, pc_addr, pc_ptr, b0->pc_length, CRYPTO_256_MODE, ENGAGE_DMA_SPI);
    }
    else if (b0_entry->sig_magic == KCH_EXPECTED_SIG_MAGIC_FOR_384)
    {
        return verify_sha((alt_u32*) b0->pc_hash384, pc_addr, pc_ptr, b0->pc_length, CRYPTO_384_MODE, ENGAGE_DMA_SPI);
    }
    return 0;
}

/**
 * @brief This function validates a Block 1 root entry
 *
 * @param root_entry pointer to the Block 1 root entry
 * @return alt_u32 1 if this root entry is valid; 0, otherwise
 */
static alt_u32 is_root_entry_valid(KCH_BLOCK1_ROOT_ENTRY* root_entry)
{
    // Verify magic number
    if (root_entry->magic != BLOCK1_ROOT_ENTRY_MAGIC)
    {
        return 0;
    }

    // Must have the required permissions (-1).
    if (root_entry->permissions != SIGN_ALL)
    {
        return 0;
    }

    // Must have the required cancellation (-1).
    if (root_entry->key_id != KCH_KEY_NON_CANCELLABLE)
    {
        return 0;
    }

    alt_u8 sha_data[PFR_ROOT_KEY_DATA_SIZE_FOR_384];
    alt_u8* pubkey_x = (alt_u8*) root_entry->pubkey_x;
    alt_u8* pubkey_y = (alt_u8*) root_entry->pubkey_y;

    // Collect root public key X and Y, in little endian byte order.
    if (root_entry->curve_magic == KCH_EXPECTED_CURVE_MAGIC_FOR_256)
    {
        for (alt_u32 byte_i = 0; byte_i < SHA256_LENGTH; byte_i++)
        {
            sha_data[byte_i] = pubkey_x[31 - byte_i];
            sha_data[byte_i + 32] = pubkey_y[31 - byte_i];
        }

        // The calculated hash of hashed region must match the Root Key Hash stored in the PFR
        return verify_sha(get_ufm_pfr_data()->root_key_hash_256, 0, (alt_u32*) sha_data, PFR_ROOT_KEY_DATA_SIZE_FOR_256, CRYPTO_256_MODE, DISENGAGE_DMA);
    }
    else if (root_entry->curve_magic == KCH_EXPECTED_CURVE_MAGIC_FOR_384)
    {
        for (alt_u32 byte_i = 0; byte_i < SHA384_LENGTH; byte_i++)
        {
            sha_data[byte_i] = pubkey_x[47 - byte_i];
            sha_data[byte_i + 48] = pubkey_y[47 - byte_i];
        }

        // The calculated hash of hashed region must match the Root Key Hash stored in the PFR
        return verify_sha(get_ufm_pfr_data()->root_key_hash_384, 0, (alt_u32*) sha_data, PFR_ROOT_KEY_DATA_SIZE_FOR_384, CRYPTO_384_MODE, DISENGAGE_DMA);
    }
    return 0;
}

/**
 * @brief This function validates a Block 1 csk entry
 *
 * @param key_perm_mask The required key permission mask
 * @param prev_entry Previous entry (the root entry)
 * @param csk_entry pointer to the Block 1 csk entry
 *
 * @return alt_u32 1 if this csk entry is valid; 0, otherwise
 */
static alt_u32 is_csk_entry_valid(
        KCH_BLOCK1_ROOT_ENTRY* prev_entry, KCH_BLOCK1_CSK_ENTRY* csk_entry, alt_u32 cur_addr, alt_u32 pc_type)
{
    // Verify magic number
    if (csk_entry->magic != BLOCK1_CSK_ENTRY_MAGIC)
    {
        return 0;
    }

    // Verify curve magic number
    if (csk_entry->curve_magic != prev_entry->curve_magic)
    {
        return 0;
    }

    // The key must have the required permissions
    if (!(csk_entry->permissions & get_required_perm(pc_type)))
    {
        return 0;
    }

    // Check the CSK key ID
    if (!is_csk_key_valid(pc_type, csk_entry->key_id))
    {
        return 0;
    }

    // Check for the 0s in the reserved field
    // This reduces the degree of freedom for attackers
    for (alt_u32 word_i = 0; word_i < BLOCK1_CSK_ENTRY_RESERVED_SIZE / 4; word_i++)
    {
        if (csk_entry->reserved[word_i] != 0)
        {
            return 0;
        }
    }
    // A signature over the hashed region using the Root Key in the previous entry must be valid.
    // The hashed region starts at the curve magic field
    // Check for the curve magic and also signature magic for either secp384 or secp256
    if (csk_entry->curve_magic == KCH_EXPECTED_CURVE_MAGIC_FOR_384)
    {
        if (csk_entry->sig_magic != SIG_SECP384R1_MAGIC)
        {
            return 0;
        }
        return verify_ecdsa_and_sha(prev_entry->pubkey_x,
                prev_entry->pubkey_y,
                csk_entry->sig_r,
                csk_entry->sig_s,
				cur_addr,
                &csk_entry->curve_magic,
                BLOCK1_CSK_ENTRY_HASH_REGION_SIZE,
                CRYPTO_384_MODE,
				ENGAGE_DMA_SPI);
    }
    else if (csk_entry->curve_magic == KCH_EXPECTED_CURVE_MAGIC_FOR_256)
    {
        if (csk_entry->sig_magic != SIG_SECP256R1_MAGIC)
        {
            return 0;
        }
        return verify_ecdsa_and_sha(prev_entry->pubkey_x,
                prev_entry->pubkey_y,
                csk_entry->sig_r,
                csk_entry->sig_s,
				cur_addr,
                &csk_entry->curve_magic,
                BLOCK1_CSK_ENTRY_HASH_REGION_SIZE,
                CRYPTO_256_MODE,
				ENGAGE_DMA_SPI);
    }
    return 0;
}

/**
 * @brief This function validates a Block 1 block 0 entry
 *
 * @param prev_entry entry in the Block 1 prior to this entry (the csk entry)
 * @param b0_entry pointer to the Block 1 block 0 entry
 * @param b0 pointer to Block 0
 *
 * @return alt_u32 1 if this block 0 entry is valid; 0, otherwise
 */
static alt_u32 is_b0_entry_valid(
        alt_u32 prev_entry_sig_magic, alt_u32* prev_entry_pubkey_x, alt_u32* prev_entry_pubkey_y, KCH_BLOCK1_B0_ENTRY* b0_entry, alt_u32 b0_addr, KCH_BLOCK0* b0, alt_u32 is_key_can_cert)
{
    // Verify magic number
    if (b0_entry->magic != BLOCK1_B0_ENTRY_MAGIC)
    {
        return 0;
    }

    // If it is a key cancellation cert, the previous entry is root entry which does not have signature magic
    if (!is_key_can_cert)
    {
        // Check for the previous entry signature magic
        if (b0_entry->sig_magic != prev_entry_sig_magic)
        {
            return 0;
        }
    }
    // The signature over the hash of block 0 using the CSK Pubkey must be valid.
    if(b0_entry->sig_magic == KCH_EXPECTED_SIG_MAGIC_FOR_256)
    {
        return verify_ecdsa_and_sha(prev_entry_pubkey_x,
                prev_entry_pubkey_y,
                b0_entry->sig_r,
                b0_entry->sig_s,
                b0_addr,
                (alt_u32*) b0,
                BLOCK0_SIZE,
                CRYPTO_256_MODE,
				ENGAGE_DMA_SPI);
    }
    else if (b0_entry->sig_magic == KCH_EXPECTED_SIG_MAGIC_FOR_384)
    {
        return verify_ecdsa_and_sha(prev_entry_pubkey_x,
                prev_entry_pubkey_y,
                b0_entry->sig_r,
                b0_entry->sig_s,
                b0_addr,
                (alt_u32*) b0,
                BLOCK0_SIZE,
                CRYPTO_384_MODE,
				ENGAGE_DMA_SPI);
    }
    // The signature over the hash of block 0 using the CSK Pubkey must be valid.
    return 0;
}

/**
 * @brief This function validates Block 1
 *
 * @param b0 pointer to block 0
 * @param b1 pointer to block 1
 * @param is_key_cancellation_cert 1 if this signature is part of a signed key cancellation certificate.
 *
 * @return alt_u32 1 if this Block 1 is valid; 0, otherwise
 */
static alt_u32 is_block1_valid(
        KCH_BLOCK0* b0, KCH_BLOCK1* b1, alt_u32 sig_addr, alt_u32 is_key_cancellation_cert)
{
    // Verify magic number
    if (b1->magic != BLOCK1_MAGIC)
    {
        return 0;
    }

    // Validate Block1 Root Entry
    KCH_BLOCK1_ROOT_ENTRY* root_entry = &b1->root_entry;
    if (!is_root_entry_valid(root_entry))
    {
        return 0;
    }

    if (is_key_cancellation_cert)
    {
        // In the signature of the Key Cancellation Certificate, there's no CSK entry and
        // B0 entry follows immediately after the root entry.
        KCH_BLOCK1_B0_ENTRY* b0_entry = (KCH_BLOCK1_B0_ENTRY*) &b1->csk_entry;

        // Validate Block 0 Entry in Block 1
        return is_b0_entry_valid(root_entry->curve_magic, root_entry->pubkey_x, root_entry->pubkey_y, b0_entry, sig_addr, b0, is_key_cancellation_cert);
    }

    // Validate Block1 CSK Entry
    KCH_BLOCK1_CSK_ENTRY* csk_entry = &b1->csk_entry;
    alt_u32 cur_magic_addr = sig_addr + BLOCK0_SIZE + BLOCK1_HEADER_SIZE + BLOCK1_ROOT_ENTRY_SIZE + 4;
    if (!is_csk_entry_valid(root_entry, csk_entry, cur_magic_addr, get_kch_pc_type(b0)))
    {
        return 0;
    }
    // Validate Block 0 Entry in Block 1
    KCH_BLOCK1_B0_ENTRY* b0_entry = &b1->b0_entry;
    return is_b0_entry_valid(csk_entry->sig_magic, csk_entry->pubkey_x, csk_entry->pubkey_y, b0_entry, sig_addr, b0, 0);
}

/**
 * @brief This function validates the content of a key cancellation certificate.
 * The 124 bytes of reserved field must be all 0s. The CSK Key ID must be within 0-127 (inclusive).
 *
 * @param cert pointer to the key cancellation certificate.
 *
 * @return alt_u32 1 if this key cancellation certificate is valid; 0, otherwise
 */
static alt_u32 is_key_can_cert_valid(KCH_CAN_CERT* cert)
{
    // Check for the 0s in the reserved field
    // This reduces the degree of freedom for attackers
    alt_u32* key_can_cert_reserved = cert->reserved;
    for (alt_u32 word_i = 0; word_i < KCH_CAN_CERT_RESERVED_SIZE / 4; word_i++)
    {
        if (key_can_cert_reserved[word_i] != 0)
        {
            return 0;
        }
    }

    // If the key ID is within 0-127 (inclusive), return 1
    return cert->csk_id <= KCH_MAX_KEY_ID;
}

/**
 * @brief This function authenticate a given signed payload in the SPI flash (assuming the mux has been set correctly).
 * Please refer to the specification regarding the format of signed payload.
 * This function authenticate the Block 0 (containing hash of the payload) first,
 * then authenticate the Block 1 (containing signature over Block0).
 * For key cancellation certificate, this function also validate the certificate
 * content for security reasons.
 *
 * @param signature_addr the start address of the signed payload (i.e. beginning of a signature.) in SPI flash.
 *
 * @return alt_u32 1 if this keychain is valid; 0, otherwise
 */
static alt_u32 is_signature_valid(alt_u32 signature_addr)
{
    KCH_SIGNATURE* signature = (KCH_SIGNATURE*) get_spi_flash_ptr_with_offset(signature_addr);

    // Get pointers and address value to Block0, Block1 and protected content
    KCH_BLOCK0* b0 = (KCH_BLOCK0*) &signature->b0;
    KCH_BLOCK1* b1 = (KCH_BLOCK1*) &signature->b1;
    alt_u32* pc = incr_alt_u32_ptr((alt_u32*) signature, SIGNATURE_SIZE);

    // Check if this is a key cancellation certificate
    alt_u32 is_key_cancellation_cert = b0->pc_type & KCH_PC_TYPE_KEY_CAN_CERT_MASK;
    // Additional check for key cancellation certificate
    if (is_key_cancellation_cert)
    {
        // Validate the content of key cancellation certificate
        // Here, it is okay to read PC without first authenticating its signature, because
        // Nios is simply checking values of the 128 bytes in PC.
        // Moving this block to after Block1/Block0 authentication would cause more code space,
        // which is a scarce resource in this design.
        if (!is_key_can_cert_valid((KCH_CAN_CERT*) pc))
        {
            return 0;
        }
    }

    if (is_block1_valid(b0, b1, signature_addr, is_key_cancellation_cert))
    {
    	return is_block0_valid(b0, b1, signature_addr + SIGNATURE_SIZE);
    }
    return 0;
}

#endif /* EAGLESTREAM_INC_AUTHENTICATION_H_ */
