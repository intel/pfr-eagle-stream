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
 * @file decompression.h
 * @brief Decompression algorithm to extract BMC/PCH firmware from recovery/update capsule.
 */

#ifndef EAGLESTREAM_INC_DECOMPRESSION_H
#define EAGLESTREAM_INC_DECOMPRESSION_H

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "keychain_utils.h"
#include "pbc.h"
#include "pbc_utils.h"
#include "pfm_utils.h"
#include "pfr_pointers.h"
#include "spi_rw_utils.h"
#include "ufm_utils.h"

/**
 * Define type of decompression action. 
 * If it's a static region only decompression, Nios only decompress SPI region, that
 * is read allowed but not write allowed, in PFM definition.
 * If it's a dynamic region only decompression, Nios only decompress SPI region, that
 * is both read allowed and write allowed, in PFM definition.
 * The last option is to recover both static and dynamic regions. 
 */
typedef enum
{
    DECOMPRESSION_STATIC_REGIONS_MASK             = 0b1,
    DECOMPRESSION_DYNAMIC_REGIONS_MASK            = 0b10,
    DECOMPRESSION_STATIC_AND_DYNAMIC_REGIONS_MASK = 0b11,
} DECOMPRESSION_TYPE_MASK_ENUM;

/**
 * @brief Decompress a SPI region from the a signed firmware update capsule.
 * First, Nios firmware find the bit that represents the first page of this 
 * SPI region in the compression bitmap and active bitmap. A pointer is used 
 * to skip pages in the compressed payload along the process. 
 *
 * Once Nios firmware reaches the correct bit, it checks the value of that bit. 
 * If the bit is 1 in the active bitmap, then Nios would erase that page. If the 
 * bit is 1 in the compression bitmap, then Nios would copy that page from the 
 * compressed payload and overwrite the corresponding page of this SPI region. 
 * This process ends when Nios finishes with the last page of this SPI region.
 * 
 * For every 8 pages it processed in compressed payload, Nios firmware would
 * pet the hardware watchdog timer to prevent timer expiry.
 * 
 * @param region_start_addr Start address of the SPI region
 * @param region_end_addr End address of the SPI region
 * @param signed_capsule pointer to the start of a signed firmware update capsule
 * @param is_afm_capsule indicate if is AFM capsule
 *
 * @return 0 if write to flash fail, 1 other wise
 */
