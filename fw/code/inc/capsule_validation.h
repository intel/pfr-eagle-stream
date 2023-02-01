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
 * @file capsule_validation.h
 * @brief Perform validation on a firmware update capsule.
 */

#ifndef EAGLESTREAM_INC_CAPSULE_VALIDATION_H
#define EAGLESTREAM_INC_CAPSULE_VALIDATION_H

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "authentication.h"
#include "cpld_update.h"
#include "pbc_utils.h"


/**
 * @brief This function validates a PBC structure.
 * For example, this function checks the tag, version and page size.
 *
 * @param pbc pointer to a PBC structure.
 *
 * @return alt_u32 1 if all checks passed; 0, otherwise.
 */
static alt_u32 is_pbc_valid(PBC_HEADER* pbc)
{
    // Check Magic number
    if (pbc->tag != PBC_EXPECTED_TAG)
    {
        return 0;
    }

    // Check version
    if (pbc->version != PBC_EXPECTED_VERSION)
    {
        return 0;
    }

    // Check pattern
    if (pbc->pattern != PBC_EXPECTED_PATTERN)
    {
        return 0;
    }

    // only 4kB pages are valid for SPI NOR flash used in PFR systems.
    if (pbc->page_size != PBC_EXPECTED_PAGE_SIZE)
    {
        return 0;
    }

    // Number of bits in bitmap must be multiple of 8
    // This is because Nios processes bitmap byte by byte.
    if (pbc->bitmap_nbit % 8)
    {
        return 0;
    }

    return 1;
}

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)


/**
 * @brief This function validates the content of AFM header. Signature of the afm capsule has been
 * validated prior to calling this function.
 *
 * For an active AFM update, each of the AFM(s) in the active image can be updated simultaneously. For example,
 * user can choose to update just a single AFM or all the AFM(s) in the active SPI flash.
 * For a recovery AFM update, single AFM update is not supported unless the entire AFM contains only a single
 * AFM bodies for a device. This is to ensure that attacker cannot erase existing AFM(s) using this update.
 * Nios will checks for the AFM(s) present before allowing the recovery update.
 *
 * Checks performed here are SVN, AFM identity etc..
 *
 *
 * @param signed_capsule_addr: address of signed afm capsule.
 * @param is_active_update: recovery or active update
 *
 * @return alt_u32 1 if all checks passed; 0, otherwise.
 */
