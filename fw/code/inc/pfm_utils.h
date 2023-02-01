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
 * @file pfm_utils.h
 * @brief Responsible for extracting and applying PFM rules.
 */

#ifndef EAGLESTREAM_INC_PFM_UTILS_H_
#define EAGLESTREAM_INC_PFM_UTILS_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "keychain.h"
#include "pfm.h"
#include "spi_ctrl_utils.h"
#include "spi_flash_state.h"
#include "spi_rw_utils.h"
#include "ufm_utils.h"
#include "utils.h"
#include "afm.h"

/**
 * @brief Return size (in bytes) of the given SPI region definition @p region_def.
 * A SPI region definition has variable length because there can be no hash, SHA256 hash or SHA384 hash.
 *
 * @param region_def a SPI region definition
 * @return size of region_def
 */
static alt_u32 get_size_of_spi_region_def(PFM_SPI_REGION_DEF* region_def)
{
    // The default size assumes no hash is present
    alt_u32 size = SPI_REGION_DEF_MIN_SIZE;

    if (region_def->hash_algorithm & PFM_HASH_ALGO_SHA256_MASK)
    {
        size += SHA256_LENGTH;
    }
    if (region_def->hash_algorithm & PFM_HASH_ALGO_SHA384_MASK)
    {
        size += SHA384_LENGTH;
    }

    return size;
}

/**
 * @brief Return the pointer to the start of the PFM of the active firmware from a specific flash device.
 * This function first retrieves the offset of signed active PFM from UFM (assuming it has been provisioned)
 * Then, it creates a pointer to the flash AvMM location. After skipping over the signature (Block 0/1),
 * the pointer is returned.
 *
 * @param spi_flash_type indicates BMC or PCH flash
 *
 * @return active pfm pointer
 */
static PFM* get_active_pfm(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    alt_u32* active_signed_pfm_ptr = get_spi_active_pfm_ptr(spi_flash_type);
    return (PFM*) incr_alt_u32_ptr(active_signed_pfm_ptr, SIGNATURE_SIZE);
}

/**
 * @brief Return a pointer to the start address of PFM structure in a signed firmware update capsule.
 * @param signed_capsule indicates signed capsule starting location
 *
 * @return capsule pfm pointer
 */
static PFR_ALT_INLINE PFM* PFR_ALT_ALWAYS_INLINE get_capsule_pfm(alt_u32* signed_capsule)
{
    // Skip capsule signature and PFM signature
    return (PFM*) incr_alt_u32_ptr(signed_capsule, SIGNATURE_SIZE * 2);
}

#ifdef PLATFORM_SEAMLESS_FEATURES_ENABLED

/**
 * @brief Return a pointer to the start address of FVM structure in a signed Seamless FV update capsule.
 *
 * @param signed_capsule indicates signed capsule starting location
 *
 * @return capsule fvm pointer
 */
static PFR_ALT_INLINE FVM* PFR_ALT_ALWAYS_INLINE get_seamless_capsule_fvm(alt_u32* signed_capsule)
{
    // Skip capsule signature and FVM signature
    return (FVM*) incr_alt_u32_ptr(signed_capsule, SIGNATURE_SIZE * 2);
}

/**
 * @brief Return the address of the signed FVM in a PCH firmware capsule.
 *
 * The capsule PFM contains FVM address definitions, which have start addresses of signed FVMs. But
 * these addresses pointing to the FVMs in Active PFR region. This function uses those addresses to
 * compute the addresses of signed FVMs in the firmware capsule.
 *
 * @oaram active_fvm_addr the start address of the corresponding FVM in Active
 * PFR region (as specificed in FVM address definition)
 *
 * @return the start address of this signed FVM in SPI flash.
 */
static PFR_ALT_INLINE alt_u32 PFR_ALT_ALWAYS_INLINE get_addr_of_signed_fvm_in_capsule
(alt_u32 active_fvm_addr, alt_u32 signed_capsule_addr)
{
    // Skip capsule signature and FVM signature
    return active_fvm_addr - get_active_region_offset(SPI_FLASH_PCH) + signed_capsule_addr + SIGNATURE_SIZE;
}

