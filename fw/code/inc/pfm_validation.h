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
 * @file pfm_validation.h
 * @brief Perform validation on PFM, including the PFM SPI region definition and SMBus rule definition.
 */

#ifndef EAGLESTREAM_INC_PFM_VALIDATION_H_
#define EAGLESTREAM_INC_PFM_VALIDATION_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "authentication.h"
#include "crypto.h"
#include "pfm.h"
#include "pfm_utils.h"
#include "pfr_pointers.h"
#include "smbus_relay_utils.h"
#include "ufm_rw_utils.h"

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)
#include "afm.h"
#endif


/**
 * @brief Check if the SMBus device address in a rule definition is valid.
 * The address is valid when it matches the I2C address for the given bus ID
 * and rule ID based on gen_smbus_relay_config.h
 *
 * @param rule_def : smbus rule definition location
 *
 * @return 1 if success else 0
 */
static PFR_ALT_INLINE alt_u32 PFR_ALT_ALWAYS_INLINE is_device_addr_valid(PFM_SMBUS_RULE_DEF* rule_def)
{
    return rule_def->device_addr == smbus_device_addr[rule_def->bus_id - 1][rule_def->rule_id - 1];
}

/**
 * @brief Perform validation on a PFM SMBus rule definition
 * @param rule_def : smbus rule definition location
 *
 * @return 1 if success else 0
 */
static alt_u32 is_smbus_rule_valid(PFM_SMBUS_RULE_DEF* rule_def)
{
    // Check rule ID and Bus ID
    if ((rule_def->rule_id > MAX_I2C_ADDRESSES_PER_RELAY) || (rule_def->bus_id > NUM_RELAYS))
    {
        return 0;
    }

    // Check device address
    return is_device_addr_valid(rule_def);
}

/**
 * @brief Perform validation on a PFM defined SPI region
 * @param region_def : spi rule definition location
 *
 * @return 1 if success else 0
 */
static alt_u32 is_spi_region_valid(PFM_SPI_REGION_DEF* region_def)
{
    // Only check hash for static region
    if (is_spi_region_static(region_def))
    {
        if (region_def->hash_algorithm & PFM_HASH_ALGO_SHA256_MASK)
        {
            return verify_sha(&region_def->region_hash_start,
                              region_def->start_addr,
                              get_spi_flash_ptr_with_offset(region_def->start_addr),
                              region_def->end_addr - region_def->start_addr,
                              CRYPTO_256_MODE, ENGAGE_DMA_SPI);
        }
        if (region_def->hash_algorithm & PFM_HASH_ALGO_SHA384_MASK)
        {
            return verify_sha(&region_def->region_hash_start,
                              region_def->start_addr,
                              get_spi_flash_ptr_with_offset(region_def->start_addr),
                              region_def->end_addr - region_def->start_addr,
                              CRYPTO_384_MODE, ENGAGE_DMA_SPI);
        }

        return 1;
    }
    else
    {
        return 1;
    }
}

/**
 * @brief Validate a SPI Region definition and if it passes all the checks, apply
 * its SPI write protection rules in SPI filter.
 *
 * @param region_def : spi rule definition location
 *
 * @see is_spi_region_valid
 * @see apply_spi_write_protection
 *
 * @return 1 if the region_def passes all the checks; 0, otherwise.
 */
static alt_u32 validate_spi_region_and_apply_write_protection(PFM_SPI_REGION_DEF* region_def)
{
    alt_u32 is_valid = is_spi_region_valid(region_def);
    if (is_valid)
    {
        // If region_def passes all the checks, apply its write protection rules
        apply_spi_write_protection(
                region_def->start_addr,
                region_def->end_addr,
                region_def->protection_mask & SPI_REGION_PROTECT_MASK_WRITE_ALLOWED
        );
    }

    return is_valid;
}

/**
 * @brief Validate a SMBus Rule definition and if it passes all the checks, apply
 * its rules in the SMBus filter.
 *
 * All SMBus rules in PCH PFM is ignored. This function returns 1, if Nios is currently
 * reading from PCH SPI flash.
 *
 * @param rule_def : smbus rule definition location
 *
 * @see is_smbus_rule_valid
 * @see apply_smbus_rule
 *
 * @return 1 if the rule_def passes all the checks; 0, otherwise.
 */
