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
 * @file ufm_svn_utils.h
 * @brief SVN policies management in the UFM
 */

#ifndef EAGLESTREAM_INC_UFM_SVN_UTILS_H_
#define EAGLESTREAM_INC_UFM_SVN_UTILS_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "ufm.h"


#define UFM_SVN_POLICY_BMC get_ufm_pfr_data()->svn_policy_bmc
#define UFM_SVN_POLICY_PCH get_ufm_pfr_data()->svn_policy_pch
#define UFM_SVN_POLICY_CPLD get_ufm_pfr_data()->svn_policy_cpld

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)
// Save the SVN policies for AFM in UFM_PFR_DATA struct
#define UFM_SVN_POLICY_AFM get_ufm_pfr_data()->svn_policy_afm
// Save individual SVN policises for AFM
#define UFM_SVN_POLICY_AFM_UUID(uuid) \
    incr_alt_u32_ptr((alt_u32*) get_ufm_pfr_data(), sizeof(UFM_PFR_DATA) + (UFM_MAX_SUPPORTED_FV_TYPE * UFM_SVN_POLICY_SIZE) + (uuid * UFM_SVN_POLICY_SIZE))
#endif

#ifdef PLATFORM_SEAMLESS_FEATURES_ENABLED
// Save the SVN policies for FVMs immediately after the UFM_PFR_DATA struct in UFM
#define UFM_SVN_POLICY_FVM(fv_type) \
    incr_alt_u32_ptr((alt_u32*) get_ufm_pfr_data(), sizeof(UFM_PFR_DATA) + (fv_type * UFM_SVN_POLICY_SIZE))
#endif

/**
 * @brief Translates the SVN in UFM from a bit field to a number
 *
 * The intention of this is to work with write_ufm_svn and hide the internal implementation of
 * the svn and to handle all the bit manipulations
 *
 * @param svn_policy a pointer to the SVN policy in the UFM.
 * @return An integer representing the SVN
 */
static alt_u32 get_ufm_svn(alt_u32* svn_policy)
{
    for (alt_u32 i = 0; i < 64; i++)
    {
        if ((svn_policy[(i / 32)] & (1 << (i % 32))) != 0)
        {
            return i;
        }
    }
    // a svn of all 0s is defined as 64
    return 64;
}

/**
 * @brief Check if the given SVN is acceptable.
 *
 * An SVN is acceptable if it is greater or equal to the SVN in UFM and smaller than 64,
 *
 * @param svn_policy a pointer to the SVN policy in the UFM.
 * @param svn the Security Version Number to be checked
 */
static alt_u32 is_svn_valid(alt_u32* svn_policy, alt_u32 svn)
{
    return (svn >= get_ufm_svn(svn_policy)) && (svn <= UFM_MAX_SVN);
}

/**
 * @brief Writes the SVN into UFM with the correct formatting
 *
 * The intention of this is to work with get_ufm_svn hide the internal implementation of the svn
 * and to handle all the bit manipulations
 *
 * This function overwrites the SVN in UFM, if the input SVN is larger than the SVN in UFM and smaller than max
 * SVN. UFM flash hardware has limited number of erase/write, so this function do not perform unnecessary write.
 *
 * @see is_svn_valid
 *
 * @param svn_policy a pointer to the SVN policy in the UFM.
 * @param svn An integer from 0-64 representing the SVN. This will be translated to a bit field and written to UFM
 *
 * @return none
 */
static void write_ufm_svn(alt_u32* svn_policy, alt_u32 svn)
{
    if ((svn > get_ufm_svn(svn_policy)) && (svn <= UFM_MAX_SVN))
    {
        alt_u32 new_svn_policy = ~((1 << (svn % 32)) - 1);
        if (svn < 32)
        {
            svn_policy[0] = new_svn_policy;
        }
        else
        {
            svn_policy[0] = 0;
            if (svn < 64)
            {
                svn_policy[1] = new_svn_policy;
            }
            else if (svn == 64)
            {
                svn_policy[1] = 0;
            }
        }
    }
}

#endif /* EAGLESTREAM_INC_UFM_SVN_UTILS_H_ */