/**
 * @brief Given the firmware volume type, return the address of its FVM in the Active PFR region.
 *
 * @param fv_type indicates fv type
 *
 * @return active fvm address
 */
static alt_u32 get_active_fvm_addr(alt_u16 fv_type)
{
    PFM* active_pfm = get_active_pfm(SPI_FLASH_PCH);
    alt_u32 pfm_body_size = active_pfm->length - PFM_HEADER_SIZE;
    alt_u32* pfm_body = active_pfm->pfm_body;

    // Go through the PFM Body
    while (pfm_body_size)
    {
        alt_u32 incr_size = 0;

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
            incr_size = get_size_of_spi_region_def(region_def);
        }
        else if (def_type == FVM_ADDR_DEF_TYPE)
        {
            PFM_FVM_ADDR_DEF* fvm_addr_def = (PFM_FVM_ADDR_DEF*) pfm_body;

            if (fvm_addr_def->fv_type == fv_type)
            {
                return fvm_addr_def->fvm_addr;
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

    return 0;
}

/**
 * @brief Given the firmware volume type start and end address, check if the address
 * of capsule matches thos fvm in active regions.
 *
 *
 * @param start_addr indicates start address of fvm
 * @param end_addr indicates end address of fvm
 * @param fv_type indicates fv type
 *
 * @return 1 if found; 0 otherwise
 */

static alt_u32 get_capsule_fvm_spi_region(alt_u32 start_addr, alt_u32 end_addr, alt_u16 fv_type)
{
    PFM* active_pfm = get_active_pfm(SPI_FLASH_PCH);
    alt_u32 pfm_body_size = active_pfm->length - PFM_HEADER_SIZE;
    alt_u32* pfm_body = active_pfm->pfm_body;

    // Go through the PFM Body
    while (pfm_body_size)
    {
        alt_u32 incr_size = 0;

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
            incr_size = get_size_of_spi_region_def(region_def);
        }
        else if (def_type == FVM_ADDR_DEF_TYPE)
        {
            PFM_FVM_ADDR_DEF* fvm_addr_def = (PFM_FVM_ADDR_DEF*) pfm_body;

            if (fvm_addr_def->fv_type == fv_type)
            {
                FVM* fvm = (FVM*) get_spi_flash_ptr_with_offset(fvm_addr_def->fvm_addr + SIGNATURE_SIZE);

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

                        if ((region_def->start_addr == start_addr) && (region_def->end_addr == end_addr))
                        {
                            return 1;
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

    return 0;
}

#endif /* PLATFORM_SEAMLESS_FEATURES_ENABLED */

/**
 * @brief Check if the given SPI region is a static region.
 * A static region is a read-only region for PCH/BMC firmware.
 *
 * @param spi_region_def : spi region def location
 *
 * @return 1 if static; 0 otherwise
 */
static alt_u32 is_spi_region_static(PFM_SPI_REGION_DEF* spi_region_def)
{
    return ((spi_region_def->protection_mask & SPI_REGION_PROTECT_MASK_READ_ALLOWED) &&
            ((spi_region_def->protection_mask & SPI_REGION_PROTECT_MASK_WRITE_ALLOWED) == 0));
}

/**
 * @brief Check if the given SPI region is a dynamic region.
 * A dynamic region is a Read & Write allowed region for PCH/BMC firmware.
 *
 * @param spi_region_def : spi region def location
 *
 * @return 1 if dynamic; 0 otherwise
 */
static alt_u32 is_spi_region_dynamic(PFM_SPI_REGION_DEF* spi_region_def)
{
    return ((spi_region_def->protection_mask & SPI_REGION_PROTECT_MASK_READ_ALLOWED) &&
            (spi_region_def->protection_mask & SPI_REGION_PROTECT_MASK_WRITE_ALLOWED));
}

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)

/**
 * @brief Check the number of empty spaces allocated for AFM storage.
 * AFM is stored in the UFM and this function will return available spaces in the
 * UFM to store more AFM
 *
 * @return total number of empty location
 */

static alt_u32 get_ufm_empty_afm_number()
{
    alt_u32 max_afm = MAX_NO_OF_AFM_SUPPORTED;
    alt_u32 ufm_afm_addr = UFM_AFM_1;
    alt_u32 empty_region_number = 0;

    while (max_afm)
    {
        alt_u32* ufm_afm_ptr = get_ufm_ptr_with_offset(ufm_afm_addr);
        if (*ufm_afm_ptr == 0xFFFFFFFF)
        {
            empty_region_number++;
        }
        ufm_afm_addr += (UFM_CPLD_PAGE_SIZE >> 2);
        max_afm--;
    }
    return empty_region_number;
}

/**
 * @brief Find an empty location within the UFM page (allocated for AFM)
 * This functions search for the first available empty location address offset in the UFM
 * allocated for the AFM(s).
 *
 * @return first available empty address offset in UFM
 */

static alt_u32 get_ufm_empty_afm_offset()
{
    alt_u32 max_afm = MAX_NO_OF_AFM_SUPPORTED;
    alt_u32 ufm_afm_addr = UFM_AFM_1;

    while (max_afm)
    {
        alt_u32* ufm_afm_ptr = get_ufm_ptr_with_offset(ufm_afm_addr);
        if (*ufm_afm_ptr == 0xFFFFFFFF)
        {
            return ufm_afm_addr;
        }
        ufm_afm_addr += (UFM_CPLD_PAGE_SIZE >> 2);
        max_afm--;
    }
    return 0;
}

/**
 * @brief Find the stored AFM in the UFM based on the UUID.
 * This funcntion search for the corresponding AFM of a particular UUID in the
 * UFM and returns the starting address offset.
 *
 * @param uuid : UUID of hte AFM
 *
 * @return address offset of the AFM of certain uuid
 */

static alt_u32 get_ufm_afm_addr_offset(alt_u16 uuid)
{
    alt_u32 max_afm = MAX_NO_OF_AFM_SUPPORTED;
    alt_u32 ufm_afm_addr = UFM_AFM_1;

    while (max_afm)
    {
        alt_u32* ufm_afm_ptr = get_ufm_ptr_with_offset(ufm_afm_addr);
        alt_u16 stored_uuid = *((alt_u16*) ufm_afm_ptr);

        if (stored_uuid == uuid)
        {
            return ufm_afm_addr;
        }
        // Need to increase sizeof(afm) + size of measurement
        ufm_afm_addr += (UFM_CPLD_PAGE_SIZE >> 2);
        max_afm--;
    }
    return 0;
}

/**
 * @brief Search for the stored public key of the AFM.
 * This functions search for the stored public key of an AFM given the UUID
 * inside the UFM
 *
 * @param uuid : UUID pf AFM
 *
 * @return address of the public key
 */
static alt_u32* retrieve_afm_pub_key(alt_u16 uuid)
{
    AFM_BODY* stored_pub_key = (AFM_BODY*)get_ufm_ptr_with_offset(get_ufm_afm_addr_offset(uuid));
    return (alt_u32*)&stored_pub_key->pubkey_xy;
}


/**
 * @brief Search for the measurement stored based on uuid and index number
 * This functions search for the measurements stored in the UFM of a particular AFM.
 * For example, use this function to search for desired index measurement of a particular AFM basd on UUID.
 * This functions also saves the total possible measurement number of given index for reference.
 *
 * @param uuid : AFM UUID
 * @param index : Index number of the measurement
 * @param poss_meas_num  : Number of possible measurement number of the index
 * @param poss_meas_size : Size of possible measurements of the index
 *
 * @return the location of the measurement
 */
static alt_u32* retrieve_afm_index_measurement(alt_u16 uuid, alt_u8 index, alt_u8* poss_meas_num, alt_u32* poss_meas_size)
{
    AFM_BODY* stored_index = (AFM_BODY*) get_ufm_ptr_with_offset(get_ufm_afm_addr_offset(uuid));
    alt_u16* poss_meas_size_u16 = (alt_u16*) poss_meas_size;

    if (!index)
    {
    	return 0;
    }

    if (index == 1)
    {
    	*poss_meas_num = stored_index->possible_meas_num;
    	*poss_meas_size_u16 = stored_index->dmtf_spec_meas_val_size;
        return (alt_u32*) &stored_index->measurement;
    }
    AFM_DMTF_SPEC_MEASUREMENT* afm_measurement = (AFM_DMTF_SPEC_MEASUREMENT*) &stored_index->possible_meas_num;

    for (alt_u8 i = 1; i < index; i++)
    {
        alt_u32 dmtf_spec_meas_struct_size = 4;
        // measurements are padded to 4-byte aligned sizes
        alt_u32 padded_dmtf_spec_meas_val_size = round_up_to_multiple_of_4((alt_u32) afm_measurement->dmtf_spec_meas_val_size);
        dmtf_spec_meas_struct_size += afm_measurement->possible_meas_num * padded_dmtf_spec_meas_val_size;

        afm_measurement = (AFM_DMTF_SPEC_MEASUREMENT*) &afm_measurement[dmtf_spec_meas_struct_size/4];
    }

    *poss_meas_num = afm_measurement->possible_meas_num;
    *poss_meas_size_u16 = afm_measurement->dmtf_spec_meas_val_size;
    return (alt_u32*) &afm_measurement->measurement;
}

/**
 * @brief Check for the total number of available measurement of incides
 * This function scans for the total number of measurement of indices in the UFM based on UUID
 *
 * @param uuid : AFM UUID
 *
 * @return Total number of measurement (all index measurmeents)
 */
static alt_u32 get_total_number_of_measurements(alt_u16 uuid)
{
    AFM_BODY* stored_index = (AFM_BODY*) get_ufm_ptr_with_offset(get_ufm_afm_addr_offset(uuid));
    return stored_index->measurement_num;
}

/**
 * @brief Get size of afm body
 * This function will stop calculating the size once it exceeds AFM_BODY_UFM_ALLOCATED_SIZE, and returns 0
 *
 * @param afm_body : AFM_BODY
 *
 * @return size of afm body; returns 0 if size exceeds AFM_BODY_UFM_ALLOCATED_SIZE
 */
static alt_u32 get_afm_body_size(AFM_BODY* afm_body)
{
    alt_u32 afm_size = AFM_BODY_HEADER_SIZE;

    alt_u32 total_meas_num = afm_body->measurement_num;
    AFM_DMTF_SPEC_MEASUREMENT* afm_measurement = (AFM_DMTF_SPEC_MEASUREMENT*) &afm_body->possible_meas_num;

    for (alt_u32 meas_num = 0; meas_num < total_meas_num; meas_num++ ) {
    	alt_u32 dmtf_spec_meas_struct_size = 4;
    	// measurements are padded to 4-byte aligned sizes
    	alt_u32 padded_dmtf_spec_meas_val_size = round_up_to_multiple_of_4((alt_u32) afm_measurement->dmtf_spec_meas_val_size);
    	dmtf_spec_meas_struct_size += afm_measurement->possible_meas_num * padded_dmtf_spec_meas_val_size;

    	afm_size += dmtf_spec_meas_struct_size;
    	// if afm_size has exceeded valid max size, return 0
    	if ( afm_size > AFM_BODY_UFM_ALLOCATED_SIZE ) {
    		return 0;
    	}
    	afm_measurement = (AFM_DMTF_SPEC_MEASUREMENT*) &afm_measurement[dmtf_spec_meas_struct_size/4];
    }

    return afm_size;
}

#endif

#endif /* EAGLESTREAM_INC_PFM_UTILS_H_ */