static alt_u32 validate_and_apply_smbus_rules(PFM_SMBUS_RULE_DEF* rule_def)
{
    if (!check_bit(U_GPO_1_ADDR, GPO_1_SPI_MASTER_BMC_PCHN))
    {
        // Nios is currently reading rules from PCH flash
        // Ignore all the SMBus Rules definitions in PCH PFM
        return 1;
    }

    alt_u32 is_valid = is_smbus_rule_valid(rule_def);
    if (is_valid)
    {
        // If rule_def passes all the checks, apply its SMBus rules
        apply_smbus_rule(rule_def);
    }

    return is_valid;
}

#ifdef PLATFORM_SEAMLESS_FEATURES_ENABLED

/**
 * @brief Validate a signed FVM given its address definition in the Active PFR region.
 *
 * Nios iterates through the FVM body. Nios validates all SPI Region Definition instances and
 * apply the SPI filtering rule once they passed all the checks.
 *
 * Nios is expecting to see only two types of definitions inside FVM body: SPI Region Definition
 * and Capability Definition.
 *
 *
 * @see validate_spi_region_and_apply_write_protection
 *
 * @param signed_fvm_addr the address of this signed FVM inside a SPI flash
 * @param expected_fv_type the expected FV type of this FVM
 *
 * @return 1 if success else 0
 */
