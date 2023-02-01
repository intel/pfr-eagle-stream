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
 * @file ufm_utils.h
 * @brief Utility functions for accessing/updating UFM.
 */

#ifndef EAGLESTREAM_INC_UFM_UTILS_H_
#define EAGLESTREAM_INC_UFM_UTILS_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "pfm.h"
#include "pfr_pointers.h"
#include "spi_common.h"
#include "ufm_svn_utils.h"

/**
 * @brief Set ufm status (provisioning status, etc...)
 */
static alt_u32 set_ufm_status(alt_u32 ufm_status_bit_mask)
{
    return get_ufm_pfr_data()->ufm_status &= ~ufm_status_bit_mask;
}

/**
 * @brief Check ufm status (provisioning status, etc...)
 */
static alt_u32 check_ufm_status(alt_u32 ufm_status_bit_mask)
{
    return (((~get_ufm_pfr_data()->ufm_status) & ufm_status_bit_mask) == ufm_status_bit_mask);
}

/**
 * @brief Return whether the UFM has been provisioned with root key hash and flash offsets.
 *
 * @return 1 if UFM is provisioned; 0, otherwise.
 */
static PFR_ALT_INLINE alt_u32 PFR_ALT_ALWAYS_INLINE is_ufm_provisioned()
{
    return check_ufm_status(UFM_STATUS_PROVISIONED_BIT_MASK);
}

/**
 * @brief Return whether the UFM has been provisioned with flash offsets.
 *
 * @return 1 if UFM Spi Geo is provisioned; 0, otherwise.
 */
static PFR_ALT_INLINE alt_u32 PFR_ALT_ALWAYS_INLINE is_ufm_spi_geo_provisioned()
{
    return (((~get_ufm_pfr_data()->ufm_status) & UFM_STATUS_PROVISIONED_BIT_MASK)
             == UFM_STATUS_SPI_GEO_PROVISIONED_BIT_MASK);
}


/**
 * @brief Return whether the provisioned data in UFM has been locked.
 *
 * @return 1 if UFM is locked; 0, otherwise.
 */
static PFR_ALT_INLINE alt_u32 PFR_ALT_ALWAYS_INLINE is_ufm_locked()
{
    return check_ufm_status(UFM_STATUS_LOCK_BIT_MASK);
}

/**
 * @brief Return a pointer to the appropriate CSK cancellation policy, given the protected content type.
 */
static alt_u32* get_ufm_key_cancel_policy_ptr(alt_u32 pc_type)
{
    if (pc_type == KCH_PC_PFR_CPLD_UPDATE_CAPSULE)
    {
        return get_ufm_pfr_data()->csk_cancel_cpld_update_cap;
    }
    else if (pc_type == KCH_PC_PFR_PCH_PFM)
    {
        return get_ufm_pfr_data()->csk_cancel_pch_pfm;
    }
    else if (pc_type == KCH_PC_PFR_BMC_PFM)
    {
        return get_ufm_pfr_data()->csk_cancel_bmc_pfm;
    }
    else if (pc_type == KCH_PC_PFR_BMC_UPDATE_CAPSULE)
    {
    	return get_ufm_pfr_data()->csk_cancel_bmc_update_cap;
    }
#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)
    else if (pc_type == KCH_PC_PFR_AFM)
    {
        return get_ufm_pfr_data()->csk_cancel_bmc_afm;
    }
#endif
    // pc_type == KCH_PC_PFR_PCH_UPDATE_CAPSULE
    return get_ufm_pfr_data()->csk_cancel_pch_update_cap;
}


/**
 * @brief Set a flag in UFM that indicates a CPLD recovery update is in progress.
 *
 * Nios writes 0x0 to CPLD update status. It means that a CPLD recovery update is in progress and
 * that after a successful boot, Nios needs to back up the staging capsule into the CPLD recovery region.
 *
 * This update status needs to be in UFM because everything else will be lost after a CPLD update.
 */
static void set_cpld_rc_update_in_progress_ufm_flag()
{
    alt_u32* cpld_update_status = get_ufm_ptr_with_offset(UFM_CPLD_UPDATE_STATUS_OFFSET);
    *cpld_update_status = 0;
}

/**
 * @brief Check if there's a CPLD recovery update in progress.
 *
 * If CPLD update status is set to 0, it means that a CPLD recovery update is in progress and that after
 * a successful boot, Nios needs to back up the staging capsule into the CPLD recovery region.
 *
 * @return alt_u32 1 if there's an CPLD recovery update in progress; 0, otherwise
 */
static alt_u32 is_cpld_rc_update_in_progress()
{
    alt_u32* cpld_update_status = get_ufm_ptr_with_offset(UFM_CPLD_UPDATE_STATUS_OFFSET);
    return *cpld_update_status == 0;
}

#endif /* EAGLESTREAM_INC_UFM_UTILS_H_ */