static alt_u32 is_afm_capsule_valid(alt_u32 signed_capsule_addr, alt_u32 is_active_update)
{
    AFM* capsule_afm = (AFM*)get_capsule_afm_ptr(SPI_FLASH_BMC);

    // Get the signature of first AFM
    alt_u32* afm_data = get_capsule_first_afm_ptr(signed_capsule_addr);

    // The uuid must be defined in the active region. Rather than going through the entire loop to find the uuid
    // in the active AFM region, browse through the UFM as the AFM of the active region has been stored.
    alt_u32* afm_header = capsule_afm->afm_addr_def;
    alt_u32 afm_addr_def_size = capsule_afm->length;

    // If its a recovery update, check that all AFM are present in the AFM capsule
    // Single AFM recovery update is not permitted due to the difference in structure between the capsule and what's stored in the image
    if (!is_active_update)
    {
        alt_u32 num_of_afm_present = MAX_NO_OF_AFM_SUPPORTED - get_ufm_empty_afm_number();
        if ((afm_addr_def_size / sizeof(AFM_ADDR_DEF)) != num_of_afm_present)
        {
            // The number of AFM(s) stored does not match the incoming number of AFM(s)
            // Incomplete AFM recovery update is not permitted
            log_update_failure(MINOR_ERROR_UNKNOWN_AFM, 1);
            return 0;
        }
    }

    // Iterate through the afm address definition
    while (afm_addr_def_size)
    {
        alt_u32 incr_size = 0;

        AFM_BODY* capsule_afm_body = (AFM_BODY*) (afm_data + (SIGNATURE_SIZE >> 2));
        AFM_ADDR_DEF* afm_addr_def = (AFM_ADDR_DEF*) afm_header;

        // Process data according to the definition type
        alt_u8 def_type = *((alt_u8*) afm_header);
        if (def_type == AFM_SPI_REGION_DEF_TYPE)
        {
            alt_u32 ufm_afm_addr = get_ufm_afm_addr_offset(afm_addr_def->uuid);
            if (!ufm_afm_addr)
            {
                // Cannot find corresponding AFM in the Active PFR region, log an error
                log_update_failure(MINOR_ERROR_UNKNOWN_AFM, 1);
                return 0;
            }

            // If this is an active update, the SVN of AFM capsule must match the active AFM
            if (is_active_update)
            {
                // Check that the SVN of the AFM matches, can be done by checking what's stored in the UFM instead
                AFM_BODY* afm_body = (AFM_BODY*) get_ufm_ptr_with_offset(ufm_afm_addr);
                if (afm_body->svn != capsule_afm_body->svn)
                {
                    // The capsule AFM SVN must match what's in the active AFM SVN
                    log_update_failure(MINOR_ERROR_INVALID_SVN, 1);
                    return 0;
                }
            }
            else
            {
                // For recovery update, as long as the SVN for all AFM(s) are valid
            	// and does not have to match what's in the active region
                alt_u32 ufm_location_number = (get_ufm_afm_addr_offset(capsule_afm_body->uuid) - UFM_AFM_1) /
                                              (AFM_BODY_UFM_ALLOCATED_SIZE);
                alt_u32* svn_policy = UFM_SVN_POLICY_AFM_UUID(ufm_location_number);

                if (!is_svn_valid(svn_policy, capsule_afm_body->svn))
                {
                    log_update_failure(MINOR_ERROR_INVALID_SVN, 1);
                    return 0;
                }
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
        afm_data = incr_alt_u32_ptr(afm_data, afm_addr_def->afm_length);
        afm_addr_def_size -= incr_size;
    }
    return 1;
}


#endif



#ifdef PLATFORM_SEAMLESS_FEATURES_ENABLED

/**
 * @brief Validate the FVM inside the given Seamless capsule.
 *
 * These are the checks:
 * - Nios uses the FV type of this FVM to find the corresponding FVM in Active region. If
 * it cannot be find, Nios logs an error about this unknown FV type and rejects this capsule.
 * - Nios compares the SVN of this FVM and the corresponding FVM in the Active region. If
 * they are different, Nios logs an error about this invalid SVN and rejects this capsule.
 * - Nios compares the length of this FVM and the corresponding FVM in the Active region. If
 * they are different, Nios logs an error and rejects this capsule. It's expected that the
 * SPI regions that this new FVM covers match the Active FVM. Checking length is a convenient
 * way to confirm that.
 *
 * @param signed_capsule_addr start address of a signed capsule
 * @return 1 if the capsule's FVM passed all the checks; 0, otherwise.
 */
static alt_u32 is_seamless_capsule_fvm_valid(alt_u32 signed_capsule_addr)
{
    alt_u32* signed_capsule = get_spi_flash_ptr_with_offset(signed_capsule_addr);
    FVM* capsule_fvm = get_seamless_capsule_fvm(signed_capsule);
    alt_u32 minor_err = 0;

    // This FV type must be defined in the Active PFR region
    alt_u32 dest_signed_fvm_addr = get_active_fvm_addr(capsule_fvm->fv_type);
    if (dest_signed_fvm_addr == 0)
    {
        // Cannot find corresponding FVM in the Active PFR region, log an error
        minor_err = MINOR_ERROR_UNKNOWN_FV_TYPE;
    }
    else
    {
        FVM* active_fvm = (FVM*) get_spi_flash_ptr_with_offset(dest_signed_fvm_addr + SIGNATURE_SIZE);

        if (active_fvm->svn != capsule_fvm->svn)
        {
            // Incoming SVN must match the SVN in the Active FVM
            minor_err = MINOR_ERROR_INVALID_SVN;
        }
        else if (active_fvm->length != capsule_fvm->length)
        {
            // Incoming FVM must have the same length with the Active FVM
            minor_err = MINOR_ERROR_AUTHENTICATION_FAILED;
        }
    }

    // If there's any error recorded, log the error and exit with 0
    if (minor_err)
    {
        log_update_failure(minor_err, 1);
    }

    return minor_err == 0;
}

/**
 * @brief Validate the FV types of a staging capsule.
 *
 * @param staging_capsule_addr: Capsule address in BMC/PCH spi flash
 *
 * @return 1 if successful; 0 otherwise
 */
static alt_u32 is_fw_capsule_fv_type_valid(alt_u32 staging_capsule_addr)
{
    alt_u32* staging_capsule = get_spi_flash_ptr_with_offset(staging_capsule_addr);
    PFM* capsule_pfm = get_capsule_pfm(staging_capsule);

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

            // Check if FV type is bigger than supported type
            if (fvm->fv_type > UFM_MAX_SUPPORTED_FV_TYPE)
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
 * @brief Validate FVMs inside a PCH firmware update capsule.
 * Nios finds all the FVMs by looking at the FVM address definitions in PFM body. If
 * any signature of these FVMs is not authentic, Nios logs error and returns. If
 * any SVN of these FVMs is bannded, Nios logs error and returns.
 *
 * @param signed_capsule_addr start address of a signed capsule
 * @return 1 if all FVMs inside this capsule passed validation; 0, otherwise.
 */
static alt_u32 are_fw_capsule_fvms_valid(alt_u32 signed_capsule_addr)
{
    alt_u32* signed_capsule = get_spi_flash_ptr_with_offset(signed_capsule_addr);

    PFM* capsule_pfm = get_capsule_pfm(signed_capsule);
    alt_u32 pfm_body_size = capsule_pfm->length - PFM_HEADER_SIZE;
    alt_u32* pfm_body = capsule_pfm->pfm_body;

    // Iterate through the PFM body to authenticate all FVM Address Definitions and FVMs
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
            PFM_FVM_ADDR_DEF* fvm_addr_def = (PFM_FVM_ADDR_DEF*) pfm_body;
            alt_u32 fvm_addr_in_capsule = get_addr_of_signed_fvm_in_capsule(fvm_addr_def->fvm_addr, signed_capsule_addr);

            if (!is_signature_valid(fvm_addr_in_capsule))
            {
                log_update_failure(MINOR_ERROR_AUTHENTICATION_FAILED, 1);
                return 0;
            }

            // If FVM's signature passes authentication, validate its SVN.
            // As long as the SVN is not banned, this FVM is fine.
            // Note that this is different to the PFM SVN. For example, in active update,
            // capsule's PFM SVN must match active region's PFM SVN. FVM doesn't have this requirement,
            // since the incoming list of FVMs may be different to the list of FVMs in Active Region.
            FVM* fvm = (FVM*) get_spi_flash_ptr_with_offset(fvm_addr_in_capsule + SIGNATURE_SIZE);
            if (fvm->fv_type > UFM_MAX_SUPPORTED_FV_TYPE)
            {
                log_update_failure(MINOR_ERROR_UNKNOWN_FV_TYPE, 1);
                return 0;
            }

            alt_u32* svn_policy = UFM_SVN_POLICY_FVM(fvm->fv_type);
            if (!is_svn_valid(svn_policy, fvm->svn))
            {
                log_update_failure(MINOR_ERROR_INVALID_SVN, 1);
                return 0;
            }

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

    return 1;
}

#endif /* PLATFORM_SEAMLESS_FEATURES_ENABLED */


/**
 * @brief Validate the content of a signed firmware capsule.
 *
 * This includes validating expected data in the PBC structure and
 * verifying signature of its PFM.
 *
 * For AFM update, PBC validation is skipped as AFM capsules are not compressed.
 *
 * @param signed_capsule_addr start address of a signed capsule
 * @param update_intent_part1 value read from the PCH or BMC update intent part 1 register
 * @return alt_u32 1 if the capsule content is valid; 0, otherwise.
 */
static alt_u32 is_fw_capsule_content_valid(alt_u32 signed_capsule_addr, alt_u32 update_intent_part1, alt_u32 update_intent_part2)
{
    alt_u32 is_valid = 1;

    alt_u32* signed_capsule = get_spi_flash_ptr_with_offset(signed_capsule_addr);
    alt_u32 pc_type = ((KCH_SIGNATURE*) signed_capsule)->b0.pc_type;
    alt_u32 signed_pfm_addr = signed_capsule_addr + SIGNATURE_SIZE;

    // If this is an AFM capsule, skip PBC validation
    if (!update_intent_part1 && (update_intent_part2 & MB_UPDATE_INTENT_AFM_FW_UPDATE_MASK))
    {
        // Verify AFM signature
        is_valid = is_valid && is_signature_valid(signed_pfm_addr);
    }
    else
    {
        // Validate PFM signature and PBC structure
        is_valid = is_valid && is_signature_valid(signed_pfm_addr);
    	is_valid = is_valid && is_pbc_valid(get_pbc_ptr_from_signed_capsule(signed_capsule));
    }

    if (is_valid)
    {
        // Validate PFM SVN next
        alt_u8 new_svn = get_capsule_pfm(signed_capsule)->svn;
        if (pc_type == KCH_PC_PFR_PCH_UPDATE_CAPSULE)
        {
            // Incoming PFM must have a valid SVN
            is_valid = is_valid && is_svn_valid(UFM_SVN_POLICY_PCH, new_svn);
            // If this is a recovery update, any valid SVN can be accepted (i.e. can be >= to SVN of RC image)
            // If this is an active update, the SVN must match the SVN of active/recovery image
            is_valid = is_valid &&
                    ((update_intent_part1 & MB_UPDATE_INTENT_PCH_RECOVERY_MASK)
                            || (get_active_pfm(SPI_FLASH_PCH)->svn == new_svn));

            if (!is_valid)
            {
                log_update_failure(MINOR_ERROR_INVALID_SVN, 1);
            }

#ifdef PLATFORM_SEAMLESS_FEATURES_ENABLED
            // PCH firmware capsule may have FVMs. Validate them.
            is_valid = is_valid && are_fw_capsule_fvms_valid(signed_capsule_addr);
#endif /* PLATFORM_SEAMLESS_FEATURES_ENABLED */
        }
        else if (pc_type == KCH_PC_PFR_BMC_UPDATE_CAPSULE)
        {
            // Incoming PFM must have a valid SVN
            is_valid = is_valid && is_svn_valid(UFM_SVN_POLICY_BMC, new_svn);
            // If this is a recovery update, any valid SVN can be accepted (i.e. can be >= to SVN of RC image)
            // If this is an active update, the SVN must match the SVN of active/recovery image
            is_valid = is_valid &&
                    ((update_intent_part1 & MB_UPDATE_INTENT_BMC_RECOVERY_MASK)
                            || (get_active_pfm(SPI_FLASH_BMC)->svn == new_svn));
            if (!is_valid)
            {
                log_update_failure(MINOR_ERROR_INVALID_SVN, 1);
            }
        }
#ifdef PLATFORM_SEAMLESS_FEATURES_ENABLED
        else if (pc_type == KCH_PC_PFR_PCH_SEAMLESS_CAPSULE)
        {
            // This is a Seamless Capsule
            is_valid = is_valid && is_seamless_capsule_fvm_valid(signed_capsule_addr);
        }
#endif /* PLATFORM_SEAMLESS_FEATURES_ENABLED */
#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)
        else if (pc_type == KCH_PC_PFR_AFM)
        {
            AFM* capsule_afm = (AFM*)get_capsule_afm_ptr(SPI_FLASH_BMC);
            AFM* active_afm = (AFM*) get_spi_flash_ptr_with_offset(BMC_STAGING_REGION_ACTIVE_AFM_OFFSET + SIGNATURE_SIZE);
            // Incoming AFM must have a valid SVN
            is_valid = is_valid && is_svn_valid(UFM_SVN_POLICY_AFM, capsule_afm->svn);
            // If this is a recovery update, any valid SVN can be accepted (i.e. can be >= to SVN of RC image)
            // If this is an active update, the SVN must match the SVN of active/recovery image
            is_valid = is_valid &&
                    ((update_intent_part2 & MB_UPDATE_INTENT_AFM_RECOVERY_MASK)
                            || (active_afm->svn == capsule_afm->svn));
            if (!is_valid)
            {
                log_update_failure(MINOR_ERROR_INVALID_SVN, 1);
            }

            // Active update requires the SVN of the incoming SVN to match the SVN of the AFM in the active resion
            // Recovery update requires a full AFM update
            // This update does not require the SVN to match the active AFM SVN but as long as the SVN are all valid, it will be fine
            // This is an AFM Capsule, verify all SVN(s) next
            is_valid = is_valid && is_afm_capsule_valid(signed_capsule_addr, update_intent_part2 & MB_UPDATE_INTENT_AFM_ACTIVE_MASK);
        }
#endif
    }
    else
    {
        log_update_failure(MINOR_ERROR_AUTHENTICATION_FAILED, 1);
    }

    return is_valid;
}

/**
 * @brief Return non-zero value if the pc type in the given signature matches the update intent.
 *
 * If this capsule is a signed key cancellation certificate, then no need to check update intent. User
 * can use any update intent to send in key cancellation certificate.
 *
 * Otherwise, if the update intent is BMC active/recovery firwmare update, then the PC type must be BMC
 * update capsule.
 * If the update intent is PCH active/recovery firwmare update, then the PC type must be PCH
 * update capsule.
 * If the update intent is CPLD active/recovery firwmare update, then the PC type must be CPLD
 * update capsule.
 *
 * @return alt_u32 1 if pc type matches the update intent; 0, otherwise.
 */
static alt_u32 does_pc_type_match_update_intent(KCH_BLOCK0* sig_b0, alt_u32 update_intent_part1, alt_u32 update_intent_part2)
{
    // Allow user to issue key cancellation with any update intent
    if ((sig_b0->pc_type & KCH_PC_TYPE_KEY_CAN_CERT_MASK))
    {
        return 1;
    }
    else if (sig_b0->pc_type & KCH_PC_TYPE_DECOMM_CAP_MASK)
    {
        return update_intent_part1 & MB_UPDATE_INTENT_CPLD_MASK;
    }
    else if (sig_b0->pc_type == KCH_PC_PFR_BMC_UPDATE_CAPSULE)
    {
        // If it's a BMC update capsule, the update intent must be BMC firmware update
        return update_intent_part1 & MB_UPDATE_INTENT_BMC_FW_UPDATE_MASK;
    }
    else if (sig_b0->pc_type == KCH_PC_PFR_PCH_UPDATE_CAPSULE)
    {
        // If it's a PCH update capsule, the update intent must be PCH firmware update
        return update_intent_part1 & MB_UPDATE_INTENT_PCH_FW_UPDATE_MASK;
    }
    else if (sig_b0->pc_type == KCH_PC_PFR_CPLD_UPDATE_CAPSULE)
    {
        // If it's a CPLD update capsule, the update intent must be CPLD firmware update
        return update_intent_part1 & MB_UPDATE_INTENT_CPLD_MASK;
    }
#ifdef PLATFORM_SEAMLESS_FEATURES_ENABLED
    else if (sig_b0->pc_type == KCH_PC_PFR_PCH_SEAMLESS_CAPSULE)
    {
        // If it's a seamles update capsule, the update intent must be seamless update
        return update_intent_part2 == MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK;
    }
#endif
#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)
    else if (sig_b0->pc_type == KCH_PC_PFR_AFM)
    {
        // If it's an AFM update capsule, the update intent must be AFM firmware update
        return update_intent_part2 & MB_UPDATE_INTENT_AFM_FW_UPDATE_MASK;
    }
#endif

    return 0;
}

/**
 * @brief Validate the signature of a capsule.
 *
 * Nios checks the PC type of the capsule matches the update intent first, before authentication.
 * Attacker may place a valid capsule at the unexpected location, which can cause Nios trouble during
 * authentication. For example, a BMC capsule signature may be put in at CPLD staging area in BMC SPI
 * flash. CPLD staging area is near the end of BMC SPI flash. Hashing the protected content would
 * require CPLD to read beyond the SPI AvMM memory space.
 *
 * @param signed_capsule_addr : to the start address of a signed capsule
 * @param update_intent_part1 : value read from the PCH or BMC update intent part 1 register
 * @param update_intent_part2 : value read from the PCH or BMC update intent part 2 register
 *
 * @return alt_u32 1 if Nios should proceed to perform the CPLD or FW update with the @p signed_capsule; 0, otherwise.
 *
 */
static alt_u32 is_capsule_sig_valid(alt_u32 signed_capsule_addr, alt_u32 update_intent_part1, alt_u32 update_intent_part2)
{
    KCH_SIGNATURE* signed_capsule = (KCH_SIGNATURE*) get_spi_flash_ptr_with_offset(signed_capsule_addr);

    alt_u32 is_valid = does_pc_type_match_update_intent(&signed_capsule->b0, update_intent_part1, update_intent_part2);
    is_valid = is_valid && is_signature_valid(signed_capsule_addr);

    if (!is_valid)
    {
        log_update_failure(MINOR_ERROR_AUTHENTICATION_FAILED, 1);
    }

    return is_valid;
}

/**
 * @brief Pre-process the update capsule prior to performing the FW or CPLD update.
 *
 * Assume the capsule is in the correct flash.
 * Nios ensures the PC type of the capsule matches the update intent first, before authentication.
 * Attacker may put in valid capsule at the unexpected and cause Nios problem doing authentication. For
 * example, a BMC capsule signature may be put in at CPLD staging area in BMC SPI flash. CPLD staging
 * area is near the end of BMC SPI flash. Hashing the protected content would require CPLD to read beyond
 * the SPI AvMM memory space.
 *
 * After that, Nios authenticate the capsule signature. If that passes, Nios moves on to validate its content.
 * If this capsule is a key cancellation certificate, Nios cancels the key.
 * If this capsule is a decommission capsule, Nios erases UFM and then reconfig into CFM1 (Active Image).
 * If this capsule is a CPLD update capsule, this function returns 1 if its SVN is valid.
 * If this capsule is a firmware update capsule, Nios proceeds to check the signature of PFM. Once that passes,
 * SVN is then checked. The SVN checks here are more involved. The capsule PFM SVN is validated against the
 * SVN policy stored in UFM. Also, if this is an active firmware update, its SVN must be the same as the SVN
 * of recovery image. If user wishes to bump the SVN, a recovery firmware update, which update both active and
 * recovery firmware, must be triggered.
 *
 * If any check fails, Nios logs a major/minor error and increments number of failed update attempts counter.
 *
 * @param signed_capsule pointer to the start address of a signed capsule
 * @param update_intent_part1 value read from the PCH or BMC update intent part 1 register
 * @param update_intent_part2 value read from the PCH or BMC update intent part 2 register
 *
 * @return alt_u32 1 if Nios should proceed to perform the CPLD or FW update with the @p signed_capsule; 0, otherwise.
 *
 * @see act_on_update_intent
 * @see is_svn_valid
 * @see does_pc_type_match_update_intent
 * @see is_signature_valid
 */
static alt_u32 check_capsule_before_update(
        alt_u32 signed_capsule_addr, alt_u32 update_intent_part1, alt_u32 update_intent_part2)
{
    KCH_SIGNATURE* signed_capsule = (KCH_SIGNATURE*) get_spi_flash_ptr_with_offset(signed_capsule_addr);
    // This minor error code will be posted to the Mailbox, when any check failed

    KCH_BLOCK0* b0 = (KCH_BLOCK0*) &signed_capsule->b0;
    // The PC type of the capsule must match the update intent
    // Then Nios authenticates the capsule's signature
    if (is_capsule_sig_valid(signed_capsule_addr, update_intent_part1, update_intent_part2))
    {
        alt_u32* capsule_pc = incr_alt_u32_ptr((alt_u32*) signed_capsule, SIGNATURE_SIZE);
        if (b0->pc_type & KCH_PC_TYPE_KEY_CAN_CERT_MASK)
        {
            alt_u32 pc_type = get_kch_pc_type(b0);
            alt_u32 key_id = ((KCH_CAN_CERT*) capsule_pc)->csk_id;

            if (is_csk_key_unused(pc_type, key_id))
            {
                // If this is a key cancellation certificate, proceed to cancel this key.
                // Only cancel the key if not being used
                cancel_key(pc_type, key_id);
                // let Nios proceed to leave this function with return value 0, so that no update will be done.
            }
            else
            {
                log_update_failure(MINOR_ERROR_AUTHENTICATION_FAILED, 1);
            }
        }
        else if (b0->pc_type & KCH_PC_TYPE_DECOMM_CAP_MASK)
        {
            // This is a valid decommission capsule; proceed.

            // Perform UFM page erase to clear PFR data
            ufm_erase_page(0);

            // Reconfig into Active image and it will run in un-provisioned mode
            perform_cfm_switch(CPLD_CFM1);
        }
        else if (b0->pc_type == KCH_PC_PFR_CPLD_UPDATE_CAPSULE)
        {
            // This is a CPLD update
            if (is_svn_valid(UFM_SVN_POLICY_CPLD, ((CPLD_UPDATE_PC*) capsule_pc)->svn))
            {
                // Yes, this is a valid CPLD update capsule. Proceed with this CPLD update
                return 1;
            }
            // Failed SVN check
            log_update_failure(MINOR_ERROR_INVALID_SVN, 1);
        }
        else
        {
            // This is a firmware update
            // Validate capsule content: PFM signature, compression structure header, SVN, etc
            return is_fw_capsule_content_valid(signed_capsule_addr, update_intent_part1, update_intent_part2);
        }
    }

    // If Nios reaches here, no fw/cpld update should be done with the given capsule
    return 0;
}

/**
 * @brief Compare staged firmware against active firmware.
 * Nios concludes that active firwmare and staged firmware are identical, if
 * the hashes of their PFM or AFM match exactly.
 *
 * @param spi_flash_type indicate BMC or PCH SPI flash device
 * @param is_afm_capsule indicate if is AFM capsule
 *
 * @return alt_u32 1 if active firmware and staged firmware match exactly; 0, otherwise
 */
static alt_u32 does_staged_fw_image_match_active_fw_image(SPI_FLASH_TYPE_ENUM spi_flash_type, alt_u32 is_afm_capsule)
{
    KCH_BLOCK0* active_pfm_sig_b0 = (KCH_BLOCK0*) get_spi_active_pfm_ptr(spi_flash_type);

    KCH_BLOCK0* capsule_pfm_sig_b0 = (KCH_BLOCK0*) get_spi_flash_ptr_with_offset(get_staging_region_offset(spi_flash_type) + SIGNATURE_SIZE);
    KCH_BLOCK1* capsule_pfm_sig_b1 = (KCH_BLOCK1*) get_spi_flash_ptr_with_offset(get_staging_region_offset(spi_flash_type) + SIGNATURE_SIZE + BLOCK0_SIZE);

    if (is_afm_capsule)
    {
        // Get the address of active afm
        active_pfm_sig_b0 = (KCH_BLOCK0*) get_spi_active_afm_ptr(spi_flash_type);
    }

    KCH_BLOCK1_ROOT_ENTRY* capsule_pfm_sig_b1_root_entry = &capsule_pfm_sig_b1->root_entry;
    alt_u32 pfm_sig_b1_curve_magic = capsule_pfm_sig_b1_root_entry->curve_magic;
    alt_u32 sha_length = (pfm_sig_b1_curve_magic == CURVE_SECP384R1_MAGIC) ? SHA384_LENGTH : SHA256_LENGTH;

    alt_u32* active_pfm_hash = active_pfm_sig_b0->pc_hash256;
    alt_u32* capsule_pfm_hash = capsule_pfm_sig_b0->pc_hash256;
    // If the hashes of PFM or AFM match, the active image and staging image must be the same firmware.
    if (pfm_sig_b1_curve_magic == CURVE_SECP384R1_MAGIC)
    {
        active_pfm_hash = active_pfm_sig_b0->pc_hash384;
        capsule_pfm_hash = capsule_pfm_sig_b0->pc_hash384;
    }

    for (alt_u32 word_i = 0; word_i < sha_length / 4; word_i++)
    {
        if (active_pfm_hash[word_i] != capsule_pfm_hash[word_i])
        {
            // There's a hash mismatch. Image in capsule is different from active image.
            return 0;
        }
    }
    return 1;
}

/**
 * @brief Authenticate staging capsule and also ensure it matches the active image,
 * before firmware recovery update.
 *
 * For the second check, Nios compares the hashes of PFM in staged capsule and active image. If
 * they match, Nios says staged image matches active image and returns 1.
 *
 * @param spi_flash_type indicate BMC or PCH SPI flash device
 * @param skip_hash_check allow user to skip the second check
 *
 * @return alt_u32 1 if staging capsule passed the two checks; 0, otherwise.
 */
static alt_u32 check_capsule_before_fw_recovery_update(SPI_FLASH_TYPE_ENUM spi_flash_type, alt_u32 skip_hash_check, alt_u32 is_afm_capsule)
{
    switch_spi_flash(spi_flash_type);
    alt_u32 staging_capsule_addr = get_staging_region_offset(spi_flash_type);

    if (is_signature_valid(staging_capsule_addr))
    {
#ifdef PLATFORM_SEAMLESS_FEATURES_ENABLED
        if ((!is_afm_capsule) && (!is_fw_capsule_fv_type_valid(staging_capsule_addr)))
        {
            return 0;
        }
#endif
        // Nios can trust this staging capsule now.
        // Next, check if the staged image is identical to the active image.
        return skip_hash_check || does_staged_fw_image_match_active_fw_image(spi_flash_type, is_afm_capsule);
    }

    return 0;
}

#endif /* EAGLESTREAM_INC_CAPSULE_VALIDATION_H */