static alt_u32 is_signed_fvm_valid(alt_u32 signed_fvm_addr, alt_u16 expected_fv_type)
{
    FVM* fvm = (FVM*) get_spi_flash_ptr_with_offset(signed_fvm_addr + SIGNATURE_SIZE);

    // Check the signature of this FVM first
    if (!is_signature_valid(signed_fvm_addr))
    {
        return 0;
    }

    // Check magic number of FVM
    if (fvm->tag != FVM_MAGIC)
    {
        return 0;
    }

    // FV type in thie FVM must match the that of FVM address definition
    if (fvm->fv_type != expected_fv_type)
    {
        return 0;
    }

    // Check supported FV type
    if (fvm->fv_type > UFM_MAX_SUPPORTED_FV_TYPE)
    {
        return 0;
    }

    alt_u32 fvm_body_size = fvm->length - FVM_HEADER_SIZE;
    alt_u32* fvm_body = fvm->fvm_body;

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

            if (!validate_spi_region_and_apply_write_protection(region_def))
            {
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
 * @brief Validate an FVM Address Definition instance inside the PFM of Active firmware.
 *
 * @param fvm_addr_def an instance of FVM address definition
 * @return 1 if success else 0
 */
static alt_u32 is_fvm_addr_def_valid(PFM_FVM_ADDR_DEF* fvm_addr_def)
{
    // The boundary of an FVM must be 4KB aligned
    if (fvm_addr_def->fvm_addr % 0x1000 == 0)
    {
        // Validate the FVM this fvm_addr_def points to
        return is_signed_fvm_valid(fvm_addr_def->fvm_addr, fvm_addr_def->fv_type);
    }
    return 0;
}

#endif /* PLATFORM_SEAMLESS_FEATURES_ENABLED */

/**
 * @brief Validate the data in the active firmware PFM body and apply SPI/SMBus rules where appropriate.
 *
 * Nios iterates through the PFM body. Nios validates all definitions such as SPI Region Definition
 * and SMBus Rule Definition. Nios also applies the SPI/SMBus filtering rules from these definitions,
 * after they passed the checks.
 * @param pfm : PFM location
 *
 * @see validate_spi_region_and_apply_write_protection
 * @see validate_and_apply_smbus_rules
 *
 * @return 1 if success else 0
 */
static alt_u32 is_active_pfm_body_valid(PFM* pfm)
{
    alt_u32 pfm_body_size = pfm->length - PFM_HEADER_SIZE;
    alt_u32* pfm_body = pfm->pfm_body;

    // Go through the PFM Body
    while (pfm_body_size)
    {
        alt_u32 incr_size = 0;

        // Process data according to the definition type
        alt_u8 def_type = *((alt_u8*) pfm_body);
        if (def_type == SMBUS_RULE_DEF_TYPE)
        {
            // This is an SMBus Rule Definition
            PFM_SMBUS_RULE_DEF* rule_def = (PFM_SMBUS_RULE_DEF*) pfm_body;
            if (!validate_and_apply_smbus_rules(rule_def))
            {
                return 0;
            }
            incr_size = sizeof(PFM_SMBUS_RULE_DEF);
        }
        else if (def_type == SPI_REGION_DEF_TYPE)
        {
            // This is a SPI Region Definition
            PFM_SPI_REGION_DEF* region_def = (PFM_SPI_REGION_DEF*) pfm_body;
            if (!validate_spi_region_and_apply_write_protection(region_def))
            {
                return 0;
            }
            incr_size = get_size_of_spi_region_def(region_def);
        }
#ifdef PLATFORM_SEAMLESS_FEATURES_ENABLED
        else if (def_type == FVM_ADDR_DEF_TYPE)
        {
            PFM_FVM_ADDR_DEF* fvm_addr_def = (PFM_FVM_ADDR_DEF*) pfm_body;
            if (!is_fvm_addr_def_valid(fvm_addr_def))
            {
                return 0;
            }
            incr_size = sizeof(PFM_FVM_ADDR_DEF);
        }
        else if (def_type == PFM_CAPABILITY_DEF_TYPE)
        {
            // CPLD doesn't use this structure; skip it.
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

    return 1;
}

/**
 * @brief Validate the PFM of active firmware.
 *
 * @param pfm a pointer to the start of a PFM data
 * @return 1 if success else 0
 */
static alt_u32 is_active_pfm_valid(PFM* pfm)
{
    if (pfm->tag == PFM_MAGIC)
    {
        // Validate all definitions inside the PFM body
        return is_active_pfm_body_valid(pfm);
    }

    return 0;
}



#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)

/**
 * @brief Saves the AFM(s) SVN into the UFM after validation.
 *
 * Nios iterates through the AFM body in the SPI Flash recovery region and searches for all the AFM bodies.
 * Nios then saves SVN of each AFM into the UFM after they passed the checks. Only copy the SVN if
 * the active AFM has been authenticated and is valid.
 *
 * @see authenticate_and_recover_spi_flash
 *
 * @param is_active_valid: If active AFM is valid
 *
 * @return none.
 *
 */
static void write_afm_svn_to_ufm(alt_u32 is_active_valid)
{
    // Get the address of the recovery AFM(s)
    alt_u32 recovery_afm_sig_addr = get_staging_region_offset(SPI_FLASH_BMC) + BMC_STAGING_REGION_AFM_RECOVERY_OFFSET + SIGNATURE_SIZE ;
    alt_u32* recovery_afm_sig_addr_ptr = get_spi_flash_ptr_with_offset(recovery_afm_sig_addr);

    AFM* recovery_afm = (AFM*) get_spi_flash_ptr_with_offset(recovery_afm_sig_addr + SIGNATURE_SIZE);

    // Get the signature of first AFM(s) (AFM bodies)
    alt_u32* afm_data = get_spi_flash_ptr_with_offset(recovery_afm_sig_addr + SIGNATURE_SIZE +
                                   ((KCH_BLOCK0*) recovery_afm_sig_addr_ptr)->pc_length);
    alt_u32* afm_header = recovery_afm->afm_addr_def;
    alt_u32 afm_header_size = recovery_afm->length;

    alt_u32 ufm_location_number = UFM_AFM_1;

    // Iterate through the AFM bodies in the recovery region
    while (afm_header_size)
    {
        // Process data according to the definition type
        alt_u8 def_type = *((alt_u8*) afm_header);
        AFM_ADDR_DEF* afm_addr_def = (AFM_ADDR_DEF*) afm_header;
        AFM_BODY* afm_body = (AFM_BODY*) (afm_data + (SIGNATURE_SIZE >> 2));

        // Check for right definition type
        if (def_type == AFM_SPI_REGION_DEF_TYPE)
        {
            // Save to check here as active image is valid, it means AFM(s) has been saved into UFM
            if (is_active_valid)
            {
                // Obtain the SVN of a particular device AFM before writing the SVN into the UFM
                ufm_location_number = (get_ufm_afm_addr_offset(afm_addr_def->uuid) - UFM_AFM_1) /
                                      (AFM_BODY_UFM_ALLOCATED_SIZE);

                // save the SVN to the UFM
                write_ufm_svn(UFM_SVN_POLICY_AFM_UUID(ufm_location_number), afm_body->svn);
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
        ufm_location_number += 0x2000;
    }
}

/**
 * @brief Copies the AFM(s) content which has UUID/measurements etc etc.. into the UFM after authentication.
 * Saves up to 4 AFM for 4 different devices.
 *
 * Nios iterates through the AFM body in the SPI Flash active region and searches for all the AFM bodies.
 * Before Nios copies the AFM(s) into the UFM, Nios authenticates the AFM image in the active SPI Flash and
 * ensure they pass. Before the actual copy process is done, Nios checks whats in the UFM content before
 * deciding to erase them to preserve the erase cycle of the UFM. This handles situation if user decides to
 * power cycle the platform repeatedly with same spi image.
 *
 * Nios erase UFM if:
 * 1) The content of AFM in the UFM has changed for a particular device (uuid).
 *    -> Can come from active/recovery update/recovery
 * 2) The number of AFM place holders are less than the total number of new AFM(s) in the active image.
 *    -> Can come from authentication active recovery
 *
 * Nios does not erase UFM if:
 * 1) Similar content of AFM already stored in UFM
 *   -> Can come from AC cycle
 * 2) The number of AFM place holders can accommodate new AFM(s), Nios simply copies them over.
 *   -> Can come from authentication recovery. For example, if there are 3 AFMs in UFM and there are now
 *      4 AFM(s) in the active region, if the 3 AFM are matching, Nios will only copy the last AFM.
 *
 *
 * @see is_active_afm_valid
 *
 * @param afm: afm structure
 * @param matched_uuid: Number of stored AFM with similar uuid to active AFM
 *
 * @return always return 1 when copying is done or not needed
 */
static alt_u32 copy_afm_to_ufm(AFM* afm, alt_u32 matched_uuid)
{
    alt_u32* afm_header = afm->afm_addr_def;
    alt_u32 afm_header_size = afm->length;
    // Check number of new AFM(s) in the active region
    alt_u32 total_new_afm = afm_header_size/sizeof(AFM_ADDR_DEF);

    // Check how much empty location in the UFM
    alt_u32 empty_location_number = get_ufm_empty_afm_number();
    alt_u32 stored_afm = MAX_NO_OF_AFM_SUPPORTED - empty_location_number;

    // If exact same AFM(s) are already stored, simply return.
    if ((total_new_afm == matched_uuid) && (total_new_afm == stored_afm))
    {
        return 1;
    }

    // If UFM empty locations cannot fit the new AFM(s)
    alt_u32 new_afm = total_new_afm - matched_uuid;
    if ((new_afm > empty_location_number) || (total_new_afm < stored_afm))
    {
        // Erase page to make way for new aAFM(s)
        ufm_erase_page(UFM_AFM_1);
    }

    // Go through AFM header body
    while (afm_header_size)
    {
        alt_u32 incr_size = 0;

        // Process data according to the definition type
        alt_u8 def_type = *((alt_u8*) afm_header);
        if (def_type == AFM_SPI_REGION_DEF_TYPE)
        {
            AFM_ADDR_DEF* afm_addr_def = (AFM_ADDR_DEF*) afm_header;

            // get the afm body from address definition
            AFM_BODY* afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

            alt_u32 ufm_afm_addr_offset = get_ufm_afm_addr_offset(afm_body->uuid);

            // Get address to the AFM based on UUID and replace them
            if (!ufm_afm_addr_offset)
            {
                ufm_afm_addr_offset = get_ufm_empty_afm_offset();

                alt_u32* ufm_afm_ptr = get_ufm_ptr_with_offset(ufm_afm_addr_offset);

                // Calculate the AFM size based on the measurements
                alt_u32 afm_size = get_afm_body_size(afm_body);

                alt_u32_memcpy_s(ufm_afm_ptr, UFM_CPLD_PAGE_SIZE, (alt_u32*) afm_body, afm_size);
            }

            incr_size = sizeof(AFM_ADDR_DEF);
        }
        else
        {
            // No more definition, break out from loop
            break;
        }

        // Move the AFM body pointer to the next afm definition
        afm_header = incr_alt_u32_ptr(afm_header, incr_size);
        afm_header_size -= incr_size;
    }

    return 1;
}

/**
 * @brief Verifies the signature and the content of the AFM bodies. Also checks the content in the UFM.
 *
 * Nios checks the signature of the AFM body for a particular device and then verify the content. Nios also
 * checks if the afm body has been stored into the UFM previously. If the content of the AFM of a particular
 * device has changed, Nios will erase the entire page in the UFM (containing other AFMs as well). This is
 * required as the new AFM for the same device needs to be updated.
 *
 * @see is_afm_addr_def_valid
 *
 * @param signed_afm_addr: Address of signed afm body
 * @param expected_uuid: UUID of the afm body
 * @param matched_afm: number of matched afm
 *
 * @return 1 if pass, 0 if fail
 *
 */
static alt_u32 is_signed_afm_body_valid(alt_u32 signed_afm_addr, alt_u16 expected_uuid, alt_u32* matched_afm)
{
	AFM_BODY* afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(signed_afm_addr + SIGNATURE_SIZE);

    // Verify the signature of afm body
    if (!is_signature_valid(signed_afm_addr))
    {
        return 0;
    }

    // No SVN should be larger
    if (afm_body->svn > UFM_MAX_SVN)
    {
        return 0;
    }

    // Check if the uuid reflected in the address definition is consistent with the body
    if (afm_body->uuid != expected_uuid)
    {
        return 0;
    }

    KCH_SIGNATURE* signature_addr = (KCH_SIGNATURE*) get_spi_flash_ptr_with_offset(signed_afm_addr);
    KCH_BLOCK1* b1 = (KCH_BLOCK1*) &signature_addr->b1;

    KCH_BLOCK1_B0_ENTRY* b0_entry = (KCH_BLOCK1_B0_ENTRY*) &b1->b0_entry;

    if (b0_entry->sig_magic == KCH_EXPECTED_SIG_MAGIC_FOR_384)
    {
        if (afm_body->curve_magic != AFM_CURVE_MAGIC_SECP384R1)
        {
            return 0;
        }
    }
    else if (b0_entry->sig_magic == KCH_EXPECTED_SIG_MAGIC_FOR_256)
    {
        if (afm_body->curve_magic != AFM_CURVE_MAGIC_SECP256R1)
        {
            return 0;
        }
    }

    // obtain the size of the afm_body in bytes
    alt_u32 afm_size = get_afm_body_size(afm_body);

    // get_afm_body_size() returns 0 if calculate afm_body size exceeds AFM_BODY_UFM_ALLOCATED_SIZE
    if (afm_size == 0)
    {
        return 0;
    }

    // First find any empty section of the ufm page
    alt_u32 ufm_afm_addr_offset = get_ufm_afm_addr_offset(expected_uuid);
    alt_u32* ufm_afm_ptr = get_ufm_ptr_with_offset(ufm_afm_addr_offset);

    // Check if this afm body has been stored in the ufm
    // Afm could have been stored on previous AC cycles
    if (ufm_afm_addr_offset)
    {
        if (is_data_matching_stored_data((alt_u32*) afm_body, ufm_afm_ptr, afm_size))
        {
            //Matched uuid and matched afm data
            *matched_afm += 1;
        }
        else
        {
            // Matching uuid but different data.
            // Thus need to be overwritten
            ufm_erase_page(UFM_AFM_1);
        }
    }
    return 1;
}

/**
 * @brief Verifies the address definition of AFM which contains the address to the AFM bodies.
 *
 * Nios perform checks on the address and ensure their validness before proceeding to search for
 * the afm bodies defined by the address.
 *
 * @see is_active_afm_valid
 *
 * @param afm_addr_def: Address to the address def
 * @param matched_afm: number of matched afm
 *
 * @return 1 if pass, 0 if fail
 *
 */
static alt_u32 is_afm_addr_def_valid(AFM_ADDR_DEF* afm_addr_def, alt_u32* matched_afm)
{
    // Length of 1 AFM must be 8kB max
    if (afm_addr_def->afm_length > UFM_CPLD_PAGE_SIZE)
    {
        return 0;
    }
    // Address definition must be 4kB aligned
    if (afm_addr_def->afm_addr % 0x1000 == 0)
    {
        return is_signed_afm_body_valid(afm_addr_def->afm_addr, afm_addr_def->uuid, matched_afm);
    }
    return 0;
}

/**
 * @brief Verifies the content of the active AFM before copying the AFM(s) into UFM. Signature has
 * been previously validated.
 *
 * Nios iterates through the afm header and proceeds to validate all the AFM(s) in the active SPI flash.
 * Nios only save the AFM(s) if active AFM(s) are valid.  Alignment of the address definition are also
 * checked.
 *
 * @see authenticate_and_recover_spi_flash
 * @see is_staging_active_afm_valid
 *
 * @param afm: Address to the AFM
 * @param is_active_afm: active afm indicator
 *
 * @return 1 if pass, 0 if fail
 *
 */
static alt_u32 is_active_afm_valid(AFM* afm, alt_u32 is_active_afm)
{
    // Check max SVN
    if (afm->svn > UFM_MAX_SVN)
    {
        return 0;
    }
    alt_u32 number_of_matched_afm = 0;

    // Afm tag
    if (afm->tag == AFM_TAG)
    {
        alt_u32* afm_header = afm->afm_addr_def;
        alt_u32 afm_header_size = afm->length;

        // Max number for allowed AFM is 4
        // Size value must also be aligned
        if ((afm_header_size % 4 != 0) || (afm_header_size > 4*sizeof(AFM_ADDR_DEF)))
        {
            return 0;
        }

        // Go through AFM header body
        while (afm_header_size)
        {
            alt_u32 incr_size = 0;

            // Process data according to the definition type
            alt_u8 def_type = *((alt_u8*) afm_header);
            if (def_type == AFM_SPI_REGION_DEF_TYPE)
            {
                AFM_ADDR_DEF* afm_addr_def = (AFM_ADDR_DEF*) afm_header;
                // Proceeds to authenticate the content of address definition and afm bodies
                if (!is_afm_addr_def_valid(afm_addr_def, &number_of_matched_afm))
                {
                    return 0;
                }
                incr_size = sizeof(AFM_ADDR_DEF);
            }
            else
            {
                // No more definition, break out from loop
                break;
            }

            // Move the AFM body pointer to the next afm definition
            afm_header = incr_alt_u32_ptr(afm_header, incr_size);
            afm_header_size -= incr_size;
        }
    }
    else
    {
        return 0;
    }

    // Only copy AFM(s) over to UFM if is active afm
    if (is_active_afm)
    {
        return copy_afm_to_ufm(afm, number_of_matched_afm);
    }
    return 1;
}
#endif

/**
 * @brief Perform validation on the active region PFM.
 * First, it verifies the signature of the PFM. Then, it verifies
 * the content (e.g. SPI region and SMBus rule definitions) of the PFM.
 *
 * @param spi_flash_type : BMC or PCH
 *
 * @return 1 if the active region is valid; 0, otherwise.
 */
static alt_u32 is_active_region_valid(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    alt_u32 active_pfm_addr = get_active_region_offset(spi_flash_type);

    // Verify the signature of the PFM first, then SPI region definitions and other content in PFM.
    return is_signature_valid(active_pfm_addr) &&
           is_active_pfm_valid((PFM*) get_spi_flash_ptr_with_offset(active_pfm_addr + SIGNATURE_SIZE));
}

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)

/**
 * @brief Perform validation on the active region AFM.
 * First, it verifies the signature of the AFM. Then, it verifies
 * the content (e.g. AFM address definition and AFM bodies).
 *
 * @param spi_flash_type: flash type
 *
 * @return 1 if the active region is valid; 0, otherwise.
 */
static alt_u32 is_staging_active_afm_valid(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    // The active AFM is stored in the staging region of BMC SPI flash
    alt_u32 active_afm_addr = get_staging_region_offset(spi_flash_type) + BMC_STAGING_REGION_AFM_ACTIVE_OFFSET;
    // Verify the signature of AFM first, then the content of the AFM body
    return is_signature_valid(active_afm_addr) &&
           is_active_afm_valid((AFM*)get_spi_flash_ptr_with_offset(active_afm_addr + SIGNATURE_SIZE), 1);
}

/**
 * @brief Perform validation on the recovery region AFM.
 * Verify the signature of the recovery image.
 *
 * @param spi_flash_type: flash type
 *
 * @return 1 if the recovery region is valid; 0, otherwise.
 */
static alt_u32 is_staging_recovery_afm_valid(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    // The active AFM is stored in the staging region of BMC SPI flash
    alt_u32 recovery_afm_addr = get_staging_region_offset(spi_flash_type) + BMC_STAGING_REGION_AFM_RECOVERY_OFFSET;
    // Verify the signature of AFM recovery
    return is_signature_valid(recovery_afm_addr);
}

#endif

#endif /* EAGLESTREAM_INC_PFM_VALIDATION_H_ */
