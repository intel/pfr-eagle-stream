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
 * @file pbc_utils.h
 * @brief This header file contains functions to work with PBC structure.
 */

#ifndef EAGLESTREAM_INC_PBC_UTILS_H_
#define EAGLESTREAM_INC_PBC_UTILS_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "pbc.h"
#include "pfm_utils.h"
#include "pfr_pointers.h"

#ifdef PLATFORM_SEAMLESS_FEATURES_ENABLED
/**
 * @brief Return the size (in bytes) that PFM and all FVMs occupy in the given capsule.
 *
 * @param signed_capsule : Signed capsule location
 *
 * @return size of pfm fvm in capsule
 */
static PFR_ALT_INLINE alt_u32 PFR_ALT_ALWAYS_INLINE get_pfm_fvms_size_in_capsule(alt_u32* signed_capsule)
{
    alt_u32* signed_pfm = incr_alt_u32_ptr(signed_capsule, SIGNATURE_SIZE);
    PFM* capsule_pfm = (PFM*) incr_alt_u32_ptr(signed_pfm, SIGNATURE_SIZE);
    alt_u32 pfm_body_size = capsule_pfm->length - PFM_HEADER_SIZE;
    alt_u32* pfm_body = capsule_pfm->pfm_body;
    alt_u32 last_fvm_addr = 0;

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
            // Skip SPI Region Definition
            PFM_SPI_REGION_DEF* region_def = (PFM_SPI_REGION_DEF*) pfm_body;
            incr_size = get_size_of_spi_region_def(region_def);
        }
        else if (def_type == FVM_ADDR_DEF_TYPE)
        {
            PFM_FVM_ADDR_DEF* fvm_addr_def = (PFM_FVM_ADDR_DEF*) pfm_body;
            if (fvm_addr_def->fvm_addr > last_fvm_addr)
            {
                last_fvm_addr = fvm_addr_def->fvm_addr;
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

    // There must be exactly one PFM
    alt_u32 pfm_fvms_size = get_signed_payload_size(signed_pfm);

    if (last_fvm_addr)
    {
        // If there's any FVM, add the space containing the FVMs

        // The FVM address in FVM Address Definition is the start address of FVM in active PFR region.
        // Calculate the offset of the last FVM relative to the signed PFM in capsule
        alt_u32 last_fvm_offset_from_capsule_pfm = last_fvm_addr - get_active_region_offset(SPI_FLASH_PCH);
        alt_u32* signed_fvm = incr_alt_u32_ptr(signed_pfm, last_fvm_offset_from_capsule_pfm);

        // Compression header follows immediately after the last signed FVM
        pfm_fvms_size = last_fvm_offset_from_capsule_pfm + get_signed_payload_size(signed_fvm);
    }

    return pfm_fvms_size;
}
#endif /* PLATFORM_SEAMLESS_FEATURES_ENABLED */

/**
 * @brief Return a pointer to the start address of PBC structure in a signed firmware update capsule.
 * @param signed_capsule : Signed capsule location
 *
 * @return pbc of signed capsule
 */
static PBC_HEADER* get_pbc_ptr_from_signed_capsule(alt_u32* signed_capsule)
{
    // Skip capsule signature first
    alt_u32* signed_pfm = incr_alt_u32_ptr(signed_capsule, SIGNATURE_SIZE);

    // Number of bytes to increment the pointer further
    alt_u32 incr_bytes = get_signed_payload_size(signed_pfm);
#ifdef PLATFORM_SEAMLESS_FEATURES_ENABLED
    incr_bytes = get_pfm_fvms_size_in_capsule(signed_capsule);
#endif /* PLATFORM_SEAMLESS_FEATURES_ENABLED */

    return (PBC_HEADER*) incr_alt_u32_ptr(signed_pfm, incr_bytes);
}

/**
 * @brief Return the size of the bitmap in bytes. 
 * 
 * @param pbc pointer to the start of a PBC_HEADER structure
 * @return alt_u32 the size of the bitmap in bytes
 */
static PFR_ALT_INLINE alt_u32 PFR_ALT_ALWAYS_INLINE get_bitmap_size(PBC_HEADER* pbc)
{
    return pbc->bitmap_nbit / 8;
}

/**
 * @brief Return the pointer to the active bitmap in PBC. 
 * 
 * @param pbc pointer to the start of a PBC_HEADER structure
 * @return alt_u32* the pointer to the active bitmap in PBC
 */
static PFR_ALT_INLINE alt_u32* PFR_ALT_ALWAYS_INLINE get_active_bitmap(PBC_HEADER* pbc)
{
    return (alt_u32*) (pbc + 1);
}

/**
 * @brief Return the pointer to the compression bitmap in PBC. 
 * 
 * @param pbc pointer to the start of a PBC_HEADER structure
 * @return alt_u32* the pointer to the compression bitmap in PBC
 */
static alt_u32* get_compression_bitmap(PBC_HEADER* pbc)
{
    alt_u32* active_bitmap_addr = get_active_bitmap(pbc);
    return incr_alt_u32_ptr(active_bitmap_addr, get_bitmap_size(pbc));
}

/**
 * @brief Return the pointer to the compressed payload in PBC. 
 * 
 * @param pbc pointer to the start of a PBC_HEADER structure
 * @return alt_u32* the pointer to the compressed payload in PBC
 */
static alt_u32* get_compressed_payload(PBC_HEADER* pbc)
{
    alt_u32* compression_bitmap_addr = get_compression_bitmap(pbc);
    return incr_alt_u32_ptr(compression_bitmap_addr, get_bitmap_size(pbc));
}

#endif /* EAGLESTREAM_INC_PBC_UTILS_H_ */
