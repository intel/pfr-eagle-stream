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
 * @file keychain_utils.h
 * @brief Utility functions for easier handling of key chaining structures.
 */

#ifndef EAGLESTREAM_INC_KEYCHAIN_UTILS_H_
#define EAGLESTREAM_INC_KEYCHAIN_UTILS_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "keychain.h"
#include "pfr_pointers.h"
#include "utils.h"

/**
 * @brief Extract the lower 8 bits of the protected content type from Block 0. 
 */ 
static PFR_ALT_INLINE alt_u32 PFR_ALT_ALWAYS_INLINE get_kch_pc_type(KCH_BLOCK0* b0)
{
    return b0->pc_type & KCH_PC_TYPE_MASK;
}

/**
 * @brief Return the key permissions mask that is associated with a protected content type.
 *
 * @param pc_type protected content type
 * @return key permissions mask
 */
static alt_u32 get_required_perm(alt_u32 pc_type)
{
    if (pc_type == KCH_PC_PFR_CPLD_UPDATE_CAPSULE)
    {
        return SIGN_CPLD_UPDATE_CAPSULE_MASK;
    }
    if (pc_type == KCH_PC_PFR_PCH_PFM)
    {
        return SIGN_PCH_PFM_MASK;
    }
    if (pc_type == KCH_PC_PFR_PCH_UPDATE_CAPSULE)
    {
        return SIGN_PCH_UPDATE_CAPSULE_MASK;
    }
    if (pc_type == KCH_PC_PFR_BMC_PFM)
    {
        return SIGN_BMC_PFM_MASK;
    }
    if (pc_type == KCH_PC_PFR_BMC_UPDATE_CAPSULE)
    {
        return SIGN_BMC_UPDATE_CAPSULE_MASK;
    }
#ifdef PLATFORM_SEAMLESS_FEATURES_ENABLED
    if (pc_type == KCH_PC_PFR_PCH_SEAMLESS_CAPSULE)
    {
        return SIGN_PCH_UPDATE_CAPSULE_MASK;
    }
#endif
#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)
    if (pc_type == KCH_PC_PFR_AFM)
    {
        return SIGN_AFM_UPDATE_CAPSULE_MASK;
    }
#endif

    return 0;
}

/**
 * @brief Get the size (in bytes) of the signed payload (signature + payload).
 *
 * @param signed_payload_addr the start address of the signed payload
 * @return the size of the signed payload
 */
static PFR_ALT_INLINE alt_u32 PFR_ALT_ALWAYS_INLINE get_signed_payload_size(alt_u32* signed_payload_addr)
{
    return SIGNATURE_SIZE + ((KCH_BLOCK0*) signed_payload_addr)->pc_length;
}

#endif /* EAGLESTREAM_INC_KEYCHAIN_UTILS_H_ */