static alt_u32 decompress_spi_region_from_capsule(alt_u32 region_start_addr, alt_u32 region_end_addr,
                                               alt_u32* signed_capsule, alt_u32 is_signed_afm)
{
    alt_u32 region_start_bit = region_start_addr / PBC_EXPECTED_PAGE_SIZE;
    alt_u32 region_end_bit = region_end_addr / PBC_EXPECTED_PAGE_SIZE;

    // Destination address to be updated in SPI flash
    alt_u32 dest_addr = region_start_addr;

    // Pointers to various places in the capsule
    PBC_HEADER* pbc = (PBC_HEADER*) incr_alt_u32_ptr(signed_capsule, SIGNATURE_SIZE);

    if (!is_signed_afm)
    {
        pbc = get_pbc_ptr_from_signed_capsule(signed_capsule);
    }

    alt_u32* src_ptr = get_compressed_payload(pbc);
    alt_u8* active_bitmap = (alt_u8*) get_active_bitmap(pbc);
    alt_u8* comp_bitmap = (alt_u8*) get_compression_bitmap(pbc);
    /*
     * Erase
     * Perform erase on the target SPI region first, according to the Active Bitmap.
     * Coalesce the pages where possible to erase them at once. This allows 64KB Erase to be used.
     */
    // This records the first bit in this chunk of pages to be erased
    alt_u32 erase_start_bit = 0xffffffff;
    for (alt_u32 bit_in_bitmap = region_start_bit; bit_in_bitmap < region_end_bit; bit_in_bitmap++)
    {
        alt_u8 active_bitmap_byte = active_bitmap[bit_in_bitmap >> 3];
        if (active_bitmap_byte & (1 << (7 - (bit_in_bitmap % 8))))
        {
            // Erase this page
            if (erase_start_bit == 0xffffffff)
            {
                erase_start_bit = bit_in_bitmap;
            }
        }
        else
        {
            // Don't erase this page
            if (erase_start_bit != 0xffffffff)
            {
                // Pages [erase_start_bit, bit_in_bitmap) needs to be erased
                alt_u32 erase_bits = bit_in_bitmap - erase_start_bit;
                if (erase_bits)
                {
                    erase_spi_region(erase_start_bit * PBC_EXPECTED_PAGE_SIZE, erase_bits * PBC_EXPECTED_PAGE_SIZE);
                    erase_start_bit = 0xffffffff;
                }
            }
        }
    }
    if (erase_start_bit != 0xffffffff)
    {
        // Erase to the end of the SPI region
        alt_u32 erase_start_addr = erase_start_bit * PBC_EXPECTED_PAGE_SIZE;
        erase_spi_region(erase_start_addr, region_end_addr - erase_start_addr);
    }

    /*
     * Copy
     * Perform copy from compressed payload to the target SPI region, according to the Compression Bitmap.
     * Nios go through the bitmap bit by bit and copy page by page.
     * Nios starts from bit 0 of the bitmap. If any bit, before the bit representing the start of
     * this SPI region, is set, Nios will skip that page in the compressed payload.
     */
    alt_u32 cur_bit = 0;
    if (is_signed_afm)
    {
        cur_bit = region_start_bit;
    }

    while(cur_bit < region_end_bit)
    {
        // Read a byte in compression bitmap and active bitmap
        alt_u8 comp_bitmap_byte = *comp_bitmap;
        comp_bitmap++;

        // Process 8 bits (representing 8 pages)
        for (alt_u8 bit_mask = 0b10000000; bit_mask > 0; bit_mask >>= 1)
        {
            alt_u32 copy_this_page = comp_bitmap_byte & bit_mask;

            // The current page is part of the SPI region
            if ((region_start_bit <= cur_bit) && (cur_bit < region_end_bit))
            {
                if (copy_this_page)
                {
                    // Value of '1' indicates a copy operation is needed. Perform the copy.
                    //alt_u32_memcpy(get_spi_flash_ptr_with_offset(dest_addr), src_ptr, PBC_EXPECTED_PAGE_SIZE);
                    //alt_u32_memcpy_s(get_spi_flash_ptr_with_offset(dest_addr), region_end_addr - region_start_addr, src_ptr, PBC_EXPECTED_PAGE_SIZE);
                    // Wait for the writes to complete, before moving on to next page
                    if (!memcpy_s_flash_successful(get_spi_flash_ptr_with_offset(dest_addr), region_end_addr - region_start_addr, src_ptr, PBC_EXPECTED_PAGE_SIZE))
                    {
                        return 0;
                    }
                }
                // Done with this page. Increment for updating the next page.
                dest_addr += PBC_EXPECTED_PAGE_SIZE;
            }

            if (copy_this_page)
            {
                // This page has been processed, moving to the next page in the compressed payload
                src_ptr = incr_alt_u32_ptr(src_ptr, PBC_EXPECTED_PAGE_SIZE);
            }

            // Move on to the next page (i.e. next bit in the bitmap)
            cur_bit++;
        }

        // Reset HW timer after 8 SPI pages have been processed
        reset_hw_watchdog();
    }
    return 1;
}

#ifdef PLATFORM_SEAMLESS_FEATURES_ENABLED
/**
 * @brief Find SPI region definitions in the given FVM and decompress those SPI region accordingly. 
 *
 * @param signed_capsule pointer to the start of a signed firmware update capsule
 * @param fvm_addr_def fvm address definition
 * @param decomp_type static or dynamic
 *
 * @return 0 if write to flash fail, 1 other wise
 */
