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
 * @file ufm.h
 * @brief Define the UFM PFR data structure.
 */

#ifndef EAGLESTREAM_INC_UFM_H_
#define EAGLESTREAM_INC_UFM_H_

#include "pfr_sys.h"
#include "pfm.h"
#include "rfnvram.h"

#define UFM_PFR_DATA_SIZE 316

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)
#undef UFM_PFR_DATA_SIZE
#define UFM_PFR_DATA_SIZE 340
#endif

#define UFM_FLASH_PAGE_SIZE 0x2000

#define UFM_MAX_SVN 64
#define UFM_SVN_POLICY_SIZE 8

#define UFM_MAX_SUPPORTED_FV_TYPE 20

#define UFM_STATUS_LOCK_BIT_MASK                      0b1
#define UFM_STATUS_PROVISIONED_ROOT_KEY_HASH_BIT_MASK 0b10
#define UFM_STATUS_PROVISIONED_PCH_OFFSETS_BIT_MASK   0b100
#define UFM_STATUS_PROVISIONED_BMC_OFFSETS_BIT_MASK   0b1000
#define UFM_STATUS_PROVISIONED_PIT_ID_BIT_MASK        0b10000
#define UFM_STATUS_PIT_L1_ENABLE_BIT_MASK             0b100000
#define UFM_STATUS_PIT_L2_ENABLE_BIT_MASK             0b1000000
#define UFM_STATUS_PIT_HASH_STORED_BIT_MASK           0b10000000
#define UFM_STATUS_PIT_L2_PASSED_BIT_MASK             0b100000000
#define UFM_STATUS_ENABLE_CPLD_TO_CHALLENGE           0b1000000000
#define UFM_STATUS_DISALLOW_CPLD_TO_CHALLENGE         0b10000000000

// If root key hash, pch and bmc offsets are provisioned, we say CPLD has been provisioned
#define UFM_STATUS_PROVISIONED_BIT_MASK               0b000001110

// If pch/bmc offsets are provisioned, CPLD now has SPI Geometry provisioned
#define UFM_STATUS_SPI_GEO_PROVISIONED_BIT_MASK       0b000001100


/*!
 * Structure of the data that is provisioned/stored onto on-chip UFM flash
 * 1. UFM Status.  Status bits used for the UFM provisioning and PIT enablement.
 *    - Bit0 = 0: UFM has been locked. Future modification is not allowed.
 *    - Bit1 = 0: UFM has been provisioned with root key hash and BMC/PCH offsets.
 *    - Bit2 = 0: UFM has been provisioned with PCH offsets.
 *    - Bit3 = 0: UFM has been provisioned with BMC offsets.
 *    - Bit4 = 0: UFM has been provisioned with PIT ID
 *    - Bit5 = 0: Enable PIT level 1 protection
 *    - Bit6 = 0: Enable PIT level 2 protection
 *    - Bit7 = 0: PIT Platform firmware (BMC/PCH) hash stored
 *    - Bit8 = 0: PIT L2 protection passed and disabled
 *    - Bit9 = 0: Enable CPLD to challenge other devices
 *    - Bit10 = 0 : Disallow CPLD to challenge other devices
 * 2. OEM PFM authentication root key
 * 3. Start address of Active region PFM, Recovery region and Staging region for PCH SPI flash
 * 4. Start address of Active region PFM, Recovery region and Staging region for BMC SPI flash
 * 5. PIT ID
 *    - Identification used in Protect-in-Transit level 1 verification.
 * 6. PIT PCH FW HASH. This is used for PIT Level 2 "FW Sealing" protection.
 * 7. PIT BMC FW HASH. This is used for PIT Level 2 "FW Sealing" protection.
 * 8. PFR SVN enforcement policies.
 *    - Track the minimum SVN that is acceptable for an update
 *    - This is a 64-bit bitfield. This bitfield translate to a number within 0-64, inclusive.
 *    - 0xFFFFFFFE 0xFFFFFFFF: min SVN is 1
 *    - 0xFFFFFFF0 0xFFFFFFFF: min SVN is 4
 * 9. CSK cancellation policies
 *    - Track the CSK keys that are cancelled.
 *    - This is a 128-bit bitmap. The least significant bit representing CSK Key ID 0,
 *      while the most significant bit represent CSK Key ID 127.
 *    - 0101_1111_ ... _1111: CSK Key ID 0 and 2 have been cancelled.
 *    - 1111_1111_ ... _1110: CSK Key ID 127 has been cancelled.
 */
typedef struct
{
    // UFM Status
    alt_u32 ufm_status;
    // OEM PFM authentication root key
    alt_u32 root_key_hash_256[SHA256_LENGTH / 4];
    alt_u32 root_key_hash_384[SHA384_LENGTH / 4];
    // Start address of Active region PFM, Recovery region and Staging region for PCH SPI flash
    alt_u32 pch_active_pfm;
    alt_u32 pch_recovery_region;
    alt_u32 pch_staging_region;
    // Start address of Active region PFM, Recovery region and Staging region for BMC SPI flash
    alt_u32 bmc_active_pfm;
    alt_u32 bmc_recovery_region;
    alt_u32 bmc_staging_region;
    // PIT ID
    alt_u32 pit_id[RFNVRAM_PIT_ID_LENGTH / 4];
    // PIT PCH FW HASH
    alt_u32 pit_pch_fw_hash[SHA384_LENGTH / 4];
    // PIT BMC FW HASH
    alt_u32 pit_bmc_fw_hash[SHA384_LENGTH / 4];
    // PFR SVN enforcement policies
    alt_u32 svn_policy_cpld[UFM_SVN_POLICY_SIZE / 4];
    alt_u32 svn_policy_pch[UFM_SVN_POLICY_SIZE / 4];
    alt_u32 svn_policy_bmc[UFM_SVN_POLICY_SIZE / 4];
    // CSK cancellation policies
    alt_u32 csk_cancel_pch_pfm[4];
    alt_u32 csk_cancel_pch_update_cap[4];
    alt_u32 csk_cancel_bmc_pfm[4];
    alt_u32 csk_cancel_bmc_update_cap[4];
    alt_u32 csk_cancel_cpld_update_cap[4];
#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)
    alt_u32 csk_cancel_bmc_afm[4];
    alt_u32 svn_policy_afm[UFM_SVN_POLICY_SIZE / 4];
#endif
} UFM_PFR_DATA;


#endif /* EAGLESTREAM_INC_UFM_H_ */