static alt_u32 decompress_spi_regions_in_fvm(
        alt_u32* signed_capsule, PFM_FVM_ADDR_DEF* fvm_addr_def, DECOMPRESSION_TYPE_MASK_ENUM decomp_type)
{
    // Get pointer to FVM
    PFM* capsule_pfm = get_capsule_pfm(signed_capsule);
    alt_u32 fvm_offset_relative_to_pfm = fvm_addr_def->fvm_addr - get_active_region_offset(SPI_FLASH_PCH);
    FVM* fvm = (FVM*) incr_alt_u32_ptr((alt_u32*) capsule_pfm, fvm_offset_relative_to_pfm);

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

            if (is_spi_region_static(region_def))
            {
                if (decomp_type & DECOMPRESSION_STATIC_REGIONS_MASK)
                {
                    // Recover all regions that do not allow write
                    if (!decompress_spi_region_from_capsule(region_def->start_addr, region_def->end_addr, signed_capsule, 0))
                    {
                        return 0;
                    }
                }
            }
            else if (is_spi_region_dynamic(region_def) && region_def->start_addr != get_staging_region_offset(SPI_FLASH_PCH))
            {
                // This SPI region is a dynamic region and not staging region
                if (decomp_type & DECOMPRESSION_DYNAMIC_REGIONS_MASK)
                {
                    // Recover all regions that allows write
                    if (!decompress_spi_region_from_capsule(region_def->start_addr, region_def->end_addr, signed_capsule, 0))
                    {
                        return 0;
                    }
                }
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
#endif /* PLATFORM_SEAMLESS_FEATURES_ENABLED */

/**
 * @brief Decompress some types of SPI regions from a firmware update capsule.
 *
 * At this point, the capsule has been authenticated. Hence, Nios trusts the user settings in capsule. However,
 * if user has incorrect settings in PFM or Compression Structure Definition, unintended overwrite can
 * happen. For example, if a write-allowed SPI region includes a part of the recovery region, then
 * a dynamic firmware update would corrupt the recovery firmware. A more serious example would be: If
 * the staging region is not defined by just one SPI region definition, then a request to decompress
 * dynamic SPI regions would lead Nios to erase staging region while reading from it.
 *
 * Requirements on PFM and Compression Structure Definition are included in the MAS. When there's more
 * code space available, some of these requirements can be turned into checks in T-1 authentication.
 *
 * @param signed_capsule pointer to the start of a signed firmware update capsule
 * @param spi_flash_type indicate BMC or PCH SPI flash device
 * @param decomp_type indicate the type of this decompression action. This can be static region only
 * decompression, dynamic region only decompression, or decompression for both static and dynamic regions. 
 */
static void decompress_capsule(
        alt_u32* signed_capsule, SPI_FLASH_TYPE_ENUM spi_flash_type, DECOMPRESSION_TYPE_MASK_ENUM decomp_type)
{
    // Get addresses of active pfm and staging region
    alt_u32 active_pfm_addr = get_ufm_pfr_data()->bmc_active_pfm;
    alt_u32 staging_region_addr = get_ufm_pfr_data()->bmc_staging_region;
    if (spi_flash_type == SPI_FLASH_PCH)
    {
        active_pfm_addr = get_ufm_pfr_data()->pch_active_pfm;
        staging_region_addr = get_ufm_pfr_data()->pch_staging_region;
    }

    // Iterate through all the SPI region definitions in PFM body
    PFM* capsule_pfm = get_capsule_pfm(signed_capsule);
    alt_u32 pfm_body_size = capsule_pfm->length - PFM_HEADER_SIZE;
    alt_u32* capsule_pfm_body = capsule_pfm->pfm_body;

    // Go through the PFM Body
    while (pfm_body_size)
    {
        alt_u32 incr_size = 0;

        // Process data according to the definition type
        alt_u8 def_type = *((alt_u8*) capsule_pfm_body);
        if (def_type == SMBUS_RULE_DEF_TYPE)
        {
            // Skip SMBus Rule Definition
            incr_size = sizeof(PFM_SMBUS_RULE_DEF);
        }
        else if (def_type == SPI_REGION_DEF_TYPE)
        {
            // This is a SPI Region Definition
            PFM_SPI_REGION_DEF* region_def = (PFM_SPI_REGION_DEF*) capsule_pfm_body;

            if (is_spi_region_static(region_def))
            {
                if (decomp_type & DECOMPRESSION_STATIC_REGIONS_MASK)
                {
                    // Recover all regions that do not allow write
                    if (!decompress_spi_region_from_capsule(region_def->start_addr, region_def->end_addr, signed_capsule, 0))
                    {
                        return;
                    }
                }
            }
            else if (is_spi_region_dynamic(region_def) && region_def->start_addr != staging_region_addr)
            {
                // This SPI region is a dynamic region and not staging region
                if (decomp_type & DECOMPRESSION_DYNAMIC_REGIONS_MASK)
                {
                    // Recover all regions that allows write
                    if (!decompress_spi_region_from_capsule(region_def->start_addr, region_def->end_addr, signed_capsule, 0))
                    {
                        return;
                    }
                }
            }
            incr_size = get_size_of_spi_region_def(region_def);
        }
#ifdef PLATFORM_SEAMLESS_FEATURES_ENABLED
        else if (def_type == FVM_ADDR_DEF_TYPE)
        {
            // Decompress the SPI region definitions in FVM as well
            PFM_FVM_ADDR_DEF* fvm_addr_def = (PFM_FVM_ADDR_DEF*) capsule_pfm_body;
            if (!decompress_spi_regions_in_fvm(signed_capsule, fvm_addr_def, decomp_type))
            {
                return;
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
        capsule_pfm_body = incr_alt_u32_ptr(capsule_pfm_body, incr_size);
        pfm_body_size -= incr_size;
    }

    // If this decompression involves static region, also copy the PFM (in capsule) to replace the active PFM
    if (decomp_type & DECOMPRESSION_STATIC_REGIONS_MASK)
    {
        // Erase the current PFM SPI region first
        erase_spi_region(active_pfm_addr, SIGNED_PFM_MAX_SIZE);

        // Prep for memcpy
        alt_u32* signed_capsule_pfm = incr_alt_u32_ptr(signed_capsule, SIGNATURE_SIZE);
        alt_u32 nbytes = 0;
#ifdef PLATFORM_SEAMLESS_FEATURES_ENABLED
        nbytes = get_pfm_fvms_size_in_capsule(signed_capsule);
#else
        nbytes = get_signed_payload_size(signed_capsule_pfm);
#endif /* PLATFORM_SEAMLESS_FEATURES_ENABLED */

        // Copy the capsule PFM over
        // Do not need to poll WIP here as no more write after, saving some code space
        alt_u32_memcpy_s(get_spi_flash_ptr_with_offset(active_pfm_addr), SIGNED_PFM_MAX_SIZE, signed_capsule_pfm, nbytes);
    }
}

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)

/**
 * @brief Recovers AFM active image from a recovery capsule. Prior to this function, the active image
 * must have been corrupted.
 *
 * At this point, the capsule has been authenticated. Hence, Nios trusts the user settings in capsule. However,
 * if user has incorrect settings in AFM address Definition, unintended overwrite can
 * happen. For example, if a an address definition of an AFM body overlaps with another address definition,
 * unintended overwrite can happened.
 *
 * A recovery AFM capsule does not have PBC structure and thus, is not compressed. Nios simply replaces the signature and
 * the afm header of the active region before copying all the signed afm bodies to their respective addresses
 * as defined in the address definitnion.
 *
 * An address definition in the recovery capsule must reflect the intended address in the active afm image
 * and not the address of the afm bodies within the recovery capsule.
 *
 * @param signed_capsule pointer to the start of a signed firmware update capsule
 *
 * @return none
 */
static void perform_afm_active_recovery(alt_u32* signed_capsule)
{
    // Get addresses of active afm and recovery afm
    alt_u32 active_afm_start_addr = get_staging_region_offset(SPI_FLASH_BMC) + BMC_STAGING_REGION_AFM_ACTIVE_OFFSET;
    alt_u32 recovery_afm_sig_addr = get_staging_region_offset(SPI_FLASH_BMC) + BMC_STAGING_REGION_AFM_RECOVERY_OFFSET + SIGNATURE_SIZE;
	alt_u32* active_afm_start_addr_ptr = get_spi_flash_ptr_with_offset(active_afm_start_addr);
    alt_u32* recovery_afm_sig_addr_ptr = get_spi_flash_ptr_with_offset(recovery_afm_sig_addr);

    // Erase destination area first
    erase_spi_region(active_afm_start_addr, (SPI_FLASH_PAGE_SIZE_OF_64KB << 1));

    // Copy the signature and afm header over
    //alt_u32_memcpy_s(active_afm_start_addr_ptr, UFM_CPLD_PAGE_SIZE, recovery_afm_sig_addr_ptr, SIGNATURE_SIZE + ((KCH_BLOCK0*) recovery_afm_sig_addr_ptr)->pc_length);
    if (!memcpy_s_flash_successful(active_afm_start_addr_ptr, UFM_CPLD_PAGE_SIZE, recovery_afm_sig_addr_ptr, SIGNATURE_SIZE + ((KCH_BLOCK0*) recovery_afm_sig_addr_ptr)->pc_length))
    {
        return;
    }
    // Now copy all the AFM(s) over to the active region
    AFM* recovery_afm = (AFM*) get_spi_flash_ptr_with_offset(recovery_afm_sig_addr + SIGNATURE_SIZE);

    // Get the signature of first AFM
    alt_u32* afm_data = get_spi_flash_ptr_with_offset(recovery_afm_sig_addr + SIGNATURE_SIZE +
    		                             ((KCH_BLOCK0*) recovery_afm_sig_addr_ptr)->pc_length);

    alt_u32* afm_header = recovery_afm->afm_addr_def;
    alt_u32 afm_header_size = recovery_afm->length;

    // Iterate through the AFM header
    while (afm_header_size)
    {
        // Process data according to the definition type
        alt_u8 def_type = *((alt_u8*) afm_header);
        AFM_ADDR_DEF* afm_addr_def = (AFM_ADDR_DEF*) afm_header;
        if (def_type == AFM_SPI_REGION_DEF_TYPE)
        {
            alt_u32* active_afm_addr_def = get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr);
            // Copy the AFM over to active region
            //alt_u32_memcpy_s(active_afm_addr_def, UFM_CPLD_PAGE_SIZE, afm_data, afm_addr_def->afm_length);
            if (!memcpy_s_flash_successful(active_afm_addr_def, UFM_CPLD_PAGE_SIZE, afm_data, afm_addr_def->afm_length))
            {
                return;
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
    }
}
#endif

#endif /* EAGLESTREAM_INC_DECOMPRESSION_H */
